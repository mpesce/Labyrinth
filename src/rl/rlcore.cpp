/*
 * rlcore.cpp -- Reality Lab compatibility layer: objects, frames, meshes,
 * lights, materials, textures, wraps and file loaders.
 *
 * The arithmetic follows the 1995 library where it matters for appearance:
 * frame orientation (0x429f50), axis/angle rotation (0x429df0), combine
 * order (0x425810), wrap texture coordinates (0x42d660) and the XOF reader
 * (0x42da20-0x42ea10).
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <set>
#include "rlint.h"

RLFrameRec *rlRoot;
static RLError lastError;
static std::set<RLObjectRec *> liveObjects;

RLError rlSetError(RLError e)
{
    lastError = e;
    return e;
}

RLError RLGetLastError(void)
{
    return lastError;
}

/* ---- matrices ------------------------------------------------------------ */

void matIdentity(Mat &a)
{
    memset(&a, 0, sizeof(a));
    a.m[0][0] = a.m[1][1] = a.m[2][2] = a.m[3][3] = 1.0f;
}

Mat matMul(const Mat &a, const Mat &b)
{
    Mat r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] +
                        a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
    return r;
}

bool matInverse(Mat &out, const Mat &in)
{
    double a[4][8];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            a[i][j] = in.m[i][j];
            a[i][j + 4] = (i == j) ? 1.0 : 0.0;
        }
    }
    for (int c = 0; c < 4; c++) {
        int p = c;
        for (int r = c + 1; r < 4; r++)
            if (fabs(a[r][c]) > fabs(a[p][c]))
                p = r;
        if (fabs(a[p][c]) < 1e-20) {
            matIdentity(out);
            return false;
        }
        if (p != c)
            for (int j = 0; j < 8; j++) {
                double t = a[c][j];
                a[c][j] = a[p][j];
                a[p][j] = t;
            }
        double d = a[c][c];
        for (int j = 0; j < 8; j++)
            a[c][j] /= d;
        for (int r = 0; r < 4; r++) {
            if (r == c)
                continue;
            double f = a[r][c];
            if (f != 0.0)
                for (int j = 0; j < 8; j++)
                    a[r][j] -= f * a[c][j];
        }
    }
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            out.m[i][j] = (float)a[i][j + 4];
    return true;
}

/* Rows: x = up x dir, y = up made perpendicular to dir, z = dir (0x429f50). */
void matFromOrientation(Mat &a, double dx, double dy, double dz,
                        double ux, double uy, double uz)
{
    matIdentity(a);
    double l = dx * dx + dy * dy + dz * dz;
    if (l == 0.0) {
        rlSetError(RLBadValue);
        return;
    }
    l = 1.0 / sqrt(l);
    dx *= l; dy *= l; dz *= l;
    double d = ux * dx + uy * dy + uz * dz;
    ux -= d * dx; uy -= d * dy; uz -= d * dz;
    l = ux * ux + uy * uy + uz * uz;
    if (l == 0.0) {
        rlSetError(RLBadValue);
        return;
    }
    l = 1.0 / sqrt(l);
    ux *= l; uy *= l; uz *= l;
    a.m[0][0] = (float)(uy * dz - uz * dy);
    a.m[0][1] = (float)(uz * dx - ux * dz);
    a.m[0][2] = (float)(ux * dy - uy * dx);
    a.m[1][0] = (float)ux;
    a.m[1][1] = (float)uy;
    a.m[1][2] = (float)uz;
    a.m[2][0] = (float)dx;
    a.m[2][1] = (float)dy;
    a.m[2][2] = (float)dz;
}

/* Rotation by theta about (x,y,z), with Reality Lab's sign (0x429df0). */
void matRotation(Mat &a, double x, double y, double z, double theta)
{
    matIdentity(a);
    double l = sqrt(x * x + y * y + z * z);
    if (l <= 0.0) {
        rlSetError(RLBadValue);
        return;
    }
    x /= l; y /= l; z /= l;
    double c = cos(-theta), s = sin(-theta), t = 1.0 - c;
    a.m[0][0] = (float)(x * x * t + c);
    a.m[1][0] = (float)(y * x * t + z * s);
    a.m[2][0] = (float)(z * x * t - y * s);
    a.m[0][1] = (float)(y * x * t - z * s);
    a.m[1][1] = (float)(y * y * t + c);
    a.m[2][1] = (float)(y * z * t + x * s);
    a.m[0][2] = (float)(z * x * t + y * s);
    a.m[1][2] = (float)(y * z * t - x * s);
    a.m[2][2] = (float)(z * z * t + c);
}

RLVector xformPoint(const RLVector &p, const Mat &m)
{
    RLVector r;
    r.x = p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0];
    r.y = p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1];
    r.z = p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2];
    return r;
}

RLVector xformDir(const RLVector &p, const Mat &m)
{
    RLVector r;
    r.x = p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0];
    r.y = p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1];
    r.z = p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2];
    return r;
}

void xform4(float out[4], const float in[4], const Mat &m)
{
    float r[4];
    for (int j = 0; j < 4; j++)
        r[j] = in[0] * m.m[0][j] + in[1] * m.m[1][j] + in[2] * m.m[2][j] + in[3] * m.m[3][j];
    memcpy(out, r, sizeof(r));
}

RLVector *RLVectorNormalise(RLVector *v)                        /* 429ec0 */
{
    float l = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    if (l <= 0.0f) {
        rlSetError(RLBadValue);
        return NULL;
    }
    l = 1.0f / l;
    v->x *= l;
    v->y *= l;
    v->z *= l;
    return v;
}

void RLVectorCrossProduct(RLVector *d, RLVector *a, RLVector *b) /* 42b360 */
{
    RLVector r;
    r.x = a->y * b->z - a->z * b->y;
    r.y = a->z * b->x - a->x * b->z;
    r.z = a->x * b->y - a->y * b->x;
    *d = r;
}

/* ---- objects -------------------------------------------------------------- */

RLObjectRec::RLObjectRec(RLClass c) : cls(c), appData(0), refs(0), userDestroyed(0)
{
    liveObjects.insert(this);
}

RLObjectRec::~RLObjectRec()
{
    liveObjects.erase(this);
}

