/*
 * rlrender.cpp -- Reality Lab compatibility layer: devices, viewports,
 * rendering (OpenGL 1.1), picking and RLTick.
 *
 * The 1995 library rendered in software into an 8-bit palettised DIB and
 * refused to run on anything but an 8-bit display.  This version draws with
 * OpenGL on any display, in true colour, but keeps Reality Lab's camera
 * model exactly (0x432500-0x432b30): a perspective window given at the front
 * plane, and "uniform" scaling that fits the field across the larger of the
 * viewport's width and height.
 */
#include <windows.h>
#include <dwmapi.h>
#include <GL/gl.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include "rlint.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

std::vector<RLDeviceRec *> rlDevices;

static int twoSided;

void RLSetTwoSided(int on)
{
    twoSided = on;
}

static int rlTwoSided(void)
{
    return twoSided;
}

static std::map<HWND, HGLRC> contexts;      /* one GL context per window */
static HGLRC sharedContext;
static HDC sharedDC;
static int npotSupported = -1;

typedef BOOL (WINAPI *SwapIntervalProc)(int);

/*
 * (new) Each device draws into a multisampled framebuffer object, which is
 * resolved into the window's back buffer: antialiased edges, at the
 * window's full resolution (Labyrinth is per-monitor DPI aware).  Without
 * framebuffer objects, or with Antialias=0, it draws into the back buffer.
 */
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER          0x8D40
#define GL_RENDERBUFFER         0x8D41
#define GL_COLOR_ATTACHMENT0    0x8CE0
#define GL_DEPTH_ATTACHMENT     0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER     0x8CA8
#define GL_DRAW_FRAMEBUFFER     0x8CA9
#endif
#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES          0x8D57
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24    0x81A6
#endif
#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP      0x8191
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

typedef void (APIENTRY *GenProc)(GLsizei, GLuint *);
typedef void (APIENTRY *DeleteProc)(GLsizei, const GLuint *);
typedef void (APIENTRY *BindProc)(GLenum, GLuint);
typedef void (APIENTRY *StorageMSProc)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *AttachProc)(GLenum, GLenum, GLenum, GLuint);
typedef GLenum (APIENTRY *StatusProc)(GLenum);
typedef void (APIENTRY *BlitProc)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                                  GLbitfield, GLenum);

static struct {
    GenProc genFramebuffers, genRenderbuffers;
    DeleteProc deleteFramebuffers, deleteRenderbuffers;
    BindProc bindFramebuffer, bindRenderbuffer;
    StorageMSProc renderbufferStorageMultisample;
    AttachProc framebufferRenderbuffer;
    StatusProc checkFramebufferStatus;
    BlitProc blitFramebuffer;
} fb;
static int fbState = -1;                    /* -1 not looked up, 0 unavailable, 1 ready */
static int antialias = 4;                   /* samples; 0 or 1 is off */
static int renderedAny;

void RLSetAntialias(int samples)
{
    antialias = samples;
}

static PROC glProc(const char *core, const char *ext)
{
    PROC p = wglGetProcAddress(core);
    if (!p)
        p = wglGetProcAddress(ext);
    return p;
}

/* Look up the framebuffer object entry points (a context must be current). */
static int framebuffersReady(void)
{
    if (fbState >= 0)
        return fbState;
    fb.genFramebuffers = (GenProc)glProc("glGenFramebuffers", "glGenFramebuffersEXT");
    fb.genRenderbuffers = (GenProc)glProc("glGenRenderbuffers", "glGenRenderbuffersEXT");
    fb.deleteFramebuffers = (DeleteProc)glProc("glDeleteFramebuffers", "glDeleteFramebuffersEXT");
    fb.deleteRenderbuffers = (DeleteProc)glProc("glDeleteRenderbuffers", "glDeleteRenderbuffersEXT");
    fb.bindFramebuffer = (BindProc)glProc("glBindFramebuffer", "glBindFramebufferEXT");
    fb.bindRenderbuffer = (BindProc)glProc("glBindRenderbuffer", "glBindRenderbufferEXT");
    fb.renderbufferStorageMultisample = (StorageMSProc)glProc("glRenderbufferStorageMultisample",
                                                              "glRenderbufferStorageMultisampleEXT");
    fb.framebufferRenderbuffer = (AttachProc)glProc("glFramebufferRenderbuffer", "glFramebufferRenderbufferEXT");
    fb.checkFramebufferStatus = (StatusProc)glProc("glCheckFramebufferStatus", "glCheckFramebufferStatusEXT");
    fb.blitFramebuffer = (BlitProc)glProc("glBlitFramebuffer", "glBlitFramebufferEXT");
    fbState = fb.genFramebuffers && fb.genRenderbuffers && fb.deleteFramebuffers &&
              fb.deleteRenderbuffers && fb.bindFramebuffer && fb.bindRenderbuffer &&
              fb.renderbufferStorageMultisample && fb.framebufferRenderbuffer &&
              fb.checkFramebufferStatus && fb.blitFramebuffer;
    return fbState;
}

