/*
 * labwin.cpp -- Labyrinth's portable window framework, Windows half.
 * Reconstructed from LABYRINTH.EXE 0x4228e0-0x424590.
 */
#include <string.h>
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include "labwin.h"
#include "resource.h"

extern "C" void LabError(const char *fmt, ...);
extern "C" void LabInitWWW(void);

HINSTANCE labInstance;                                          /* 4b99f0 */

static HCURSOR cursorArrow;                                     /* 4b99f4 */
static HCURSOR cursorWait;                                      /* 4b99f8 */
static HCURSOR cursorMove;                                      /* 4b99fc */
static HCURSOR cursorLink;                                      /* 4b9a00 */
static HCURSOR cursorRotate;                                    /* 4b9a04 */
static int shellClassRegistered;                                /* 4b9a08 */
static int childClassRegistered;                                /* 4b9a0c */

static const char shellClass[] = "InterSpace";
static const char childClass[] = "LABWIN_DRAWING_WINDOW";

static LRESULT CALLBACK LabWndProc(HWND, UINT, WPARAM, LPARAM);

/* ---- LabPlatform / LabApp ---------------------------------------------- */

LabPlatform::LabPlatform()                                      /* 422af0 */
{
    instance = labInstance;
}

LabApp::LabApp()                                                /* 4228e0 */
{
    /* The application object brings up the WWW library as it starts. */
    LabInitWWW();
    platform = new LabPlatform;
    windows = 0;
    if (!cursorArrow)
        cursorArrow = LoadCursor(NULL, IDC_ARROW);
    if (!cursorWait)
        cursorWait = LoadCursor(NULL, IDC_WAIT);
    if (!cursorMove)
        cursorMove = LoadCursor(NULL, IDC_SIZEALL);   /* IDC_SIZE originally */
    if (!cursorLink)
        cursorLink = LoadCursor(platform->instance, MAKEINTRESOURCE(IDC_LABLINK));
    if (!cursorRotate)
        cursorRotate = LoadCursor(platform->instance, MAKEINTRESOURCE(IDC_LABROTATE));
}

BOOL LabApp::MessagePending()                                   /* 4229b0 */
{
    MSG msg;
    return PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
}

/*
 * The original took one message with GetMessage.  In a loop that renders
 * between messages, a blocking GetMessage stalls the picture whenever
 * PeekMessage has reported something GetMessage then waits for, so every
 * waiting message is now handled without ever blocking.
 */
void LabApp::DispatchOne()                                      /* 4229d0 */
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            Exit();         /* the original never posted one */
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void LabApp::Exit()                                             /* 422a10 */
{
    exit(0);
}

int LabApp::DoDialog(LabWindow *owner, int id)                  /* 422a20 */
{
    return (int)DialogBoxParam(platform->instance, MAKEINTRESOURCE(id),
                               owner->GetPeer()->hwnd, LabAboutProc, 0);
}

int LabApp::Alert(LabWindow *owner, const char *text)           /* 422a60 */
{
    MessageBox(owner ? owner->GetPeer()->hwnd : NULL, text, "Labyrinth", MB_OK);
    return 1;
}

static const char *promptText;                                  /* 4a8fb8 */
static char *promptBuf;                                         /* 4a8fb0 */
static int promptLen;                                           /* 4a8fb4 */

int LabApp::Prompt(LabWindow *owner, const char *prompt, char *buf, int len) /* 422a90 */
{
    promptText = prompt;
    promptBuf = buf;
    promptLen = len;
    return (int)DialogBoxParam(platform->instance, MAKEINTRESOURCE(IDD_PROMPT),
                               owner->GetPeer()->hwnd, LabPromptProc, 0);
}

void LabApp::AddWindow(LabWindow *w)                            /* 423250 */
{
    if (!windows)
        windows = new LabList;
    windows->Append(w ? &w->link : 0);
}