bool rlValid(RLObjectRec *o, RLClass cls)
{
    if (!o || liveObjects.find(o) == liveObjects.end()) {
        rlSetError(RLBadObject);
        return false;
    }
    if (o->cls != cls) {
        rlSetError(RLBadType);
        return false;
    }
    return true;
}

static bool rlAlive(RLObjectRec *o)
{
    return o && liveObjects.find(o) != liveObjects.end();
}

void rlAddRef(RLObjectRec *o)
{
    if (o)
        o->refs++;
}

/* Drop a reference held by another object.  Visuals, lights, textures and
 * materials go away with their last reference. */
void rlRelease(RLObjectRec *o)
{
    if (!rlAlive(o))
        return;
    if (o->refs > 0)
        o->refs--;
    if (o->refs == 0)
        delete o;
}

RLError RLObjectDestroy(RLObject o)                             /* 426560 */
{
    if (!rlAlive(o))
        return rlSetError(RLBadObject);
    if (o->cls == RLC_FRAME) {
        RLFrameRec *f = (RLFrameRec *)o;
        if (f->parent) {
            std::vector<RLFrameRec *> &c = f->parent->children;
            c.erase(std::remove(c.begin(), c.end(), f), c.end());
            f->parent = 0;
        }
        delete f;
        return RLNoError;
    }
    if (o->refs > 0) {
        o->userDestroyed = 1;
        return RLNoError;
    }
    delete o;
    return RLNoError;
}

RLError RLObjectSetAppData(RLObject o, void *data)              /* 426f10 */
{
    if (!rlAlive(o))
        return rlSetError(RLBadObject);
    o->appData = data;
    return RLNoError;
}

void *RLObjectGetAppData(RLObject o)                            /* 42ed40 */
{
    if (!rlAlive(o)) {
        rlSetError(RLBadObject);
        return NULL;
    }
    return o->appData;
}

RLError RLMalloc(void **p, int size)                            /* 426a10 */
{
    if (size == 0) {
        *p = NULL;
        return RLNoError;
    }
    *p = malloc(size);
    if (!*p)
        return rlSetError(RLBadAlloc);
    return RLNoError;
}

void RLFree(void *p)                                            /* 426af0 */
{
    free(p);
}

static int clampByte(RLValue v)
{
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    return (int)(v * 255.0);
}

RLColour RLCreateColourRGB(RLValue r, RLValue g, RLValue b)     /* 42f930 */
{
    return 0xff000000UL | ((RLColour)clampByte(r) << 16) |
           ((RLColour)clampByte(g) << 8) | (RLColour)clampByte(b);
}

/* ---- frames --------------------------------------------------------------- */

static RLFrameRec *root(void)
{
    if (!rlRoot)
        rlRoot = new RLFrameRec;
    return rlRoot;
}

Mat RLFrameRec::world() const
{
    if (!parent || parent == rlRoot)
        return local;
    return matMul(local, parent->world());
}

RLFrameRec::~RLFrameRec()
{
    std::vector<RLFrameRec *> kids = children;
    children.clear();
    for (size_t i = 0; i < kids.size(); i++) {
        kids[i]->parent = 0;
        delete kids[i];
    }
    for (size_t i = 0; i < visuals.size(); i++)
        rlRelease(visuals[i]);
    for (size_t i = 0; i < lights.size(); i++) {
        if (rlAlive(lights[i]) && lights[i]->frame == this)
            lights[i]->frame = 0;
        rlRelease(lights[i]);
    }
}

RLFrameRec *rlSceneOf(RLFrameRec *f)                            /* 4263b0 */
{
    while (f && f->parent && f->parent != rlRoot)
        f = f->parent;
    return f;
}

static void attach(RLFrameRec *parent, RLFrameRec *child)
{
    if (child->parent) {
        std::vector<RLFrameRec *> &c = child->parent->children;
        c.erase(std::remove(c.begin(), c.end(), child), c.end());
    }
    child->parent = parent;
    parent->children.push_back(child);
}

RLFrame RLCreateFrame(RLFrame parent)                           /* 425a60 */
{
    if (parent && !rlValid(parent, RLC_FRAME))
        return NULL;
    RLFrameRec *f = new RLFrameRec;
    attach(parent ? (RLFrameRec *)parent : root(), f);
    return f;
}

RLError RLFrameSetPosition(RLFrame frame, RLFrame ref,
                           RLValue x, RLValue y, RLValue z)     /* 425b20 */
{
    if (!rlValid(frame, RLC_FRAME) || (ref && !rlValid(ref, RLC_FRAME)))
        return lastError;
    RLFrameRec *f = (RLFrameRec *)frame;
    RLVector p = { (float)x, (float)y, (float)z };
    if (ref)
        p = xformPoint(p, ((RLFrameRec *)ref)->world());
    if (f->parent && f->parent != rlRoot) {
        Mat inv;
        matInverse(inv, f->parent->world());
        p = xformPoint(p, inv);
    }
    f->local.m[3][0] = p.x;
    f->local.m[3][1] = p.y;
    f->local.m[3][2] = p.z;
    return RLNoError;
}

RLError RLFrameSetOrientation(RLFrame frame, RLFrame ref,
                              RLValue dx, RLValue dy, RLValue dz,
                              RLValue ux, RLValue uy, RLValue uz) /* 425c50 */
{
    if (!rlValid(frame, RLC_FRAME) || (ref && !rlValid(ref, RLC_FRAME)))
        return lastError;
    RLFrameRec *f = (RLFrameRec *)frame;
    Mat m;
    RLError before = lastError;
    lastError = RLNoError;
    matFromOrientation(m, dx, dy, dz, ux, uy, uz);
    if (lastError != RLNoError)
        return lastError;
    lastError = before;
    if (ref)
        m = matMul(m, ((RLFrameRec *)ref)->world());
    if (f->parent && f->parent != rlRoot) {
        Mat inv;
        matInverse(inv, f->parent->world());
        m = matMul(m, inv);
    }
    /* the position is kept */
    m.m[3][0] = f->local.m[3][0];
    m.m[3][1] = f->local.m[3][1];
    m.m[3][2] = f->local.m[3][2];
    m.m[0][3] = m.m[1][3] = m.m[2][3] = 0.0f;
    m.m[3][3] = 1.0f;
    f->local = m;
    return RLNoError;
}

