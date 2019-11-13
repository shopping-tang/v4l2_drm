#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <string.h>

#include "v4l2.h"
#if 0
void unchar_to_Mat(unsigned char *_buffer,cv::Mat& img)
{
    for (int i = 0;i < (BUFFER_SIZE_det);i++)
    {
        img.at<cv::Vec3b>((IMAGE_HEIGHT-1-(i / (IMAGE_WIDTH*CHANNEL))),((i % (IMAGE_WIDTH*CHANNEL))/CHANNEL))[i%CHANNEL] = _buffer[i]; // BGR Mat
    }
}
#endif
void yuyv2bgr24(unsigned char*yuyv, unsigned char*rgb)
{   unsigned int width = IMAGE_WIDTH;
    unsigned int height = IMAGE_HEIGHT;
    unsigned int i, in, rgb_index = 0;
    unsigned char y0, u0, y1, v1;
    int r, g, b;
    unsigned int x , y;

    for(in = 0; in < width * height * 2; in += 4)
    {
    y0 = yuyv[in+0];
    u0 = yuyv[in+1];
    y1 = yuyv[in+2];
    v1 = yuyv[in+3];

    for (i = 0; i < 2; i++)
    {
        if (i)
            y = y1;
        else
            y = y0;
        r = y + (140 * (v1-128))/100;  //r
        g = y - (34 * (u0-128))/100 - (71 * (v1-128))/100; //g
        b = y + (177 * (u0-128))/100; //b
        if(r > 255)   r = 255;
            if(g > 255)   g = 255;
            if(b > 255)   b = 255;
            if(r < 0)     r = 0;
            if(g < 0)     g = 0;
            if(b < 0)     b = 0;

        y = height - rgb_index/width -1;
        x = rgb_index%width;
        rgb[(y*width+x)*3+0] = b;
        rgb[(y*width+x)*3+1] = g;
        rgb[(y*width+x)*3+2] = r;
        rgb_index++;
    }
    }
}

void yuyv2bgra32(unsigned char*yuyv, unsigned char*bgra)
{   unsigned int width = IMAGE_WIDTH;
    unsigned int height = IMAGE_HEIGHT;
    unsigned int i, in, bgra_index = 0;
    unsigned char y0, u0, y1, v1;
    int a = 0xFF, r, g, b;
    unsigned int x , y;

    for(in = 0; in < width * height * 2; in += 4)
    {
    y0 = yuyv[in+0];
    u0 = yuyv[in+1];
    y1 = yuyv[in+2];
    v1 = yuyv[in+3];

    for (i = 0; i < 2; i++)
    {
        if (i)
            y = y1;
        else
            y = y0;

        r = y + (140 * (v1-128))/100;  //r
        g = y - (34 * (u0-128))/100 - (71 * (v1-128))/100; //g
        b = y + (177 * (u0-128))/100; //b

        if(r > 255)   r = 255;
            if(g > 255)   g = 255;
            if(b > 255)   b = 255;
            if(r < 0)     r = 0;
            if(g < 0)     g = 0;
            if(b < 0)     b = 0;

        y = height - bgra_index/width -1;
        x = bgra_index%width;
        bgra[(y*width+x)*4+0] = b;
        bgra[(y*width+x)*4+1] = g;
        bgra[(y*width+x)*4+2] = r;
        bgra[(y*width+x)*4+3] = a;
        bgra_index++;
    }
    }
}