LabWindow *LabApp::FirstWindow()                                /* 423360 */
{
    return windows ? LabWindow::FromLink(windows->First()) : 0;
}

LabWindow *LabApp::LastWindow()                                 /* 423380 */
{
    return windows ? LabWindow::FromLink(windows->Last()) : 0;
}

LabWindow *LabApp::NextWindow(LabWindow *w)                     /* 4233a0 */
{
    return windows ? LabWindow::FromLink(LabList::Next(&w->link)) : 0;
}

LabWindow *LabApp::PrevWindow(LabWindow *w)                     /* 4233f0 */
{
    return windows ? LabWindow::FromLink(LabList::Prev(&w->link)) : 0;
}

/* Common setup of the two file dialogs. */
static void initOpenFileName(OPENFILENAME *ofn, LabWindow *owner, char *file,
                             char *filter, char *title)
{
    memset(ofn, 0, sizeof(*ofn));
    ofn->lStructSize = sizeof(*ofn);
    ofn->hwndOwner = owner->GetPeer()->hwnd;
    ofn->lpstrFilter = filter;
    ofn->lpstrFile = file;
    ofn->lpstrFileTitle = title;
    ofn->nMaxFileTitle = 0;
    ofn->nFilterIndex = 1;
    ofn->nMaxFile = 256;
    ofn->lpstrDefExt = "*.wrl";         /* sic */
}

static char wrlFilter[] = "VRML Files (*.wrl)\0*.WRL\0All Files (*.*)\0*.*\0";

BOOL LabApp::GetOpenFileName(LabWindow *owner, char *file)      /* 423f00 */
{
    OPENFILENAME ofn;
    char title[256];
    file[0] = '\0';
    title[0] = '\0';
    initOpenFileName(&ofn, owner, file, wrlFilter, title);
    ofn.lpstrTitle = "Open a World File";
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    return ::GetOpenFileName(&ofn);
}

BOOL LabApp::GetSaveFileName(LabWindow *owner, char *file)      /* 424000 */
{
    OPENFILENAME ofn;
    char title[256];
    file[0] = '\0';
    title[0] = '\0';
    initOpenFileName(&ofn, owner, file, wrlFilter, title);
    ofn.lpstrTitle = "Save a World File";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    return ::GetSaveFileName(&ofn);
}

int LabModifiers(WPARAM keys)                                   /* 422b00 */
{
    int mods = 0;
    if (keys & MK_CONTROL)
        mods = LabModControl;
    if (keys & MK_SHIFT)
        mods |= LabModShift;
    return mods;
}

/* ---- LabPeer: message dispatch ----------------------------------------- */

LabPeer::LabPeer()                                              /* 423440 */
{
    ps = 0;
    hdc = 0;
    hwnd = 0;
    cursor = cursorArrow;
    foreground = 0;
    background = GetSysColor(COLOR_WINDOW);
    destroyed = 0;
    menuPopup = -1;
}

