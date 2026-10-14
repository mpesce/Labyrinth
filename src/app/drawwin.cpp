/*
 * drawwin.cpp -- the 3D view, world building and navigation.
 * Reconstructed from LABYRINTH.EXE 0x40ecc0-0x411490.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "drawwin.h"
#include "mainwin.h"
#include "labwww.h"
#include "labimage.h"
#include "inflate.h"
#include "qvbuild.h"
#include "../wc/wc.h"

extern "C" void LabError(const char *fmt, ...);
extern "C" void LabLog(const char *fmt, ...);

LabDrawingWindow::LabDrawingWindow(LabWindow *parent, int x, int y,
                                   int w, int h)                /* 40ecc0 */
    : LabChildWindow(parent, x, y, w, h, Visible | Border)
{
    wcCamera = new WcCamera;            /* the camera block of each world */
    view = new WcCamera;                /* where we are looking now */
    scene = RLCreateFrame(NULL);
    camera = RLCreateFrame(scene);
    selected = 0;
    marker = 0;
    extraFrame = 0;
    ambient = 0;
    lastX = lastY = 0;
    width = w;
    height = h;
    unused38 = 0;

    device = RLWinCreateDevice(peer->hwnd, GetWidth(), GetHeight());
    RLDeviceSetDither(device, 0);
    RLDeviceSetColours(device, 0);
    RLDeviceSetQuality(device, RLRenderGouraud);
    viewport = RLCreateViewport(device, camera, 0, 0,
                                RLDeviceGetWidth(device), RLDeviceGetHeight(device));
    RLViewportSetBack(viewport, 5000.0);
}

void LabDrawingWindow::OnPaint(LabPaint *p)                     /* 410040 */
{
    RLWinHandlePaint(device, p->hdc);
}

/* Rebuild the viewport for the new size; the device only ever grows. */
void LabDrawingWindow::OnResize(int w, int h)                   /* 410060 */
{
    RLValue l, r, b, t;
    width = w;
    height = h;
    int dw = RLDeviceGetWidth(device);
    int dh = RLDeviceGetHeight(device);
    RLViewportGetPlane(viewport, &l, &r, &b, &t);
    RLObjectDestroy(viewport);
    if (dw < width || dh < height) {
        int dither = RLDeviceGetDither(device);
        RLRenderQuality quality = RLDeviceGetQuality(device);
        int colours = RLDeviceGetColours(device);
        int shades = RLDeviceGetShades(device);
        RLObjectDestroy(device);
        device = RLWinCreateDevice(peer->hwnd, width, height);
        RLDeviceSetDither(device, dither);
        RLDeviceSetQuality(device, quality);
        RLDeviceSetColours(device, colours);
        RLDeviceSetShades(device, shades);
    }
    viewport = RLCreateViewport(device, camera, 0, 0, width, height);
    RLViewportSetBack(viewport, 5000.0);
    RLViewportSetPlane(viewport, l, r, b, t);
}

void LabDrawingWindow::OnMouseDown(int x, int y, int mods)      /* 4101d0 */
{
    if (mods == 0)
        FollowLink(x, y);
    else if (mods > 0 && mods <= 2)
        BeginDrag(x, y);
}

void LabDrawingWindow::OnMouseUp(int x, int y, int mods)        /* 410210 */
{
    EndDrag();
}

/* Track what is under the mouse: show links, drag the selection. */
void LabDrawingWindow::OnMouseMove(int x, int y, int mods)      /* 410230 */
{
    RLVisual visual;
    RLFrame frame;
    int cursor = LabCursorArrow;
    char text[200];

    memset(text, 0, sizeof(text));
    strcpy(text, "        ");
    if (RLViewportPick(viewport, &visual, &frame, x, y) != RLNotFound) {
        if (mods == 0) {
            WcElement *e = (WcElement *)RLObjectGetAppData(frame);
            if (e && e->GetURL() && strlen(e->GetURL()) != 0) {
                cursor = LabCursorLink;
                strncpy(text, e->GetURL(), sizeof(text) - 1);
            }
        } else if (mods & LabModShift) {
            cursor = LabCursorMove;
        } else if (mods & LabModControl) {
            cursor = LabCursorRotate;
        }
    }
    SetCursor(cursor);
    labMainWindow->SetStatus(text);
    if (mods == LabModShift) {
        if (selected)
            DragMove(x, y);
    } else if (mods == LabModControl) {
        if (selected)
            DragRotate(x, y);
    }
}