static void freeFramebuffer(RLDeviceRec *d)
{
    if (!d->fbo)
        return;
    fb.deleteFramebuffers(1, &d->fbo);
    fb.deleteRenderbuffers(1, &d->colourBuffer);
    fb.deleteRenderbuffers(1, &d->depthBuffer);
    d->fbo = d->colourBuffer = d->depthBuffer = 0;
    d->fbWidth = d->fbHeight = 0;
}

/* Make the device's multisampled framebuffer match the window; 0 means
 * draw straight into the back buffer instead. */
static int useFramebuffer(RLDeviceRec *d, int width, int height)
{
    if (antialias <= 1 || d->fbFailed || !framebuffersReady())
        return 0;
    if (d->fbo && d->fbWidth == width && d->fbHeight == height)
        return 1;
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    int samples = antialias < maxSamples ? antialias : maxSamples;
    if (samples <= 1) {
        d->fbFailed = 1;
        return 0;
    }
    if (!d->fbo) {
        fb.genFramebuffers(1, &d->fbo);
        fb.genRenderbuffers(1, &d->colourBuffer);
        fb.genRenderbuffers(1, &d->depthBuffer);
    }
    fb.bindRenderbuffer(GL_RENDERBUFFER, d->colourBuffer);
    fb.renderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
    fb.bindRenderbuffer(GL_RENDERBUFFER, d->depthBuffer);
    fb.renderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
    fb.bindRenderbuffer(GL_RENDERBUFFER, 0);
    fb.bindFramebuffer(GL_FRAMEBUFFER, d->fbo);
    fb.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, d->colourBuffer);
    fb.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, d->depthBuffer);
    int ok = fb.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    fb.bindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!ok) {
        freeFramebuffer(d);
        d->fbFailed = 1;
        return 0;
    }
    d->fbWidth = width;
    d->fbHeight = height;
    return 1;
}

static int setPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc, &pfd);
    return pf && SetPixelFormat(hdc, pf, &pfd);
}

static RLError err(RLError e)
{
    return rlSetError(e);
}

/* ---- GL resources --------------------------------------------------------- */

static void makeSomethingCurrent(void)
{
    if (!wglGetCurrentContext() && sharedContext && sharedDC)
        wglMakeCurrent(sharedDC, sharedContext);
}

void rlFreeDisplayList(unsigned int list)
{
    makeSomethingCurrent();
    if (wglGetCurrentContext())
        glDeleteLists(list, 1);
}

void rlFreeTexture(unsigned int name)
{
    makeSomethingCurrent();
    if (wglGetCurrentContext())
        glDeleteTextures(1, &name);
}