LRESULT LabPeer::HandleMessage(LabWindow *w, HWND hwnd, UINT msg,
                               WPARAM wp, LPARAM lp)           /* 423490 */
{
    switch (msg) {
    case WM_SIZE:
        if (wp == SIZE_MAXIMIZED || wp == SIZE_RESTORED)
            w->OnResize(LOWORD(lp), HIWORD(lp));
        break;

    case WM_DESTROY:
        destroyed = 1;
        w->OnDestroy();
        break;

    case WM_PAINT: {
        PAINTSTRUCT paint;
        LabPaint p;
        hdc = BeginPaint(hwnd, &paint);
        this->ps = &paint;
        p.ps = &paint;
        p.hdc = hdc;
        w->OnPaint(&p);
        EndPaint(hwnd, &paint);
        this->ps = 0;
        hdc = 0;
        break;
    }

    case WM_SETFOCUS:
        break;

    case WM_SHOWWINDOW:
        if (wp && lp == SW_PARENTOPENING)
            w->OnShow(1);
        break;

    case WM_KEYDOWN:
        /* the virtual key is also run through the MK_ conversion, as the
         * original did */
        w->OnKey((int)(wp & 0xff), LabModifiers(wp));
        break;

    case WM_SETCURSOR:
        if (cursor == 0 || LOWORD(lp) != HTCLIENT)
            return DefWindowProc(hwnd, msg, wp, lp);
        ::SetCursor(cursor);
        return TRUE;

    case WM_COMMAND:
        if (lp == 0) {
            if (menuPopup != -1)
                w->OnCommand(menuPopup, LOWORD(wp));
            menuPopup = -1;
        }
        break;

    case WM_HSCROLL:
    case WM_VSCROLL:
    case WM_MENUSELECT:
        break;

    case WM_INITMENU:
        menuPopup = -1;
        break;

    case WM_INITMENUPOPUP:
        if (HIWORD(lp) == 0) {
            w->OnMenuInit(LOWORD(lp));
            menuPopup = LOWORD(lp);
        }
        break;

    case WM_MOUSEMOVE:
        w->OnMouseMove((short)LOWORD(lp), (short)HIWORD(lp), LabModifiers(wp));
        break;

    case WM_LBUTTONDOWN:
        ::SetCapture(hwnd);
        w->OnMouseDown((short)LOWORD(lp), (short)HIWORD(lp), LabModifiers(wp & ~(MK_LBUTTON | MK_RBUTTON)));
        break;

    case WM_LBUTTONUP:
        ReleaseCapture();
        w->OnMouseUp((short)LOWORD(lp), (short)HIWORD(lp), LabModifiers(wp & ~(MK_LBUTTON | MK_RBUTTON)));
        break;

    case WM_LBUTTONDBLCLK:
        w->OnDoubleClick((short)LOWORD(lp), (short)HIWORD(lp), LabModifiers(wp & ~(MK_LBUTTON | MK_RBUTTON)));
        break;

    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

/* The shell's peer adds activation, closing and dropped files. */
class LabShellPeer : public LabPeer {
public:
    LabShellPeer(LabWindow *w, const char *title, int x, int y, int cx, int cy); /* 423bf0 */
    LRESULT HandleMessage(LabWindow *w, HWND hwnd, UINT msg,
                          WPARAM wp, LPARAM lp);                /* 423e20 */
    HMENU GetSubMenu(HMENU menu, int pos)                       /* 423d50 */
        { return ::GetSubMenu(menu, pos); }
    HMENU menu;             /* +24 */
};

class LabChildPeer : public LabPeer {
public:
    LabChildPeer(LabWindow *w, LabWindow *parent,
                 int x, int y, int cx, int cy, int style);      /* 424230 */
};

static void registerShellClass(HINSTANCE inst)                  /* 423d70 */
{
    WNDCLASS wc;
    if (shellClassRegistered)
        return;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.hInstance = inst;
    wc.lpfnWndProc = LabWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG_PTR);
    /* the original used the stock application icon; this one comes in
     * sizes up to 256 pixels */
    wc.hIcon = LoadIcon(inst, MAKEINTRESOURCE(IDI_LABYRINTH));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    /* (new) the window face colour; the original used the MDI workspace grey */
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = shellClass;
    if (!RegisterClass(&wc))
        LabError("Can't register shell class");
    shellClassRegistered = 1;
}

/* (new) title-bar and task-switcher icons drawn at the window's scale */
static void setIcons(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    int bigSize = GetSystemMetricsForDpi(SM_CXICON, dpi);
    int smallSize = GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    HICON b = (HICON)LoadImage(inst, MAKEINTRESOURCE(IDI_LABYRINTH), IMAGE_ICON, bigSize, bigSize, 0);
    HICON s = (HICON)LoadImage(inst, MAKEINTRESOURCE(IDI_LABYRINTH), IMAGE_ICON, smallSize, smallSize, 0);
    HICON old;
    if (b && (old = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)b)) != NULL)
        DestroyIcon(old);
    if (s && (old = (HICON)SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)s)) != NULL)
        DestroyIcon(old);
}

