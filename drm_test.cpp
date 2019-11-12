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

#include "v4l2.h"
#include "drm_dev.h"
#include "drm_bo.h"
#include "drm_modeset.h"

static int terminate = 0;

static void sigint_handler(int arg)
{
    terminate = 1;
}

int main()
{   /************************* V4L2 parameter *************************/
    //struct v4l2_buffer dequeue_1 ;
    //dequeue_1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    //dequeue_1.memory = V4L2_MEMORY_MMAP ;
    //BUF *buffer_1 ;
    //buffer_1 = (struct buffer *)malloc(COUNT*(sizeof (*buffer_1)));
    char video_1[] = "/dev/video10" ;
    unsigned char *srcBuffer_1 = (unsigned char*)malloc(sizeof(unsigned char) * BUFFER_SIZE_src);  // yuyv
    unsigned char *dstBuffer_1 = (unsigned char*)malloc(sizeof(unsigned char) * BUFFER_SIZE_det);  // RGB
    /************************* DRM parameter *************************/
    struct sp_dev *dev = NULL;
    struct sp_plane **plane = NULL;
    struct sp_crtc *test_crtc = NULL;

    int card = 0, crtc_index = 0, plane_index = 0;
    uint32_t video_format =  DRM_FORMAT_BGR888 ;
    printf("video_format = %d\n",video_format);
    uint32_t display_x = 128 , display_y = 0 ;
    uint32_t display_w = 1280 , display_h = 960 ; // display frame width height
    uint32_t frame_depth = 16, frame_bpp = 24 ;
    /************************* V4L2 function *************************/
    printf("***** v4l2 function start *****\n");
    BUF *buffer_1 = v4l2(video_1) ;
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

    plane = (sp_plane **)calloc(dev->num_planes, sizeof(*plane));
    if (!plane) {
        printf("Failed to allocate plane array\n");
        return (FALSE);
    }

    /* Create our planes */
    /*
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

        //memset(plane[i]->bo->map_addr ,255 ,plane[i]->bo->size) ;
    }*/

    plane[plane_index] = get_sp_plane(dev, test_crtc);
    if (!plane[plane_index]) {
       printf("no unused planes available\n");
       return (FALSE);
    }

    plane[plane_index]->bo = create_sp_bo(dev, IMAGE_WIDTH, IMAGE_HEIGHT, frame_depth, frame_bpp,plane[plane_index]->format, 0);
    if (!plane[plane_index]->bo) {
       printf("failed to create plane bo\n");
       return (FALSE);
    }

    memset(plane[plane_index]->bo->map_addr ,255 ,plane[plane_index]->bo->size) ;

    printf("plane size = %d\n",plane[plane_index]->bo->size);
    printf("plane format = %d\n",plane[plane_index]->bo->format);
    /************************* loop start *************************/
    printf("/***************** loop start *****************/\n");
    while(1)
    {
        srcBuffer_1 = get_img(buffer_1, srcBuffer_1);
        yuyv2bgr24(srcBuffer_1, dstBuffer_1);

        memcpy(plane[plane_index]->bo->map_addr, (void *)dstBuffer_1, plane[plane_index]->bo->size );
        ret = set_sp_plane(dev, plane[plane_index], test_crtc, display_x, display_y, display_w, display_h);
        if (ret) {
            printf("failed to set plane 0 %d\n", ret);
            return (FALSE);
        }
        usleep(15 * 1000);
    }

    //for (int i = 0; i < num_test_planes; i++)
        //put_sp_plane(plane[i]);
    put_sp_plane(plane[plane_index]);

    free(srcBuffer_1);
    free(dstBuffer_1);
    close_v4l2(buffer_1);

    destroy_sp_dev(dev);
    free(plane);

    return(TRUE);
}
