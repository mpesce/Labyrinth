/*
 * mainwin.cpp -- Labyrinth's main window and control panels.
 * Reconstructed from LABYRINTH.EXE 0x40e120-0x40ec00 and 0x411490-0x411890.
 */
#include <stdio.h>
#include <string.h>
#include "mainwin.h"
#include "drawwin.h"
#include "labwww.h"
#include "resource.h"
#include "../rl/rl.h"

/* Rendering quality chosen from the View menu or with the R key (4b73d8) */
static int labQuality = RLRenderGouraud;

/* (new) the View menu marks the quality in use with a radio bullet */
static const int viewMenu = 2;          /* position of the View menu */

static void showQuality(LabShellWindow *w)
{
    int id = IDM_GOURAUD;
    switch (labQuality) {
    case RLRenderWireframe: id = IDM_WIREFRAME; break;
    case RLRenderFlat: id = IDM_FLAT; break;
    case RLRenderPhong: id = IDM_PHONG; break;
    }
    w->CheckSubMenuRadio(viewMenu, IDM_WIREFRAME, IDM_PHONG, id);
}

/* Sample worlds on the HotSpots menu and documents on the Help menu */
/* (new) the 1995 Help documents at eit.com and net.org are gone; these are
 * their counterparts at hyperreal.org: the June 1994 VRML 1.3.1 working
 * specification, the EIT introduction to VRML, and the Labyrinth * MOTIF
 * user's guide, whose last section covers driving */
static const char urlVrmlSpec[] = "http://hyperreal.org/~mpesce/script.html";
static const char urlHowTo[] = "http://hyperreal.org/~mpesce/eitvrml.html";
static const char urlNavigate[] = "http://hyperreal.org/~mpesce/labnav.html";
static const char urlWaxWeb[] = "http://bug.village.virginia.edu/vrml/";
/* (new) the 1995 address, http://www.eit.com/~mark/ushm/ushm.wrl, is gone;
 * the world and its shape files live on at hyperreal.org */
static const char urlUSHM[] = "http://hyperreal.org/~mpesce/ushm/ushm.wrl";
static const char urlSOMA[] = "http://www.eit.com/~mark/soma/soma.wrl";
static const char urlIUMA[] = "http://www.iuma.com/vrml/";
/* (new) more worlds from hyperreal.org, the first being the first VRML world */
static const char urlBanana[] = "http://hyperreal.org/~mpesce/banana.wrl";
static const char urlSamhain[] = "http://hyperreal.org/~mpesce/samhain/wrl/samhain.wrl";
static const char urlTardis[] = "http://hyperreal.org/~mpesce/ushm/doctor.wrl";
static const char urlMall[] = "http://hyperreal.org/~mpesce/mall/GBC.wrl";
static const char urlWebEarth[] = "http://hyperreal.org/~mpesce/wearth.wrl";
static const char urlCoven[] = "http://hyperreal.org/~mpesce/coven.wrl";

/* ---- LabMainWindow ------------------------------------------------------ */

LabMainWindow::LabMainWindow(LabApp *app, const char *title,
                             int x, int y, int w, int h)        /* 40e120 */
    : LabShellWindow(app, title, x, y, w, h), view(0), info(0), nav(0)
{
    /* (new) w and h are at 96 DPI; the layout is in the display's pixels */
    w = Scale(w);
    h = Scale(h);
    view = new LabDrawingWindow(this, 0, 0, w - Scale(80), h - Scale(30));
    info = new LabInfoPanel(this, w - Scale(80), 0, Scale(80), h - Scale(110));
    nav = new LabNavPanel(this, w - Scale(80), h - Scale(110), Scale(80), Scale(80));
    unused = 0;
    statusRect.x = Scale(8);
    statusRect.y = h - Scale(27);
    statusRect.w = w - Scale(16);
    statusRect.h = Scale(26);
    status[0] = '\0';
    SetMenu(IDM_LABYRINTH);
    showQuality(this);
}