/* Called on every idle pass: keep the hover feedback up to date. */
void LabDrawingWindow::PollMouse()                              /* 4103b0 */
{
    int x, y, mods;
    if (!HasCapture()) {
        GetMouse(&x, &y, &mods);
        OnMouseMove(x, y, mods);
    }
}

/* Clicking on an object with a URL: worlds are loaded in place, anything
 * else goes to the WWW browser. */
void LabDrawingWindow::FollowLink(int x, int y)                 /* 410400 */
{
    RLVisual visual;
    RLFrame frame;
    if (RLViewportPick(viewport, &visual, &frame, x, y) == RLNotFound)
        return;
    WcElement *e = (WcElement *)RLObjectGetAppData(frame);
    if (!e || !e->GetURL() || strlen(e->GetURL()) == 0)
        return;
    if (strstr(e->GetURL(), ".wrl")) {
        SetCursor(LabCursorWait);
        ClearScene();
        LabFetchWorld(e->GetURL());
        LoadWorld(LAB_WORLD_FILE);
        SetCursor(LabCursorArrow);
        return;
    }
    SetCursor(LabCursorWait);
    LabLaunchBrowser(e->GetURL());
    SetCursor(LabCursorArrow);
}

/* Shift-drag: move the selected object parallel to the screen.
 * (fixed) The original took the object's position relative to the camera
 * and passed it to RLViewportTransform, which wants world coordinates, so
 * objects jumped away unless the camera was at the origin. */
void LabDrawingWindow::DragMove(int x, int y)                   /* 4104f0 */
{
    RLVector pos;
    float scr[4];
    if (!selected)
        return;
    int oldX = lastX;
    lastX = x;
    float dy = (float)(y - lastY);
    lastY = y;
    RLFrameGetPosition(selected, scene, &pos);
    RLViewportTransform(viewport, scr, &pos);
    scr[0] = (float)(x - oldX) * scr[3] + scr[0];
    scr[1] = scr[3] * (float)(int)dy + scr[1];
    RLViewportInverseTransform(viewport, &pos, scr);
    RLFrameSetPosition(selected, scene, pos.x, pos.y, pos.z);
}

/* Turn v about the unit axis k (Reality Lab's sense of rotation). */
static void turnVector(RLVector &v, const RLVector &k, double angle)
{
    double c = cos(angle), s = sin(angle);
    double kv = k.x * v.x + k.y * v.y + k.z * v.z;
    RLVector cr = { (float)(k.y * v.z - k.z * v.y), (float)(k.z * v.x - k.x * v.z),
                    (float)(k.x * v.y - k.y * v.x) };
    v.x = (float)(v.x * c + cr.x * s + k.x * kv * (1 - c));
    v.y = (float)(v.y * c + cr.y * s + k.y * kv * (1 - c));
    v.z = (float)(v.z * c + cr.z * s + k.z * kv * (1 - c));
}

/* Ctrl-drag: tilt the selected object.
 * (fixed) The original pushed the object's direction and up vectors
 * through the viewport transformation as if they were points, which spun
 * objects unpredictably.  Turn the object about the camera's up and right
 * axes instead, so that its near side follows the mouse. */
void LabDrawingWindow::DragRotate(int x, int y)                 /* 4105d0 */
{
    RLVector dir, up, cdir, cup;
    if (!selected)
        return;
    int dx = x - lastX;
    lastX = x;
    int dy = y - lastY;
    lastY = y;
    RLFrameGetOrientation(selected, scene, &dir, &up);
    RLFrameGetOrientation(camera, scene, &cdir, &cup);
    RLVector right = { cup.y * cdir.z - cup.z * cdir.y, cup.z * cdir.x - cup.x * cdir.z,
                       cup.x * cdir.y - cup.y * cdir.x };
    double len = sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    if (len == 0.0)
        return;
    right.x = (float)(right.x / len);
    right.y = (float)(right.y / len);
    right.z = (float)(right.z / len);
    double k = 0.01 * 96.0 / GetDpi();     /* (new) the same turn per inch at any scale */
    turnVector(dir, cup, -dx * k);
    turnVector(up, cup, -dx * k);
    turnVector(dir, right, -dy * k);
    turnVector(up, right, -dy * k);
    RLFrameSetOrientation(selected, scene, dir.x, dir.y, dir.z, up.x, up.y, up.z);
}

