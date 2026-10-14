/*
 * qvbuild.cpp -- turning a VRML 1.0 scene graph into Reality Lab objects.
 *
 * New code (Labyrinth 2.0a2 parsed VRML but did not draw it).  Called from
 * the traversal methods in qvglue.cpp while QvLib walks the scene, it
 * builds frames, meshes and lights under the scene frame the same way the
 * Labyrinth world compiler does, so everything else in Labyrinth -- the
 * camera, navigation, picking, hot links -- works on VRML worlds unchanged:
 *
 *  - transformations are taken from QvLib's transformation stack at each
 *    shape and baked into the vertices;
 *  - VRML is right-handed and Reality Lab left-handed, so z is negated and
 *    the polygons' vertex order reversed;
 *  - shapes inside a WWWAnchor are put in a frame of their own whose
 *    application data is a WcElement carrying the URL, which is what the
 *    drawing window looks for when the mouse is over an object;
 *  - WWWInline worlds are fetched (and gunzipped) and read in place;
 *  - the first camera sets the view; with none, the view is fitted to the
 *    world; with no lights, a headlight is added to the camera.
 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <map>
#include <string>
#include <vector>
#include <QvDB.h>
#include <QvInput.h>
#include <QvNodes.h>
#include <QvState.h>
#include <QvElement.h>
#include <QvUnknownNode.h>
#include "qvbuild.h"
#include "drawwin.h"
#include "mainwin.h"
#include "labwww.h"
#include "labimage.h"
#include "inflate.h"
#include "../wc/wc.h"
#include <GL/gl.h>
#include <GL/glu.h>

extern "C" char *LabStrdup(const char *s);

static const double PI = 3.14159265358979323846;

/* ---- small column-vector matrix helpers (VRML convention) ----------------- */

struct M4 {
    double m[4][4];
};

static M4 identity()
{
    M4 r;
    memset(&r, 0, sizeof(r));
    r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0;
    return r;
}

static M4 mul(const M4 &a, const M4 &b)
{
    M4 r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] +
                        a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
    return r;
}

static M4 translation(double x, double y, double z)
{
    M4 r = identity();
    r.m[0][3] = x;
    r.m[1][3] = y;
    r.m[2][3] = z;
    return r;
}

static M4 scaling(double x, double y, double z)
{
    M4 r = identity();
    r.m[0][0] = x;
    r.m[1][1] = y;
    r.m[2][2] = z;
    return r;
}

static M4 rotation(const float axis[3], double angle)
{
    M4 r = identity();
    double x = axis[0], y = axis[1], z = axis[2];
    double l = sqrt(x * x + y * y + z * z);
    if (l == 0.0)
        return r;
    x /= l; y /= l; z /= l;
    double c = cos(angle), s = sin(angle), t = 1.0 - c;
    r.m[0][0] = t * x * x + c;     r.m[0][1] = t * x * y - s * z; r.m[0][2] = t * x * z + s * y;
    r.m[1][0] = t * x * y + s * z; r.m[1][1] = t * y * y + c;     r.m[1][2] = t * y * z - s * x;
    r.m[2][0] = t * x * z - s * y; r.m[2][1] = t * y * z + s * x; r.m[2][2] = t * z * z + c;
    return r;
}

static M4 invertRotation(const float axis[3], double angle)
{
    return rotation(axis, -angle);
}

struct V3 {
    double x, y, z;
};

static V3 point(const M4 &m, double x, double y, double z)
{
    V3 r;
    r.x = m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z + m.m[0][3];
    r.y = m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z + m.m[1][3];
    r.z = m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z + m.m[2][3];
    return r;
}

static V3 vec(const M4 &m, double x, double y, double z)
{
    V3 r;
    r.x = m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z;
    r.y = m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z;
    r.z = m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z;
    return r;
}

static V3 normalize(V3 v)
{
    double l = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (l > 0.0) {
        v.x /= l; v.y /= l; v.z /= l;
    }
    return v;
}

/* Inverse transpose of the upper 3x3, for transforming normals. */
static M4 normalMatrix(const M4 &a)
{
    double c[3][3];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            int i1 = (i + 1) % 3, i2 = (i + 2) % 3, j1 = (j + 1) % 3, j2 = (j + 2) % 3;
            c[i][j] = a.m[i1][j1] * a.m[i2][j2] - a.m[i1][j2] * a.m[i2][j1];
        }
    M4 r = identity();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            r.m[i][j] = c[i][j];        /* cofactor matrix = det * inverse transpose */
    double det = a.m[0][0] * c[0][0] + a.m[0][1] * c[0][1] + a.m[0][2] * c[0][2];
    if (det < 0.0)
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                r.m[i][j] = -r.m[i][j];
    return r;
}

static double determinant3(const M4 &a)
{
    return a.m[0][0] * (a.m[1][1] * a.m[2][2] - a.m[1][2] * a.m[2][1]) -
           a.m[0][1] * (a.m[1][0] * a.m[2][2] - a.m[1][2] * a.m[2][0]) +
           a.m[0][2] * (a.m[1][0] * a.m[2][1] - a.m[1][1] * a.m[2][0]);
}

/* The matrix of one transformation node. */
static M4 nodeMatrix(QvElement *e)
{
    switch (e->type) {
    case QvElement::Translation: {
        QvTranslation *t = (QvTranslation *)e->data;
        return translation(t->translation.value[0], t->translation.value[1], t->translation.value[2]);
    }
    case QvElement::Rotation: {
        QvRotation *r = (QvRotation *)e->data;
        return rotation(r->rotation.axis, r->rotation.angle);
    }
    case QvElement::Scale: {
        QvScale *s = (QvScale *)e->data;
        return scaling(s->scaleFactor.value[0], s->scaleFactor.value[1], s->scaleFactor.value[2]);
    }
    case QvElement::MatrixTransform: {
        /* Inventor matrices are stored for row vectors */
        QvMatrixTransform *mt = (QvMatrixTransform *)e->data;
        M4 r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                r.m[i][j] = mt->matrix.value[j][i];
        return r;
    }
    case QvElement::Transform: {
        /* T * C * R * SR * S * SR^-1 * C^-1 */
        QvTransform *t = (QvTransform *)e->data;
        M4 m = translation(t->translation.value[0], t->translation.value[1], t->translation.value[2]);
        m = mul(m, translation(t->center.value[0], t->center.value[1], t->center.value[2]));
        m = mul(m, rotation(t->rotation.axis, t->rotation.angle));
        m = mul(m, rotation(t->scaleOrientation.axis, t->scaleOrientation.angle));
        m = mul(m, scaling(t->scaleFactor.value[0], t->scaleFactor.value[1], t->scaleFactor.value[2]));
        m = mul(m, invertRotation(t->scaleOrientation.axis, t->scaleOrientation.angle));
        m = mul(m, translation(-t->center.value[0], -t->center.value[1], -t->center.value[2]));
        return m;
    }
    default:
        return identity();
    }
}

/* The current modelling matrix: the product of the transformation stack. */
static M4 currentMatrix(QvState *state)
{
    std::vector<QvElement *> elts;
    for (QvElement *e = state->getTopElement(QvState::TransformationIndex); e; e = e->next)
        elts.push_back(e);
    M4 m = identity();
    for (size_t i = elts.size(); i-- > 0; )
        m = mul(m, nodeMatrix(elts[i]));
    return m;
}

static QvNode *top(QvState *state, QvState::StackIndex index)
{
    QvElement *e = state->getTopElement(index);
    return e ? e->data : 0;
}

/* ---- build state ---------------------------------------------------------- */

struct Build {
    RLFrame scene;          /* the drawing window's scene frame */
    RLFrame world;          /* frame holding this world */
    RLFrame target;         /* where new shapes go: world or an anchor's frame */
    RLFrame camera;         /* the camera frame (first child of the scene) */
    int lights;
    int haveCamera;
    int inlineDepth;
    float bmin[3], bmax[3];
    int haveBox;
    double fieldAspect;
    std::map<QvTexture2 *, RLTexture> textures;
    std::map<std::string, RLTexture> texturesByURL;
};