void LabMainWindow::OnPaint(LabPaint *p)                        /* 40e2b0 */
{
    DrawStatus(0);
}

/* (new) the sizes are scaled for the display */
void LabMainWindow::OnResize(int w, int h)                      /* 40e2e0 */
{
    int panel = Scale(80);
    if (view)
        view->Resize(w - panel, h - Scale(30));
    if (info) {
        info->Move(w - panel, 0);
        /* (fixed) the original passed w-80 for the panel's width */
        info->Resize(panel, h - Scale(110));
    }
    if (nav) {
        nav->Move(w - panel, h - Scale(110));
        nav->Resize(panel, panel);
    }
    statusRect.y = h - Scale(27);
    statusRect.x = Scale(8);
    statusRect.h = Scale(26);
    statusRect.w = w - Scale(16);
}

void LabMainWindow::OnKey(int key, int mods)                    /* 40e380 */
{
    switch (key) {
    case 'H':
    case 'h':
        if (view)
            view->ResetCamera();
        break;
    case 'R':
    case 'r':
        switch (labQuality) {
        case RLRenderWireframe: labQuality = RLRenderFlat; break;
        case RLRenderFlat: labQuality = RLRenderGouraud; break;
        case RLRenderGouraud: labQuality = RLRenderPhong; break;
        case RLRenderPhong: labQuality = RLRenderWireframe; break;
        }
        RLDeviceSetQuality(view->GetDevice(), (RLRenderQuality)labQuality);
        showQuality(this);
        break;
    }
}

void LabMainWindow::OnDestroy()                                 /* 40e370 */
{
    app->Exit();
}

void LabMainWindow::OnClose()                                   /* 40e360 */
{
    Close();
}

void LabMainWindow::OnActivate(int active)                      /* 40e2c0 */
{
    RLWinHandleActivate(view->GetDevice(), (short)active);
}

/* Fetch a world from the Web and show it. */
static void viewRemoteWorld(LabDrawingWindow *view, const char *url)
{
    view->ClearScene();
    LabFetchWorld(url);
    view->LoadWorld(LAB_WORLD_FILE);
}