static int isPow2(int n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

static int nextPow2(int n)
{
    int p = 1;
    while (p < n && p < 2048)
        p <<= 1;
    return p;
}

void rlTextureUpload(RLTextureRec *t)
{
    if (t->glName || t->rgba.empty())
        return;
    if (npotSupported < 0) {
        const char *ver = (const char *)glGetString(GL_VERSION);
        const char *ext = (const char *)glGetString(GL_EXTENSIONS);
        npotSupported = (ver && atof(ver) >= 2.0) ||
                        (ext && strstr(ext, "GL_ARB_texture_non_power_of_two"));
    }
    /* (extension) texels that are mostly transparent are cut out with the
     * alpha test, for VRML textures with an alpha channel */
    t->cutout = 0;
    for (size_t i = 3; i < t->rgba.size() && !t->cutout; i += 4)
        if (t->rgba[i] < 128)
            t->cutout = 1;
    const unsigned char *pixels = &t->rgba[0];
    std::vector<unsigned char> scaled;
    t->texWidth = t->width;
    t->texHeight = t->height;
    if (!npotSupported && (!isPow2(t->width) || !isPow2(t->height))) {
        t->texWidth = nextPow2(t->width);
        t->texHeight = nextPow2(t->height);
        scaled.resize((size_t)t->texWidth * t->texHeight * 4);
        for (int y = 0; y < t->texHeight; y++) {
            int sy = y * t->height / t->texHeight;
            for (int x = 0; x < t->texWidth; x++) {
                int sx = x * t->width / t->texWidth;
                memcpy(&scaled[((size_t)y * t->texWidth + x) * 4],
                       &t->rgba[((size_t)sy * t->width + sx) * 4], 4);
            }
        }
        pixels = &scaled[0];
    }
    glGenTextures(1, &t->glName);
    glBindTexture(GL_TEXTURE_2D, t->glName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* (new) mipmapped and anisotropic, so distant textures do not shimmer */
    const char *ext = (const char *)glGetString(GL_EXTENSIONS);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (ext && strstr(ext, "GL_EXT_texture_filter_anisotropic")) {
        GLfloat most = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &most);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, most < 8.0f ? most : 8.0f);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, t->clampS ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t->clampT ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->texWidth, t->texHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

RLError RLTextureSetClamp(RLTexture texture, int clampS, int clampT)
{
    if (!rlValid(texture, RLC_TEXTURE))
        return RLGetLastError();
    RLTextureRec *t = (RLTextureRec *)texture;
    t->clampS = clampS;
    t->clampT = clampT;
    if (t->glName) {
        makeSomethingCurrent();
        glBindTexture(GL_TEXTURE_2D, t->glName);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampS ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampT ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    }
    return RLNoError;
}

/* ---- devices -------------------------------------------------------------- */

RLDeviceRec::~RLDeviceRec()
{
    for (size_t i = 0; i < viewports.size(); i++)
        viewports[i]->device = 0;
    rlDevices.erase(std::remove(rlDevices.begin(), rlDevices.end(), this), rlDevices.end());
    if (fbo) {
        makeSomethingCurrent();
        freeFramebuffer(this);
    }
    if (hdc && hdc != sharedDC)
        ReleaseDC(hwnd, hdc);
}

RLDevice RLWinCreateDevice(void *window, int width, int height) /* 424cf0 */
{
    HWND hwnd = (HWND)window;
    if (!hwnd || width <= 0 || height <= 0) {
        err(RLBadValue);
        return NULL;
    }
    RLDeviceRec *d = new RLDeviceRec;
    d->hwnd = hwnd;
    d->width = width;
    d->height = height;

    d->hdc = GetDC(hwnd);

    std::map<HWND, HGLRC>::iterator it = contexts.find(hwnd);
    if (it != contexts.end()) {
        d->glrc = it->second;
    } else {
        if (!setPixelFormat(d->hdc)) {
            ReleaseDC(hwnd, d->hdc);
            d->hdc = 0;
            delete d;
            err(RLBadDevice);
            return NULL;
        }
        d->glrc = wglCreateContext(d->hdc);
        if (!d->glrc) {
            ReleaseDC(hwnd, d->hdc);
            d->hdc = 0;
            delete d;
            err(RLBadDevice);
            return NULL;
        }
        if (sharedContext)
            wglShareLists(sharedContext, d->glrc);
        else {
            sharedContext = d->glrc;
            sharedDC = d->hdc;
        }
        contexts[hwnd] = d->glrc;
        wglMakeCurrent(d->hdc, d->glrc);
        /* the frame rate is kept to the display's by DwmFlush in RLTick;
         * waiting in SwapBuffers instead kept a CPU core busy */
        SwapIntervalProc swapInterval = (SwapIntervalProc)wglGetProcAddress("wglSwapIntervalEXT");
        if (swapInterval)
            swapInterval(0);
    }
    rlDevices.push_back(d);
    return d;
}

RLError RLWinHandlePaint(RLDevice device, void *hdc)            /* 424dd0 */
{
    if (!rlValid(device, RLC_DEVICE))
        return RLGetLastError();
    rlRenderDevice((RLDeviceRec *)device);
    return RLNoError;
}

RLError RLWinHandleActivate(RLDevice device, int active)        /* 424ed0 */
{
    /* palette realisation is not needed in true colour */
    if (!rlValid(device, RLC_DEVICE))
        return RLGetLastError();
    return RLNoError;
}

RLError RLDeviceSetQuality(RLDevice device, RLRenderQuality q)  /* 424f40 */
{
    if (!rlValid(device, RLC_DEVICE))
        return RLGetLastError();
    ((RLDeviceRec *)device)->quality = q;
    return RLNoError;
}

RLRenderQuality RLDeviceGetQuality(RLDevice device)             /* 42ec90 */
{
    if (!rlValid(device, RLC_DEVICE))
        return (RLRenderQuality)-1;
    return ((RLDeviceRec *)device)->quality;
}

RLError RLDeviceSetDither(RLDevice device, int dither)          /* 424f80 */
{
    if (!rlValid(device, RLC_DEVICE))
        return RLGetLastError();
    ((RLDeviceRec *)device)->dither = dither;
    return RLNoError;
}

int RLDeviceGetDither(RLDevice device)                          /* 424fe0 */
{
    if (!rlValid(device, RLC_DEVICE))
        return 0;
    return ((RLDeviceRec *)device)->dither;
}

RLError RLDeviceSetShades(RLDevice device, int shades)          /* 42eb90 */
{
    if (!rlValid(device, RLC_DEVICE))
        return RLGetLastError();
    if (shades < 0 || shades > 256 || (shades & (shades - 1)) != 0)
        return err(RLBadValue);
    ((RLDeviceRec *)device)->shades = shades;
    return RLNoError;
}

int RLDeviceGetShades(RLDevice device)                          /* 42ec10 */
{
    if (!rlValid(device, RLC_DEVICE))
        return -1;
    return ((RLDeviceRec *)device)->shades;
}

RLError RLDeviceSetColours(RLDevice device, int colours)        /* 425300 */
{
    if (!rlValid(device, RLC_DEVICE))
        return RLGetLastError();
    ((RLDeviceRec *)device)->colours = colours;
    return RLNoError;
}

int RLDeviceGetColours(RLDevice device)                         /* 42ec50 */
{
    if (!rlValid(device, RLC_DEVICE))
        return -1;
    return ((RLDeviceRec *)device)->colours;
}

int RLDeviceGetWidth(RLDevice device)                           /* 425280 */
{
    if (!rlValid(device, RLC_DEVICE))
        return 0;
    return ((RLDeviceRec *)device)->width;
}

int RLDeviceGetHeight(RLDevice device)                          /* 4252c0 */
{
    if (!rlValid(device, RLC_DEVICE))
        return 0;
    return ((RLDeviceRec *)device)->height;
}

/* ---- viewports ------------------------------------------------------------ */

RLViewportRec::~RLViewportRec()
{
    if (device) {
        std::vector<RLViewportRec *> &v = device->viewports;
        v.erase(std::remove(v.begin(), v.end(), this), v.end());
    }
}

static float scaleOf(const RLViewportRec *v, int horizontal)
{
    if (v->uniform) {
        int s = v->width > v->height ? v->width : v->height;
        return s * 0.5f;
    }
    return (horizontal ? v->width : v->height) * 0.5f;
}

/* Camera space -> perspective (before the divide), 0x4325e0 * 0x432500 */
static Mat perspective(const RLViewportRec *v)
{
    Mat plane, depth;
    matIdentity(plane);
    float w = v->right - v->left, h = v->top - v->bottom;
    plane.m[0][0] = v->front / w * 2.0f;
    plane.m[1][1] = v->front / h * 2.0f;
    plane.m[2][0] = -((v->right + v->left) / w);
    plane.m[2][1] = -((v->top + v->bottom) / h);
    matIdentity(depth);
    float d = v->back - v->front;
    depth.m[2][2] = v->back / d;
    depth.m[2][3] = 1.0f;
    depth.m[3][2] = -((v->back * v->front) / d);
    depth.m[3][3] = 0.0f;
    return matMul(plane, depth);
}

/* Camera space -> homogeneous screen pixels (0x432aa0) */
Mat RLViewportRec::projection() const
{
    Mat screen;
    matIdentity(screen);
    screen.m[0][0] = scaleOf(this, 1);
    screen.m[1][1] = -scaleOf(this, 0);
    screen.m[3][0] = width * 0.5f + x;
    screen.m[3][1] = height * 0.5f + y;
    return matMul(perspective(this), screen);
}

/* The same projection for OpenGL: uniform scaling folded in, and depth
 * moved from Reality Lab's 0..1 to OpenGL's -1..1. */
static Mat glProjection(const RLViewportRec *v)
{
    Mat d;
    matIdentity(d);
    d.m[0][0] = scaleOf(v, 1) / (v->width * 0.5f);
    d.m[1][1] = scaleOf(v, 0) / (v->height * 0.5f);
    d.m[2][2] = 2.0f;
    d.m[3][2] = -1.0f;
    return matMul(perspective(v), d);
}

RLViewport RLCreateViewport(RLDevice device, RLFrame camera,
                            int x, int y, int width, int height) /* 425080 */
{
    if (!rlValid(device, RLC_DEVICE) || !rlValid(camera, RLC_FRAME))
        return NULL;
    RLDeviceRec *d = (RLDeviceRec *)device;
    if (width < 0 || height < 0 || x < 0 || y < 0 ||
        x + width > d->width || y + height > d->height) {
        err(RLBadValue);
        return NULL;
    }
    RLViewportRec *v = new RLViewportRec;
    v->device = d;
    v->camera = (RLFrameRec *)camera;
    v->x = x;
    v->y = y;
    v->width = width;
    v->height = height;
    d->viewports.push_back(v);
    return v;
}

RLError RLViewportSetBack(RLViewport viewport, RLValue back)    /* 425020 */
{
    if (!rlValid(viewport, RLC_VIEWPORT))
        return RLGetLastError();
    RLViewportRec *v = (RLViewportRec *)viewport;
    if ((float)back <= v->front)
        return err(RLBadValue);
    v->back = (float)back;
    return RLNoError;
}

RLError RLViewportSetPlane(RLViewport viewport, RLValue left, RLValue right,
                           RLValue bottom, RLValue top)         /* 42eb20 */
{
    if (!rlValid(viewport, RLC_VIEWPORT))
        return RLGetLastError();
    RLViewportRec *v = (RLViewportRec *)viewport;
    v->left = (float)left;
    v->right = (float)right;
    v->bottom = (float)bottom;
    v->top = (float)top;
    return RLNoError;
}

RLError RLViewportGetPlane(RLViewport viewport, RLValue *left, RLValue *right,
                           RLValue *bottom, RLValue *top)       /* 42ecd0 */
{
    if (!rlValid(viewport, RLC_VIEWPORT))
        return RLGetLastError();
    RLViewportRec *v = (RLViewportRec *)viewport;
    *left = v->left;
    *right = v->right;
    *bottom = v->bottom;
    *top = v->top;
    return RLNoError;
}

RLError RLViewportForceUpdate(RLViewport viewport, int x1, int y1, int x2, int y2) /* 42ea10 */
{
    if (!rlValid(viewport, RLC_VIEWPORT))
        return RLGetLastError();
    RLViewportRec *v = (RLViewportRec *)viewport;
    if (x1 > x2 || y1 > y2 || x1 < v->x || y1 < v->y ||
        x2 > v->x + v->width || y2 > v->y + v->height)
        return err(RLBadValue);
    /* everything is redrawn every frame */
    return RLNoError;
}

RLError RLViewportTransform(RLViewport viewport, float dst[4], RLVector *src) /* 42f810 */
{
    if (!rlValid(viewport, RLC_VIEWPORT))
        return RLGetLastError();
    RLViewportRec *v = (RLViewportRec *)viewport;
    Mat inv;
    matInverse(inv, v->camera->world());
    Mat m = matMul(inv, v->projection());
    float in[4] = { src->x, src->y, src->z, 1.0f };
    xform4(dst, in, m);
    return RLNoError;
}

RLError RLViewportInverseTransform(RLViewport viewport, RLVector *dst, float src[4]) /* 42f7a0 */
{
    if (!rlValid(viewport, RLC_VIEWPORT))
        return RLGetLastError();
    RLViewportRec *v = (RLViewportRec *)viewport;
    Mat inv;
    matInverse(inv, v->projection());
    Mat m = matMul(inv, v->camera->world());
    float r[4];
    xform4(r, src, m);
    if (r[3] == 0.0f)
        r[3] = 1.0f;
    dst->x = r[0] / r[3];
    dst->y = r[1] / r[3];
    dst->z = r[2] / r[3];
    return RLNoError;
}

/* ---- picking -------------------------------------------------------------- */

struct PickHit {
    float t;
    RLVisual visual;
    RLFrame frame;
};

static bool rayTriangle(const RLVector &o, const RLVector &d,
                        const RLVector &a, const RLVector &b, const RLVector &c, float &t)
{
    RLVector e1 = { b.x - a.x, b.y - a.y, b.z - a.z };
    RLVector e2 = { c.x - a.x, c.y - a.y, c.z - a.z };
    RLVector p = { d.y * e2.z - d.z * e2.y, d.z * e2.x - d.x * e2.z, d.x * e2.y - d.y * e2.x };
    float det = e1.x * p.x + e1.y * p.y + e1.z * p.z;
    if (fabsf(det) < 1e-12f)
        return false;
    float inv = 1.0f / det;
    RLVector s = { o.x - a.x, o.y - a.y, o.z - a.z };
    float u = (s.x * p.x + s.y * p.y + s.z * p.z) * inv;
    if (u < 0.0f || u > 1.0f)
        return false;
    RLVector q = { s.y * e1.z - s.z * e1.y, s.z * e1.x - s.x * e1.z, s.x * e1.y - s.y * e1.x };
    float v = (d.x * q.x + d.y * q.y + d.z * q.z) * inv;
    if (v < 0.0f || u + v > 1.0f)
        return false;
    t = (e2.x * q.x + e2.y * q.y + e2.z * q.z) * inv;
    return t > 0.0f;
}

static void pickFrame(RLFrameRec *f, const RLVector &o, const RLVector &d, PickHit &best)
{
    if (!f->visuals.empty()) {
        Mat w = f->world();
        for (size_t i = 0; i < f->visuals.size(); i++) {
            if (f->visuals[i]->cls != RLC_MESH)
                continue;
            RLMeshRec *m = (RLMeshRec *)f->visuals[i];
            std::vector<RLVector> wv(m->vertices.size());
            for (size_t k = 0; k < wv.size(); k++)
                wv[k] = xformPoint(m->vertices[k], w);
            for (size_t j = 0; j < m->faces.size(); j++) {
                RLFaceRec *face = m->faces[j];
                for (size_t k = 1; k + 1 < face->vertices.size(); k++) {
                    float t;
                    if (rayTriangle(o, d, wv[face->vertices[0]], wv[face->vertices[k]],
                                    wv[face->vertices[k + 1]], t) && t < best.t) {
                        best.t = t;
                        best.visual = m;
                        best.frame = f;
                    }
                }
            }
        }
    }
    for (size_t i = 0; i < f->children.size(); i++)
        pickFrame(f->children[i], o, d, best);
}

RLError RLViewportPick(RLViewport viewport, RLVisual *visual, RLFrame *frame,
                       int x, int y)                            /* 42ed70 */
{
    if (!rlValid(viewport, RLC_VIEWPORT))
        return RLGetLastError();
    RLViewportRec *v = (RLViewportRec *)viewport;
    if (!v->camera)
        return RLNotFound;

    /* pixel -> Reality Lab's normalised coordinates -> a ray in camera space */
    float X = (x - (v->x + v->width * 0.5f)) / scaleOf(v, 1);
    float Y = -(y - (v->y + v->height * 0.5f)) / scaleOf(v, 0);
    float w = v->right - v->left, h = v->top - v->bottom;
    float ax = 2.0f * v->front / w, cx = -(v->right + v->left) / w;
    float ay = 2.0f * v->front / h, cy = -(v->top + v->bottom) / h;
    RLVector dir = { (X - cx) / ax, (Y - cy) / ay, 1.0f };

    Mat cam = v->camera->world();
    RLVector o = { cam.m[3][0], cam.m[3][1], cam.m[3][2] };
    RLVector d = xformDir(dir, cam);

    PickHit best;
    best.t = 1e30f;
    best.visual = 0;
    best.frame = 0;
    pickFrame(rlSceneOf(v->camera), o, d, best);
    if (!best.visual)
        return RLNotFound;
    *visual = best.visual;
    *frame = best.frame;
    return RLNoError;
}

/* ---- rendering ------------------------------------------------------------ */

static void colourOf(RLColour c, float out[4])
{
    out[0] = ((c >> 16) & 0xff) / 255.0f;
    out[1] = ((c >> 8) & 0xff) / 255.0f;
    out[2] = (c & 0xff) / 255.0f;
    out[3] = 1.0f;
}

/* Keep texture coordinates on the near side of a wrap-around seam. */
static float unwrap(float t, float ref)
{
    while (t - ref > 0.5f)
        t -= 1.0f;
    while (ref - t > 0.5f)
        t += 1.0f;
    return t;
}

static void compileMesh(RLMeshRec *m, int flat)
{
    for (size_t i = 0; i < m->faces.size(); i++)
        if (m->faces[i]->texture)
            rlTextureUpload(m->faces[i]->texture);

    if (!m->displayList)
        m->displayList = glGenLists(1);
    glNewList(m->displayList, GL_COMPILE);

    /* line and point "faces" first, unlit */
    int haveLines = 0;
    for (size_t i = 0; i < m->faces.size(); i++) {
        RLFaceRec *f = m->faces[i];
        size_t n = f->vertices.size();
        if (n == 0 || n > 2)
            continue;
        if (!haveLines) {
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            haveLines = 1;
        }
        float col[4];
        colourOf(f->colour, col);
        glColor4fv(col);
        glBegin(n == 2 ? GL_LINES : GL_POINTS);
        for (size_t k = 0; k < n; k++) {
            const RLVector &p = m->vertices[f->vertices[k]];
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }
    if (haveLines)
        glEnable(GL_LIGHTING);

    RLTextureRec *boundTex = (RLTextureRec *)1;
    RLMaterialRec *boundMat = (RLMaterialRec *)1;
    for (size_t i = 0; i < m->faces.size(); i++) {
        RLFaceRec *f = m->faces[i];
        size_t n = f->vertices.size();
        if (n < 3)
            continue;
        if (f->texture != boundTex) {
            if (f->texture && f->texture->glName) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, f->texture->glName);
                if (f->texture->cutout) {
                    glEnable(GL_ALPHA_TEST);
                    glAlphaFunc(GL_GREATER, 0.5f);
                } else {
                    glDisable(GL_ALPHA_TEST);
                }
            } else {
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_ALPHA_TEST);
            }
            boundTex = f->texture;
        }
        if (f->material != boundMat) {
            float spec[4] = { 0, 0, 0, 1 };
            float emit[4] = { 0, 0, 0, 1 };
            float shine = 0;
            if (f->material) {
                spec[0] = spec[1] = spec[2] = f->material->specular;
                shine = f->material->power;
                if (shine > 128.0f)
                    shine = 128.0f;
                for (int a = 0; a < 3; a++)
                    emit[a] = f->material->emissive[a];
            }
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shine);
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emit);
            boundMat = f->material;
        }
        float col[4];
        if (f->texture)
            col[0] = col[1] = col[2] = col[3] = 1.0f;
        else
            colourOf(f->colour, col);
        glColor4fv(col);

        RLVector fn = { 0, 0, -1 };
        if (flat) {
            /* Newell normal, as Reality Lab used for flat shading */
            RLVector s = { 0, 0, 0 };
            for (size_t k = 0; k < n; k++) {
                const RLVector &a = m->vertices[f->vertices[k]];
                const RLVector &b = m->vertices[f->vertices[(k + 1) % n]];
                s.x += (a.y - b.y) * (a.z + b.z);
                s.y += (a.z - b.z) * (a.x + b.x);
                s.z += (a.x - b.x) * (a.y + b.y);
            }
            float l = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
            if (l > 0.0f) {
                fn.x = s.x / l; fn.y = s.y / l; fn.z = s.z / l;
            }
        }

        float u0 = m->u[f->vertices[0]], v0 = m->v[f->vertices[0]];
        glBegin(GL_POLYGON);
        for (size_t k = 0; k < n; k++) {
            int vi = f->vertices[k];
            if (flat) {
                glNormal3f(fn.x, fn.y, fn.z);
            } else {
                int ni = f->normals[k];
                const RLVector &nn = m->normals[ni];
                glNormal3f(nn.x, nn.y, nn.z);
            }
            if (f->texture) {
                float tu = m->u[vi], tv = m->v[vi];
                if (f->flags & RLFaceWrapU)
                    tu = unwrap(tu, u0);
                if (f->flags & RLFaceWrapV)
                    tv = unwrap(tv, v0);
                glTexCoord2f(tu, tv);
            }
            const RLVector &p = m->vertices[vi];
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glEndList();
    m->dirty = 0;
    m->listQuality = flat;
}