RLError RLFrameAddRotation(RLFrame frame, RLCombineType combine,
                           RLValue x, RLValue y, RLValue z, RLValue theta) /* 425ea0 */
{
    if (!rlValid(frame, RLC_FRAME))
        return lastError;
    RLFrameRec *f = (RLFrameRec *)frame;
    Mat r;
    matRotation(r, x, y, z, theta);
    switch (combine) {                                          /* 425810 */
    case RLCombineReplace: f->local = r; break;
    case RLCombineBefore: f->local = matMul(r, f->local); break;
    case RLCombineAfter: f->local = matMul(f->local, r); break;
    }
    return RLNoError;
}

static RLError relative(RLFrameRec *f, RLFrame ref, Mat &m)
{
    m = f->world();
    if (ref) {
        if (!rlValid(ref, RLC_FRAME))
            return lastError;
        if ((RLFrameRec *)ref == f)
            return rlSetError(RLBadValue);
        Mat inv;
        matInverse(inv, ((RLFrameRec *)ref)->world());
        m = matMul(m, inv);
    }
    return RLNoError;
}

RLError RLFrameGetPosition(RLFrame frame, RLFrame ref, RLVector *pos) /* 426050 */
{
    if (!rlValid(frame, RLC_FRAME))
        return lastError;
    Mat m;
    RLError e = relative((RLFrameRec *)frame, ref, m);
    if (e != RLNoError)
        return e;
    pos->x = m.m[3][0];
    pos->y = m.m[3][1];
    pos->z = m.m[3][2];
    return RLNoError;
}

RLError RLFrameGetOrientation(RLFrame frame, RLFrame ref,
                              RLVector *dir, RLVector *up)      /* 425f40 */
{
    if (!rlValid(frame, RLC_FRAME))
        return lastError;
    Mat m;
    RLError e = relative((RLFrameRec *)frame, ref, m);
    if (e != RLNoError)
        return e;
    dir->x = m.m[2][0]; dir->y = m.m[2][1]; dir->z = m.m[2][2];
    up->x = m.m[1][0]; up->y = m.m[1][1]; up->z = m.m[1][2];
    return RLNoError;
}

RLError RLFrameDeleteChild(RLFrame frame, RLFrame child)        /* 426140 */
{
    if (!rlValid(frame, RLC_FRAME) || !rlValid(child, RLC_FRAME))
        return lastError;
    RLFrameRec *c = (RLFrameRec *)child;
    if (c->parent != (RLFrameRec *)frame)
        return rlSetError(RLNotFound);
    return RLObjectDestroy(c);
}

RLError RLFrameAddVisual(RLFrame frame, RLVisual visual)        /* 4261c0 */
{
    if (!rlValid(frame, RLC_FRAME))
        return lastError;
    if (!rlAlive(visual))
        return rlSetError(RLBadObject);
    if (visual->cls != RLC_MESH && visual->cls != RLC_TEXTURE && visual->cls != RLC_FRAME)
        return rlSetError(RLBadType);
    ((RLFrameRec *)frame)->visuals.push_back(visual);
    rlAddRef(visual);
    return RLNoError;
}

RLError RLFrameDeleteVisual(RLFrame frame, RLVisual visual)     /* 426240 */
{
    if (!rlValid(frame, RLC_FRAME))
        return lastError;
    std::vector<RLObjectRec *> &v = ((RLFrameRec *)frame)->visuals;
    std::vector<RLObjectRec *>::iterator i = std::find(v.begin(), v.end(), visual);
    if (i == v.end())
        return rlSetError(RLNotFound);
    v.erase(i);
    rlRelease(visual);
    return RLNoError;
}

RLError RLFrameAddLight(RLFrame frame, RLLight light)           /* 4262f0 */
{
    if (!rlValid(frame, RLC_FRAME) || !rlValid(light, RLC_LIGHT))
        return lastError;
    RLLightRec *l = (RLLightRec *)light;
    rlAddRef(l);
    if (l->frame) {
        std::vector<RLLightRec *> &v = l->frame->lights;
        v.erase(std::remove(v.begin(), v.end(), l), v.end());
        rlRelease(l);
    }
    ((RLFrameRec *)frame)->lights.push_back(l);
    l->frame = (RLFrameRec *)frame;
    return RLNoError;
}

RLError RLFrameDeleteLight(RLFrame frame, RLLight light)        /* extension */
{
    if (!rlValid(frame, RLC_FRAME) || !rlValid(light, RLC_LIGHT))
        return lastError;
    std::vector<RLLightRec *> &v = ((RLFrameRec *)frame)->lights;
    std::vector<RLLightRec *>::iterator i = std::find(v.begin(), v.end(), (RLLightRec *)light);
    if (i == v.end())
        return rlSetError(RLNotFound);
    v.erase(i);
    ((RLLightRec *)light)->frame = 0;
    rlRelease(light);
    return RLNoError;
}

RLError RLMeshSetTwoSided(RLMesh mesh, int on)                  /* extension */
{
    if (!rlValid(mesh, RLC_MESH))
        return lastError;
    ((RLMeshRec *)mesh)->twoSided = on;
    return RLNoError;
}

RLError RLFrameGetChildren(RLFrame frame, int *count, RLFrame **list) /* 4263f0 */
{
    RLFrameRec *f = frame ? (RLFrameRec *)frame : root();
    if (frame && !rlValid(frame, RLC_FRAME))
        return lastError;
    int n = (int)f->children.size();
    if (RLMalloc((void **)list, n * (int)sizeof(RLFrame)) != RLNoError)
        return lastError;
    for (int i = 0; i < n; i++)
        (*list)[i] = f->children[i];
    *count = n;
    return RLNoError;
}

RLError RLFrameGetVisuals(RLFrame frame, int *count, RLVisual **list) /* 4264a0 */
{
    if (!rlValid(frame, RLC_FRAME))
        return lastError;
    RLFrameRec *f = (RLFrameRec *)frame;
    int n = (int)f->visuals.size();
    if (RLMalloc((void **)list, n * (int)sizeof(RLVisual)) != RLNoError)
        return lastError;
    for (int i = 0; i < n; i++)
        (*list)[i] = f->visuals[i];
    *count = n;
    return RLNoError;
}

