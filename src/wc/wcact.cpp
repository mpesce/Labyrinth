/*
 * wcact.cpp -- semantic actions and driver for the Labyrinth world compiler.
 * Reconstructed from LABYRINTH.EXE 0x414550-0x415f70.
 *
 * Tony's tracing calls (WcDebug) were compiled to an empty function in the
 * shipped program; define WC_DEBUG to get them back on stdout.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "wc.h"

extern "C" void LabError(const char *fmt, ...);

LabList *wcObjects;                                     /* 4b7d0c */
LabList *wcScenes;                                      /* 4b7d10 */
WcWorld *wcWorld;                                       /* 4b7cf4 */
WcCamera *wcCamera;                                     /* 4b7d14 */
int wcInWorld;                                          /* 4b8254 */

static WcObject *curObject;                             /* 4b7cf8 */
static WcPoly *curPoly;                                 /* 4b7cfc */
static WcScene *curScene;                               /* 4b7d00 */
static WcElement *curElement;                           /* 4b7d04 */
static WcLight *curLight;                               /* 4b7d08 */

static int openStack[256];                              /* 4a7f18 */
static int openDepth;                                   /* 4b8258 */
static char errorBuf[1024];                             /* 4a7b18 */

void WcDebug(const char *fmt, ...)                      /* 414610 */
{
#ifdef WC_DEBUG
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
#else
    (void)fmt;
#endif
}

void WcError(const char *fmt, ...)                      /* 414640 */
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(errorBuf, fmt, ap);
    va_end(ap);
    LabError("Compilation error: %s\nAt or near line %d\n", errorBuf, WcLineNumber());
}

void wcerror(const char *msg)                           /* 414550 (yyerror) */
{
    WcError(msg);
}

static void WcDebugToken(int tok)                       /* 414560 */
{
    WcDebug("token: %s", WcTokenName(tok));
    switch (tok) {
    case WC_IDENTIFIER:
        WcDebug("\t\t%s\n", wclval.sval);
        break;
    case WC_INTEGER:
        WcDebug("\t\t%d\n", wclval.ival);
        break;
    case WC_DOUBLE:
        WcDebug("\t\t%g\n", wclval.dval);
        break;
    case WC_STRING:
        WcDebug("\t\t\"%s\"\n", wclval.sval);
        break;
    default:
        WcDebug("\n");
        break;
    }
}

int WcNextToken(void)                                   /* 414620 */
{
    int tok = WcLex();
    WcDebugToken(tok);
    return tok;
}

const char *WcTokenName(int token)                      /* 415540 */
{
    switch (token) {
    case '{': return "{";
    case 0: return "EOF";
    case '}': return "}";
    case WC_OBJECT: return "Object";
    case WC_POLY: return "Poly";
    case WC_VERTEX: return "Vertex";
    case WC_COLOR: return "Color";
    case WC_MATERIAL: return "Material";
    case WC_TEXTURE: return "Texture";
    case WC_SHAPEFILE: return "Shapefile";
    case WC_CAMERA: return "Camera";
    case WC_URL: return "URL";
    case WC_SCENE: return "Scene";
    case WC_ELEMENT: return "Element";
    case WC_POSITION: return "Position";
    case WC_ORIENTATION: return "Orientation";
    case WC_OPEN: return "Open";
    case WC_CLOSE: return "Close";
    case WC_BEGIN: return "Begin";
    case WC_END: return "End";
    case WC_IDENTIFIER: return "IDENTIFIER";
    case WC_INTEGER: return "INTEGER";
    case WC_DOUBLE: return "DOUBLE";
    case WC_STRING: return "STRING";
    }
    /* Anything else (including World, Field, Normal and Light, which the
     * original table forgot) comes back as a one-character string. */
    static char one[2];
    one[0] = (char)token;
    one[1] = '\0';
    return one;
}

/* Open/close keywords must pair up: Open..Close, Begin..End, {..} */
static void WcPushOpen(int token)                       /* 415760 */
{
    if (openDepth < (int)(sizeof(openStack) / sizeof(openStack[0])) - 1)
        openStack[++openDepth] = token;
}

