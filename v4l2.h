#ifndef __V4L2_H__
#define __V4L2_H__

#include <sys/mman.h>
#include <linux/types.h>
#include <string.h>

#define  TRUE   1
#define  FALSE  0
#define  IMAGE_WIDTH    640
#define  IMAGE_HEIGHT   480
#define  COUNT  4

#define CHANNEL 4
#define BUFFER_SIZE_src IMAGE_WIDTH*IMAGE_HEIGHT*2
#define BUFFER_SIZE_det IMAGE_WIDTH*IMAGE_HEIGHT*CHANNEL

typedef struct buffer
{
    int fd;
    void * start;
} BUF ;

extern "C"
{
    void yuyv2bgr24(unsigned char*yuyv, unsigned char*rgb);
    void yuyv2bgra32(unsigned char*yuyv, unsigned char*bgra);
    BUF *v4l2(char *FILE_VIDEO, BUF *buffers);
    unsigned char *get_img(struct v4l2_buffer dequeue,BUF *buffers,unsigned char *srcBuffer);
    int close_v4l2(BUF *buffers);

}


#endif