void LabMainWindow::OnCommand(int menu, int id)                 /* 40e460 */
{
    char file[256];
    char url[256];
    char msg[200];
    char *name;

    switch (id) {
    case IDM_EXIT:
        Close();
        break;

    case IDM_OPEN_URL:
        if (GetApp()->Prompt(this, "Enter URL:", url, 255)) {
            view->ClearScene();
            name = url;
            if (strstr(url, ":")) {
                LabFetchWorld(url);
                name = LAB_WORLD_FILE;
            } else {
                LabSetBaseFile(url);
            }
            view->LoadWorld(name);
            sprintf(msg, "Viewing world %s.", url);
            SetStatus(msg);
        }
        break;

    case IDM_OPEN_LOCAL:
        if (GetApp()->GetOpenFileName(this, file)) {
            sprintf(msg, "Viewing world %s.", file);
            SetStatus(msg);
            SetCursor(LabCursorWait);
            view->ClearScene();
            LabSetBaseFile(file);
            view->LoadWorld(file);
            SetCursor(LabCursorArrow);
        }
        break;

    case IDM_SAVE_CAMERA:
        if (GetApp()->GetSaveFileName(this, file))
            view->SaveCamera(file);
        break;

    case IDM_CUT:
    case IDM_COPY:
    case IDM_PASTE:
    case IDM_DELETE:
        break;

    case IDM_WIREFRAME:
        labQuality = RLRenderWireframe;
        RLDeviceSetQuality(view->GetDevice(), RLRenderWireframe);
        showQuality(this);
        break;
    case IDM_FLAT:
        labQuality = RLRenderFlat;
        RLDeviceSetQuality(view->GetDevice(), RLRenderFlat);
        showQuality(this);
        break;
    case IDM_GOURAUD:
        labQuality = RLRenderGouraud;
        RLDeviceSetQuality(view->GetDevice(), RLRenderGouraud);
        showQuality(this);
        break;
    case IDM_PHONG:
        /* drawn as Gouraud until the renderer lights per pixel */
        labQuality = RLRenderPhong;
        RLDeviceSetQuality(view->GetDevice(), RLRenderPhong);
        showQuality(this);
        break;
    case IDM_RESTORE:
        if (view)
            view->ResetCamera();
        break;
    case IDM_DITHER:
        /* greyed on the menu (new): there is nothing to dither in true colour */
        RLDeviceSetDither(view->GetDevice(), !RLDeviceGetDither(view->GetDevice()));
        CheckSubMenuItem(viewMenu, IDM_DITHER, RLDeviceGetDither(view->GetDevice()));
        break;

    case IDM_BANANA:
        viewRemoteWorld(view, urlBanana);
        break;
    case IDM_SOMA:
        viewRemoteWorld(view, urlSOMA);
        break;
    case IDM_USHM:
        viewRemoteWorld(view, urlUSHM);
        break;
    case IDM_WEARTH:
        viewRemoteWorld(view, urlWebEarth);
        break;
    case IDM_WAXWEB:
        viewRemoteWorld(view, urlWaxWeb);
        break;
    case IDM_IUMA:
        viewRemoteWorld(view, urlIUMA);
        break;
    case IDM_SAMHAIN:
        viewRemoteWorld(view, urlSamhain);
        break;
    case IDM_TARDIS:
        viewRemoteWorld(view, urlTardis);
        break;
    case IDM_MALL:
        viewRemoteWorld(view, urlMall);
        break;
    case IDM_COVEN:
        viewRemoteWorld(view, urlCoven);
        break;

    case IDM_HELP_NAVIGATION:
        LabLaunchBrowser(urlNavigate);
        break;
    case IDM_HELP_CREATING:
        LabLaunchBrowser(urlHowTo);
        break;
    case IDM_HELP_SCRIPTING:
        LabLaunchBrowser(urlVrmlSpec);
        break;
    case IDM_ABOUT:
        GetApp()->DoDialog(this, IDD_ABOUT);
        break;
    }
}

void LabMainWindow::OnMenuInit(int menu)                        /* 40e930 */
{
    /* Cut, Copy and Delete are available while an object is selected */
    if (menu == 1) {
        int selected = view->GetSelection() != 0;
        EnableSubMenuItem(1, IDM_CUT, selected);
        EnableSubMenuItem(1, IDM_COPY, selected);
        EnableSubMenuItem(1, IDM_DELETE, selected);
    }
}

void LabMainWindow::OnNotify(LabWindow *from, int code, int arg, void *data) /* 40e9f0 */
{
    double *d = (double *)data;
    if (code != LabNotifyNavigate)
        return;
    switch (arg) {
    case LabNavTurnMove:
        view->Navigate(d[0], d[1]);
        break;
    case LabNavSlide:
        view->Slide(d[0], d[1]);
        break;
    case LabNavElevate:
        view->Elevate(d[1]);
        break;
    }
}

void LabMainWindow::OnDropFile(const char *file)                /* 40ead0 */
{
    char msg[200];
    sprintf(msg, "Viewing world %s.", file);
    SetStatus(msg);
    SetCursor(LabCursorWait);
    view->ClearScene();
    LabSetBaseFile(file);
    view->LoadWorld(file);
    SetCursor(LabCursorArrow);
}

void LabMainWindow::Idle()                                      /* 40eb30 */
{
    view->PollMouse();
    nav->Idle();
}

void LabMainWindow::SetStatus(const char *text)                 /* 40eb50 */
{
    int n;
    if (text == 0 || (n = (int)strlen(text)) == 0) {
        status[0] = '\0';
        return;
    }
    if (n > 511)
        n = 511;
    if (strncmp(status, text, n) != 0) {
        strncpy(status, text, n);
        status[n] = '\0';
        DrawStatus(1);
    }
}

