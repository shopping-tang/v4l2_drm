#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <xf86drm.h>
#include <linux/videodev2.h>
#include <linux/types.h>
#include <drm_fourcc.h>
#include <thread>

#include "v4l2.h"
#include "drm_dev.h"
#include "drm_bo.h"
#include "drm_modeset.h"

using namespace std;
static int _terminate = 0;

static void sigint_handler(int arg)
{
    _terminate = 1;
}

int video_display(int cpuid, int video_index, int crtc_index, int plane_index, uint32_t display_x, uint32_t display_y, uint32_t display_w, uint32_t display_h)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);

    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    {
        printf("set thread affinity failed");
        return(FALSE);
    }
    printf("Bind CameraCapture process to CPU %d\n", cpuid);
    /************************* V4L2 parameter *************************/
    char video[25];
    snprintf(video, sizeof(video), "/dev/video%d", video_index);
    unsigned char *srcBuffer = (unsigned char*)malloc(sizeof(unsigned char) * BUFFER_SIZE_src);  // yuyv
    unsigned char *dstBuffer = (unsigned char*)malloc(sizeof(unsigned char) * BUFFER_SIZE_det);  // RGB
    /************************* DRM parameter *************************/
    struct sp_dev *dev = NULL;
    struct sp_plane **plane = NULL;
    struct sp_crtc *test_crtc = NULL;

    int card = 0 ;
    uint32_t video_format =  DRM_FORMAT_BGR888 ;
    printf("video_format = %d\n",video_format);
    uint32_t frame_depth = 16, frame_bpp = 24 ;
    /************************* V4L2 function *************************/
    printf("***** v4l2 function start *****\n");
    BUF *buffer = v4l2(video) ;
    /************************* DRM function *************************/
    printf("***** drm function start *****\n");
    dev = create_sp_dev(card, video_format);
    if (!dev) {
        printf("Failed to create sp_dev\n");
        return (FALSE);
    }

    if (crtc_index >= dev->num_crtcs) {
        printf("Invalid crtc %d (num=%d)\n", crtc_index, dev->num_crtcs);
        return (FALSE);
    }

    int ret = initialize_screens(dev); // ready to display FB
    if (ret) {
        printf("Failed to initialize screens\n");
        return (FALSE);
    }
    test_crtc = &dev->crtcs[crtc_index]; // crtc_index is a parameter , i used to set it equal to 0 .
    drmModeCrtcPtr save_crtc = test_crtc->crtc ;
    drmModeConnector *save_connector = dev->connectors[0].conn;
    
    plane = (sp_plane **)calloc(dev->num_planes, sizeof(*plane));
    if (!plane) {
        printf("Failed to allocate plane array\n");
        return (FALSE);
    }

    /* Create our planes */
    int num_test_planes = test_crtc->num_planes;
    for (int i = 0; i < num_test_planes; i++) {
        plane[i] = get_sp_plane(dev, test_crtc);
        if (!plane[i]) {
            printf("no unused planes available\n");
            return (FALSE);
        }

        plane[i]->bo = create_sp_bo(dev, IMAGE_WIDTH, IMAGE_HEIGHT, frame_depth, frame_bpp,
                plane[i]->format, 0);
        if (!plane[i]->bo) {
            printf("failed to create plane bo\n");
            return (FALSE);
        }

        memset(plane[i]->bo->map_addr ,255 ,plane[i]->bo->size) ;
    }
    printf("plane size = %d\n",plane[plane_index]->bo->size);
    printf("plane format = %d\n",plane[plane_index]->bo->format);
    /************************* loop start *************************/
    printf("/***************** loop start *****************/\n");
    while(!_terminate)
    {
        srcBuffer = get_img(buffer, srcBuffer);
        yuyv2bgr24(srcBuffer, dstBuffer);

        //memcpy(plane[plane_index]->bo->map_addr, (void *)dstBuffer, plane[plane_index]->bo->size );
        memcpy(plane[plane_index]->bo->map_addr, (void *)dstBuffer, BUFFER_SIZE_det );
        ret = set_sp_plane(dev, plane[plane_index], test_crtc, display_x, display_y, display_w, display_h);
        if (ret) {
            printf("failed to set plane %d %d\n",plane_index, ret);
            return (FALSE);
        }
        usleep(15 * 1000);
    }
    
    ret = drmModeSetCrtc(dev->fd, save_crtc->crtc_id, save_crtc->buffer_id,
                         save_crtc->x, save_crtc->y,
                         &save_connector->connector_id, 1, &save_crtc->mode);
    if (ret) {
        printf("failed to restore crtc .\n");
    }
    
    for (int i = 0; i < num_test_planes; i++)
        put_sp_plane(plane[i]);
    

    free(srcBuffer);
    free(dstBuffer);
    close_v4l2(buffer);

    destroy_sp_dev(dev);
    free(plane);

    return(TRUE);
}

int main()    
{
    signal(SIGINT, sigint_handler);
    array<thread, 2> threads;
    threads = {thread(video_display, 0, 10, 0, 0, 128, 0, 1280, 960),
               thread(video_display, 1, 12, 0, 2, 128, 1088, 1280, 960),
              };
    for (int i = 0; i < 2; i++){
        threads[i].join();
    }
    return(TRUE);
}