/* ---- lights and materials ------------------------------------------------- */

RLLight RLCreateLightRGB(RLLightType type, RLValue r, RLValue g, RLValue b) /* 426950 */
{
    RLLightRec *l = new RLLightRec;
    l->type = type;
    l->r = (float)r;
    l->g = (float)g;
    l->b = (float)b;
    return l;
}

RLMaterial RLCreateMaterial(RLValue specular, RLValue power)    /* 42ba20 */
{
    if (!(specular > 0.0 && specular <= 1.0 && power > 0.0)) {
        rlSetError(RLBadValue);
        return NULL;
    }
    RLMaterialRec *m = new RLMaterialRec;
    m->specular = (float)specular;
    m->power = (float)power;
    return m;
}

RLMaterial RLCreateMaterialEmissive(RLValue specular, RLValue power,
                                    RLValue r, RLValue g, RLValue b)   /* extension */
{
    if (!(specular >= 0.0 && specular <= 1.0 && power >= 0.0)) {
        rlSetError(RLBadValue);
        return NULL;
    }
    RLMaterialRec *m = new RLMaterialRec;
    m->specular = (float)specular;
    m->power = (float)power;
    m->emissive[0] = (float)r;
    m->emissive[1] = (float)g;
    m->emissive[2] = (float)b;
    return m;
}

/* ---- meshes and faces ----------------------------------------------------- */

RLFaceRec::~RLFaceRec()
{
    rlRelease(texture);
    rlRelease(material);
}

RLMeshRec::~RLMeshRec()
{
    for (size_t i = 0; i < faces.size(); i++)
        delete faces[i];
    extern void rlFreeDisplayList(unsigned int);
    if (displayList)
        rlFreeDisplayList(displayList);
}

static void setFaceTexture(RLFaceRec *f, RLTextureRec *t)
{
    if (t)
        rlAddRef(t);
    rlRelease(f->texture);
    f->texture = t;
}

static void setFaceMaterial(RLFaceRec *f, RLMaterialRec *m)
{
    if (m)
        rlAddRef(m);
    rlRelease(f->material);
    f->material = m;
}

RLMesh RLCreateMesh(void)                                       /* 428ba0 */
{
    return new RLMeshRec;
}

static int addVertex(RLMeshRec *m, float x, float y, float z)
{
    RLVector p = { x, y, z };
    m->vertices.push_back(p);
    m->u.push_back(0.0f);
    m->v.push_back(0.0f);
    m->touch();
    return (int)m->vertices.size() - 1;
}

int RLMeshAddVertex(RLMesh mesh, RLValue x, RLValue y, RLValue z) /* 429330 */
{
    if (!rlValid(mesh, RLC_MESH))
        return -1;
    return addVertex((RLMeshRec *)mesh, (float)x, (float)y, (float)z);
}

int RLMeshAddNormal(RLMesh mesh, RLValue x, RLValue y, RLValue z) /* 429400 */
{
    if (!rlValid(mesh, RLC_MESH))
        return -1;
    RLMeshRec *m = (RLMeshRec *)mesh;
    RLVector n = { (float)x, (float)y, (float)z };
    m->normals.push_back(n);
    m->touch();
    return (int)m->normals.size() - 1;
}

RLFace RLMeshCreateFace(RLMesh mesh)                            /* 4292a0 */
{
    if (!rlValid(mesh, RLC_MESH))
        return NULL;
    RLMeshRec *m = (RLMeshRec *)mesh;
    RLFaceRec *f = new RLFaceRec;
    f->mesh = m;
    m->faces.push_back(f);
    m->touch();
    return f;
}

static RLVector faceNormal(RLMeshRec *m, const std::vector<int> &vi)
{
    RLVector n = { 0, 0, 0 };
    size_t k = vi.size();
    for (size_t i = 0; i < k; i++) {       /* Newell's method */
        const RLVector &a = m->vertices[vi[i]];
        const RLVector &b = m->vertices[vi[(i + 1) % k]];
        n.x += (a.y - b.y) * (a.z + b.z);
        n.y += (a.z - b.z) * (a.x + b.x);
        n.z += (a.x - b.x) * (a.y + b.y);
    }
    /* Reality Lab faces are clockwise seen from the front, so in its
     * left-handed space Newell's sum points the right way. */
    RLError e = lastError;
    if (!RLVectorNormalise(&n)) {
        n.x = 0; n.y = 0; n.z = -1;
    }
    lastError = e;
    return n;
}

/* Add vertices, normals and faces in one go.  'data' holds, for each face,
 * a vertex count followed by that many vertex indices -- each followed by a
 * normal index when normals are supplied -- and ends with a zero. */
RLError RLMeshAddFaces(RLMesh mesh, int vcount, RLVector *verts,
                       int ncount, RLVector *norms, int *data,
                       int *fcount, RLFace **faces)             /* 428330 */
{
    if (!rlValid(mesh, RLC_MESH))
        return lastError;
    RLMeshRec *m = (RLMeshRec *)mesh;
    int vbase = (int)m->vertices.size();
    int nbase = (int)m->normals.size();
    std::vector<RLFaceRec *> made;

    for (int i = 0; i < vcount; i++)
        addVertex(m, verts[i].x, verts[i].y, verts[i].z);
    if (ncount > 0) {
        for (int i = 0; i < ncount; i++) {
            RLVector n = norms[i];
            RLError e = lastError;
            RLVectorNormalise(&n);
            lastError = e;
            m->normals.push_back(n);
        }
    }

    std::vector<RLVector> accum;
    if (ncount <= 0)
        accum.assign(vcount, RLVector());
    for (int *p = data; *p; ) {
        int n = *p++;
        RLFaceRec *f = new RLFaceRec;
        f->mesh = m;
        for (int k = 0; k < n; k++) {
            int vi = *p++;
            int ni = vi;
            if (ncount > 0)
                ni = *p++;
            f->vertices.push_back(vbase + vi);
            f->normals.push_back(nbase + ni);
        }
        if (ncount <= 0) {          /* average the face normals per vertex */
            RLVector fn = faceNormal(m, f->vertices);
            for (int k = 0; k < n; k++) {
                int vi = f->vertices[k] - vbase;
                if (vi >= 0 && vi < vcount) {
                    accum[vi].x += fn.x;
                    accum[vi].y += fn.y;
                    accum[vi].z += fn.z;
                }
            }
        }
        m->faces.push_back(f);
        made.push_back(f);
    }
    if (ncount <= 0) {
        for (int i = 0; i < vcount; i++) {
            RLError e = lastError;
            if (!RLVectorNormalise(&accum[i])) {
                accum[i].x = 0; accum[i].y = 0; accum[i].z = -1;
            }
            lastError = e;
            m->normals.push_back(accum[i]);
        }
    }
    if (fcount)
        *fcount = (int)made.size();
    if (faces) {
        if (RLMalloc((void **)faces, (int)made.size() * (int)sizeof(RLFace)) != RLNoError)
            return lastError;
        for (size_t i = 0; i < made.size(); i++)
            (*faces)[i] = made[i];
    }
    m->touch();
    return RLNoError;
}