struct LightState {
    float ambient[3];
    int count;
};

static void addLights(RLFrameRec *f, LightState &ls)
{
    if (!f->lights.empty()) {
        Mat w = f->world();
        for (size_t i = 0; i < f->lights.size(); i++) {
            RLLightRec *l = f->lights[i];
            if (l->type == RLLightAmbient) {
                ls.ambient[0] += l->r;
                ls.ambient[1] += l->g;
                ls.ambient[2] += l->b;
                continue;
            }
            if (ls.count >= 8)
                continue;
            GLenum id = GL_LIGHT0 + ls.count++;
            float col[4] = { l->r, l->g, l->b, 1.0f };
            float black[4] = { 0, 0, 0, 1 };
            RLVector dir = { w.m[2][0], w.m[2][1], w.m[2][2] };
            RLVectorNormalise(&dir);
            glEnable(id);
            glLightfv(id, GL_AMBIENT, black);
            glLightfv(id, GL_DIFFUSE, col);
            glLightfv(id, GL_SPECULAR, col);
            glLightf(id, GL_SPOT_CUTOFF, 180.0f);
            glLightf(id, GL_SPOT_EXPONENT, 0.0f);
            glLightf(id, GL_CONSTANT_ATTENUATION, 1.0f);
            glLightf(id, GL_LINEAR_ATTENUATION, 0.0f);
            glLightf(id, GL_QUADRATIC_ATTENUATION, 0.0f);
            if (l->type == RLLightDirectional) {
                /* a directional light shines along its frame's z axis */
                float pos[4] = { -dir.x, -dir.y, -dir.z, 0.0f };
                glLightfv(id, GL_POSITION, pos);
            } else {
                float pos[4] = { w.m[3][0], w.m[3][1], w.m[3][2], 1.0f };
                glLightfv(id, GL_POSITION, pos);
                glLightf(id, GL_CONSTANT_ATTENUATION, l->constant);
                glLightf(id, GL_LINEAR_ATTENUATION, l->linear);
                glLightf(id, GL_QUADRATIC_ATTENUATION, l->quadratic);
                if (l->type == RLLightSpot) {
                    float sd[3] = { dir.x, dir.y, dir.z };
                    glLightfv(id, GL_SPOT_DIRECTION, sd);
                    glLightf(id, GL_SPOT_CUTOFF, l->penumbra * 0.5f * 57.29578f);
                    glLightf(id, GL_SPOT_EXPONENT, 2.0f);
                }
            }
        }
    }
    for (size_t i = 0; i < f->children.size(); i++)
        addLights(f->children[i], ls);
}