static void WcMatchClose(int token)                     /* 415780 */
{
    int expected = 0;
    switch (openStack[openDepth]) {
    case '{': expected = '}'; break;
    case WC_OPEN: expected = WC_CLOSE; break;
    case WC_BEGIN: expected = WC_END; break;
    default: LabError("Internal error: bad match token"); break;
    }
    if (openDepth > 0)
        openDepth--;
    if (expected != token)
        WcError("Expected \"%s\", saw \"%s\"", WcTokenName(expected), WcTokenName(token));
}

/* The object that Color/Material/Texture apply to: the current polygon,
 * else the current object, else the current scene. */
static WcObject *attributeTarget(void)
{
    if (curPoly) return curPoly;
    if (curObject) return curObject;
    return curScene;
}

static int mapType(char *map)
{
    if (strcmp(map, "Flat") == 0) return WcMapFlat;
    if (strcmp(map, "Cylinder") == 0) return WcMapCylinder;
    if (strcmp(map, "Sphere") == 0) return WcMapSphere;
    WcError("Text map type must be Flat, Cylinder or Sphere");
    return WcMapFlat;
}

static const char *mapName(int map)
{
    return map == WcMapFlat ? "Flat" : map == WcMapSphere ? "Sphere" : "Cylinder";
}

void WcBeginWorld(char *name, int open)                 /* 414760 */
{
    WcPushOpen(open);
    WcDebug("WcBeginWorld(objname = %s, opentoken = %s)\n", name, WcTokenName(open));
    wcInWorld = 1;
    wcWorld = new WcWorld(name);
}

void WcEndWorld(int close)                              /* 4147c0 */
{
    WcMatchClose(close);
    WcDebug("WcEndWorld(closetoken = %s)\n", WcTokenName(close));
    wcInWorld = 0;
}

void WcBeginObject(char *name, int open)                /* 414800 */
{
    WcPushOpen(open);
    WcDebug("WcBeginObject(objname = %s, opentoken = %s)\n", name, WcTokenName(open));
    curObject = new WcObject(name);
}

void WcEndObject(int close)                             /* 414860 */
{
    WcMatchClose(close);
    WcDebug("WcEndObject(closetoken = %s)\n", WcTokenName(close));
    if (!wcObjects)
        wcObjects = new LabList;
    wcObjects->Append(curObject);
    curObject = 0;
}

void WcBeginPoly(int open)                              /* 414910 */
{
    WcPushOpen(open);
    WcDebug("WcBeginPoly(opentoken = %s)\n", WcTokenName(open));
    curPoly = new WcPoly;
}

void WcEndPoly(int close)                               /* 414960 */
{
    WcMatchClose(close);
    WcDebug("WcEndPoly(closetoken = %s)\n", WcTokenName(close));
    int n = curPoly->GetVertexCount();
    if (n < 3 || n > 4)
        WcError("Polygons must have 3 or 4 vertices");
    if (curObject)
        curObject->AddPoly(curPoly);
    curPoly = 0;
}

void WcAddVertex(double x, double y, double z)          /* 4149d0 */
{
    WcDebug("WcAddVertex(x = %g, y = %g, z = %g)\n", x, y, z);
    if (curPoly->GetVertexCount() > 3) {
        WcError("Polygons may only have 3 or 4 vertices");
        return;     /* the original stored the vertex anyway, overrunning */
    }
    curPoly->AddVertex(x, y, z);
}

void WcSetColor(double r, double g, double b)           /* 414a50 */
{
    WcDebug("WcSetColor(r = %g, g = %g, b = %g)\n", r, g, b);
    WcObject *t = attributeTarget();
    if (t)
        t->SetColor(r, g, b);
}

void WcSetMaterial(double specular, double power)       /* 414ad0 */
{
    WcDebug("WcSetMaterial(specular-component = %g, power-component = %g)\n", specular, power);
    WcObject *t = attributeTarget();
    if (t)
        t->SetMaterial(specular, power);
}