RLError RLMeshSetTextureCoordinates(RLMesh mesh, int index, RLValue u, RLValue v) /* 429210 */
{
    if (!rlValid(mesh, RLC_MESH))
        return lastError;
    RLMeshRec *m = (RLMeshRec *)mesh;
    if (index < 0 || index >= (int)m->vertices.size())
        return rlSetError(RLBadValue);
    m->u[index] = (float)u;
    m->v[index] = (float)v;
    m->touch();
    return RLNoError;
}

RLError RLMeshSetColour(RLMesh mesh, RLColour colour)           /* 429970 */
{
    if (!rlValid(mesh, RLC_MESH))
        return lastError;
    RLMeshRec *m = (RLMeshRec *)mesh;
    for (size_t i = 0; i < m->faces.size(); i++)
        m->faces[i]->colour = colour;
    m->touch();
    return RLNoError;
}

RLError RLMeshSetColourRGB(RLMesh mesh, RLValue r, RLValue g, RLValue b) /* 4299d0 */
{
    return RLMeshSetColour(mesh, RLCreateColourRGB(r, g, b));
}

RLError RLMeshSetTexture(RLMesh mesh, RLTexture texture)        /* 429a10 */
{
    if (!rlValid(mesh, RLC_MESH) || (texture && !rlValid(texture, RLC_TEXTURE)))
        return lastError;
    RLMeshRec *m = (RLMeshRec *)mesh;
    for (size_t i = 0; i < m->faces.size(); i++)
        setFaceTexture(m->faces[i], (RLTextureRec *)texture);
    m->touch();
    return RLNoError;
}

RLError RLMeshSetMaterial(RLMesh mesh, RLMaterial material)     /* 429ae0 */
{
    if (!rlValid(mesh, RLC_MESH) || (material && !rlValid(material, RLC_MATERIAL)))
        return lastError;
    RLMeshRec *m = (RLMeshRec *)mesh;
    for (size_t i = 0; i < m->faces.size(); i++)
        setFaceMaterial(m->faces[i], (RLMaterialRec *)material);
    m->touch();
    return RLNoError;
}

RLError RLMeshGetBox(RLMesh mesh, RLBox *box)                   /* 429bd0 */
{
    if (!rlValid(mesh, RLC_MESH))
        return lastError;
    RLMeshRec *m = (RLMeshRec *)mesh;
    if (m->vertices.empty()) {
        memset(box, 0, sizeof(*box));
        return RLNoError;
    }
    box->min = box->max = m->vertices[0];
    for (size_t i = 1; i < m->vertices.size(); i++) {
        const RLVector &p = m->vertices[i];
        if (p.x < box->min.x) box->min.x = p.x;
        if (p.y < box->min.y) box->min.y = p.y;
        if (p.z < box->min.z) box->min.z = p.z;
        if (p.x > box->max.x) box->max.x = p.x;
        if (p.y > box->max.y) box->max.y = p.y;
        if (p.z > box->max.z) box->max.z = p.z;
    }
    return RLNoError;
}

RLError RLMeshGetFaces(RLMesh mesh, int *count, RLFace **faces) /* 429cb0 */
{
    if (!rlValid(mesh, RLC_MESH))
        return lastError;
    RLMeshRec *m = (RLMeshRec *)mesh;
    int n = (int)m->faces.size();
    if (RLMalloc((void **)faces, n * (int)sizeof(RLFace)) != RLNoError)
        return lastError;
    for (int i = 0; i < n; i++)
        (*faces)[i] = m->faces[i];
    *count = n;
    return RLNoError;
}

RLError RLFaceAddVertexAndNormalIndexed(RLFace face, int vertex, int normal) /* 4272a0 */
{
    if (!rlValid(face, RLC_FACE))
        return lastError;
    RLFaceRec *f = (RLFaceRec *)face;
    if (!f->mesh)
        return rlSetError(RLNotFound);
    if (vertex < 0 || vertex >= (int)f->mesh->vertices.size() ||
        normal < 0 || normal >= (int)f->mesh->normals.size())
        return rlSetError(RLBadValue);
    f->vertices.push_back(vertex);
    f->normals.push_back(normal);
    if (f->vertices.size() > 3)
        f->flags &= ~0;     /* (more than a triangle: no fast path) */
    f->mesh->touch();
    return RLNoError;
}

RLError RLFaceSetColour(RLFace face, RLColour colour)           /* 4273d0 */
{
    if (!rlValid(face, RLC_FACE))
        return lastError;
    RLFaceRec *f = (RLFaceRec *)face;
    f->colour = colour;
    if (f->mesh)
        f->mesh->touch();
    return RLNoError;
}

RLError RLFaceSetColourRGB(RLFace face, RLValue r, RLValue g, RLValue b) /* 427420 */
{
    return RLFaceSetColour(face, RLCreateColourRGB(r, g, b));
}

RLError RLFaceSetTexture(RLFace face, RLTexture texture)        /* 427460 */
{
    if (!rlValid(face, RLC_FACE) || (texture && !rlValid(texture, RLC_TEXTURE)))
        return lastError;
    RLFaceRec *f = (RLFaceRec *)face;
    setFaceTexture(f, (RLTextureRec *)texture);
    if (f->mesh)
        f->mesh->touch();
    return RLNoError;
}