static void drawFrame(RLFrameRec *f, const Mat &view, int flat)
{
    if (!f->visuals.empty()) {
        Mat mv = matMul(f->world(), view);
        glLoadMatrixf(&mv.m[0][0]);
        for (size_t i = 0; i < f->visuals.size(); i++) {
            if (f->visuals[i]->cls != RLC_MESH)
                continue;
            RLMeshRec *m = (RLMeshRec *)f->visuals[i];
            if (m->dirty || m->listQuality != flat || !m->displayList)
                compileMesh(m, flat);
            if (m->twoSided && !twoSided) {
                glDisable(GL_CULL_FACE);
                glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
                glCallList(m->displayList);
                glEnable(GL_CULL_FACE);
                glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
            } else {
                glCallList(m->displayList);
            }
        }
    }
    for (size_t i = 0; i < f->children.size(); i++)
        drawFrame(f->children[i], view, flat);
}

static void renderViewport(RLViewportRec *v, int clientHeight)
{
    if (!v->camera || v->width <= 0 || v->height <= 0)
        return;
    int gy = clientHeight - (v->y + v->height);
    glViewport(v->x, gy, v->width, v->height);
    glScissor(v->x, gy, v->width, v->height);
    glEnable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    /* the scene background: black */
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* (new) lines and points keep their size on high-DPI displays */
    float px = v->device ? GetDpiForWindow(v->device->hwnd) / 96.0f : 1.0f;
    if (px < 1.0f)
        px = 1.0f;
    glLineWidth(px);
    glPointSize(px * 2.0f);

    Mat proj = glProjection(v);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&proj.m[0][0]);

    Mat view;
    matInverse(view, v->camera->world());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&view.m[0][0]);

    RLDeviceRec *d = v->device;
    int flat = d->quality == RLRenderFlat || d->quality == RLRenderUnlitFlat;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    /* Faces are front facing when clockwise as seen by the viewer.  A field
     * with left > right (or bottom > top) mirrors the picture -- worlds
     * exported from 3D Studio used that to show right-handed models -- and
     * with it the winding seen on the screen. */
    int mirrored = ((v->right - v->left) < 0.0f) != ((v->top - v->bottom) < 0.0f);
    if (rlTwoSided()) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glFrontFace(mirrored ? GL_CCW : GL_CW);
        glCullFace(GL_BACK);
    }
    glShadeModel(flat ? GL_FLAT : GL_SMOOTH);
    glPolygonMode(GL_FRONT_AND_BACK, d->quality == RLRenderWireframe ? GL_LINE : GL_FILL);
    glEnable(GL_NORMALIZE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, rlTwoSided() ? GL_TRUE : GL_FALSE);
    for (int i = 0; i < 8; i++)
        glDisable(GL_LIGHT0 + i);

    RLFrameRec *scene = rlSceneOf(v->camera);
    LightState ls = { { 0, 0, 0 }, 0 };
    addLights(scene, ls);
    float amb[4] = { ls.ambient[0], ls.ambient[1], ls.ambient[2], 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

    drawFrame(scene, view, flat);
    glDisable(GL_SCISSOR_TEST);
}