static Build *b;
static RLLight headlight;
static RLFrame headlightFrame;
static std::vector<WcElement *> anchorElements;    /* owned; freed per load */

static void removeHeadlight(void)
{
    if (headlight && headlightFrame) {
        /* the camera held the light's only reference, so this frees it;
         * a new one is made for the next world that needs it */
        RLFrameDeleteLight(headlightFrame, headlight);
        headlightFrame = 0;
        headlight = 0;
    }
}

static void addHeadlight(RLFrame camera)
{
    if (!headlight)
        headlight = RLCreateLightRGB(RLLightDirectional, 1.0, 1.0, 1.0);
    if (RLFrameAddLight(camera, headlight) == RLNoError)
        headlightFrame = camera;
}

/* Called for worlds that are not VRML, so they keep their original lighting. */
void QvBuildForget(void)
{
    removeHeadlight();
}

/* (new) A Labyrinth world with no lights of its own had only the drawing
 * window's dim ambient light, so its objects showed flat at a fifth of their
 * colour; it is given the VRML headlight instead.  The camera is the scene's
 * first child, as in QvBuildBegin. */
void QvBuildHeadlight(RLFrame scene)
{
    int count;
    RLFrame *children;
    removeHeadlight();
    if (RLFrameGetChildren(scene, &count, &children) == RLNoError) {
        if (count > 0)
            addHeadlight(children[0]);
        RLFree(children);
    }
}

void QvBuildBegin(RLFrame scene, const char *file)
{
    removeHeadlight();
    for (size_t i = 0; i < anchorElements.size(); i++)
        delete anchorElements[i];
    anchorElements.clear();

    b = new Build;
    b->scene = scene;
    b->camera = 0;
    int count;
    RLFrame *children;
    if (RLFrameGetChildren(scene, &count, &children) == RLNoError) {
        if (count > 0)
            b->camera = children[0];
        RLFree(children);
    }
    b->world = RLCreateFrame(scene);
    b->target = b->world;
    b->lights = 0;
    b->haveCamera = 0;
    b->inlineDepth = 0;
    b->haveBox = 0;
    b->fieldAspect = 1.0;
    LabDrawingWindow *view = labMainWindow ? labMainWindow->GetView() : 0;
    if (view && view->GetHeight() > 0) {
        double w = view->GetWidth(), h = view->GetHeight();
        b->fieldAspect = w > h ? w / h : 1.0;
    }
}

/* Place the camera so that the whole world is in view. */
static void fitCamera(void)
{
    if (!b->haveBox || !wcCamera)
        return;
    double cx = (b->bmin[0] + b->bmax[0]) * 0.5;
    double cy = (b->bmin[1] + b->bmax[1]) * 0.5;
    double cz = (b->bmin[2] + b->bmax[2]) * 0.5;
    double dx = b->bmax[0] - b->bmin[0], dy = b->bmax[1] - b->bmin[1], dz = b->bmax[2] - b->bmin[2];
    double r = 0.5 * sqrt(dx * dx + dy * dy + dz * dz);
    if (r <= 0.0)
        r = 1.0;
    double f = 0.5;
    /* Reality Lab looks along +z; stand back on the -z side of the centre */
    double dist = r / f * 1.1 + r;
    wcCamera->SetPosition(cx, cy, cz - dist);
    wcCamera->SetOrientation(0, 0, 1, 0, 1, 0);
    wcCamera->SetField(-f, f, -f, f);
}

void QvBuildEnd(void)
{
    if (!b)
        return;
    if (!b->haveCamera)
        fitCamera();
    if (b->lights == 0 && b->camera) {
        /* VRML browsers light the world with a headlight */
        addHeadlight(b->camera);
    }
    delete b;
    b = 0;
}

void QvBuildPush(void) {}
void QvBuildPop(void) {}
void QvBuildPushTransform(void) {}
void QvBuildPopTransform(void) {}

/* ---- textures --------------------------------------------------------------- */

static RLTexture imageTexture(QvSFImage &img)
{
    int w = img.size[0], h = img.size[1], nc = img.numComponents;
    if (w <= 0 || h <= 0 || nc < 1 || nc > 4 || !img.bytes)
        return NULL;
    std::vector<unsigned char> px((size_t)w * h * 4);
    for (int y = 0; y < h; y++) {
        /* SFImage rows run from the bottom up */
        const unsigned char *src = img.bytes + (size_t)(h - 1 - y) * w * nc;
        unsigned char *dst = &px[(size_t)y * w * 4];
        for (int x = 0; x < w; x++, src += nc, dst += 4) {
            unsigned char r, g, bl, a = 255;
            if (nc <= 2) {
                r = g = bl = src[0];
                if (nc == 2) a = src[1];
            } else {
                r = src[0]; g = src[1]; bl = src[2];
                if (nc == 4) a = src[3];
            }
            dst[0] = bl; dst[1] = g; dst[2] = r; dst[3] = a;
        }
    }
    RLImage image;
    memset(&image, 0, sizeof(image));
    image.width = w;
    image.height = h;
    image.aspectx = image.aspecty = 1;
    image.depth = 32;
    image.rgb = 1;
    image.bytes_per_line = w * 4;
    image.buffer1 = &px[0];
    image.red_mask = 0x00ff0000;
    image.green_mask = 0x0000ff00;
    image.blue_mask = 0x000000ff;
    image.alpha_mask = 0xff000000;
    return RLCreateTexture(&image);
}

static RLTexture textureFor(QvTexture2 *t)
{
    if (!t)
        return NULL;
    std::map<QvTexture2 *, RLTexture>::iterator i = b->textures.find(t);
    if (i != b->textures.end())
        return i->second;
    RLTexture tex = NULL;
    const char *name = t->filename.value.getString();
    if (name && *name) {
        char abs[2048];
        std::string key = LabResolveURL(name, abs, sizeof(abs)) ? abs : name;
        std::map<std::string, RLTexture>::iterator j = b->texturesByURL.find(key);
        if (j != b->texturesByURL.end()) {
            tex = j->second;
        } else {
            labMainWindow->SetStatus(name);
            const char *file = LabFetchFile(name);
            if (file)
                tex = LabLoadImage(LabGunzipFile(file));
            b->texturesByURL[key] = tex;
        }
    } else {
        tex = imageTexture(t->image);
    }
    if (tex)
        RLTextureSetClamp(tex, t->wrapS.value == QvTexture2::CLAMP,
                          t->wrapT.value == QvTexture2::CLAMP);
    b->textures[t] = tex;
    return tex;
}

/* ---- meshes ----------------------------------------------------------------- */

struct Corner {
    int coord;              /* index into the shape's coordinates */
    float s, t;             /* VRML texture coordinates */
    V3 normal;              /* in VRML object space */
};

struct Face {
    std::vector<Corner> corners;
    RLColour colour;
};

struct Material {
    RLColour colour;
    float specular, power;
};

static int isBlack(const float *c)
{
    return c[0] == 0.0f && c[1] == 0.0f && c[2] == 0.0f;
}

/* A face's diffuse colour.  Emissive colours are given to the mesh's
 * material (see emitMesh) unless there is a list of them, when a black
 * diffuse colour takes the emissive one instead.  Lines and points are
 * unlit: VRML 1.0 browsers drew them in the diffuse colour, but many
 * worlds were written expecting the emissive colour (the VRML 97 rule),
 * so that is used when no diffuse colour was given. */
static RLColour vrmlColour(QvMaterial *m, int i, int unlit = 0)
{
    if (!m)
        return RLCreateColourRGB(0.8, 0.8, 0.8);
    static const float grey[3] = { 0.8f, 0.8f, 0.8f };
    const float *d = m->diffuseColor.num > 0 ?
        &m->diffuseColor.values[3 * (i % m->diffuseColor.num)] : grey;
    if (m->emissiveColor.num > 0) {
        const float *e = &m->emissiveColor.values[3 * (i % m->emissiveColor.num)];
        int useEmissive = unlit ? (m->diffuseColor.isDefault() || isBlack(d))
                                : (m->emissiveColor.num > 1 && isBlack(d));
        if (useEmissive && !isBlack(e))
            d = e;
    }
    return RLCreateColourRGB(d[0], d[1], d[2]);
}