LabShellPeer::LabShellPeer(LabWindow *w, const char *title,
                           int x, int y, int cx, int cy)        /* 423bf0 */
{
    HINSTANCE inst = w->GetApp()->GetPlatform()->instance;
    /* (new) the size is given at 96 DPI: scale it for the display, and let
     * Windows work out the frame.  As in the original, the menu bar comes
     * out of the client area. */
    UINT dpi = GetDpiForSystem();
    RECT r = { 0, 0, MulDiv(cx, dpi, 96), MulDiv(cy, dpi, 96) };
    AdjustWindowRectExForDpi(&r, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);

    x = x == -1 ? CW_USEDEFAULT : MulDiv(x, dpi, 96);
    y = y == -1 ? CW_USEDEFAULT : MulDiv(y, dpi, 96);
    menu = 0;
    if (!shellClassRegistered)
        registerShellClass(inst);
    hwnd = CreateWindowEx(0, shellClass, title,
                          WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                          x, y, r.right - r.left, r.bottom - r.top,
                          NULL, NULL, inst, NULL);
    if (hwnd == NULL)
        LabError("Can't create shell");
    setIcons(hwnd);
    DragAcceptFiles(hwnd, TRUE);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)w);
    background = GetSysColor(COLOR_BTNFACE);
}

LRESULT LabShellPeer::HandleMessage(LabWindow *w, HWND hwnd, UINT msg,
                                    WPARAM wp, LPARAM lp)      /* 423e20 */
{
    LabShellWindow *shell = (LabShellWindow *)w;
    switch (msg) {
    case WM_ACTIVATE:
        shell->OnActivate((int)wp);
        break;
    case WM_CLOSE:
        shell->OnClose();
        break;
    case WM_DPICHANGED: {
        /* (new) moved to a display of another scale, or the scale changed:
         * take the size Windows suggests and lay the window out again */
        RECT *r = (RECT *)lp;
        SetWindowPos(hwnd, NULL, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        RECT c;
        GetClientRect(hwnd, &c);
        w->OnResize(c.right, c.bottom);
        setIcons(hwnd);
        RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
        break;
    }
    case WM_GETMINMAXINFO: {
        /* (new) keep room for the view, the panels and the status line */
        MINMAXINFO *mm = (MINMAXINFO *)lp;
        UINT dpi = GetDpiForWindow(hwnd);
        RECT r = { 0, 0, MulDiv(320, dpi, 96), MulDiv(240, dpi, 96) };
        AdjustWindowRectExForDpi(&r, WS_OVERLAPPEDWINDOW, TRUE, 0, dpi);
        mm->ptMinTrackSize.x = r.right - r.left;
        mm->ptMinTrackSize.y = r.bottom - r.top;
        break;
    }
    case WM_DROPFILES: {
        HDROP drop = (HDROP)wp;
        char file[1024];
        if (DragQueryFile(drop, 0xffffffff, NULL, 0) == 1) {
            UINT n = DragQueryFile(drop, 0, NULL, 0);
            if (n != 0 && n < sizeof(file)) {
                n = DragQueryFile(drop, 0, file, n + 1);
                file[n] = '\0';
                shell->OnDropFile(file);
            }
        }
        DragFinish(drop);
        break;
    }
    default:
        return LabPeer::HandleMessage(w, hwnd, msg, wp, lp);
    }
    return 0;
}

static void registerChildClass(HINSTANCE inst)                  /* 424360 */
{
    WNDCLASS wc;
    if (childClassRegistered)
        return;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.hInstance = inst;
    wc.lpfnWndProc = LabWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG_PTR);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = childClass;
    if (!RegisterClass(&wc))
        LabError("Can't register drawing window class");
    childClassRegistered = 1;
}