/*
 * Test hook: with LABYRINTH_SNAPSHOT=file.bmp in the environment the
 * picture is saved after LABYRINTH_SNAPSHOT_FRAMES frames (default 30)
 * and the program exits.  Used to check rendering without a display.
 */
static void snapshotIfRequested(int width, int height)
{
    static int frames = -1;
    static char file[MAX_PATH];
    if (frames == -1) {
        char n[32];
        if (!GetEnvironmentVariableA("LABYRINTH_SNAPSHOT", file, MAX_PATH)) {
            frames = -2;
            return;
        }
        frames = 30;
        if (GetEnvironmentVariableA("LABYRINTH_SNAPSHOT_FRAMES", n, sizeof(n)))
            frames = atoi(n);
    }
    if (frames == -2 || --frames > 0)
        return;
    std::vector<unsigned char> px((size_t)width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_BGR_EXT, GL_UNSIGNED_BYTE, &px[0]);
    int stride = (width * 3 + 3) & ~3;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    memset(&bf, 0, sizeof(bf));
    memset(&bi, 0, sizeof(bi));
    bi.biSize = sizeof(bi);
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biSizeImage = stride * height;
    bf.bfType = 0x4d42;
    bf.bfOffBits = sizeof(bf) + sizeof(bi);
    bf.bfSize = bf.bfOffBits + bi.biSizeImage;
    FILE *fp = fopen(file, "wb");
    if (fp) {
        fwrite(&bf, sizeof(bf), 1, fp);
        fwrite(&bi, sizeof(bi), 1, fp);
        unsigned char pad[3] = { 0, 0, 0 };
        for (int y = 0; y < height; y++) {
            fwrite(&px[(size_t)y * width * 3], 1, width * 3, fp);
            fwrite(pad, 1, stride - width * 3, fp);
        }
        fclose(fp);
    }
    ExitProcess(0);
}