/* Select the object under the mouse and mark it with a white outline. */
void LabDrawingWindow::BeginDrag(int x, int y)                  /* 410720 */
{
    RLVisual visual;
    RLFrame frame;
    RLVisual *visuals;
    int count;
    RLBox box;

    EndDrag();
    if (RLViewportPick(viewport, &visual, &frame, x, y) == RLNotFound) {
        LabLog("No item was picked.\n");
        return;
    }
    RLObjectGetAppData(frame);
    RLFrameGetVisuals(frame, &count, &visuals);
    RLMeshGetBox(visuals[0], &box);
    marker = LabCreateSelectionMarker(&box);
    RLFrameAddVisual(frame, marker);
    RLFree(visuals);
    selected = frame;
    lastX = x;
    lastY = y;
}

/* (fixed) The original removed the frame's second visual, taking it to be
 * the marker; that deleted part of the object when a frame held several. */
void LabDrawingWindow::EndDrag()                                /* 4107f0 */
{
    if (!selected)
        return;
    if (marker)
        RLFrameDeleteVisual(selected, marker);
    marker = 0;
    selected = 0;
}

/* Turn about the camera's up vector and move along its direction. */
void LabDrawingWindow::Navigate(double turn, double move)       /* 40fc90 */
{
    double px, py, pz, dx, dy, dz, ux, uy, uz;
    double step = move * 0.125;

    view->GetPosition(&px, &py, &pz);
    view->GetOrientation(&dx, &dy, &dz, &ux, &uy, &uz);
    px += dx * step;
    py += dy * step;
    pz += dz * step;
    RLFrameSetPosition(camera, scene, px, py, pz);
    view->SetPosition(px, py, pz);

    double angle = turn * 0.01;
    if (angle <= 6.28) {
        RLVector dir, up;
        RLFrameSetPosition(camera, scene, 0.0, 0.0, 0.0);
        RLFrameAddRotation(camera, RLCombineAfter, ux, uy, uz, angle);
        RLFrameSetPosition(camera, scene, px, py, pz);
        RLFrameGetOrientation(camera, scene, &dir, &up);
        view->SetOrientation(dir.x, dir.y, dir.z, up.x, up.y, up.z);
    }
}

/* Move along the camera's up vector. */
void LabDrawingWindow::Slide(double unused, double amount)      /* 40ff30 */
{
    double px, py, pz, dx, dy, dz, ux, uy, uz;
    double step = amount * 0.125;
    view->GetPosition(&px, &py, &pz);
    view->GetOrientation(&dx, &dy, &dz, &ux, &uy, &uz);
    px += ux * step;
    py += uy * step;
    pz += uz * step;
    RLFrameSetPosition(camera, scene, px, py, pz);
    view->SetPosition(px, py, pz);
}

/* Move straight up or down. */
void LabDrawingWindow::Elevate(double amount)                   /* 40fea0 */
{
    double px, py, pz;
    view->GetPosition(&px, &py, &pz);
    py += amount * 0.1;
    RLFrameSetPosition(camera, scene, px, py, pz);
    view->SetPosition(px, py, pz);
}

/* Return to the camera given by the world. */
void LabDrawingWindow::ResetCamera()                            /* 40fab0 */
{
    double px, py, pz, dx, dy, dz, ux, uy, uz, l, r, b, t;
    wcCamera->GetPosition(&px, &py, &pz);
    view->SetPosition(px, py, pz);
    wcCamera->GetOrientation(&dx, &dy, &dz, &ux, &uy, &uz);
    view->SetOrientation(dx, dy, dz, ux, uy, uz);
    wcCamera->GetField(&l, &r, &b, &t);
    view->SetField(l, r, b, t);
    RLFrameSetPosition(camera, scene, px, py, pz);
    RLFrameSetOrientation(camera, scene, dx, dy, dz, ux, uy, uz);
    RLViewportSetPlane(viewport, l, r, b, t);
    RLViewportForceUpdate(viewport, 0, 0, RLDeviceGetWidth(device), RLDeviceGetHeight(device));
}