static int binding(QvState *state, QvState::StackIndex index)
{
    QvNode *n = top(state, index);
    if (!n)
        return QvMaterialBinding::DEFAULT;
    if (index == QvState::MaterialBindingIndex)
        return ((QvMaterialBinding *)n)->value.value;
    return ((QvNormalBinding *)n)->value.value;
}

/* Reality Lab drew convex polygons only.  Split any face that is not
 * convex (VRML's UNKNOWN_FACE_TYPE, or a world that forgot to say so) into
 * triangles by ear clipping, after dropping repeated vertices. */
static void splitConcave(std::vector<V3> &coords, std::vector<Face> &faces)
{
    size_t count = faces.size();
    for (size_t f = 0; f < count; f++) {
        std::vector<Corner> &cs = faces[f].corners;
        if (cs.size() < 4)
            continue;
        std::vector<Corner> poly;
        for (size_t k = 0; k < cs.size(); k++) {
            if (cs[k].coord < 0 || cs[k].coord >= (int)coords.size())
                continue;
            const V3 &p = coords[cs[k].coord];
            if (!poly.empty()) {
                const V3 &q = coords[poly.back().coord];
                if (p.x == q.x && p.y == q.y && p.z == q.z)
                    continue;
            }
            poly.push_back(cs[k]);
        }
        while (poly.size() > 1) {
            const V3 &p = coords[poly.back().coord], &q = coords[poly.front().coord];
            if (p.x != q.x || p.y != q.y || p.z != q.z)
                break;
            poly.pop_back();
        }
        size_t n = poly.size();
        if (n < 4) {
            cs = poly;
            continue;
        }
        /* project onto the plane most nearly facing the polygon */
        V3 nn = { 0, 0, 0 };
        for (size_t k = 0; k < n; k++) {
            const V3 &a = coords[poly[k].coord], &b = coords[poly[(k + 1) % n].coord];
            nn.x += (a.y - b.y) * (a.z + b.z);
            nn.y += (a.z - b.z) * (a.x + b.x);
            nn.z += (a.x - b.x) * (a.y + b.y);
        }
        double ax = fabs(nn.x), ay = fabs(nn.y), az = fabs(nn.z);
        int u = ax >= ay && ax >= az ? 1 : 0, v = ax >= ay && ax >= az ? 2 :
                (ay >= az ? 2 : 1);
        std::vector<double> px(n), py(n);
        for (size_t k = 0; k < n; k++) {
            const V3 &a = coords[poly[k].coord];
            double c[3] = { a.x, a.y, a.z };
            px[k] = c[u];
            py[k] = c[v];
        }
        double area = 0.0;
        for (size_t k = 0; k < n; k++)
            area += px[k] * py[(k + 1) % n] - px[(k + 1) % n] * py[k];
        double sign = area < 0.0 ? -1.0 : 1.0;
        int convex = 1;
        for (size_t k = 0; k < n && convex; k++) {
            size_t a = (k + n - 1) % n, b = (k + 1) % n;
            double cross = (px[k] - px[a]) * (py[b] - py[k]) - (py[k] - py[a]) * (px[b] - px[k]);
            if (cross * sign < 0.0)
                convex = 0;
        }
        if (convex) {
            cs = poly;
            continue;
        }
        std::vector<size_t> left(n);
        for (size_t k = 0; k < n; k++)
            left[k] = k;
        std::vector<std::vector<Corner> > tris;
        while (left.size() > 3) {
            size_t m = left.size(), ear = m;
            for (size_t k = 0; k < m && ear == m; k++) {
                size_t a = left[(k + m - 1) % m], b = left[k], c = left[(k + 1) % m];
                double cross = (px[b] - px[a]) * (py[c] - py[b]) - (py[b] - py[a]) * (px[c] - px[b]);
                if (cross * sign <= 0.0)
                    continue;
                int empty = 1;
                for (size_t j = 0; j < m && empty; j++) {
                    size_t o = left[j];
                    if (o == a || o == b || o == c)
                        continue;
                    double d1 = (px[b] - px[a]) * (py[o] - py[a]) - (py[b] - py[a]) * (px[o] - px[a]);
                    double d2 = (px[c] - px[b]) * (py[o] - py[b]) - (py[c] - py[b]) * (px[o] - px[b]);
                    double d3 = (px[a] - px[c]) * (py[o] - py[c]) - (py[a] - py[c]) * (px[o] - px[c]);
                    if (d1 * sign >= 0.0 && d2 * sign >= 0.0 && d3 * sign >= 0.0)
                        empty = 0;
                }
                if (empty)
                    ear = k;
            }
            if (ear == m)
                ear = 0;    /* degenerate: just cut off a corner */
            std::vector<Corner> t;
            t.push_back(poly[left[(ear + m - 1) % m]]);
            t.push_back(poly[left[ear]]);
            t.push_back(poly[left[(ear + 1) % m]]);
            tris.push_back(t);
            left.erase(left.begin() + ear);
        }
        std::vector<Corner> last;
        for (size_t k = 0; k < 3; k++)
            last.push_back(poly[left[k]]);
        cs = last;
        for (size_t t = 0; t < tris.size(); t++) {
            Face nf;
            nf.corners = tris[t];
            nf.colour = faces[f].colour;
            faces.push_back(nf);
        }
    }
}

