/*
 * rlint.h -- internals of the Reality Lab compatibility layer.
 *
 * Conventions follow Reality Lab's (recovered from the 1995 library):
 *  - left-handed coordinates: x right, y up, z into the screen;
 *  - row vectors: a point transforms as p' = p * M, and a frame's matrix
 *    holds its local x, y and z axes in rows 0-2 and its position in row 3;
 *  - a frame's world matrix is local * parent's world matrix;
 *  - every frame hangs (directly or indirectly) off a hidden root frame;
 *    the children of the root are "scenes".
 */
#ifndef RLINT_H
#define RLINT_H

#include <vector>
#include <windows.h>
#include "rl.h"

struct Mat {
    float m[4][4];
};

void matIdentity(Mat &a);
Mat matMul(const Mat &a, const Mat &b);
bool matInverse(Mat &out, const Mat &in);
void matFromOrientation(Mat &a, double dx, double dy, double dz,
                        double ux, double uy, double uz);
void matRotation(Mat &a, double x, double y, double z, double theta);
RLVector xformPoint(const RLVector &p, const Mat &m);
RLVector xformDir(const RLVector &p, const Mat &m);
void xform4(float out[4], const float in[4], const Mat &m);

enum RLClass {
    RLC_FRAME, RLC_MESH, RLC_FACE, RLC_LIGHT, RLC_MATERIAL, RLC_TEXTURE,
    RLC_WRAP, RLC_DEVICE, RLC_VIEWPORT
};

struct RLObjectRec {
    RLClass cls;
    void *appData;
    int refs;           /* references held by other objects */
    int userDestroyed;  /* RLObjectDestroy called while still referenced */
    RLObjectRec(RLClass c);
    virtual ~RLObjectRec();
};

struct RLFrameRec;
struct RLMeshRec;
struct RLTextureRec;
struct RLMaterialRec;

struct RLLightRec : RLObjectRec {
    RLLightType type;
    float r, g, b;
    float umbra, penumbra;
    float constant, linear, quadratic;
    RLFrameRec *frame;
    RLLightRec() : RLObjectRec(RLC_LIGHT), type(RLLightAmbient), r(1), g(1), b(1),
        umbra(0.4f), penumbra(0.5f), constant(1), linear(0), quadratic(0), frame(0) {}
};

struct RLMaterialRec : RLObjectRec {
    float specular, power;
    float emissive[3];          /* extension */
    RLMaterialRec() : RLObjectRec(RLC_MATERIAL), specular(0), power(0)
        { emissive[0] = emissive[1] = emissive[2] = 0; }
};

struct RLTextureRec : RLObjectRec {
    int width, height;          /* of the original image */
    int texWidth, texHeight;    /* as uploaded (power of two if needed) */
    std::vector<unsigned char> rgba;
    unsigned int glName;
    int colours;
    int cutout;                 /* has transparent texels (extension) */
    int clampS, clampT;         /* VRML wrapS/wrapT CLAMP (extension) */
    RLTextureRec() : RLObjectRec(RLC_TEXTURE), width(0), height(0),
        texWidth(0), texHeight(0), glName(0), colours(0), cutout(0),
        clampS(0), clampT(0) {}
    ~RLTextureRec();
};

enum { RLFaceWrapU = 4, RLFaceWrapV = 8 };

struct RLFaceRec : RLObjectRec {
    RLMeshRec *mesh;
    std::vector<int> vertices;
    std::vector<int> normals;
    RLColour colour;
    RLTextureRec *texture;
    RLMaterialRec *material;
    int flags;
    RLFaceRec() : RLObjectRec(RLC_FACE), mesh(0), colour(0xffffffff),
        texture(0), material(0), flags(0) {}
    ~RLFaceRec();
};

struct RLMeshRec : RLObjectRec {
    std::vector<RLVector> vertices;
    std::vector<float> u, v;
    std::vector<RLVector> normals;
    std::vector<RLFaceRec *> faces;
    unsigned int displayList;
    int listQuality;
    int dirty;
    int twoSided;
    RLMeshRec() : RLObjectRec(RLC_MESH), displayList(0), listQuality(-1), dirty(1),
        twoSided(0) {}
    ~RLMeshRec();
    void touch() { dirty = 1; }
};

struct RLFrameRec : RLObjectRec {
    RLFrameRec *parent;
    std::vector<RLFrameRec *> children;
    std::vector<RLObjectRec *> visuals;
    std::vector<RLLightRec *> lights;
    Mat local;
    RLFrameRec() : RLObjectRec(RLC_FRAME), parent(0) { matIdentity(local); }
    ~RLFrameRec();
    Mat world() const;
};

struct RLWrapRec : RLObjectRec {
    RLWrapType type;
    RLFrameRec *ref;
    Mat toWrap;                 /* world -> wrap coordinates */
    double ou, ov, su, sv;
    RLWrapRec() : RLObjectRec(RLC_WRAP), type(RLWrapFlat), ref(0),
        ou(0), ov(0), su(1), sv(1) {}
};

struct RLViewportRec;

struct RLDeviceRec : RLObjectRec {
    HWND hwnd;
    HDC hdc;
    HGLRC glrc;
    int width, height;
    RLRenderQuality quality;
    int dither, shades, colours;
    std::vector<RLViewportRec *> viewports;
    /* the multisampled framebuffer it draws into (see rlrender.cpp) */
    unsigned int fbo, colourBuffer, depthBuffer;
    int fbWidth, fbHeight, fbFailed;
    RLDeviceRec() : RLObjectRec(RLC_DEVICE), hwnd(0), hdc(0), glrc(0), width(0),
        height(0), quality(RLRenderGouraud), dither(0), shades(32), colours(0),
        fbo(0), colourBuffer(0), depthBuffer(0), fbWidth(0), fbHeight(0), fbFailed(0) {}
    ~RLDeviceRec();
};

struct RLViewportRec : RLObjectRec {
    RLDeviceRec *device;
    RLFrameRec *camera;
    int x, y, width, height;
    float front, back;
    float left, right, bottom, top;
    int uniform;
    RLViewportRec() : RLObjectRec(RLC_VIEWPORT), device(0), camera(0), x(0), y(0),
        width(0), height(0), front(1.0f), back(100.0f), left(-0.5f), right(0.5f),
        bottom(-0.5f), top(0.5f), uniform(1) {}
    ~RLViewportRec();
    Mat projection() const;     /* camera space -> homogeneous screen pixels */
};

/* shared state */
extern RLFrameRec *rlRoot;
extern std::vector<RLDeviceRec *> rlDevices;
RLError rlSetError(RLError e);
bool rlValid(RLObjectRec *o, RLClass cls);
void rlAddRef(RLObjectRec *o);
void rlRelease(RLObjectRec *o);
RLFrameRec *rlSceneOf(RLFrameRec *f);

/* rlrender.cpp */
void rlRenderDevice(RLDeviceRec *d);
void rlTextureUpload(RLTextureRec *t);

#endif /* RLINT_H */
