#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include "drm_bo.h"
#include "drm_dev.h"

static int add_fb_sp_bo(struct sp_bo *bo, uint32_t format)
{
    int ret;
    uint32_t handles[4], pitches[4], offsets[4];

    handles[0] = bo->handle;
    pitches[0] = bo->pitch; // pitch = width * (bpp / 8)
    offsets[0] = 0;

    handles[1] = bo->handle;
    pitches[1] = pitches[0];
    offsets[1] = pitches[0] * bo->height; // offsets[1] equal to a image's size --> width*height*channel

    ret = drmModeAddFB2(bo->dev->fd, bo->width, bo->height,
            format, handles, pitches, offsets,
            &bo->fb_id, bo->flags);
    if (ret) {
        printf("failed to create fb ret=%d\n", ret);
        return ret;
    }
    return 0;
}

static int map_sp_bo(struct sp_bo *bo)
{
    int ret;
    struct drm_mode_map_dumb md;

    if (bo->map_addr)
        return 0;

    md.handle = bo->handle;
    ret = drmIoctl(bo->dev->fd, DRM_IOCTL_MODE_MAP_DUMB, &md);
    if (ret) {
        printf("failed to map sp_bo ret=%d\n", ret);
        return ret;
    }

    bo->map_addr = mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                bo->dev->fd, md.offset);
    if (bo->map_addr == MAP_FAILED) {
        printf("failed to map bo ret=%d\n", -errno);
        return -errno;
    }
    return 0;
}

struct sp_bo *create_sp_bo(struct sp_dev *dev, uint32_t width, uint32_t height,
        uint32_t depth,uint32_t _bpp, uint32_t format, uint32_t flags)
{
    int ret;
    struct drm_mode_create_dumb cd;
    struct sp_bo *bo;

    bo = (sp_bo *)calloc(1, sizeof(*bo));
    if (!bo)
        return NULL;

    cd.height = height;
    cd.width = width;
    cd.bpp = _bpp;
    cd.flags = flags;

    ret = drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATE_DUMB, &cd);
    if (ret) {
        printf("failed to create sp_bo %d\n", ret);
        goto err;
    }

    bo->dev = dev;
    bo->width = width;
    bo->height = height;
    bo->depth = depth;
    bo->bpp = _bpp;
    bo->format = format;
    printf("format = %d\n",format);
    bo->flags = flags;

    bo->handle = cd.handle;
    bo->pitch = cd.pitch;
    bo->size = cd.size;

    ret = add_fb_sp_bo(bo, format);
    if (ret) {
        printf("failed to add fb ret=%d\n", ret);
        goto err;
    }

    ret = map_sp_bo(bo);
    if (ret) {
        printf("failed to map bo ret=%d\n", ret);
        goto err;
    }

    return bo;

err:
    free_sp_bo(bo);
    return NULL;
}

void free_sp_bo(struct sp_bo *bo)
{
    int ret;
    struct drm_mode_destroy_dumb dd;

    if (!bo)
        return;

    if (bo->map_addr)
        munmap(bo->map_addr, bo->size);

    if (bo->fb_id) {
        ret = drmModeRmFB(bo->dev->fd, bo->fb_id);
        if (ret)
            printf("Failed to rmfb ret=%d!\n", ret);
    }

    if (bo->handle) {
        dd.handle = bo->handle;
        ret = drmIoctl(bo->dev->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dd);
        if (ret)
            printf("Failed to destroy buffer ret=%d\n", ret);
    }

    free(bo);
}