LabChildPeer::LabChildPeer(LabWindow *w, LabWindow *parent,
                           int x, int y, int cx, int cy, int style) /* 424230 */
{
    HINSTANCE inst = parent->GetApp()->GetPlatform()->instance;
    DWORD ws = WS_CHILD | WS_CLIPSIBLINGS;

    if (style & LabChildWindow::Visible)
        ws = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    if (style & LabChildWindow::Border) {
        ws |= WS_BORDER;
        x -= GetSystemMetrics(SM_CXBORDER);
        y -= GetSystemMetrics(SM_CYBORDER);
        cx += GetSystemMetrics(SM_CXBORDER) * 2;
        cy += GetSystemMetrics(SM_CYBORDER) * 2;
    }
    if (!childClassRegistered)
        registerChildClass(inst);
    hwnd = CreateWindowEx(0, childClass, "", ws, x, y, cx, cy,
                          parent->GetPeer()->hwnd, NULL, inst, NULL);
    if (hwnd == NULL)
        LabError("Can't create drawing window");
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)w);
}

static LRESULT CALLBACK LabWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) /* 424410 */
{
    LabWindow *w = (LabWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (w)
        return w->GetPeer()->HandleMessage(w, hwnd, msg, wp, lp);
    return DefWindowProc(hwnd, msg, wp, lp);
}

INT_PTR CALLBACK LabAboutProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) /* 424470 */
{
    if (msg == WM_INITDIALOG)
        return TRUE;
    if (msg != WM_COMMAND)
        return FALSE;
    if (LOWORD(wp) == IDOK) {
        EndDialog(dlg, 1);
        return TRUE;
    }
    if (LOWORD(wp) == IDCANCEL) {
        EndDialog(dlg, 0);
        return TRUE;
    }
    return FALSE;
}

INT_PTR CALLBACK LabPromptProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) /* 4244e0 */
{
    if (msg == WM_INITDIALOG) {
        SetDlgItemText(dlg, IDC_PROMPT_TEXT, promptText);
        return TRUE;
    }
    if (msg != WM_COMMAND)
        return FALSE;
    if (LOWORD(wp) == IDOK) {
        if (promptBuf) {
            GetDlgItemText(dlg, IDC_PROMPT_EDIT, promptBuf, promptLen);
            EndDialog(dlg, 1);
        } else {
            EndDialog(dlg, 0);
        }
        return TRUE;
    }
    if (LOWORD(wp) == IDCANCEL) {
        EndDialog(dlg, 0);
        return TRUE;
    }
    return FALSE;
}

/* ---- LabWindow ---------------------------------------------------------- */

LabWindow::LabWindow() : link(this)                             /* 422b20 */
{
    peer = 0;
    parent = 0;
    children = 0;
}

LabWindow::~LabWindow()                                         /* 422b40 */
{
    /* see the note on LabList::DeleteLinks: the links are not freed */
    delete children;
    children = 0;
}

void LabWindow::Move(int x, int y)                              /* 422bb0 */
{
    if (GetWindowLong(peer->hwnd, GWL_STYLE) & WS_BORDER) {
        x -= GetSystemMetrics(SM_CXBORDER);
        y -= GetSystemMetrics(SM_CYBORDER);
    }
    MoveWindow(peer->hwnd, x, y, GetWidth(), GetHeight(), TRUE);
}

void LabWindow::Resize(int w, int h)                            /* 422c20 */
{
    if (GetWindowLong(peer->hwnd, GWL_STYLE) & WS_BORDER) {
        w += GetSystemMetrics(SM_CXBORDER) * 2;
        h += GetSystemMetrics(SM_CYBORDER) * 2;
    }
    MoveWindow(peer->hwnd, GetX(), GetY(), w, h, TRUE);
}

int LabWindow::GetX()                                           /* 422c90 */
{
    RECT r;
    POINT p;
    GetWindowRect(peer->hwnd, &r);
    p.x = r.left;
    p.y = r.top;
    if (parent)
        ScreenToClient(parent->GetPeer()->hwnd, &p);
    return p.x;
}