void WcSetTexture(char *file, double ox, double oy, char *map)  /* 414b40 */
{
    WcDebug("WcSetTexture(txtname = %s, orientx = %g, orienty = %g, mapping = %s)\n",
            file, ox, oy, map);
    WcObject *t = attributeTarget();
    int type = WcMapFlat;
    if (map == 0)
        WcError("Internal error: null texture map type");
    else
        type = mapType(map);
    if (t)
        t->SetTexture(file, ox, oy, type);
}

void WcSetNormal(double x, double y, double z)          /* 414c80 */
{
    WcDebug("WcNormalColor(x = %g, y = %g, z = %g)\n", x, y, z);
    curPoly->SetNormal(x, y, z);
}

void WcSetShapeURL(char *url)                           /* 414ce0 */
{
    WcDebug("WcSetURL(shapeURL = %s)\n", url);
    curObject->SetShapeURL(url);
}

void WcSetObjectURL(char *url)                          /* 414d10 */
{
    WcDebug("WcSetURL(urlname = %s)\n", url);
    curObject->SetURL(url);
}

void WcBeginScene(char *name, int open)                 /* 414d40 */
{
    WcPushOpen(open);
    WcDebug("WcBeginScene(scenename = %s, opentoken = %s)\n", name, WcTokenName(open));
    curScene = new WcScene(name);
}

void WcEndScene(int close)                              /* 414da0 */
{
    WcMatchClose(close);
    WcDebug("WcEndScene(closetoken = %s)\n", WcTokenName(close));
    if (!wcScenes)
        wcScenes = new LabList;
    wcScenes->Append(curScene);
    if (wcWorld)
        wcWorld->AddScene(curScene);
    curScene = 0;
}

void WcBeginElement(int open)                           /* 414e80 */
{
    WcPushOpen(open);
    WcDebug("WcBeginElement(opentoken = %s)\n", WcTokenName(open));
    curElement = new WcElement;
}

void WcEndElement(int close)                            /* 414ed0 */
{
    WcMatchClose(close);
    WcDebug("WcEndElement(closetoken = %s)\n", WcTokenName(close));
    if (curElement->GetObject() == 0)
        WcError("Scene elements require an object");
    curScene->AddElement(curElement);
    curElement = 0;
}

void WcSetElementObject(char *name)                     /* 414f30 */
{
    WcDebug("WcSetElementObject(objname = %s)\n", name);
    curElement->SetObject(name);
}

void WcSetElementPosition(double x, double y, double z) /* 414f60 */
{
    WcDebug("WcSetElementPosition(x = %g, y = %g, z = %g)\n", x, y, z);
    curElement->SetPosition(x, y, z);
}

void WcSetElementOrientation(double dx, double dy, double dz,
                             double ux, double uy, double uz)   /* 414fc0 */
{
    WcDebug("WcSetElementOrientation(dx = %g, dy = %g, dz = %g, ux = %g, uy = %g, uz = %g\n",
            dx, dy, dz, ux, uy, uz);
    curElement->SetOrientation(dx, dy, dz, ux, uy, uz);
}

void WcSetElementURL(char *url)                         /* 415060 */
{
    WcDebug("WcSetElementURL(urlspec = \"%s\")\n", url);
    curElement->SetURL(url);
}

void WcBeginLight(int open)                             /* 415090 */
{
    WcPushOpen(open);
    WcDebug("WcBeginLight(opentoken = %s)\n", WcTokenName(open));
    curLight = new WcLight;
}

void WcEndLight(int close)                              /* 4150e0 */
{
    WcMatchClose(close);
    WcDebug("WcEndLight(closetoken = %s)\n", WcTokenName(close));
    if (curLight->GetObject() == 0)
        WcError("Scene lights require an object");
    curScene->AddElement(curLight);
    curLight = 0;
}