/* Turn faces in VRML object space into a Reality Lab mesh in world space. */
static void emitMesh(QvState *state, std::vector<V3> &coords, std::vector<Face> &faces,
                     RLTexture texture, int isSolidShape)
{
    if (faces.empty())
        return;
    splitConcave(coords, faces);
    M4 m = currentMatrix(state);
    M4 nm = normalMatrix(m);
    int flip = determinant3(m) < 0.0;   /* a mirroring transformation */

    QvShapeHints *hints = (QvShapeHints *)top(state, QvState::ShapeHintsIndex);
    int ordering = hints ? hints->vertexOrdering.value : QvShapeHints::UNKNOWN_ORDERING;
    int solid = hints ? hints->shapeType.value == QvShapeHints::SOLID : 0;
    if (isSolidShape) {
        ordering = QvShapeHints::COUNTERCLOCKWISE;
        solid = 1;
    }
    /* Front faces are counterclockwise in VRML and clockwise in Reality Lab;
     * negating z turns one into the other, so the order is reversed --
     * unless the shape says its vertices run clockwise. */
    int reverse = ordering != QvShapeHints::CLOCKWISE;
    if (flip)
        reverse = !reverse;

    /* (new) each shape gets a frame of its own, centred on it, so that it
     * can be picked out and dragged about like the objects of a Labyrinth
     * world; it carries its anchor's link, if any */
    double lo[3] = { 0, 0, 0 }, hi[3] = { 0, 0, 0 };
    int any = 0;
    for (size_t f = 0; f < faces.size(); f++) {
        for (size_t k = 0; k < faces[f].corners.size(); k++) {
            int ci = faces[f].corners[k].coord;
            if (ci < 0 || ci >= (int)coords.size())
                continue;
            V3 p = point(m, coords[ci].x, coords[ci].y, coords[ci].z);
            double q[3] = { p.x, p.y, -p.z };
            for (int a = 0; a < 3; a++) {
                if (!any || q[a] < lo[a]) lo[a] = q[a];
                if (!any || q[a] > hi[a]) hi[a] = q[a];
            }
            any = 1;
        }
    }
    if (!any)
        return;
    double centre[3] = { (lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2 };
    RLFrame frame = RLCreateFrame(b->target);
    RLFrameSetPosition(frame, b->target, centre[0], centre[1], centre[2]);
    RLObjectSetAppData(frame, RLObjectGetAppData(b->target));

    QvMaterial *mat = (QvMaterial *)top(state, QvState::MaterialIndex);
    RLMesh mesh = RLCreateMesh();
    std::map<long long, int> vertexIndex;
    for (size_t f = 0; f < faces.size(); f++) {
        Face &face = faces[f];
        size_t n = face.corners.size();
        if (n == 0)
            continue;
        RLFace rf = RLMeshCreateFace(mesh);
        for (size_t k = 0; k < n; k++) {
            Corner &c = face.corners[reverse ? (n - 1 - k) : k];
            if (c.coord < 0 || c.coord >= (int)coords.size())
                continue;
            /* one Reality Lab vertex per position/texture pair */
            int si = (int)floorf(c.s * 4096.0f + 0.5f), ti = (int)floorf(c.t * 4096.0f + 0.5f);
            long long key = ((long long)c.coord << 32) ^ ((long long)(si & 0xffff) << 16) ^ (ti & 0xffff);
            int vi;
            std::map<long long, int>::iterator it = vertexIndex.find(key);
            if (it != vertexIndex.end()) {
                vi = it->second;
            } else {
                V3 p = point(m, coords[c.coord].x, coords[c.coord].y, coords[c.coord].z);
                vi = RLMeshAddVertex(mesh, p.x - centre[0], p.y - centre[1], -p.z - centre[2]);
                RLMeshSetTextureCoordinates(mesh, vi, c.s, 1.0 - c.t);
                vertexIndex[key] = vi;
                float q[3] = { (float)p.x, (float)p.y, (float)-p.z };
                for (int a = 0; a < 3; a++) {
                    if (!b->haveBox || q[a] < b->bmin[a]) b->bmin[a] = q[a];
                    if (!b->haveBox || q[a] > b->bmax[a]) b->bmax[a] = q[a];
                }
                b->haveBox = 1;
            }
            V3 nn = normalize(vec(nm, c.normal.x, c.normal.y, c.normal.z));
            int ni = RLMeshAddNormal(mesh, nn.x, nn.y, -nn.z);
            RLFaceAddVertexAndNormalIndexed(rf, vi, ni);
        }
        RLFaceSetColour(rf, face.colour);
        if (texture && n >= 3)
            RLFaceSetTexture(rf, texture);
    }
    if (texture) {
        int count;
        RLFace *list;
        if (RLMeshGetFaces(mesh, &count, &list) == RLNoError) {
            for (int i = 0; i < count; i++)
                RLFaceSetColour(list[i], RLCreateColourRGB(1, 1, 1));
            RLFree(list);
        }
    }
    if (mat) {
        double spec = 0.0;
        if (mat->specularColor.num > 0) {
            float *s = mat->specularColor.values;
            spec = (s[0] + s[1] + s[2]) / 3.0;
        }
        double power = mat->shininess.num > 0 ? mat->shininess.values[0] * 128.0 : 25.6;
        const float *e = mat->emissiveColor.num == 1 ? mat->emissiveColor.values : NULL;
        if (e && !isBlack(e))
            RLMeshSetMaterial(mesh, RLCreateMaterialEmissive(spec > 1.0 ? 1.0 : spec,
                                                             power, e[0], e[1], e[2]));
        else if (spec > 0.0 && power > 0.0)
            RLMeshSetMaterial(mesh, RLCreateMaterial(spec > 1.0 ? 1.0 : spec, power));
    }
    if (!(solid && ordering != QvShapeHints::UNKNOWN_ORDERING))
        RLMeshSetTwoSided(mesh, 1);
    RLFrameAddVisual(frame, mesh);
}

static V3 newell(std::vector<V3> &coords, const std::vector<int> &idx)
{
    V3 n = { 0, 0, 0 };
    size_t k = idx.size();
    for (size_t i = 0; i < k; i++) {
        const V3 &a = coords[idx[i]];
        const V3 &c = coords[idx[(i + 1) % k]];
        n.x += (a.y - c.y) * (a.z + c.z);
        n.y += (a.z - c.z) * (a.x + c.x);
        n.z += (a.x - c.x) * (a.y + c.y);
    }
    return normalize(n);
}

/* Default texture coordinates of an IndexedFaceSet: S along the longest
 * side of the bounding box, T along the second longest. */
static void boxMapping(std::vector<V3> &coords, int &sAxis, int &tAxis, double mn[3], double &size)
{
    double mx[3];
    for (int a = 0; a < 3; a++) {
        mn[a] = 1e30;
        mx[a] = -1e30;
    }
    for (size_t i = 0; i < coords.size(); i++) {
        double p[3] = { coords[i].x, coords[i].y, coords[i].z };
        for (int a = 0; a < 3; a++) {
            if (p[a] < mn[a]) mn[a] = p[a];
            if (p[a] > mx[a]) mx[a] = p[a];
        }
    }
    double ext[3] = { mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2] };
    sAxis = 0;
    for (int a = 1; a < 3; a++)
        if (ext[a] > ext[sAxis])
            sAxis = a;
    tAxis = sAxis == 0 ? 1 : 0;
    for (int a = 0; a < 3; a++)
        if (a != sAxis && ext[a] > ext[tAxis])
            tAxis = a;
    size = ext[sAxis] > 0.0 ? ext[sAxis] : 1.0;
}

static void textureTransform(QvState *state, float &s, float &t)
{
    QvTexture2Transform *x = (QvTexture2Transform *)top(state, QvState::Texture2TransformationIndex);
    if (!x)
        return;
    double cs = x->center.value[0], ct = x->center.value[1];
    double u = (s - cs) * x->scaleFactor.value[0];
    double v = (t - ct) * x->scaleFactor.value[1];
    double c = cos(x->rotation.value), sn = sin(x->rotation.value);
    double ru = u * c - v * sn, rv = u * sn + v * c;
    s = (float)(ru + cs + x->translation.value[0]);
    t = (float)(rv + ct + x->translation.value[1]);
}