/* ---- textures ------------------------------------------------------------- */

RLTextureRec::~RLTextureRec()
{
    extern void rlFreeTexture(unsigned int);
    if (glName)
        rlFreeTexture(glName);
}

static int maskShift(unsigned long mask)
{
    int s = 0;
    if (!mask)
        return 0;
    while (!(mask & 1)) {
        mask >>= 1;
        s++;
    }
    return s;
}

static int maskBits(unsigned long mask)
{
    int n = 0;
    mask >>= maskShift(mask);
    while (mask & 1) {
        mask >>= 1;
        n++;
    }
    return n;
}

static unsigned char expand(unsigned long pixel, unsigned long mask)
{
    if (!mask)
        return 255;
    int bits = maskBits(mask);
    unsigned long v = (pixel & mask) >> maskShift(mask);
    if (bits >= 8)
        return (unsigned char)(v >> (bits - 8));
    return (unsigned char)((v * 255) / ((1UL << bits) - 1));
}

RLTexture RLCreateTexture(RLImage *image)                       /* 42d370 */
{
    if (!image || image->width <= 0 || image->height <= 0 || !image->buffer1) {
        rlSetError(RLBadValue);
        return NULL;
    }
    RLTextureRec *t = new RLTextureRec;
    t->width = image->width;
    t->height = image->height;
    t->rgba.resize((size_t)t->width * t->height * 4);
    const unsigned char *row = (const unsigned char *)image->buffer1;
    for (int y = 0; y < t->height; y++, row += image->bytes_per_line) {
        unsigned char *out = &t->rgba[(size_t)y * t->width * 4];
        for (int x = 0; x < t->width; x++, out += 4) {
            if (!image->rgb && image->depth == 8) {
                int i = row[x];
                if (image->palette && i < image->palette_size) {
                    out[0] = image->palette[i].red;
                    out[1] = image->palette[i].green;
                    out[2] = image->palette[i].blue;
                } else {
                    out[0] = out[1] = out[2] = 0;
                }
                out[3] = 255;
            } else {
                unsigned long p = 0;
                int bytes = image->depth / 8;
                for (int k = 0; k < bytes; k++)
                    p |= (unsigned long)row[x * bytes + k] << (8 * k);
                out[0] = expand(p, image->red_mask);
                out[1] = expand(p, image->green_mask);
                out[2] = expand(p, image->blue_mask);
                out[3] = image->alpha_mask ? expand(p, image->alpha_mask) : 255;
            }
        }
    }
    return t;
}

RLError RLTextureSetColours(RLTexture texture, int colours)    /* 42d3e0 */
{
    if (!rlValid(texture, RLC_TEXTURE))
        return lastError;
    ((RLTextureRec *)texture)->colours = colours;
    return RLNoError;
}

/* Read a PPM picture (the original accepted only raw "P6" files). */
RLTexture RLLoadTexture(const char *file)                       /* 42b3f0 */
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        rlSetError(RLFileNotFound);
        return NULL;
    }
    char magic[3] = { 0, 0, 0 };
    int vals[3], n = 0;
    if (fread(magic, 1, 2, fp) != 2 || magic[0] != 'P' || (magic[1] != '6' && magic[1] != '3')) {
        fclose(fp);
        rlSetError(RLBadFile);
        return NULL;
    }
    while (n < 3) {                         /* width, height, maxval */
        int c = getc(fp);
        if (c == EOF)
            break;
        if (c == '#') {
            while (c != '\n' && c != EOF)
                c = getc(fp);
            continue;
        }
        if (c >= '0' && c <= '9') {
            int v = 0;
            while (c >= '0' && c <= '9') {
                v = v * 10 + (c - '0');
                c = getc(fp);
            }
            vals[n++] = v;
        }
    }
    if (n < 3 || vals[0] <= 0 || vals[1] <= 0 || vals[2] <= 0) {
        fclose(fp);
        rlSetError(RLBadFile);
        return NULL;
    }
    int w = vals[0], h = vals[1], maxval = vals[2];
    RLTextureRec *t = new RLTextureRec;
    t->width = w;
    t->height = h;
    t->rgba.resize((size_t)w * h * 4);
    for (int i = 0; i < w * h; i++) {
        for (int k = 0; k < 3; k++) {
            int v;
            if (magic[1] == '6') {
                v = getc(fp);
                if (maxval > 255)
                    v = (v << 8) | getc(fp);
            } else if (fscanf(fp, "%d", &v) != 1) {
                v = 0;
            }
            if (v < 0)
                v = 0;
            t->rgba[i * 4 + k] = (unsigned char)((v * 255) / maxval);
        }
        t->rgba[i * 4 + 3] = 255;
    }
    fclose(fp);
    return t;
}

/* ---- wraps ---------------------------------------------------------------- */

RLWrap RLCreateWrap(RLWrapType type, RLFrame ref,
                    RLValue ox, RLValue oy, RLValue oz,
                    RLValue dx, RLValue dy, RLValue dz,
                    RLValue ux, RLValue uy, RLValue uz,
                    RLValue ou, RLValue ov, RLValue su, RLValue sv) /* 42d460 */
{
    if (ref && !rlValid(ref, RLC_FRAME))
        return NULL;
    RLWrapRec *w = new RLWrapRec;
    w->type = type;
    w->ref = (RLFrameRec *)ref;
    Mat m;
    matFromOrientation(m, dx, dy, dz, ux, uy, uz);
    m.m[3][0] = (float)ox;
    m.m[3][1] = (float)oy;
    m.m[3][2] = (float)oz;
    matInverse(w->toWrap, m);
    w->ou = ou;
    w->ov = ov;
    w->su = su;
    w->sv = sv;
    return w;
}

