#ifndef __DRM_MODESET_H__
#define __DRM_MODESET_H__

#include "drm_dev.h"

struct sp_dev;
struct sp_crtc;

extern "C"
{
int initialize_screens(struct sp_dev *dev);
struct sp_plane *get_sp_plane(struct sp_dev *dev, struct sp_crtc *crtc);
void put_sp_plane(struct sp_plane *plane);

int set_sp_plane(struct sp_dev *dev, struct sp_plane *plane,
        struct sp_crtc *crtc, uint32_t _x, uint32_t _y, uint32_t _w, uint32_t _h);

int set_sp_plane_pset(struct sp_dev *dev, struct sp_plane *plane,
        drmModeAtomicReqPtr pset, struct sp_crtc *crtc, int x, int y);
}

#endif