int LabWindow::GetY()                                           /* 422ce0 */
{
    RECT r;
    POINT p;
    GetWindowRect(peer->hwnd, &r);
    p.x = r.left;
    p.y = r.top;
    if (parent)
        ScreenToClient(parent->GetPeer()->hwnd, &p);
    return p.y;
}

int LabWindow::GetWidth()                                       /* 422d30 */
{
    RECT r;
    int extra = 0;
    if (GetWindowLong(peer->hwnd, GWL_STYLE) & WS_BORDER)
        extra = GetSystemMetrics(SM_CXBORDER) * 2;
    GetClientRect(peer->hwnd, &r);
    return r.right - r.left + extra;
}

int LabWindow::GetHeight()                                      /* 422d90 */
{
    RECT r;
    int extra = 0;
    if (GetWindowLong(peer->hwnd, GWL_STYLE) & WS_BORDER)
        extra = GetSystemMetrics(SM_CXBORDER) * 2;    /* sic: X metric */
    GetClientRect(peer->hwnd, &r);
    return r.bottom - r.top + extra;
}

int LabWindow::GetDpi()
{
    UINT dpi = peer && peer->hwnd ? GetDpiForWindow(peer->hwnd) : 0;
    return dpi ? (int)dpi : 96;
}

void LabWindow::Show()                                          /* 422df0 */
{
    ShowWindow(peer->hwnd, SW_SHOWNORMAL);
}

void LabWindow::Hide()                                          /* 422e00 */
{
    ShowWindow(peer->hwnd, SW_HIDE);
}

void LabWindow::Close()                                         /* 422e10 */
{
    if (!peer->destroyed)
        DestroyWindow(peer->hwnd);
}

BOOL LabWindow::HasCapture()                                    /* 422e30 */
{
    return GetCapture() == peer->hwnd;
}

void LabWindow::GetMouse(int *x, int *y, int *mods)             /* 422e50 */
{
    POINT p;
    int m = 0;
    GetCursorPos(&p);
    ScreenToClient(peer->hwnd, &p);
    *x = p.x;
    *y = p.y;
    if (GetKeyState(VK_CONTROL) & 0xfffe)
        m = LabModControl;
    if (GetKeyState(VK_SHIFT) & 0xfffe)
        m |= LabModShift;
    *mods = m;
}

void LabWindow::SetCursor(int shape)                            /* 422ed0 */
{
    switch (shape) {
    case LabCursorWait: peer->cursor = cursorWait; break;
    case LabCursorMove: peer->cursor = cursorMove; break;
    case LabCursorLink: peer->cursor = cursorLink; break;
    case LabCursorRotate: peer->cursor = cursorRotate; break;
    default: peer->cursor = cursorArrow; break;
    }
    /* The original only changed the cursor at the next WM_SETCURSOR; show
     * the busy cursor straight away so that long loads give feedback. */
    POINT p;
    GetCursorPos(&p);
    if (WindowFromPoint(p) == peer->hwnd)
        ::SetCursor(peer->cursor);
}

void LabWindow::SetForeground(const char *colour)               /* 422f40 */
{
    if (strcmp(colour, "black") == 0)
        peer->foreground = RGB(0, 0, 0);
    else if (strcmp(colour, "white") == 0)
        peer->foreground = RGB(255, 255, 255);
    else
        peer->foreground = RGB(0, 0, 0);
}

void LabWindow::DrawLine(int x1, int y1, int x2, int y2)        /* 422fd0 */
{
    int width = Scale(1) > 1 ? Scale(1) : 1;     /* (new) scaled */
    HPEN pen = CreatePen(PS_SOLID, width, peer->foreground);
    HDC dc = GetDC(peer->hwnd);
    HGDIOBJ old = SelectObject(dc, pen);
    MoveToEx(dc, x1, y1, NULL);
    LineTo(dc, x2, y2);
    SelectObject(dc, old);
    ReleaseDC(peer->hwnd, dc);
    DeleteObject(pen);
}