static void wrapVertex(RLWrapRec *w, RLMeshRec *m, int i)       /* 42d660 */
{
    RLVector p = xformPoint(m->vertices[i], w->toWrap);
    double a, b;
    switch (w->type) {
    case RLWrapFlat:
        m->u[i] = (float)(w->su * p.x - w->ou);
        m->v[i] = (float)(w->sv * p.y - w->ov);
        break;
    case RLWrapCylinder:
        a = (p.x == 0.0f && p.y == 0.0f) ? 0.0 : atan2(p.x, p.y) / (2.0 * 3.14159265358979);
        if (a < 0.0)
            a += 1.0;
        m->u[i] = (float)(w->su * a - w->ou);
        m->v[i] = (float)(w->sv * p.z - w->ov);
        break;
    case RLWrapSphere:
        a = (p.x == 0.0f && p.y == 0.0f) ? 0.0 : atan2(p.x, p.y) / (2.0 * 3.14159265358979);
        if (a < 0.0)
            a += 1.0;
        if (p.x == 0.0f && p.y == 0.0f && p.z == 0.0f)
            b = 0.0;
        else
            b = acos(p.z / sqrt(p.x * p.x + p.y * p.y + p.z * p.z)) / 3.14159265358979;
        m->u[i] = (float)(w->su * a - w->ou);
        m->v[i] = (float)(w->sv * b - w->ov);
        break;
    default:
        break;
    }
}

RLError RLWrapApply(RLWrap wrap, RLObject object)               /* 42d8c0 */
{
    if (!rlValid(wrap, RLC_WRAP))
        return lastError;
    RLWrapRec *w = (RLWrapRec *)wrap;
    if (!rlAlive(object))
        return rlSetError(RLBadObject);
    int topology = (w->type == RLWrapCylinder || w->type == RLWrapSphere) ? RLFaceWrapU : 0;
    if (object->cls == RLC_MESH) {
        RLMeshRec *m = (RLMeshRec *)object;
        if (w->type == RLWrapChrome)
            return RLNoError;
        for (size_t i = 0; i < m->vertices.size(); i++)
            wrapVertex(w, m, (int)i);
        for (size_t i = 0; i < m->faces.size(); i++)
            m->faces[i]->flags = topology;
        m->touch();
        return RLNoError;
    }
    if (object->cls == RLC_FACE) {
        RLFaceRec *f = (RLFaceRec *)object;
        for (size_t i = 0; i < f->vertices.size(); i++)
            wrapVertex(w, f->mesh, f->vertices[i]);
        f->flags = topology;
        f->mesh->touch();
        return RLNoError;
    }
    return rlSetError(RLBadType);
}

/* ---- XOF shape files (loadxof.c) ------------------------------------------ */

/*
 * An XOF file starts with "xof " and holds a sequence of chunks.  Each chunk
 * is written either in binary or, when it starts with 't', as text with one
 * value per tab- or newline-terminated field.  A chunk header gives the
 * chunk type, the element type, the elements per record (0: each record
 * starts with its own count), a byte size, the number of records and whether
 * the records are of fixed size.
 */
enum { XOF_INFO = 1, XOF_VERTICES = 3, XOF_NORMALS = 4, XOF_TEXCOORDS = 5,
       XOF_MATERIALS = 6, XOF_FACES = 8 };
enum { XT_BYTE = 1, XT_SHORT = 2, XT_LONG = 3, XT_STRING = 4, XT_USHORT = 5,
       XT_ULONG = 6, XT_FLOAT = 7, XT_DOUBLE = 8 };
static const int xofSize[9] = { 0, 1, 2, 4, 0, 2, 4, 4, 8 };

struct XofChunk {
    int text;
    int type, etype, perRecord, size, records, fixed;
    std::vector<std::vector<double> > data;     /* one vector per record */
};

static int rd16(FILE *fp)                                       /* 42da20 */
{
    int lo = getc(fp);
    int hi = getc(fp);
    if (hi == EOF)
        return -1;
    return (short)((hi << 8) | lo);
}

static long rd32(FILE *fp)                                      /* 42dab0 */
{
    int lo = rd16(fp) & 0xffff;
    int hi = rd16(fp);
    if (hi == -1)
        return -1;
    return (long)(((unsigned long)hi << 16) | (unsigned long)lo);
}

static const char *xofField(FILE *fp)                           /* 42dce0 */
{
    static char buf[128];
    int n = 0, c;
    for (;;) {
        c = getc(fp);
        if (c == '\t' || c == '\n' || c == EOF)
            break;
        if (c != '\r' && n < (int)sizeof(buf) - 1)
            buf[n++] = (char)c;
    }
    if (c == '\t') {                        /* a tab may end the line */
        c = getc(fp);
        if (c == '\r')
            c = getc(fp);
        if (c != '\n' && c != EOF)
            ungetc(c, fp);
    }
    buf[n] = '\0';
    return buf;
}

static double xofValue(FILE *fp, int text, int etype)
{
    if (text) {
        const char *s = xofField(fp);
        return atof(s);
    }
    switch (etype) {
    case XT_BYTE: return (double)getc(fp);
    case XT_SHORT: return (double)rd16(fp);
    case XT_USHORT: return (double)(rd16(fp) & 0xffff);
    case XT_LONG: return (double)rd32(fp);
    case XT_ULONG: return (double)(unsigned long)rd32(fp);
    case XT_FLOAT: {
        long bits = rd32(fp);
        float f;
        memcpy(&f, &bits, 4);
        return f;
    }
    case XT_DOUBLE: {
        double d;
        if (fread(&d, 1, 8, fp) != 8)
            return 0.0;
        return d;
    }
    }
    return 0.0;
}

static int xofChunk(FILE *fp, XofChunk &c)                      /* 42e100 */
{
    int mode = getc(fp);
    if (mode == EOF)
        return -1;
    c.text = mode == 't';
    if (c.text) {
        const char *s;
        if (!*(s = xofField(fp))) return -1; c.type = atoi(s);
        if (!*(s = xofField(fp))) return -1; c.etype = atoi(s);
        if (!*(s = xofField(fp))) return -1; c.perRecord = atoi(s);
        if (!*(s = xofField(fp))) return -1; c.size = atoi(s);
        if (!*(s = xofField(fp))) return -1; c.records = atoi(s);
        if (!*(s = xofField(fp))) return -1; c.fixed = atoi(s);
    } else {
        c.type = rd16(fp);
        c.etype = rd16(fp);
        c.perRecord = rd32(fp);
        c.size = rd32(fp);
        c.records = rd32(fp);
        c.fixed = rd16(fp);
        if (feof(fp))
            return -1;
    }
    if (c.etype < 0 || c.etype > 8 || c.records < 0 || c.records > 1000000)
        return -1;
    c.data.assign(c.records, std::vector<double>());
    for (int r = 0; r < c.records; r++) {
        int n = c.perRecord;
        if (n == 0 && c.etype == XT_STRING) {
            xofField(fp);               /* strings are kept only as text */
            continue;
        }
        if (n == 0) {                   /* record carries its own count */
            n = (int)xofValue(fp, c.text, c.etype);
            if (n < 0 || n > 100000)
                return -1;
        }
        if (c.etype == XT_STRING) {
            for (int k = 0; k < n; k++)
                xofField(fp);
            continue;
        }
        if (c.etype == XT_BYTE && !c.text) {
            for (int k = 0; k < n; k++)
                c.data[r].push_back((double)getc(fp));
            continue;
        }
        for (int k = 0; k < n; k++)
            c.data[r].push_back(xofValue(fp, c.text, c.etype));
        if (feof(fp))
            return -1;
    }
    return 0;
}

