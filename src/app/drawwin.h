/*
 * drawwin.h -- the 3D view: a child window holding a Reality Lab device,
 * viewport and scene.  Reconstructed from LABYRINTH.EXE (vtable 0x4ae438,
 * code 0x40ecc0-0x4107f0).
 */
#ifndef DRAWWIN_H
#define DRAWWIN_H

#include "labwin.h"
#include "wcmodel.h"
#include "../rl/rl.h"

class LabDrawingWindow : public LabChildWindow {
public:
    LabDrawingWindow(LabWindow *parent, int x, int y, int w, int h); /* 40ecc0 */

    void OnShow(int show) {}                                    /* 410030 */
    void OnPaint(LabPaint *p);                                  /* 410040 */
    void OnResize(int w, int h);                                /* 410060 */
    void OnMouseDown(int x, int y, int mods);                   /* 4101d0 */
    void OnDoubleClick(int x, int y, int mods) {}               /* 410220 */
    void OnMouseUp(int x, int y, int mods);                     /* 410210 */
    void OnMouseMove(int x, int y, int mods);                   /* 410230 */

    virtual void SaveCamera(const char *file);                  /* 40f0b0 */
    virtual void Navigate(double turn, double move);            /* 40fc90 */
    virtual void Slide(double unused, double amount);           /* 40ff30 */
    virtual void Elevate(double amount);                        /* 40fea0 */
    virtual void PollMouse();                                   /* 4103b0 */
    virtual void Unused() {}                                    /* 4103f0 */
    virtual void FollowLink(int x, int y);                      /* 410400 */
    virtual void DragMove(int x, int y);                        /* 4104f0 */
    virtual void DragRotate(int x, int y);                      /* 4105d0 */
    virtual void BeginDrag(int x, int y);                       /* 410720 */
    virtual void EndDrag();                                     /* 4107f0 */

    void LoadWorld(const char *file);                           /* 40efd0 */
    void ClearScene();                                          /* 40f050 */
    void ResetCamera();                                         /* 40fab0 */

    RLDevice GetDevice() { return device; }
    RLFrame GetSelection() { return selected; }

private:
    void SetupScene(RLFrame scene);                             /* 40ef30 */

    int lastX, lastY;       /* +18 */
    int width, height;      /* +20 */
    RLFrame scene;          /* +28 */
    RLFrame camera;         /* +2c */
    RLDevice device;        /* +30 */
    RLViewport viewport;    /* +34 */
    int unused38;           /* +38 */
    RLFrame extraFrame;     /* +3c */
    RLFrame selected;       /* +40 */
    WcCamera *view;         /* +44 */
    RLLight ambient;
    RLVisual marker;        /* (new) the selection marker */
};

/* World building (0x40f220-0x40fab0) */
RLFrame LabBuildWcWorld(RLFrame scene, const char *file);       /* 40f220 */
RLMesh LabBuildObject(WcElement *element);                      /* 40f6b0 */
void LabBuildPoly(WcPoly *poly, RLMesh mesh);                   /* 40f4f0 */

/* Selection marker geometry (0x410840-0x411490) */
void LabAddStrut(RLMesh mesh, double radius, float x1, float y1, float z1,
                 float x2, float y2, float z2);                 /* 410840 */
void LabAddCone(RLMesh mesh, double radius, float x1, float y1, float z1,
                float x2, float y2, float z2);                  /* 410c20 */
RLMesh LabCreateSelectionMarker(RLBox *box);                    /* 410fe0 */

/* Texture loading (gif.cpp, 0x411920) */
RLTexture LabLoadGIF(const char *file);                         /* 411920 */

/* VRML via QvLib (qvglue.cpp, 0x415f70) */
int LabLoadVRML(RLFrame scene, const char *file);               /* 415f70 */

#endif /* DRAWWIN_H */