void LabWindow::DrawBevel(LabRect *rc)                          /* 423050 */
{
    RECT r, edge;
    int t = Scale(1) > 1 ? Scale(1) : 1;        /* (new) edge width, scaled */
    SetRect(&r, rc->x, rc->y, rc->x + rc->w, rc->y + rc->h);
    HDC dc = GetDC(peer->hwnd);
    HBRUSH white = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HBRUSH grey = (HBRUSH)GetStockObject(GRAY_BRUSH);
    SetRect(&edge, r.left, r.top, r.right, r.top + t);          /* top */
    FillRect(dc, &edge, grey);
    SetRect(&edge, r.left, r.top, r.left + t, r.bottom);        /* left */
    FillRect(dc, &edge, grey);
    SetRect(&edge, r.right - t, r.top + t, r.right, r.bottom);  /* right */
    FillRect(dc, &edge, white);
    SetRect(&edge, r.left + t, r.bottom - t, r.right, r.bottom); /* bottom */
    FillRect(dc, &edge, white);
    ReleaseDC(peer->hwnd, dc);
}

static HFONT statusFont(int dpi)
{
    static HFONT font;
    static int fontDpi;
    if (font && fontDpi == dpi)
        return font;
    NONCLIENTMETRICS ncm;
    memset(&ncm, 0, sizeof(ncm));
    ncm.cbSize = sizeof(ncm);
    if (!SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0, dpi))
        return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (font)
        DeleteObject(font);
    font = CreateFontIndirect(&ncm.lfStatusFont);
    fontDpi = dpi;
    return font;
}

void LabWindow::DrawText(int x, int y, const char *s, LabRect *clip) /* 423170 */
{
    int len = (int)strlen(s);
    if (len == 0)
        return;
    HDC dc = GetDC(peer->hwnd);
    HRGN rgn = 0;
    int mode = SetBkMode(dc, TRANSPARENT);
    /* (new) the Windows status-bar font at this display's scale, centred
     * in the clip rectangle; the original used the DC's System font */
    HGDIOBJ oldFont = SelectObject(dc, statusFont(GetDpi()));
    if (clip) {
        TEXTMETRIC tm;
        GetTextMetrics(dc, &tm);
        y = clip->y + (clip->h - tm.tmHeight) / 2;
    }
    if (clip) {
        RECT r;
        SetRect(&r, clip->x, clip->y, clip->x + clip->w, clip->y + clip->h);
        rgn = CreateRectRgnIndirect(&r);
        if (rgn) {
            SelectClipRgn(dc, rgn);
            DeleteObject(rgn);
        }
    }
    TextOut(dc, x, y, s, len);
    if (rgn)
        SelectClipRgn(dc, NULL);
    SelectObject(dc, oldFont);
    SetBkMode(dc, mode);
    ReleaseDC(peer->hwnd, dc);
}

void LabWindow::ClearRect(LabRect *rc)                          /* 4232e0 */
{
    RECT r;
    HDC dc = GetDC(peer->hwnd);
    HBRUSH b = CreateSolidBrush(peer->background);
    SetRect(&r, rc->x, rc->y, rc->x + rc->w, rc->y + rc->h);
    FillRect(dc, &r, b);
    DeleteObject(b);
    ReleaseDC(peer->hwnd, dc);
}

void LabWindow::AddChild(LabWindow *w)                          /* 40ee60 */
{
    if (!children)
        children = new LabList;
    children->Append(w ? &w->link : 0);
    w->parent = this;
}

LabWindow *LabWindow::FirstChild()                              /* 40eef0 */
{
    return children ? FromLink(children->First()) : 0;
}

LabWindow *LabWindow::LastChild()                               /* 40ef10 */
{
    return children ? FromLink(children->Last()) : 0;
}

