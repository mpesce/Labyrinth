/*
 * labwin.h -- Labyrinth's small portable window framework.
 *
 * Reconstructed from LABYRINTH.EXE (code 0x4228e0-0x424590, vtables at
 * 0x4b17c8, 0x4b17f8, 0x4b1910, 0x4b1a50, 0x4b18e0, 0x4b1a20, 0x4b1b38).
 *
 * The framework keeps the application logic away from Windows: a LabWindow
 * only talks to its "peer", which owns the HWND, colours and cursor and turns
 * Windows messages into calls on the window's virtual event handlers.  The
 * virtual functions are declared in their original vtable order.
 */
#ifndef LABWIN_H
#define LABWIN_H

#include <windows.h>
#include "lablist.h"

class LabWindow;
class LabApp;

/* The link by which a window sits on its owner's list.  (The original
 * embedded a bare LabLink at offset 4 and subtracted 4 to get back to the
 * window; here the link records its owner explicitly.) */
class LabWindowLink : public LabLink {
public:
    LabWindowLink(LabWindow *w) : owner(w) {}
    LabWindow *owner;
};

/* Cursor shapes for LabWindow::SetCursor */
enum {
    LabCursorArrow = 0,
    LabCursorWait = 1,
    LabCursorMove = 2,
    LabCursorLink = 3,          /* resource cursor 100 */
    LabCursorRotate = 4         /* resource cursor 101 */
};

/* Modifier flags passed to the mouse and key handlers */
enum { LabModShift = 1, LabModControl = 2 };

/* Handed to OnPaint while a WM_PAINT is being processed */
struct LabPaint {
    PAINTSTRUCT *ps;
    HDC hdc;
};

/* A plain rectangle: origin and size */
struct LabRect {
    int x, y, w, h;
};

/* The operating-system specific half of the application: just the
 * instance handle on Windows. */
class LabPlatform {
public:
    LabPlatform();                                              /* 422af0 */
    HINSTANCE instance;
};

class LabApp {
public:
    LabApp();                                                   /* 4228e0 */
    virtual BOOL MessagePending();                              /* 4229b0 */
    virtual void DispatchOne();                                 /* 4229d0 */
    virtual void Exit();                                        /* 422a10 */
    virtual int DoDialog(LabWindow *owner, int id);             /* 422a20 */
    virtual int Alert(LabWindow *owner, const char *text);      /* 422a60 */
    virtual int Prompt(LabWindow *owner, const char *prompt,
                       char *buf, int len);                     /* 422a90 */
    virtual void AddWindow(LabWindow *w);                       /* 423250 */
    virtual LabWindow *FirstWindow();                           /* 423360 */
    virtual LabWindow *LastWindow();                            /* 423380 */
    virtual LabWindow *NextWindow(LabWindow *w);                /* 4233a0 */
    virtual LabWindow *PrevWindow(LabWindow *w);                /* 4233f0 */

    LabPlatform *GetPlatform() { return platform; }             /* 422ae0 */
    BOOL GetOpenFileName(LabWindow *owner, char *file);         /* 423f00 */
    BOOL GetSaveFileName(LabWindow *owner, char *file);         /* 424000 */

private:
    LabPlatform *platform;
    LabList *windows;
};

/* Converts MK_ flags to LabMod flags (422b00) */
int LabModifiers(WPARAM keys);

class LabPeer {
public:
    LabPeer();                                                  /* 423440 */
    virtual ~LabPeer() {}                                       /* 423480 */
    virtual LRESULT HandleMessage(LabWindow *w, HWND hwnd, UINT msg,
                                  WPARAM wp, LPARAM lp);        /* 423490 */
    virtual HMENU GetSubMenu(HMENU menu, int pos) { return 0; }

    PAINTSTRUCT *ps;        /* +04  valid during WM_PAINT */
    HDC hdc;                /* +08 */
    HWND hwnd;              /* +0c */
    COLORREF foreground;    /* +10 */
    COLORREF background;    /* +14 */
    HCURSOR cursor;         /* +18 */
    int destroyed;          /* +1c */
    int menuPopup;          /* +20  popup most recently opened, or -1 */
};

class LabWindow {
public:
    LabWindow();                                                /* 422b20 */
    virtual ~LabWindow();                                       /* 422b40 */

    /* event handlers (vtable slots 0-11) */
    virtual void OnShow(int show) {}
    virtual void OnPaint(LabPaint *p) {}
    virtual void OnResize(int w, int h) {}
    virtual void OnKey(int key, int mods) {}
    virtual void OnMouseDown(int x, int y, int mods) {}
    virtual void OnDoubleClick(int x, int y, int mods) {}
    virtual void OnMouseUp(int x, int y, int mods) {}
    virtual void OnMouseMove(int x, int y, int mods) {}
    virtual void OnDestroy() {}
    virtual void OnCommand(int menu, int id) {}
    virtual void OnMenuInit(int menu) {}
    virtual void OnNotify(LabWindow *from, int code, int arg, void *data) {}