void rlRenderDevice(RLDeviceRec *d)
{
    if (!d->hdc || !d->glrc)
        return;
    wglMakeCurrent(d->hdc, d->glrc);
    RECT rc;
    GetClientRect(d->hwnd, &rc);
    if (rc.right <= 0 || rc.bottom <= 0 || IsIconic(GetAncestor(d->hwnd, GA_ROOT)))
        return;
    int msaa = useFramebuffer(d, rc.right, rc.bottom);
    if (msaa)
        fb.bindFramebuffer(GL_FRAMEBUFFER, d->fbo);
    glViewport(0, 0, rc.right, rc.bottom);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (size_t i = 0; i < d->viewports.size(); i++)
        renderViewport(d->viewports[i], rc.bottom);
    if (msaa) {
        fb.bindFramebuffer(GL_READ_FRAMEBUFFER, d->fbo);
        fb.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        fb.blitFramebuffer(0, 0, rc.right, rc.bottom, 0, 0, rc.right, rc.bottom,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
        fb.bindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    renderedAny = 1;
    snapshotIfRequested(rc.right, rc.bottom);
    SwapBuffers(d->hdc);
}

/* Move the world on one step and redraw every viewport of every device. */
void RLTick(void)                                               /* 4245e0 */
{
    std::vector<RLDeviceRec *> devices = rlDevices;
    renderedAny = 0;
    for (size_t i = 0; i < devices.size(); i++)
        rlRenderDevice(devices[i]);
    /* wait for the desktop compositor, keeping to the display's rate; with
     * nothing to draw (minimised) just rest, rather than spin */
    if (!renderedAny || FAILED(DwmFlush()))
        Sleep(15);
}