void WcSetLightObject(char *name, char *type,
                      double r, double g, double b)     /* 415140 */
{
    WcDebug("WcSetLightObject(objname = %s)\n", name);
    curLight->SetObject(name);

    /* RLLightType values */
    int t;
    if (strcmp(type, "Ambient") == 0) t = 0;
    else if (strcmp(type, "Directional") == 0) t = 3;
    else if (strcmp(type, "ParallelPoint") == 0) t = 4;
    else if (strcmp(type, "Point") == 0) t = 1;
    else if (strcmp(type, "Spot") == 0) t = 2;
    else {
        WcError("Illegal Light type.");
        t = 0;
    }
    curLight->SetLightType(t);
    curLight->SetColor(r, g, b);
}

void WcSetLightPosition(double x, double y, double z)   /* 4152d0 */
{
    WcDebug("WcSetLightPosition(x = %g, y = %g, z = %g)\n", x, y, z);
    curLight->SetPosition(x, y, z);
}

void WcSetLightOrientation(double dx, double dy, double dz,
                           double ux, double uy, double uz)     /* 415330 */
{
    WcDebug("WcSetLightOrientation(dx = %g, dy = %g, dz = %g, ux = %g, uy = %g, uz = %g\n",
            dx, dy, dz, ux, uy, uz);
    curLight->SetOrientation(dx, dy, dz, ux, uy, uz);
}

void WcSetCameraPosition(double x, double y, double z)  /* 4153d0 */
{
    wcCamera->SetPosition(x, y, z);
    WcDebug("WcSetCameraPosition(x = %g, y = %g, z = %g)\n", x, y, z);
}

void WcSetCameraOrientation(double dx, double dy, double dz,
                            double ux, double uy, double uz)    /* 415430 */
{
    wcCamera->SetOrientation(dx, dy, dz, ux, uy, uz);
    WcDebug("WcSetCameraOrientation(dx = %g, dy = %g, dz = %g, ux = %g, uy = %g, uz = %g\n",
            dx, dy, dz, ux, uy, uz);
}

/* The file gives the field as left right top bottom; it is stored as
 * left right bottom top (and written back the same way by Save Camera). */
void WcSetCameraField(double left, double right, double top, double bottom) /* 4154d0 */
{
    wcCamera->SetField(left, right, bottom, top);
    WcDebug("WcSetCameraOrientation(left = %g, right = %g, bottom = %g, top = %g\n",
            left, right, bottom, top);
}

/* ---- debugging dump of the compiled world (4158a0-415f70) ------------- */

static WcObject *findObject(char *name)
{
    for (WcObject *o = (WcObject *)wcObjects->First(); o; o = (WcObject *)LabList::Next(o))
        if (strcmp(o->GetName(), name) == 0)
            return o;
    return 0;
}

static void WcDumpPoly(WcPoly *p)                       /* 4159f0 */
{
    double r, g, b, nx, ny, nz, x, y, z;
    p->GetColor(&r, &g, &b);
    double spec = p->GetSpecular();
    double power = p->GetPower();
    p->GetNormal(&nx, &ny, &nz);
    WcDebug("\tPoly {\n");
    for (int i = 0; i < p->GetVertexCount(); i++) {
        p->GetVertex(i, &x, &y, &z);
        WcDebug("\t\t(%g, %g, %g)\n", x, y, z);
    }
    if (r != -1.0)
        WcDebug("\t\tColor: (%g, %g, %g)\n", r, g, b);
    if (spec != -1.0)
        WcDebug("\t\tMaterial: (%g, %g)\n", spec, power);
    if (nx != -1.0)
        WcDebug("\t\tNormal: (%g, %g, %g)\n", nx, ny, nz);
    if (p->GetTexture())
        WcDebug("\t\tTexture Map: (%s, %g, %g, %s)\n", p->GetTexture(),
                p->GetOrientX(), p->GetOrientX(), mapName(p->GetMapping()));
    WcDebug("\t\t}\n");
}