void QvBuildQvIndexedFaceSet(QvIndexedFaceSet *node, QvState *state)
{
    QvCoordinate3 *c3 = (QvCoordinate3 *)top(state, QvState::Coordinate3Index);
    if (!c3 || c3->point.num == 0)
        return;
    std::vector<V3> coords(c3->point.num);
    for (int i = 0; i < c3->point.num; i++) {
        coords[i].x = c3->point.values[3 * i];
        coords[i].y = c3->point.values[3 * i + 1];
        coords[i].z = c3->point.values[3 * i + 2];
    }
    QvMaterial *mat = (QvMaterial *)top(state, QvState::MaterialIndex);
    QvNormal *nrm = (QvNormal *)top(state, QvState::NormalIndex);
    QvTextureCoordinate2 *tc = (QvTextureCoordinate2 *)top(state, QvState::TextureCoordinate2Index);
    QvShapeHints *hints = (QvShapeHints *)top(state, QvState::ShapeHintsIndex);
    RLTexture texture = textureFor((QvTexture2 *)top(state, QvState::Texture2Index));
    int mbind = binding(state, QvState::MaterialBindingIndex);
    int nbind = binding(state, QvState::NormalBindingIndex);
    double crease = hints ? hints->creaseAngle.value : 0.5;
    int cw = hints && hints->vertexOrdering.value == QvShapeHints::CLOCKWISE;

    /* split coordIndex into faces */
    std::vector<std::vector<int> > polys;
    std::vector<std::vector<int> > polyCorners;     /* position in coordIndex */
    std::vector<int> cur, curPos;
    for (int i = 0; i <= node->coordIndex.num; i++) {
        long v = i < node->coordIndex.num ? node->coordIndex.values[i] : -1;
        if (v < 0) {
            if (cur.size() >= 3) {
                polys.push_back(cur);
                polyCorners.push_back(curPos);
            }
            cur.clear();
            curPos.clear();
        } else if (v < (long)coords.size()) {
            cur.push_back((int)v);
            curPos.push_back(i);
        }
    }
    if (polys.empty())
        return;

    /* face normals (in object space, pointing out of the front) */
    std::vector<V3> fn(polys.size());
    for (size_t f = 0; f < polys.size(); f++) {
        fn[f] = newell(coords, polys[f]);
        if (cw) {
            fn[f].x = -fn[f].x; fn[f].y = -fn[f].y; fn[f].z = -fn[f].z;
        }
    }
    /* faces around each coordinate, for smoothing */
    std::vector<std::vector<int> > around;
    int haveNormals = nrm && nrm->vector.num > 0 && nbind != QvNormalBinding::DEFAULT &&
                      nbind != QvNormalBinding::OVERALL;
    if (!haveNormals && crease > 0.0) {
        around.resize(coords.size());
        for (size_t f = 0; f < polys.size(); f++)
            for (size_t k = 0; k < polys[f].size(); k++)
                around[polys[f][k]].push_back((int)f);
    }
    double cosCrease = cos(crease);

    int sAxis = 0, tAxis = 1;
    double mn[3], size = 1.0;
    int haveTexCoords = tc && tc->point.num > 0;
    if (texture && !haveTexCoords)
        boxMapping(coords, sAxis, tAxis, mn, size);
    int useTexIndex = node->textureCoordIndex.num > 0 && node->textureCoordIndex.values[0] >= 0;

    std::vector<Face> faces(polys.size());
    int vertexCounter = 0;
    for (size_t f = 0; f < polys.size(); f++) {
        Face &face = faces[f];
        int firstPos = polyCorners[f][0];
        switch (mbind) {
        case QvMaterialBinding::PER_PART:
        case QvMaterialBinding::PER_FACE:
            face.colour = vrmlColour(mat, (int)f);
            break;
        case QvMaterialBinding::PER_PART_INDEXED:
        case QvMaterialBinding::PER_FACE_INDEXED:
            face.colour = vrmlColour(mat, f < (size_t)node->materialIndex.num ?
                                         (int)node->materialIndex.values[f] : 0);
            break;
        case QvMaterialBinding::PER_VERTEX:
            face.colour = vrmlColour(mat, vertexCounter);
            break;
        case QvMaterialBinding::PER_VERTEX_INDEXED:
            face.colour = vrmlColour(mat, firstPos < node->materialIndex.num ?
                                         (int)node->materialIndex.values[firstPos] : 0);
            break;
        default:
            face.colour = vrmlColour(mat, 0);
            break;
        }
        for (size_t k = 0; k < polys[f].size(); k++, vertexCounter++) {
            Corner c;
            int pos = polyCorners[f][k];
            c.coord = polys[f][k];
            /* normal */
            c.normal = fn[f];
            if (haveNormals) {
                int ni = -1;
                switch (nbind) {
                case QvNormalBinding::PER_PART:
                case QvNormalBinding::PER_FACE: ni = (int)f; break;
                case QvNormalBinding::PER_PART_INDEXED:
                case QvNormalBinding::PER_FACE_INDEXED:
                    ni = f < (size_t)node->normalIndex.num ? (int)node->normalIndex.values[f] : -1;
                    break;
                case QvNormalBinding::PER_VERTEX: ni = vertexCounter; break;
                case QvNormalBinding::PER_VERTEX_INDEXED:
                    ni = pos < node->normalIndex.num ? (int)node->normalIndex.values[pos] : c.coord;
                    break;
                }
                if (ni >= 0 && ni < nrm->vector.num) {
                    c.normal.x = nrm->vector.values[3 * ni];
                    c.normal.y = nrm->vector.values[3 * ni + 1];
                    c.normal.z = nrm->vector.values[3 * ni + 2];
                    c.normal = normalize(c.normal);
                }
            } else if (!around.empty()) {
                V3 s = { 0, 0, 0 };
                const std::vector<int> &fs = around[c.coord];
                for (size_t a = 0; a < fs.size(); a++) {
                    const V3 &o = fn[fs[a]];
                    if (o.x * fn[f].x + o.y * fn[f].y + o.z * fn[f].z >= cosCrease) {
                        s.x += o.x; s.y += o.y; s.z += o.z;
                    }
                }
                if (s.x != 0 || s.y != 0 || s.z != 0)
                    c.normal = normalize(s);
            }
            /* texture coordinates */
            c.s = c.t = 0.0f;
            if (texture) {
                if (haveTexCoords) {
                    int ti = useTexIndex ? (pos < node->textureCoordIndex.num ?
                                            (int)node->textureCoordIndex.values[pos] : -1)
                                         : c.coord;
                    if (ti >= 0 && ti < tc->point.num) {
                        c.s = tc->point.values[2 * ti];
                        c.t = tc->point.values[2 * ti + 1];
                    }
                } else {
                    double p[3] = { coords[c.coord].x, coords[c.coord].y, coords[c.coord].z };
                    c.s = (float)((p[sAxis] - mn[sAxis]) / size);
                    c.t = (float)((p[tAxis] - mn[tAxis]) / size);
                }
                textureTransform(state, c.s, c.t);
            }
            face.corners.push_back(c);
        }
    }
    emitMesh(state, coords, faces, texture, 0);
}

void QvBuildQvIndexedLineSet(QvIndexedLineSet *node, QvState *state)
{
    QvCoordinate3 *c3 = (QvCoordinate3 *)top(state, QvState::Coordinate3Index);
    if (!c3 || c3->point.num == 0)
        return;
    std::vector<V3> coords(c3->point.num);
    for (int i = 0; i < c3->point.num; i++) {
        coords[i].x = c3->point.values[3 * i];
        coords[i].y = c3->point.values[3 * i + 1];
        coords[i].z = c3->point.values[3 * i + 2];
    }
    QvMaterial *mat = (QvMaterial *)top(state, QvState::MaterialIndex);
    int mbind = binding(state, QvState::MaterialBindingIndex);
    std::vector<Face> faces;
    int line = 0;
    long prev = -1;
    for (int i = 0; i <= node->coordIndex.num; i++) {
        long v = i < node->coordIndex.num ? node->coordIndex.values[i] : -1;
        if (v < 0 || v >= (long)coords.size()) {
            if (prev >= 0)
                line++;
            prev = -1;
            continue;
        }
        if (prev >= 0) {
            Face f;
            Corner a = { (int)prev, 0, 0, { 0, 1, 0 } };
            Corner c = { (int)v, 0, 0, { 0, 1, 0 } };
            f.corners.push_back(a);
            f.corners.push_back(c);
            int mi = 0;
            if (mbind == QvMaterialBinding::PER_PART || mbind == QvMaterialBinding::PER_FACE)
                mi = line;
            else if (mbind == QvMaterialBinding::PER_PART_INDEXED ||
                     mbind == QvMaterialBinding::PER_FACE_INDEXED)
                mi = line < node->materialIndex.num ? (int)node->materialIndex.values[line] : 0;
            f.colour = vrmlColour(mat, mi, 1);
            faces.push_back(f);
        }
        prev = v;
    }
    emitMesh(state, coords, faces, NULL, 0);
}

void QvBuildQvPointSet(QvPointSet *node, QvState *state)
{
    QvCoordinate3 *c3 = (QvCoordinate3 *)top(state, QvState::Coordinate3Index);
    if (!c3 || c3->point.num == 0)
        return;
    std::vector<V3> coords(c3->point.num);
    for (int i = 0; i < c3->point.num; i++) {
        coords[i].x = c3->point.values[3 * i];
        coords[i].y = c3->point.values[3 * i + 1];
        coords[i].z = c3->point.values[3 * i + 2];
    }
    QvMaterial *mat = (QvMaterial *)top(state, QvState::MaterialIndex);
    int start = (int)node->startIndex.value;
    int n = (int)node->numPoints.value;
    if (n < 0)
        n = (int)coords.size() - start;
    std::vector<Face> faces;
    for (int i = start; i < start + n && i < (int)coords.size(); i++) {
        Face f;
        Corner c = { i, 0, 0, { 0, 1, 0 } };
        f.corners.push_back(c);
        f.colour = vrmlColour(mat, i - start, 1);
        faces.push_back(f);
    }
    emitMesh(state, coords, faces, NULL, 0);
}