void LabDrawingWindow::SaveCamera(const char *file)             /* 40f0b0 */
{
    double px, py, pz, dx, dy, dz, ux, uy, uz, l, r, b, t;
    FILE *fp = fopen(file, "wt");
    if (!fp)
        return;
    fprintf(fp, "World \"%s\" Begin\n\n", file);
    view->GetPosition(&px, &py, &pz);
    view->GetOrientation(&dx, &dy, &dz, &ux, &uy, &uz);
    view->GetField(&l, &r, &b, &t);
    fprintf(fp, "Camera Begin\n");
    fprintf(fp, "\tPosition %g %g %g\n", px, py, pz);
    fprintf(fp, "\tOrientation %g %g %g %g %g %g\n", dx, dy, dz, ux, uy, uz);
    fprintf(fp, "\tField %g %g %g %g\n", l, r, t, b);
    fprintf(fp, "End # Camera definition\n\n");
    fprintf(fp, "End # World definition\n");
    fclose(fp);
}

/* Try the file as VRML first, then as a Labyrinth world. */
void LabDrawingWindow::LoadWorld(const char *file)              /* 40efd0 */
{
    file = LabGunzipFile(file);         /* (new) .wrl.gz */
    if (LabLoadVRML(scene, file) < 0)
        LabBuildWcWorld(scene, file);
    SetupScene(scene);
    selected = 0;
    marker = 0;
    ResetCamera();
}

/* Delete everything in the scene except the camera. */
void LabDrawingWindow::ClearScene()                             /* 40f050 */
{
    int count;
    RLFrame *children;
    selected = 0;
    marker = 0;
    RLFrameGetChildren(scene, &count, &children);
    for (int i = 0; i < count; i++)
        if (children[i] != camera)
            RLFrameDeleteChild(scene, children[i]);
    RLFree(children);
}

void LabDrawingWindow::SetupScene(RLFrame s)                    /* 40ef30 */
{
    extraFrame = RLCreateFrame(s);
    RLFrameSetPosition(extraFrame, s, 5.0, 5.0, -1.0);
    /* The original added a fresh ambient light to the scene frame on every
     * load (and ClearScene never removed it), so each world loaded made the
     * view brighter.  Only one is added here. */
    if (!ambient) {
        ambient = RLCreateLightRGB(RLLightAmbient, 0.2, 0.2, 0.2);
        RLFrameAddLight(s, ambient);
    }
}

/* ---- building a Labyrinth world --------------------------------------- */

static int isSet(double r, double g, double b)
{
    return r != 0.0 || g != 0.0 || b != 0.0;
}

/* One polygon becomes one face with a single (flat) normal. */
void LabBuildPoly(WcPoly *poly, RLMesh mesh)                    /* 40f4f0 */
{
    double x, y, z, r, g, b;
    RLVector n;

    RLFace face = RLMeshCreateFace(mesh);
    RLObjectSetAppData(face, 0);
    poly->GetNormal(&x, &y, &z);
    n.x = (float)x;
    n.y = (float)y;
    n.z = (float)z;
    if (x == -1.0 && y == -1.0 && z == -1.0) {
        /* (fixed) A poly with no Normal statement keeps the compiler's
         * "not specified" mark of (-1, -1, -1), which the original used as
         * the normal, so every such face was lit alike.  Work it out from
         * the vertices instead (Newell's method, as Reality Lab does). */
        int k = poly->GetVertexCount();
        n.x = n.y = n.z = 0.0f;
        for (int i = 0; i < k; i++) {
            double ax, ay, az, bx, by, bz;
            poly->GetVertex(i, &ax, &ay, &az);
            poly->GetVertex((i + 1) % k, &bx, &by, &bz);
            n.x += (float)((ay - by) * (az + bz));
            n.y += (float)((az - bz) * (ax + bx));
            n.z += (float)((ax - bx) * (ay + by));
        }
        if (n.x == 0.0f && n.y == 0.0f && n.z == 0.0f)
            n.z = -1.0f;
    }
    RLVectorNormalise(&n);
    int normal = RLMeshAddNormal(mesh, n.x, n.y, n.z);
    if (normal == -1)
        LabError("Received error %d from RLMeshAddNormal");
    for (int i = 0; i < poly->GetVertexCount(); i++) {
        poly->GetVertex(i, &x, &y, &z);
        int vertex = RLMeshAddVertex(mesh, x, y, z);
        if (vertex == -1)
            LabError("Received error %d from RLMeshAddVertex");
        RLFaceAddVertexAndNormalIndexed(face, vertex, normal);
    }
    poly->GetColor(&r, &g, &b);
    if (isSet(r, g, b))
        RLFaceSetColourRGB(face, r, g, b);
}