static void WcDumpObject(WcObject *o)                   /* 4158a0 */
{
    double r, g, b;
    o->GetColor(&r, &g, &b);
    WcDebug("Object \"%s\"\t[%x]{\n", o->GetName(), (unsigned)(size_t)o);
    WcDebug("\tColor: (%g, %g, %g)\n", r, g, b);
    WcDebug("\tMaterial: (%g, %g)\n", o->GetSpecular(), o->GetPower());
    WcDebug("\tTexture Map: (%s, %g, %g, %s)\n",
            o->GetTexture() ? o->GetTexture() : "(none)",
            o->GetOrientX(), o->GetOrientX(), mapName(o->GetMapping()));
    if (o->GetURL())
        WcDebug("\tURL spec: %s\n", o->GetURL());
    for (WcPoly *p = o->FirstPoly(); p; p = o->NextPoly(p))
        WcDumpPoly(p);
    WcDebug("\t}\n");
}

static void WcDumpPlacement(WcElement *e)
{
    double x, y, z, dx, dy, dz, ux, uy, uz;
    char *obj = e->GetObject();
    if (strstr(obj, ":/"))
        WcDebug("\t\tURL of Shape file %s\n", obj);
    else
        WcDebug("\t\tObject \"%s\"\t[%x]\n", obj, (unsigned)(size_t)findObject(obj));
    if (e->GetType() == WcElementObject && e->GetURL())
        WcDebug("\t\tURL \"%s\"\n", e->GetURL());
    e->GetPosition(&x, &y, &z);
    WcDebug("\t\tPosition (%g,%g,%g)\n", x, y, z);
    e->GetOrientation(&dx, &dy, &dz, &ux, &uy, &uz);
    WcDebug("\t\tOrientation (%g,%g,%g,%g,%g,%g)\n", dx, dy, dz, ux, uy, uz);
}

static void WcDumpElement(WcElement *e)                 /* 415c30 */
{
    WcDebug("\tElement {\n");
    WcDumpPlacement(e);
    WcDebug("\t\t}\n");
}

static void WcDumpLight(WcLight *l)                     /* 415dc0 */
{
    double r, g, b;
    WcDebug("\tLight {\n");
    WcDumpPlacement(l);
    l->GetColor(&r, &g, &b);
    WcDebug("\t\tColor %g, %g, %g\n", r, g, b);
    WcDebug("\t\t}\n");
}

static void WcDumpScene(WcScene *s)                     /* 415bc0 */
{
    WcDebug("Scene \"%s\"{\n", s->GetName());
    for (WcElement *e = s->FirstElement(); e; e = s->NextElement(e)) {
        if (e->GetType() == WcElementObject)
            WcDumpElement(e);
        else if (e->GetType() == WcElementLight)
            WcDumpLight((WcLight *)e);
    }
    WcDebug("\t}\n");
}

static void WcDump(void)                                /* 415810 */
{
    WcDebug("================ Objects ===================\n");
    if (wcObjects)
        for (WcObject *o = (WcObject *)wcObjects->First(); o; o = (WcObject *)LabList::Next(o))
            WcDumpObject(o);
    WcDebug("=====================Scenes =====================\n");
    if (wcScenes)
        for (WcScene *s = (WcScene *)wcScenes->First(); s; s = (WcScene *)LabList::Next(s))
            WcDumpScene(s);
}

/* Throw away the previous world's objects and scenes. */
static void WcReset(void)                               /* 414670 */
{
    if (wcObjects) {
        wcObjects->DeleteLinks();
        delete wcObjects;
    }
    if (wcScenes) {
        wcScenes->DeleteLinks();
        delete wcScenes;
    }
    wcObjects = 0;
    wcScenes = 0;
    curObject = 0;
    curPoly = 0;
    curScene = 0;
    curElement = 0;
    curLight = 0;
    openDepth = 0;
}

void WcCompile(const char *filename)                    /* 414710 */
{
    FILE *fp = fopen(filename, "r");
    if (fp == 0) {
        LabError("wc: can't open input file '%s'", filename);
        return;     /* the original carried on with a NULL file */
    }
    WcLexInit(fp);
    WcReset();
    WcParse();
    fclose(fp);
    WcDump();
}