const char *LabMainWindow::GetStatus()                          /* 40ebd0 */
{
    return status[0] ? status : 0;
}

void LabMainWindow::DrawStatus(int erase)                       /* 40ebf0 */
{
    /* (new) the insets are scaled for the display */
    LabRect inner = statusRect;
    inner.x += Scale(3);
    inner.y += Scale(2);
    inner.w -= Scale(6);
    inner.h -= Scale(4);
    DrawBevel(&statusRect);
    if (erase)
        ClearRect(&inner);
    DrawText(statusRect.x + Scale(6), statusRect.y + Scale(2), status, &inner);
}

/* ---- panels ------------------------------------------------------------- */

LabInfoPanel::LabInfoPanel(LabWindow *parent, int x, int y, int w, int h) /* 411490 */
    : LabChildWindow(parent, x, y, w, h, Visible | Border)
{
    unused[0] = unused[1] = 0;
}

LabNavPanel::LabNavPanel(LabWindow *parent, int x, int y, int w, int h) /* 411530 */
    : LabChildWindow(parent, x, y, w, h, Visible | Border)
{
    mouseX = mouseY = 0;
}

void LabNavPanel::OnPaint(LabPaint *p)                          /* 4115b0 */
{
    SetForeground("black");
    int cx = GetWidth() / 2;
    int cy = GetHeight() / 2;
    int arm = Scale(10);                /* (new) scaled */
    DrawLine(cx, cy - arm, cx, cy + arm);
    DrawLine(cx - arm, cy, cx + arm, cy);
    Flush();
}

void LabNavPanel::OnResize(int w, int h)                        /* 411610 */
{
    OnPaint(0);
}

void LabNavPanel::OnMouseDown(int x, int y, int mods)           /* 411620 */
{
    mouseX = x;
    mouseY = y;
    if (mods == 0)
        TurnAndMove(x, y);
    else if (mods == LabModShift)
        Slide(x, y);
}

void LabNavPanel::OnMouseMove(int x, int y, int mods)           /* 411680 */
{
    if (!HasCapture())
        return;
    if (mods == 0)
        TurnAndMove(x, y);
    else if (mods == LabModShift)
        Slide(x, y);
}

/* Keep moving while the button is held down. */
void LabNavPanel::Idle()                                        /* 4116d0 */
{
    int x, y, mods;
    if (HasCapture()) {
        GetMouse(&x, &y, &mods);
        OnMouseMove(x, y, mods);
    }
}

void LabNavPanel::TurnAndMove(int x, int y)                     /* 411800 */
{
    int w = GetWidth();
    int h = GetHeight();
    if (x >= 0 && x <= w && y >= 0 && y <= h) {
        double d[2];
        d[0] = (double)(w / 2 - x) / ((double)w * 0.25);   /* turn */
        d[1] = (double)(h / 2 - y) / ((double)w * 0.25);   /* move */
        parent->OnNotify(this, LabNotifyNavigate, LabNavTurnMove, d);
    }
}

void LabNavPanel::Slide(int x, int y)                           /* 411710 */
{
    int w = GetWidth();
    if (x >= 0 && x <= w) {
        int h = GetHeight();
        if (y >= 0 && y <= h) {
            double d[2];
            d[1] = (double)(h / 2 - y) / ((double)h * 0.25);
            /* the original divided by an uninitialised variable here; the
             * value is not used by LabDrawingWindow::Slide */
            d[0] = (double)(w / 2 - x) / ((double)w * 0.25);
            parent->OnNotify(this, LabNotifyNavigate, LabNavSlide, d);
        }
    }
}

void LabNavPanel::Elevate(int x, int y)                         /* 4117a0 */
{
    int h = GetHeight();
    if (y >= 0 && y <= h) {
        double d[2];
        d[0] = 0.0;
        d[1] = (double)(h / 2 - y) / ((double)h * 0.25);
        parent->OnNotify(this, LabNotifyNavigate, LabNavElevate, d);
    }
}