static RLTexture loadTextureFile(int gif)                       /* 40f690 */
{
    const char *file = LabGunzipFile(LAB_TEXTURE_FILE);    /* (new) gzipped */
    if (gif == 0) {
        RLTexture t = RLLoadTexture(file);
        if (!t)
            t = LabLoadImage(file);     /* (new) JPEG, PNG, ... */
        return t;
    }
    return LabLoadGIF(file);
}

/* Build the mesh for the object an element refers to. */
RLMesh LabBuildObject(WcElement *element)                       /* 40f6b0 */
{
    char *name = element->GetObject();
    WcObject *obj = 0;
    RLMesh mesh;
    double r, g, b, x, y, z;

    if (wcObjects)
        for (obj = (WcObject *)wcObjects->First(); obj; obj = (WcObject *)LabList::Next(obj))
            if (strcmp(obj->GetName(), name) == 0)
                break;
    if (!obj) {
        /* the original printed "Null object in list" and exited */
        LabError("Object \"%s\" is not defined.", name);
        return RLCreateMesh();
    }

    char *shape = obj->GetShapeURL();
    if (shape && strlen(shape) != 0) {
        labMainWindow->SetStatus(shape);
        LabFetchShape(shape);
        mesh = RLLoadMesh(LabGunzipFile(LAB_SHAPE_FILE));   /* (new) gzipped */
        if (!mesh)
            mesh = RLCreateMesh();
    } else {
        mesh = RLCreateMesh();
        for (WcPoly *p = obj->FirstPoly(); p; p = obj->NextPoly(p))
            LabBuildPoly(p, mesh);
    }

    obj->GetColor(&r, &g, &b);
    if (isSet(r, g, b))
        RLMeshSetColourRGB(mesh, r, g, b);

    char *texture = obj->GetTexture();
    if (texture && strlen(texture) != 0) {
        int gif = 0;
        char *ext = strrchr(texture, '.');
        if (ext && *ext && _stricmp(ext, ".gif") == 0)
            gif = 1;
        LabFetchTexture(texture);

        int nfaces;
        RLFace *faces;
        RLMeshGetFaces(mesh, &nfaces, &faces);
        if (nfaces == 1) {
            /* a single face carries the whole picture */
            RLTexture t = loadTextureFile(gif);
            RLFaceSetTexture(faces[0], t);
            RLMeshSetTextureCoordinates(mesh, 2, 0.01, 0.01);
            RLMeshSetTextureCoordinates(mesh, 1, 0.01, 0.99);
            RLMeshSetTextureCoordinates(mesh, 0, 0.99, 0.99);
            RLMeshSetTextureCoordinates(mesh, 3, 0.99, 0.01);
            RLTextureSetColours(t, 32);
            fprintf(stderr, "Made a billboard.\n");
        } else {
            RLWrapType type;
            switch (obj->GetMapping()) {
            case WcMapCylinder: type = RLWrapCylinder; break;
            case WcMapSphere: type = RLWrapSphere; break;
            default: type = RLWrapFlat; break;
            }
            RLBox box;
            RLMeshGetBox(mesh, &box);
            double h = box.max.y - box.min.y;
            RLWrap wrap = RLCreateWrap(type, NULL,
                                       0.0, 0.0, 0.0,       /* origin */
                                       0.0, 1.0, 0.0,       /* direction */
                                       0.0, 0.0, 1.0,       /* up */
                                       0.0, box.min.y / h,  /* ou, ov */
                                       1.0, -(1.0 / h));    /* su, sv */
            RLWrapApply(wrap, mesh);
            RLObjectDestroy(wrap);
            RLTexture t = loadTextureFile(gif);
            RLMeshSetTexture(mesh, t);
            RLTextureSetColours(t, 32);
        }
        RLFree(faces);
    }

    if (obj->GetSpecular() != 0.0 || obj->GetPower() != 0.0)
        RLMeshSetMaterial(mesh, RLCreateMaterial(obj->GetSpecular(), obj->GetPower()));

    element->GetPosition(&x, &y, &z);
    RLObjectSetAppData(mesh, 0);
    return mesh;
}