/* ---- primitives ------------------------------------------------------------- */

static void addQuad(std::vector<V3> &coords, std::vector<Face> &faces, RLColour colour,
                    V3 p0, V3 p1, V3 p2, V3 p3, V3 n,
                    float s0, float t0, float s1, float t1)
{
    int base = (int)coords.size();
    coords.push_back(p0);
    coords.push_back(p1);
    coords.push_back(p2);
    coords.push_back(p3);
    Face f;
    f.colour = colour;
    Corner c[4] = { { base, s0, t0, n }, { base + 1, s1, t0, n },
                    { base + 2, s1, t1, n }, { base + 3, s0, t1, n } };
    for (int i = 0; i < 4; i++)
        f.corners.push_back(c[i]);
    faces.push_back(f);
}

static RLColour partColour(QvState *state, int part)
{
    QvMaterial *mat = (QvMaterial *)top(state, QvState::MaterialIndex);
    int mbind = binding(state, QvState::MaterialBindingIndex);
    if (mbind == QvMaterialBinding::PER_PART || mbind == QvMaterialBinding::PER_PART_INDEXED ||
        mbind == QvMaterialBinding::PER_FACE || mbind == QvMaterialBinding::PER_FACE_INDEXED)
        return vrmlColour(mat, part);
    return vrmlColour(mat, 0);
}

void QvBuildQvCube(QvCube *node, QvState *state)
{
    double w = node->width.value / 2, h = node->height.value / 2, d = node->depth.value / 2;
    std::vector<V3> coords;
    std::vector<Face> faces;
    RLTexture texture = textureFor((QvTexture2 *)top(state, QvState::Texture2Index));
#define P(x, y, z) { (x), (y), (z) }
    V3 v[8] = { P(-w, -h, d), P(w, -h, d), P(w, h, d), P(-w, h, d),
                P(-w, -h, -d), P(w, -h, -d), P(w, h, -d), P(-w, h, -d) };
    V3 nf = P(0, 0, 1), nb = P(0, 0, -1), nr = P(1, 0, 0), nl = P(-1, 0, 0), nt = P(0, 1, 0), nd = P(0, -1, 0);
#undef P
    /* counterclockwise seen from outside; each face carries the whole texture */
    addQuad(coords, faces, partColour(state, 0), v[0], v[1], v[2], v[3], nf, 0, 0, 1, 1);  /* front */
    addQuad(coords, faces, partColour(state, 1), v[5], v[4], v[7], v[6], nb, 0, 0, 1, 1);  /* back */
    addQuad(coords, faces, partColour(state, 2), v[3], v[2], v[6], v[7], nt, 0, 0, 1, 1);  /* top */
    addQuad(coords, faces, partColour(state, 3), v[4], v[5], v[1], v[0], nd, 0, 0, 1, 1);  /* bottom */
    addQuad(coords, faces, partColour(state, 4), v[1], v[5], v[6], v[2], nr, 0, 0, 1, 1);  /* right */
    addQuad(coords, faces, partColour(state, 5), v[4], v[0], v[3], v[7], nl, 0, 0, 1, 1);  /* left */
    emitMesh(state, coords, faces, texture, 1);
}

/* fine enough that curved outlines stay smooth at full display resolution */
static const int SEGMENTS = 64;

/* A ring of points around the y axis, starting at the back (-z) and running
 * counterclockwise seen from above, as VRML's texture mapping requires. */
static V3 ringPoint(double r, double y, int i)
{
    double a = 2.0 * PI * i / SEGMENTS;
    V3 p = { -r * sin(a), y, -r * cos(a) };
    return p;
}

static void addDisc(std::vector<V3> &coords, std::vector<Face> &faces, RLColour colour,
                    double r, double y, int up)
{
    /* the ring runs counterclockwise seen from above, so an upward disc
     * takes it in order and a downward one in reverse */
    int base = (int)coords.size();
    Face f;
    f.colour = colour;
    V3 n = { 0, up ? 1.0 : -1.0, 0 };
    for (int i = 0; i < SEGMENTS; i++)
        coords.push_back(ringPoint(r, y, i));
    for (int i = 0; i < SEGMENTS; i++) {
        int k = up ? i : SEGMENTS - 1 - i;
        V3 &p = coords[base + k];
        Corner c = { base + k, (float)(0.5 + p.x / (2 * r)),
                     (float)(up ? 0.5 - p.z / (2 * r) : 0.5 + p.z / (2 * r)), n };
        f.corners.push_back(c);
    }
    faces.push_back(f);
}

void QvBuildQvCylinder(QvCylinder *node, QvState *state)
{
    double r = node->radius.value, h = node->height.value / 2;
    int parts = node->parts.value;
    std::vector<V3> coords;
    std::vector<Face> faces;
    RLTexture texture = textureFor((QvTexture2 *)top(state, QvState::Texture2Index));
    if (parts & QvCylinder::SIDES) {
        RLColour col = partColour(state, 0);
        for (int i = 0; i < SEGMENTS; i++) {
            V3 b0 = ringPoint(r, -h, i), b1 = ringPoint(r, -h, i + 1);
            V3 t0 = ringPoint(r, h, i), t1 = ringPoint(r, h, i + 1);
            int base = (int)coords.size();
            coords.push_back(b0); coords.push_back(b1); coords.push_back(t1); coords.push_back(t0);
            V3 n0 = normalize(ringPoint(1, 0, i)), n1 = normalize(ringPoint(1, 0, i + 1));
            float s0 = (float)i / SEGMENTS, s1 = (float)(i + 1) / SEGMENTS;
            Face f;
            f.colour = col;
            /* counterclockwise seen from outside */
            Corner c[4] = { { base, s0, 0, n0 }, { base + 1, s1, 0, n1 },
                            { base + 2, s1, 1, n1 }, { base + 3, s0, 1, n0 } };
            for (int k = 0; k < 4; k++)
                f.corners.push_back(c[k]);
            faces.push_back(f);
        }
    }
    if (parts & QvCylinder::TOP)
        addDisc(coords, faces, partColour(state, 1), r, h, 1);
    if (parts & QvCylinder::BOTTOM)
        addDisc(coords, faces, partColour(state, 2), r, -h, 0);
    emitMesh(state, coords, faces, texture, 1);
}

void QvBuildQvCone(QvCone *node, QvState *state)
{
    double r = node->bottomRadius.value, h = node->height.value / 2;
    int parts = node->parts.value;
    std::vector<V3> coords;
    std::vector<Face> faces;
    RLTexture texture = textureFor((QvTexture2 *)top(state, QvState::Texture2Index));
    if (parts & QvCone::SIDES) {
        RLColour col = partColour(state, 0);
        double slope = r / (2 * h);
        for (int i = 0; i < SEGMENTS; i++) {
            int base = (int)coords.size();
            coords.push_back(ringPoint(r, -h, i));
            coords.push_back(ringPoint(r, -h, i + 1));
            V3 apex = { 0, h, 0 };
            coords.push_back(apex);
            V3 n0 = ringPoint(1, slope, i), n1 = ringPoint(1, slope, i + 1);
            V3 na = ringPoint(1, slope, i);
            na = normalize(na);
            n0 = normalize(n0);
            n1 = normalize(n1);
            float s0 = (float)i / SEGMENTS, s1 = (float)(i + 1) / SEGMENTS;
            Face f;
            f.colour = col;
            Corner c[3] = { { base, s0, 0, n0 }, { base + 1, s1, 0, n1 },
                            { base + 2, (s0 + s1) / 2, 1, na } };
            for (int k = 0; k < 3; k++)
                f.corners.push_back(c[k]);
            faces.push_back(f);
        }
    }
    if (parts & QvCone::BOTTOM)
        addDisc(coords, faces, partColour(state, 1), r, -h, 0);
    emitMesh(state, coords, faces, texture, 1);
}