LabWindow *LabWindow::NextChild(LabWindow *w)                   /* 40efa0 */
{
    return children ? FromLink(LabList::Next(&w->link)) : 0;
}

LabWindow *LabWindow::PrevChild(LabWindow *w)                   /* 40f020 */
{
    return children ? FromLink(LabList::Prev(&w->link)) : 0;
}

LabApp *LabWindow::GetApp()                                     /* 423420 */
{
    return parent ? parent->GetApp() : 0;
}

/* ---- LabShellWindow ----------------------------------------------------- */

LabShellWindow::LabShellWindow(LabApp *a, const char *title,
                               int x, int y, int w, int h)      /* 4238d0 */
{
    app = a;
    app->AddWindow(this);
    peer = new LabShellPeer(this, title, x, y, w, h);
}

LabShellWindow::~LabShellWindow()                               /* 4239c0 */
{
    delete peer;
    peer = 0;
}

BOOL LabShellWindow::SetMenu(int id)                            /* 423a50 */
{
    LabShellPeer *p = (LabShellPeer *)peer;
    p->menu = LoadMenu(GetApp()->GetPlatform()->instance, MAKEINTRESOURCE(id));
    if (p->menu)
        ::SetMenu(p->hwnd, p->menu);
    return p->menu != 0;
}

void LabShellWindow::EnableMenuItem(int id, int enable)         /* 423aa0 */
{
    ::EnableMenuItem(((LabShellPeer *)peer)->menu, id,
                     enable ? MF_BYCOMMAND | MF_ENABLED : MF_BYCOMMAND | MF_GRAYED);
}

int LabShellWindow::IsMenuItemEnabled(int id)                   /* 423ad0 */
{
    return ~GetMenuState(((LabShellPeer *)peer)->menu, id, MF_BYCOMMAND) & 1;
}

void LabShellWindow::EnableSubMenuItem(int menu, int id, int enable) /* 423af0 */
{
    HMENU m = peer->GetSubMenu(((LabShellPeer *)peer)->menu, menu);
    if (m)
        ::EnableMenuItem(m, id, enable ? MF_ENABLED : MF_GRAYED);
}

int LabShellWindow::IsSubMenuItemEnabled(int menu, int id)      /* 423b30 */
{
    HMENU m = peer->GetSubMenu(((LabShellPeer *)peer)->menu, menu);
    if (!m)
        return -1;
    return ~GetMenuState(m, id, 0) & 1;
}

void LabShellWindow::CheckSubMenuItem(int menu, int id, int check) /* 423b70 */
{
    HMENU m = peer->GetSubMenu(((LabShellPeer *)peer)->menu, menu);
    if (m)
        CheckMenuItem(m, id, check ? MF_CHECKED : MF_UNCHECKED);
}

int LabShellWindow::IsSubMenuItemChecked(int menu, int id)      /* 423bb0 */
{
    HMENU m = peer->GetSubMenu(((LabShellPeer *)peer)->menu, menu);
    if (!m)
        return -1;
    return (GetMenuState(m, id, 0) & MF_CHECKED) >> 3;
}

/* (new) put a radio bullet on one of a run of items, clearing the others */
void LabShellWindow::CheckSubMenuRadio(int menu, int first, int last, int id)
{
    HMENU m = peer->GetSubMenu(((LabShellPeer *)peer)->menu, menu);
    if (m)
        CheckMenuRadioItem(m, first, last, id, MF_BYCOMMAND);
}

/* ---- LabChildWindow ----------------------------------------------------- */

LabChildWindow::LabChildWindow(LabWindow *par, int x, int y, int w, int h,
                               int style)                       /* 424100 */
{
    parent = par;
    par->AddChild(this);
    peer = new LabChildPeer(this, par, x, y, w, h, style);
}

LabChildWindow::~LabChildWindow()                               /* 4241b0 */
{
    delete peer;
    peer = 0;
}