/* Compile a Labyrinth world and build its first scene under 'scene'. */
RLFrame LabBuildWcWorld(RLFrame scene, const char *file)        /* 40f220 */
{
    double x, y, z, dx, dy, dz, ux, uy, uz;

    WcCompile(file);
    if (!wcScenes)
        return scene;
    WcScene *s = (WcScene *)wcScenes->First();
    int lights = 0;
    for (WcElement *e = s->FirstElement(); e; e = s->NextElement(e)) {
        RLFrame frame;
        if (e->GetType() == WcElementLight) {
            WcLight *l = (WcLight *)e;
            lights++;
            double r, g, b;
            l->GetColor(&r, &g, &b);
            RLLight light = RLCreateLightRGB((RLLightType)l->GetLightType(), r, g, b);
            frame = RLCreateFrame(scene);
            RLFrameAddLight(frame, light);
        } else {
            RLMesh mesh = LabBuildObject(e);
            RLObjectSetAppData(mesh, 0);
            frame = RLCreateFrame(scene);
            if (RLFrameAddVisual(frame, mesh) != RLNoError)
                LabError("Error on RLFrameAddVisual.");
        }
        e->GetPosition(&x, &y, &z);
        RLFrameSetPosition(frame, scene, x, y, z);
        e->GetOrientation(&dx, &dy, &dz, &ux, &uy, &uz);
        RLFrameSetOrientation(frame, scene, dx, dy, dz, ux, uy, uz);
        RLObjectSetAppData(frame, e);
    }
    if (lights == 0)
        QvBuildHeadlight(scene);        /* (new) */
    return scene;
}

/* ---- the selection marker --------------------------------------------- */

/* Face lists for RLMeshAddFaces: count, then vertex/normal index pairs. */
static int strutFaces[] = {                                     /* 4b73e0 */
    8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0,
    4, 0, 0, 1, 1, 9, 1, 8, 0,
    4, 1, 1, 2, 2, 10, 2, 9, 1,
    4, 2, 2, 3, 3, 11, 3, 10, 2,
    4, 3, 3, 4, 4, 12, 4, 11, 3,
    4, 4, 4, 5, 5, 13, 5, 12, 4,
    4, 5, 5, 6, 6, 14, 6, 13, 5,
    4, 6, 6, 7, 7, 15, 7, 14, 6,
    4, 7, 7, 0, 0, 8, 0, 15, 7,
    8, 8, 0, 9, 1, 10, 2, 11, 3, 12, 4, 13, 5, 14, 6, 15, 7,
    0
};

static int coneFaces[] = {                                      /* 4b7590 */
    8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0,
    3, 0, 0, 1, 1, 8, 1,
    3, 1, 1, 2, 2, 8, 1,
    3, 2, 2, 3, 3, 8, 1,
    3, 3, 3, 4, 4, 8, 1,
    3, 4, 4, 5, 5, 8, 1,
    3, 5, 5, 6, 6, 8, 1,
    3, 6, 6, 7, 7, 8, 1,
    3, 7, 7, 0, 0, 8, 1,
    0
};

/* The eight corners of an octagon of the given radius around the axis
 * running from (x1,y1,z1) to (x2,y2,z2), centred on the origin, and the
 * matching outward normals. */
static void octagon(double radius, float x1, float y1, float z1,
                    float x2, float y2, float z2,
                    RLVector v[8], RLVector n[8])
{
    RLVector d, p, q;
    d.x = x2 - x1;
    d.y = y2 - y1;
    d.z = z2 - z1;
    RLVectorNormalise(&d);

    /* a unit vector perpendicular to d, with no x component */
    if (fabs(d.y) == 0.0 && fabs(d.z) == 0.0) {
        p.z = 1.0f;
        p.y = 0.0f;
    } else if (fabs(d.y) == 0.0) {
        p.y = 1.0f;
        p.z = 0.0f;
    } else {
        p.z = (float)sqrt(1.0 / ((d.z * d.z) / (d.y * d.y) + 1.0));
        p.y = -((p.z / d.y) * d.z);
    }
    p.x = 0.0f;
    RLVectorCrossProduct(&q, &d, &p);

    float rad = (float)radius;
    p.x *= rad; p.y *= rad; p.z *= rad;
    q.x *= rad; q.y *= rad; q.z *= rad;

    /* tan(22.5 degrees) */
    double k = sqrt(2.0) / ((0.5 * sqrt(2.0) + 1.0) * 2.0);
#define CORNER(i, a, pa, b, pb)                         \
    v[i].x = (float)((a) * p.x * (pa) + (b) * q.x * (pb)); \
    v[i].y = (float)((a) * p.y * (pa) + (b) * q.y * (pb)); \
    v[i].z = (float)((a) * p.z * (pa) + (b) * q.z * (pb));
    CORNER(0, 1.0, 1.0, k, 1.0);        /*  p + kq */
    CORNER(1, k, 1.0, 1.0, 1.0);        /* kp +  q */
    CORNER(2, -k, 1.0, 1.0, 1.0);       /* -kp + q */
    CORNER(3, -1.0, 1.0, k, 1.0);       /* -p + kq */
    CORNER(4, -1.0, 1.0, -k, 1.0);      /* -p - kq */
    CORNER(5, -k, 1.0, -1.0, 1.0);      /* -kp - q */
    CORNER(6, k, 1.0, -1.0, 1.0);       /*  kp - q */
    CORNER(7, 1.0, 1.0, -k, 1.0);       /*  p - kq */
#undef CORNER
    for (int i = 0; i < 8; i++) {
        n[i] = v[i];
        RLVectorNormalise(&n[i]);
    }
}

