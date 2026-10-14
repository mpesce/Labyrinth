/*
 * mainwin.h -- Labyrinth's main window and its control panels.
 * Reconstructed from LABYRINTH.EXE (LabMainWindow vtable 0x4ae298,
 * panels 0x4ae548 and 0x4ae608).
 *
 *   +-----------------------------------+-------+
 *   |                                   | info  |
 *   |        drawing window (3D)        | panel |
 *   |                                   +-------+
 *   |                                   |  nav  |
 *   +-----------------------------------+-------+
 *   | status line                               |
 *   +-------------------------------------------+
 */
#ifndef MAINWIN_H
#define MAINWIN_H

#include "labwin.h"

class LabDrawingWindow;

/* The empty panel at the top right (0x4ae548). */
class LabInfoPanel : public LabChildWindow {
public:
    LabInfoPanel(LabWindow *parent, int x, int y, int w, int h); /* 411490 */
    void OnPaint(LabPaint *p) {}                                /* 411510 */
    void OnResize(int w, int h) {}                              /* 411520 */
private:
    int unused[2];
};

/* Notification sent to the parent by the navigation panel */
enum { LabNotifyNavigate = 0x11 };
enum { LabNavTurnMove = 1, LabNavSlide = 2, LabNavElevate = 3 };

/* The navigation pad at the bottom right (0x4ae608): press and hold the
 * mouse to move -- the distance from the centre gives the speed. */
class LabNavPanel : public LabChildWindow {
public:
    LabNavPanel(LabWindow *parent, int x, int y, int w, int h);  /* 411530 */
    void OnPaint(LabPaint *p);                                  /* 4115b0 */
    void OnResize(int w, int h);                                /* 411610 */
    void OnMouseDown(int x, int y, int mods);                   /* 411620 */
    void OnDoubleClick(int x, int y, int mods) {}               /* 411660 */
    void OnMouseUp(int x, int y, int mods) {}                   /* 411670 */
    void OnMouseMove(int x, int y, int mods);                   /* 411680 */

    virtual void Idle();                                        /* 4116d0 */
    virtual void TurnAndMove(int x, int y);                     /* 411800 */
    virtual void Slide(int x, int y);                           /* 411710 */
    virtual void Elevate(int x, int y);                         /* 4117a0 */
private:
    int mouseX, mouseY;     /* +18 */
};

class LabMainWindow : public LabShellWindow {
public:
    LabMainWindow(LabApp *app, const char *title,
                  int x, int y, int w, int h);                  /* 40e120 */

    void OnPaint(LabPaint *p);                                  /* 40e2b0 */
    void OnResize(int w, int h);                                /* 40e2e0 */
    void OnKey(int key, int mods);                              /* 40e380 */
    void OnDestroy();                                           /* 40e370 */
    void OnCommand(int menu, int id);                           /* 40e460 */
    void OnMenuInit(int menu);                                  /* 40e930 */
    void OnNotify(LabWindow *from, int code, int arg, void *data); /* 40e9f0 */

    void OnActivate(int active);                                /* 40e2c0 */
    void OnClose();                                             /* 40e360 */
    void OnDropFile(const char *file);                          /* 40ead0 */

    virtual void Idle();                                        /* 40eb30 */
    virtual void SetStatus(const char *text);                   /* 40eb50 */
    virtual const char *GetStatus();                            /* 40ebd0 */

    LabDrawingWindow *GetView() { return view; }

private:
    void DrawStatus(int erase);                                 /* 40ebf0 */

    LabDrawingWindow *view; /* +1c */
    LabInfoPanel *info;     /* +20 */
    LabNavPanel *nav;       /* +24 */
    int unused;             /* +28 */
    LabRect statusRect;     /* +2c */
    char status[512];       /* +3c */
};

extern LabApp *labApp;
extern LabMainWindow *labMainWindow;

#endif /* MAINWIN_H */