static void generateNormals(RLMeshRec *m)                       /* 4294f0 */
{
    std::vector<RLVector> acc(m->vertices.size(), RLVector());
    for (size_t i = 0; i < m->faces.size(); i++) {
        RLFaceRec *f = m->faces[i];
        if (f->vertices.size() < 3)
            continue;
        RLVector n = faceNormal(m, f->vertices);
        for (size_t k = 0; k < f->vertices.size(); k++) {
            acc[f->vertices[k]].x += n.x;
            acc[f->vertices[k]].y += n.y;
            acc[f->vertices[k]].z += n.z;
        }
    }
    m->normals.resize(m->vertices.size());
    for (size_t i = 0; i < acc.size(); i++) {
        RLError e = lastError;
        if (!RLVectorNormalise(&acc[i])) {
            acc[i].x = 0; acc[i].y = 0; acc[i].z = -1;
        }
        lastError = e;
        m->normals[i] = acc[i];
    }
    for (size_t i = 0; i < m->faces.size(); i++)
        m->faces[i]->normals = m->faces[i]->vertices;
}

RLMesh RLLoadMesh(const char *file)                             /* 42e3c0 */
{
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        rlSetError(RLFileNotFound);
        return NULL;
    }
    if (rd32(fp) != 0x20666f78) {       /* "xof " */
        fclose(fp);
        rlSetError(RLBadFile);
        return NULL;
    }
    XofChunk verts, norms, uvs, mats, faces, c;
    verts.records = norms.records = uvs.records = mats.records = faces.records = -1;
    while (xofChunk(fp, c) == 0) {
        switch (c.type) {
        case XOF_VERTICES: verts = c; break;
        case XOF_NORMALS: norms = c; break;
        case XOF_TEXCOORDS: uvs = c; break;
        case XOF_MATERIALS: mats = c; break;
        case XOF_FACES: faces = c; break;
        }
    }
    fclose(fp);
    if (verts.records < 0 || faces.records < 0) {
        rlSetError(RLBadFile);
        return NULL;
    }

    RLMeshRec *m = new RLMeshRec;
    for (int i = 0; i < verts.records; i++) {
        std::vector<double> &p = verts.data[i];
        addVertex(m, p.size() > 0 ? (float)p[0] : 0, p.size() > 1 ? (float)p[1] : 0,
                  p.size() > 2 ? (float)p[2] : 0);
    }
    if (norms.records >= 0) {
        for (int i = 0; i < norms.records; i++) {
            std::vector<double> &p = norms.data[i];
            RLVector n = { p.size() > 0 ? (float)p[0] : 0, p.size() > 1 ? (float)p[1] : 0,
                           p.size() > 2 ? (float)p[2] : 0 };
            m->normals.push_back(n);
        }
    }
    if (uvs.records >= 0) {
        for (int i = 0; i < uvs.records && i < (int)m->vertices.size(); i++) {
            std::vector<double> &p = uvs.data[i];
            if (p.size() >= 2) {
                m->u[i] = (float)p[0];
                m->v[i] = (float)p[1];
            }
        }
    }
    std::vector<RLColour> matColour;
    std::vector<RLMaterialRec *> matMaterial;
    if (mats.records >= 0) {
        for (int i = 0; i < mats.records; i++) {
            std::vector<double> &p = mats.data[i];
            while (p.size() < 6)
                p.push_back(0.0);
            RLMaterialRec *mm = 0;
            if (p[0] > 0.0 && p[1] > 0.0)
                mm = (RLMaterialRec *)RLCreateMaterial(p[0] > 1.0 ? 1.0 : p[0], p[1]);
            matMaterial.push_back(mm);
            matColour.push_back(RLCreateColourRGB(p[2], p[3], p[4]));
        }
    }
    for (int i = 0; i < faces.records; i++) {
        std::vector<double> &p = faces.data[i];
        if (p.size() < 3)
            continue;
        int nv = (int)p[0];
        int mat = (int)p[1];
        int flags = (int)p[2];
        RLFaceRec *f = new RLFaceRec;
        f->mesh = m;
        f->flags = ((flags & 1) ? RLFaceWrapU : 0) | ((flags & 2) ? RLFaceWrapV : 0);
        for (int k = 0; k < nv && 3 + 2 * k + 1 < (int)p.size(); k++) {
            int vi = (int)p[3 + 2 * k];
            int ni = (int)p[4 + 2 * k];
            if (vi < 0 || vi >= (int)m->vertices.size())
                continue;
            f->vertices.push_back(vi);
            f->normals.push_back(norms.records >= 0 ? ni : vi);
        }
        if (mat >= 0 && mat < (int)matColour.size()) {
            f->colour = matColour[mat];
            setFaceMaterial(f, matMaterial[mat]);
        }
        m->faces.push_back(f);
    }
    for (size_t i = 0; i < matMaterial.size(); i++)
        if (matMaterial[i] && matMaterial[i]->refs == 0)
            delete matMaterial[i];
    if (norms.records < 0) {
        generateNormals(m);
    } else {
        for (size_t i = 0; i < m->faces.size(); i++)
            for (size_t k = 0; k < m->faces[i]->normals.size(); k++)
                if (m->faces[i]->normals[k] >= (int)m->normals.size())
                    m->faces[i]->normals[k] = 0;
        if (m->normals.empty())
            generateNormals(m);
    }
    return m;
}