/* An octagonal rod from one point to another. */
void LabAddStrut(RLMesh mesh, double radius, float x1, float y1, float z1,
                 float x2, float y2, float z2)                  /* 410840 */
{
    RLVector v[16], n[8];
    octagon(radius, x1, y1, z1, x2, y2, z2, v, n);
    for (int i = 0; i < 8; i++) {
        v[8 + i].x = v[i].x + x2;
        v[8 + i].y = v[i].y + y2;
        v[8 + i].z = v[i].z + z2;
        v[i].x += x1;
        v[i].y += y1;
        v[i].z += z1;
    }
    RLMeshAddFaces(mesh, 16, v, 8, n, strutFaces, NULL, NULL);
}

/* An eight-sided cone with its base at the first point. */
void LabAddCone(RLMesh mesh, double radius, float x1, float y1, float z1,
                float x2, float y2, float z2)                   /* 410c20 */
{
    RLVector v[9], n[8];
    octagon(radius, x1, y1, z1, x2, y2, z2, v, n);
    for (int i = 0; i < 8; i++) {
        v[i].x += x1;
        v[i].y += y1;
        v[i].z += z1;
    }
    v[8].x = x2;
    v[8].y = y2;
    v[8].z = z2;
    RLMeshAddFaces(mesh, 9, v, 8, n, coneFaces, NULL, NULL);
}

/* A white box around the selected object and a pointer along its z axis. */
RLMesh LabCreateSelectionMarker(RLBox *box)                     /* 410fe0 */
{
    static float markerStart[3];                                /* 4b76b8 */
    static float markerEnd[3];                                  /* 4b76c8 */
    float x0 = box->min.x, y0 = box->min.y, z0 = box->min.z;
    float x1 = box->max.x, y1 = box->max.y, z1 = box->max.z;

    RLMesh mesh = RLCreateMesh();
    markerEnd[2] = z1 + 1.0f;
    LabAddStrut(mesh, 0.05, markerStart[0], markerStart[1], markerStart[2],
                markerEnd[0], markerEnd[1], markerEnd[2]);
    LabAddCone(mesh, 0.2, markerEnd[0], markerEnd[1], markerEnd[2],
               markerEnd[0], markerEnd[1], markerEnd[2] + 0.6f);

    LabAddStrut(mesh, 0.05, x0, y0, z0, x0, y1, z0);
    LabAddStrut(mesh, 0.05, x0, y1, z0, x1, y1, z0);
    LabAddStrut(mesh, 0.05, x1, y1, z0, x1, y0, z0);
    LabAddStrut(mesh, 0.05, x1, y0, z0, x0, y0, z0);
    LabAddStrut(mesh, 0.05, x0, y0, z0, x0, y0, z1);
    LabAddStrut(mesh, 0.05, x0, y0, z1, x1, y0, z1);
    LabAddStrut(mesh, 0.05, x1, y0, z1, x1, y1, z1);
    LabAddStrut(mesh, 0.05, x1, y1, z1, x0, y1, z1);
    LabAddStrut(mesh, 0.05, x0, y1, z1, x0, y0, z1);
    LabAddStrut(mesh, 0.05, x0, y1, z1, x0, y1, z0);
    LabAddStrut(mesh, 0.05, x1, y1, z1, x1, y1, z0);
    LabAddStrut(mesh, 0.05, x1, y0, z1, x1, y0, z0);

    RLMeshSetColour(mesh, RLCreateColourRGB(1.0, 1.0, 1.0));
    return mesh;
}