    /* geometry and visibility (12-20) */
    virtual void Move(int x, int y);                            /* 422bb0 */
    virtual void Resize(int w, int h);                          /* 422c20 */
    virtual int GetX();                                         /* 422c90 */
    virtual int GetY();                                         /* 422ce0 */
    virtual int GetWidth();                                     /* 422d30 */
    virtual int GetHeight();                                    /* 422d90 */
    virtual void Show();                                        /* 422df0 */
    virtual void Hide();                                        /* 422e00 */
    virtual void Close();                                       /* 422e10 */

    /* input (21-23) */
    virtual BOOL HasCapture();                                  /* 422e30 */
    virtual void GetMouse(int *x, int *y, int *mods);           /* 422e50 */
    virtual void SetCursor(int shape);                          /* 422ed0 */

    /* drawing (24-29) */
    virtual void SetForeground(const char *colour);             /* 422f40 */
    virtual void DrawLine(int x1, int y1, int x2, int y2);      /* 422fd0 */
    virtual void DrawBevel(LabRect *r);                         /* 423050 */
    virtual void DrawText(int x, int y, const char *s, LabRect *clip); /* 423170 */
    virtual void ClearRect(LabRect *r);                         /* 4232e0 */
    virtual void Flush() {}                                     /* 4233d0 */

    /* hierarchy (30-36) */
    virtual LabWindow *GetParent() { return parent; }           /* 40ee50 */
    virtual void AddChild(LabWindow *w);                        /* 40ee60 */
    virtual LabWindow *FirstChild();                            /* 40eef0 */
    virtual LabWindow *LastChild();                             /* 40ef10 */
    virtual LabWindow *NextChild(LabWindow *w);                 /* 40efa0 */
    virtual LabWindow *PrevChild(LabWindow *w);                 /* 40f020 */
    virtual LabApp *GetApp();                                   /* 423420 */

    LabPeer *GetPeer() { return peer; }                         /* 4233e0 */
    HWND GetHWND() { return peer ? peer->hwnd : 0; }

    /* (new) Labyrinth is per-monitor DPI aware: windows are measured in the
     * display's pixels, and layout sizes given at 96 DPI go through Scale. */
    int GetDpi();
    int Scale(int n) { return MulDiv(n, GetDpi(), 96); }

    static LabWindow *FromLink(LabLink *l)
        { return l ? ((LabWindowLink *)l)->owner : 0; }

    LabWindowLink link;     /* +04 */
protected:
    LabPeer *peer;          /* +0c */
    LabWindow *parent;      /* +10 */
    LabList *children;      /* +14 */
};

/* A top-level window with a frame, a menu bar and drag-and-drop. */
class LabShellWindow : public LabWindow {
public:
    LabShellWindow(LabApp *app, const char *title,
                   int x, int y, int w, int h);                 /* 4238d0 */
    ~LabShellWindow();                                          /* 4239c0 */

    virtual LabApp *GetApp() { return app; }                    /* 423a40 */

    virtual void OnActivate(int active) {}                      /* 423980 */
    virtual void OnClose() {}                                   /* 423990 */
    virtual void OnDropFile(const char *file) {}                /* 4239b0 */
    virtual BOOL SetMenu(int id);                               /* 423a50 */
    virtual void EnableMenuItem(int id, int enable);            /* 423aa0 */
    virtual int IsMenuItemEnabled(int id);                      /* 423ad0 */
    virtual void EnableSubMenuItem(int menu, int id, int enable); /* 423af0 */
    virtual int IsSubMenuItemEnabled(int menu, int id);         /* 423b30 */
    virtual void CheckSubMenuItem(int menu, int id, int check); /* 423b70 */
    virtual int IsSubMenuItemChecked(int menu, int id);         /* 423bb0 */
    void CheckSubMenuRadio(int menu, int first, int last, int id); /* (new) */

protected:
    LabApp *app;            /* +18 */
};

/* A child window drawn inside another window ("LABWIN_DRAWING_WINDOW"). */
class LabChildWindow : public LabWindow {
public:
    enum { Visible = 0x40, Border = 0x80 };
    LabChildWindow(LabWindow *parent, int x, int y, int w, int h,
                   int style);                                  /* 424100 */
    ~LabChildWindow();                                          /* 4241b0 */
};

/* Run the dialog procedures as the original did */
INT_PTR CALLBACK LabAboutProc(HWND, UINT, WPARAM, LPARAM);      /* 424470 */
INT_PTR CALLBACK LabPromptProc(HWND, UINT, WPARAM, LPARAM);     /* 4244e0 */

extern HINSTANCE labInstance;                                   /* 4b99f0 */

#endif /* LABWIN_H */