void QvBuildQvSphere(QvSphere *node, QvState *state)
{
    double r = node->radius.value;
    const int stacks = SEGMENTS / 2;
    std::vector<V3> coords;
    std::vector<Face> faces;
    RLTexture texture = textureFor((QvTexture2 *)top(state, QvState::Texture2Index));
    RLColour col = partColour(state, 0);
    for (int j = 0; j < stacks; j++) {
        double a0 = PI * j / stacks - PI / 2, a1 = PI * (j + 1) / stacks - PI / 2;
        for (int i = 0; i < SEGMENTS; i++) {
            V3 p[4];
            p[0] = ringPoint(r * cos(a0), r * sin(a0), i);
            p[1] = ringPoint(r * cos(a0), r * sin(a0), i + 1);
            p[2] = ringPoint(r * cos(a1), r * sin(a1), i + 1);
            p[3] = ringPoint(r * cos(a1), r * sin(a1), i);
            int base = (int)coords.size();
            for (int k = 0; k < 4; k++)
                coords.push_back(p[k]);
            float s0 = (float)i / SEGMENTS, s1 = (float)(i + 1) / SEGMENTS;
            float t0 = (float)j / stacks, t1 = (float)(j + 1) / stacks;
            Face f;
            f.colour = col;
            Corner c[4] = { { base, s0, t0, normalize(p[0]) }, { base + 1, s1, t0, normalize(p[1]) },
                            { base + 2, s1, t1, normalize(p[2]) }, { base + 3, s0, t1, normalize(p[3]) } };
            for (int k = 0; k < 4; k++)
                f.corners.push_back(c[k]);
            faces.push_back(f);
        }
    }
    emitMesh(state, coords, faces, texture, 1);
}

/* ---- text ------------------------------------------------------------------- */

/*
 * (new) AsciiText as flat polygons, as the VRML 1.0 specification asks:
 * glyph outlines come from the Windows TrueType fonts (Times New Roman,
 * Arial and Courier New for SERIF, SANS and TYPEWRITER) and are cut into
 * triangles by the GLU tessellator.  Glyphs are kept in units of the
 * font size, so each is only built once.
 */
struct Glyph {
    std::vector<V3> triangles;      /* three points per triangle, z = 0 */
    double advance;
};

static std::map<int, Glyph> glyphCache;     /* key: font << 8 | character */

struct Tessellation {
    std::vector<V3> *out;
    std::vector<double *> extra;    /* vertices made by the combine callback */
};

static void CALLBACK tessVertex(void *vertex, void *data)
{
    const double *p = (const double *)vertex;
    V3 v = { p[0], p[1], 0.0 };
    ((Tessellation *)data)->out->push_back(v);
}

static void CALLBACK tessCombine(GLdouble coords[3], void *[4], GLfloat [4],
                                 void **out, void *data)
{
    double *p = new double[3];
    p[0] = coords[0];
    p[1] = coords[1];
    p[2] = 0.0;
    ((Tessellation *)data)->extra.push_back(p);
    *out = p;
}

static void CALLBACK tessBegin(GLenum) {}
static void CALLBACK tessEnd(void) {}
static void CALLBACK tessEdgeFlag(GLboolean) {}     /* asks for triangles only */

static double fixedValue(FIXED f)
{
    return f.value + f.fract / 65536.0;
}

static const int GLYPH_EM = 1024;

static Glyph &glyphFor(int family, int style, unsigned char ch)
{
    int key = ((family * 4 + style) << 8) | ch;
    std::map<int, Glyph>::iterator it = glyphCache.find(key);
    if (it != glyphCache.end())
        return it->second;
    Glyph &g = glyphCache[key];
    g.advance = 0.5;

    static const char *faces[] = { "Times New Roman", "Arial", "Courier New" };
    HDC dc = CreateCompatibleDC(NULL);
    HFONT font = CreateFontA(-GLYPH_EM, 0, 0, 0,
                             (style & QvFontStyle::BOLD) ? FW_BOLD : FW_NORMAL,
                             (style & QvFontStyle::ITALIC) ? TRUE : FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, DEFAULT_PITCH,
                             faces[family >= 0 && family < 3 ? family : 0]);
    HGDIOBJ old = SelectObject(dc, font);
    GLYPHMETRICS gm;
    MAT2 identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
    DWORD size = GetGlyphOutlineA(dc, ch, GGO_NATIVE, &gm, 0, NULL, &identity);
    if (size != GDI_ERROR) {
        g.advance = (double)gm.gmCellIncX / GLYPH_EM;
        std::vector<char> buf(size + 1);
        if (size > 0 && GetGlyphOutlineA(dc, ch, GGO_NATIVE, &gm, size, &buf[0],
                                         &identity) != GDI_ERROR) {
            /* the outline as closed contours of straight segments */
            std::vector<std::vector<double> > contours;
            DWORD at = 0;
            while (at + sizeof(TTPOLYGONHEADER) <= size) {
                TTPOLYGONHEADER *h = (TTPOLYGONHEADER *)&buf[at];
                std::vector<double> c;
                double x = fixedValue(h->pfxStart.x), y = fixedValue(h->pfxStart.y);
                c.push_back(x);
                c.push_back(y);
                DWORD cat = at + sizeof(TTPOLYGONHEADER);
                while (cat < at + h->cb) {
                    TTPOLYCURVE *pc = (TTPOLYCURVE *)&buf[cat];
                    if (pc->wType == TT_PRIM_LINE) {
                        for (int i = 0; i < pc->cpfx; i++) {
                            x = fixedValue(pc->apfx[i].x);
                            y = fixedValue(pc->apfx[i].y);
                            c.push_back(x);
                            c.push_back(y);
                        }
                    } else {
                        /* quadratic B-spline: on-curve points lie midway
                         * between successive control points */
                        for (int i = 0; i + 1 < pc->cpfx; i++) {
                            double bx = fixedValue(pc->apfx[i].x), by = fixedValue(pc->apfx[i].y);
                            double cx = fixedValue(pc->apfx[i + 1].x), cy = fixedValue(pc->apfx[i + 1].y);
                            if (i + 2 < pc->cpfx) {
                                cx = (bx + cx) / 2.0;
                                cy = (by + cy) / 2.0;
                            }
                            for (int k = 1; k <= 4; k++) {
                                double t = k / 4.0, u = 1.0 - t;
                                c.push_back(u * u * x + 2 * u * t * bx + t * t * cx);
                                c.push_back(u * u * y + 2 * u * t * by + t * t * cy);
                            }
                            x = cx;
                            y = cy;
                        }
                    }
                    cat += sizeof(TTPOLYCURVE) + (pc->cpfx - 1) * sizeof(POINTFX);
                }
                contours.push_back(c);
                at += h->cb;
            }

            Tessellation t;
            t.out = &g.triangles;
            GLUtesselator *tess = gluNewTess();
            gluTessCallback(tess, GLU_TESS_BEGIN, (void (CALLBACK *)())tessBegin);
            gluTessCallback(tess, GLU_TESS_END, (void (CALLBACK *)())tessEnd);
            gluTessCallback(tess, GLU_TESS_EDGE_FLAG, (void (CALLBACK *)())tessEdgeFlag);
            gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (void (CALLBACK *)())tessVertex);
            gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (void (CALLBACK *)())tessCombine);
            gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
            gluTessNormal(tess, 0.0, 0.0, 1.0);
            std::vector<std::vector<double> > points(contours.size());
            gluTessBeginPolygon(tess, &t);
            for (size_t i = 0; i < contours.size(); i++) {
                std::vector<double> &c = contours[i];
                size_t n = c.size() / 2;
                points[i].resize(3 * n);
                gluTessBeginContour(tess);
                for (size_t k = 0; k < n; k++) {
                    double *p = &points[i][3 * k];
                    p[0] = c[2 * k] / GLYPH_EM;
                    p[1] = c[2 * k + 1] / GLYPH_EM;
                    p[2] = 0.0;
                    gluTessVertex(tess, p, p);
                }
                gluTessEndContour(tess);
            }
            gluTessEndPolygon(tess);
            gluDeleteTess(tess);
            for (size_t i = 0; i < t.extra.size(); i++)
                delete[] t.extra[i];
            if (g.triangles.size() % 3 != 0)
                g.triangles.resize(g.triangles.size() - g.triangles.size() % 3);
        }
    }
    SelectObject(dc, old);
    DeleteObject(font);
    DeleteDC(dc);
    return g;
}