BUF *v4l2(char *FILE_VIDEO)
{
    //int i;
    //int ret = 0;
    BUF *buffers = (BUF *)malloc(sizeof (BUF));
    //open dev
    if ((buffers->fd = open(FILE_VIDEO, O_RDWR)) == -1)
    {
        printf("Error opening V4L interface\n");
        return (FALSE);
    }

    struct v4l2_capability   cap;
    memset(&cap, 0, sizeof(cap));
    //query cap
    if (ioctl(buffers->fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        printf("Error opening device %s: unable to query device.\n",FILE_VIDEO);
        return (FALSE);
    }
    else
    {
        //printf("driver:\t\t%s\n",cap.driver);
        //printf("card:\t\t%s\n",cap.card);
        //printf("bus_info:\t%s\n",cap.bus_info);
        //printf("version:\t%d\n",cap.version);
        //printf("capabilities:\t%x\n",cap.capabilities);

        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
        {
            printf("Device %s: supports capture.\n",FILE_VIDEO);
        }

        if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
        {
            printf("Device %s: supports streaming.\n",FILE_VIDEO);
        }
    }

    //emu all support fmt
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format:\n");
    while(ioctl(buffers->fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
    {
        printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description); // get camera all support format here
        fmtdesc.index++;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    //set fmt
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // output format
    fmt.fmt.pix.height = IMAGE_HEIGHT;
    fmt.fmt.pix.width = IMAGE_WIDTH;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    //fmt.fmt.pix.priv = 1;

    if(ioctl(buffers->fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        printf("Unable to set format\n");
        return FALSE;
    }
    if(ioctl(buffers->fd, VIDIOC_G_FMT, &fmt) == -1)
    {
        printf("Unable to get format\n");
        return FALSE;
    }
    {
        //printf("fmt.type:\t\t%d\n",fmt.type);
        printf("pix.pixelformat:\t%c%c%c%c\n",fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,(fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
        printf("pix.height:\t\t%d\n",fmt.fmt.pix.height);
        printf("pix.width:\t\t%d\n",fmt.fmt.pix.width);
        printf("pix.field:\t\t%d\n",fmt.fmt.pix.field);
    }
    //set fps
    struct v4l2_streamparm setfps;
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;   // Per numerator seconds show denominator fps .
    setfps.parm.capture.timeperframe.denominator = 2;
    setfps.parm.capture.capturemode = 0;
    if(ioctl(buffers->fd, VIDIOC_S_PARM, &setfps) == -1)
    {
        printf("set fps fail !\n");
        return FALSE;
    }

    if(ioctl(buffers->fd, VIDIOC_G_PARM, &setfps) == -1)
    {
        printf("get fps fail !\n");
        return FALSE;
    }else
    {
        printf("True video fps = %d\n",(setfps.parm.capture.timeperframe.denominator)/(setfps.parm.capture.timeperframe.numerator));
    }

    printf("init %s \t[OK]\n",FILE_VIDEO);
/***************************************************/
    unsigned int n_buffers;
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    //request for 4 buffers
    req.count = COUNT;
    req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory=V4L2_MEMORY_MMAP;
    if(ioctl(buffers->fd,VIDIOC_REQBUFS,&req)==-1)
    {
        printf("request for buffers error\n");
    }

    //mmap for buffers
    if (!buffers)
    {
        printf ("Out of memory\n");
        return(FALSE);
    }

    struct v4l2_buffer buf ;
    memset( &buf, 0, sizeof(buf) );

    for (n_buffers = 0; n_buffers < COUNT; n_buffers++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        //query buffers
        if (ioctl (buffers->fd, VIDIOC_QUERYBUF, &buf) == -1)
        {
            printf("query buffer error\n");
            return(FALSE);
        }

        //map
        buffers->start[n_buffers] = mmap(NULL,buf.length,PROT_READ |PROT_WRITE, MAP_SHARED, buffers->fd, buf.m.offset);
        if (buffers->start[n_buffers] == MAP_FAILED)
        {
            printf("buffer map error\n");
            return(FALSE);
        }
    //queue
        ioctl(buffers->fd, VIDIOC_QBUF, &buf) ;
    }
    /***************************************************/

    int type = 1;
    ioctl (buffers->fd, VIDIOC_STREAMON, &type) ;

    return buffers ;
}

unsigned char *get_img(BUF *buffers, unsigned char *srcBuffer)
{
    // loop start
    struct v4l2_buffer dequeue ;
    dequeue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    dequeue.memory = V4L2_MEMORY_MMAP ;

    ioctl(buffers->fd, VIDIOC_DQBUF, &dequeue) ; // get frame

    srcBuffer = (unsigned char *)buffers->start[dequeue.index];

    ioctl(buffers->fd, VIDIOC_QBUF, &dequeue) ;
    return srcBuffer;
}

int close_v4l2(BUF *buffers)
{
     int type = 0;
     ioctl(buffers->fd, VIDIOC_STREAMOFF, &type);

     for(int i=0;i<COUNT;i++)
     {
         munmap(buffers->start[i],BUFFER_SIZE_src);
     }
     if(buffers->fd != -1)
     {
         close(buffers->fd);
         free(buffers);
         return (TRUE);
     }
     free(buffers);
     return (FALSE);
}