void QvBuildQvAsciiText(QvAsciiText *node, QvState *state)
{
    QvFontStyle *fs = (QvFontStyle *)top(state, QvState::FontStyleIndex);
    double size = fs ? fs->size.value : 10.0;
    int family = fs ? fs->family.value : QvFontStyle::SERIF;
    int style = fs ? (fs->style.value & 3) : 0;
    double spacing = node->spacing.value;

    std::vector<V3> coords;
    std::vector<Face> faces;
    V3 up = { 0, 0, 1 };
    for (int line = 0; line < node->string.num; line++) {
        const char *s = node->string.values[line].getString();
        double width = 0.0;
        for (const char *p = s; *p; p++)
            width += glyphFor(family, style, (unsigned char)*p).advance;
        double x = 0.0, y = -line * spacing * size;
        if (node->justification.value == QvAsciiText::RIGHT)
            x = -width * size;
        else if (node->justification.value == QvAsciiText::CENTER)
            x = -width * size / 2.0;
        RLColour colour = partColour(state, line);
        for (const char *p = s; *p; p++) {
            Glyph &g = glyphFor(family, style, (unsigned char)*p);
            for (size_t i = 0; i + 2 < g.triangles.size(); i += 3) {
                Face f;
                f.colour = colour;
                for (int k = 0; k < 3; k++) {
                    V3 v = { x + g.triangles[i + k].x * size,
                             y + g.triangles[i + k].y * size, 0.0 };
                    Corner c = { (int)coords.size(), 0, 0, up };
                    coords.push_back(v);
                    f.corners.push_back(c);
                }
                faces.push_back(f);
            }
            x += g.advance * size;
        }
    }
    emitMesh(state, coords, faces, NULL, 0);
}

/* ---- lights, cameras, transformations ------------------------------------- */

static void placeLight(QvState *state, RLLightType type, float *colour, float intensity,
                       const float *location, const float *direction)
{
    M4 m = currentMatrix(state);
    RLLight light = RLCreateLightRGB(type, colour[0] * intensity, colour[1] * intensity,
                                     colour[2] * intensity);
    RLFrame f = RLCreateFrame(b->world);
    RLFrameAddLight(f, light);
    if (location) {
        V3 p = point(m, location[0], location[1], location[2]);
        RLFrameSetPosition(f, b->scene, p.x, p.y, -p.z);
    }
    if (direction) {
        V3 d = normalize(vec(m, direction[0], direction[1], direction[2]));
        d.z = -d.z;
        /* any up vector not parallel to the direction will do */
        double ux = 0, uy = 1, uz = 0;
        if (fabs(d.y) > 0.9) {
            uy = 0;
            uz = 1;
        }
        RLFrameSetOrientation(f, b->scene, d.x, d.y, d.z, ux, uy, uz);
    }
    b->lights++;
}

static void setCamera(QvState *state, const float *position, const QvSFRotation &orientation,
                      double heightAngle)
{
    if (b->haveCamera || !wcCamera || b->inlineDepth > 0)
        return;
    M4 m = currentMatrix(state);
    M4 r = mul(m, rotation(orientation.axis, orientation.angle));
    V3 p = point(m, position[0], position[1], position[2]);
    V3 d = normalize(vec(r, 0, 0, -1));
    V3 u = normalize(vec(r, 0, 1, 0));
    wcCamera->SetPosition(p.x, p.y, -p.z);
    wcCamera->SetOrientation(d.x, d.y, -d.z, u.x, u.y, -u.z);
    /* the field spans the larger side of the window; VRML gives the height */
    double f = tan(heightAngle / 2.0) * b->fieldAspect;
    if (f <= 0.0 || f > 10.0)
        f = 0.5;
    wcCamera->SetField(-f, f, -f, f);
    b->haveCamera = 1;
}

void QvBuildProperty(QvNode *node, QvState *state)
{
    QvElement *e;
    if ((e = state->getTopElement(QvState::LightIndex)) && e->data == node) {
        switch (e->type) {
        case QvElement::DirectionalLight: {
            QvDirectionalLight *l = (QvDirectionalLight *)node;
            if (l->on.value)
                placeLight(state, RLLightDirectional, l->color.value, l->intensity.value,
                           NULL, l->direction.value);
            break;
        }
        case QvElement::PointLight: {
            QvPointLight *l = (QvPointLight *)node;
            if (l->on.value)
                placeLight(state, RLLightPoint, l->color.value, l->intensity.value,
                           l->location.value, NULL);
            break;
        }
        case QvElement::SpotLight: {
            /* Reality Lab's spot cone is much narrower than VRML's default;
             * a point light is the closer match */
            QvSpotLight *l = (QvSpotLight *)node;
            if (l->on.value)
                placeLight(state, RLLightPoint, l->color.value, l->intensity.value,
                           l->location.value, NULL);
            break;
        }
        default:
            break;
        }
        return;
    }
    if ((e = state->getTopElement(QvState::CameraIndex)) && e->data == node) {
        if (e->type == QvElement::PerspectiveCamera) {
            QvPerspectiveCamera *c = (QvPerspectiveCamera *)node;
            setCamera(state, c->position.value, c->orientation, c->heightAngle.value);
        } else if (e->type == QvElement::OrthographicCamera) {
            QvOrthographicCamera *c = (QvOrthographicCamera *)node;
            setCamera(state, c->position.value, c->orientation, 0.785398);
        }
        return;
    }
}

/* ---- grouping and WWW nodes ------------------------------------------------- */

void QvBuildQvLOD(QvLOD *node, QvState *state)
{
    /* the most detailed version */
    if (node->getNumChildren() > 0)
        node->getChild(0)->traverse(state);
}

void QvBuildQvWWWAnchor(QvWWWAnchor *node, QvState *state)
{
    RLFrame saved = b->target;
    const char *url = node->name.value.getString();
    if (url && *url) {
        char abs[2048];
        if (!LabResolveURL(url, abs, sizeof(abs)))
            strncpy(abs, url, sizeof(abs) - 1), abs[sizeof(abs) - 1] = '\0';
        WcElement *e = new WcElement;
        e->SetURL(LabStrdup(abs));
        e->SetObject(LabStrdup(node->description.value.getString()));
        anchorElements.push_back(e);
        b->target = RLCreateFrame(b->world);
        RLObjectSetAppData(b->target, e);
    }
    for (int i = 0; i < node->getNumChildren(); i++)
        node->getChild(i)->traverse(state);
    b->target = saved;
}

void QvBuildQvWWWInline(QvWWWInline *node, QvState *state)
{
    const char *url = node->name.value.getString();
    if (!url || !*url || b->inlineDepth >= 8)
        return;
    labMainWindow->SetStatus(url);
    const char *file = LabFetchFile(url);
    if (!file)
        return;
    std::string fetchedFrom = LabLastFetchedURL();
    std::string savedBase = LabGetBaseURL();
    file = LabGunzipFile(file);
    FILE *fp = fopen(file, "r");
    if (!fp)
        return;
    QvInput in;
    in.setFilePointer(fp);
    QvNode *root = NULL;
    int ok = QvDB::read(&in, root) && root != NULL;
    fclose(fp);
    if (!ok)
        return;
    /* the inlined world's own references are relative to it */
    LabSetBaseURL(fetchedFrom.c_str());
    b->inlineDepth++;
    root->ref();
    state->push();
    root->traverse(state);
    state->pop();
    root->unref();
    b->inlineDepth--;
    LabSetBaseURL(savedBase.c_str());
}

void QvBuildQvInfo(QvInfo *, QvState *) {}

void QvBuildQvUnknownNode(QvUnknownNode *node, QvState *state)
{
    for (int i = 0; i < node->getNumChildren(); i++)
        node->getChild(i)->traverse(state);
}
