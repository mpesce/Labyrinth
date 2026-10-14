/*
 * wcmodel.h -- in-memory world model built by the Labyrinth world compiler
 * ("wc", Tony Parisi's proto-VRML scripting language).
 *
 * Reconstructed from LABYRINTH.EXE, code 0x4129b0-0x413440.  Every numeric
 * attribute is a double, exactly as in the original; "unset" colours,
 * materials and normals are marked with -1.0.
 */
#ifndef WCMODEL_H
#define WCMODEL_H

#include "lablist.h"

/* Texture mapping types (order of the keywords Flat, Cylinder, Sphere) */
enum { WcMapFlat = 0, WcMapCylinder = 1, WcMapSphere = 2 };

/* Element kinds */
enum { WcElementObject = 0, WcElementLight = 1 };

/* An object definition: a named collection of polygons, or a reference to an
 * external XOF shape file, plus default surface attributes. */
class WcObject : public LabLink {
public:
    WcObject(char *name);                                       /* 4129b0 */
    ~WcObject();                                                /* 412a00 */

    char *GetName() { return name; }                            /* 412a60 */
    void AddPoly(class WcPoly *p);                              /* 412a70 */
    class WcPoly *FirstPoly();                                  /* 412b10 */
    class WcPoly *NextPoly(class WcPoly *p);                    /* 412b20 */

    void SetColor(double r, double g, double b)                 /* 412b40 */
        { red = r; green = g; blue = b; }
    void GetColor(double *r, double *g, double *b)              /* 412b70 */
        { *r = red; *g = green; *b = blue; }
    void SetMaterial(double spec, double pow)                   /* 412ba0 */
        { specular = spec; power = pow; }
    double GetSpecular() { return specular; }                   /* 412bc0 */
    double GetPower() { return power; }                         /* 412bd0 */
    void SetTexture(char *file, double ox, double oy, int map)  /* 412be0 */
        { texture = file; orientX = ox; orientY = oy; mapping = map; }
    char *GetTexture() { return texture; }                      /* 412c10 */
    double GetOrientX() { return orientX; }                     /* 412c20 */
    int GetMapping() { return mapping; }                        /* 412c30 */

    /* The URL and Shapefile statements both land in the same field; the
     * world builder treats a non-empty value as the address of an XOF
     * shape to fetch. */
    void SetURL(char *u) { url = u; }                           /* 412c40 */
    char *GetURL() { return url; }                              /* 412c50 */
    void SetShapeURL(char *u) { url = u; }                      /* 412c60 */
    char *GetShapeURL() { return url; }                         /* 412c70 */

protected:
    char *name;             /* +08 */
    LabList *polys;         /* +0c */
    double red, green, blue;/* +10 */
    double specular, power; /* +28 */
    char *texture;          /* +38 */
    double orientX;         /* +40 */
    double orientY;         /* +48 */
    int mapping;            /* +50 */
    char *url;              /* +54 */
};

/* A polygon: three or four vertices and an optional face normal.  It derives
 * from WcObject so that a face can override colour, material and texture. */
class WcPoly : public WcObject {
public:
    WcPoly();                                                   /* 412c80 */

    void AddVertex(double x, double y, double z)                /* 412d30 */
    {
        vert[nverts][0] = x; vert[nverts][1] = y; vert[nverts][2] = z;
        nverts++;
    }
    void SetNormal(double x, double y, double z)                /* 412d80 */
        { normal[0] = x; normal[1] = y; normal[2] = z; }
    int GetVertexCount() { return nverts; }                     /* 412dc0 */
    void GetVertex(int i, double *x, double *y, double *z)      /* 412dd0 */
        { *x = vert[i][0]; *y = vert[i][1]; *z = vert[i][2]; }
    void GetNormal(double *x, double *y, double *z)             /* 412e10 */
        { *x = normal[0]; *y = normal[1]; *z = normal[2]; }

private:
    int nverts;             /* +60 */
    double vert[4][3];      /* +68 */
    double normal[3];       /* +c8 */
};

/* The camera block of a world (or the drawing window's current view). */
class WcCamera {
public:
    WcCamera();                                                 /* 412e60 */
    void SetPosition(double x, double y, double z)              /* 412ed0 */
        { pos[0] = x; pos[1] = y; pos[2] = z; }
    void GetPosition(double *x, double *y, double *z)           /* 412f00 */
        { *x = pos[0]; *y = pos[1]; *z = pos[2]; }
    void SetOrientation(double dx, double dy, double dz,
                        double ux, double uy, double uz)        /* 412f30 */
    {
        dir[0] = dx; dir[1] = dy; dir[2] = dz;
        up[0] = ux; up[1] = uy; up[2] = uz;
    }
    void GetOrientation(double *dx, double *dy, double *dz,
                        double *ux, double *uy, double *uz)     /* 412f90 */
    {
        *dx = dir[0]; *dy = dir[1]; *dz = dir[2];
        *ux = up[0]; *uy = up[1]; *uz = up[2];
    }
    void SetField(double l, double r, double b, double t)       /* 412ff0 */
        { left = l; right = r; bottom = b; top = t; }
    void GetField(double *l, double *r, double *b, double *t)   /* 413030 */
        { *l = left; *r = right; *b = bottom; *t = top; }

private:
    double pos[3];          /* +00 */
    double dir[3];          /* +18 */
    double up[3];           /* +30 */
    double left, right, bottom, top; /* +48 */
};

class WcElement;

/* A scene: a named list of elements (object instances and lights). */
class WcScene : public WcObject {
public:
    WcScene(char *name);                                        /* 413070 */
    void AddElement(WcElement *e);                              /* 4130d0 */
    WcElement *FirstElement();                                  /* 413160 */
    WcElement *NextElement(WcElement *e);                       /* 413170 */
private:
    LabList *elements;      /* +60 */
};

/* The world: a named list of scenes.
 *
 * In the original, WcWorld::AddScene appended the scene's own (single)
 * LabLink to the world's list although the same link was already on the
 * compiler's global scene list.  With two or more scenes that makes a link
 * point at itself and the end-of-compile dump loops forever.  Here the
 * world's list holds separate reference nodes instead. */
class WcSceneRef : public LabLink {
public:
    WcSceneRef(class WcScene *s) : scene(s) {}
    class WcScene *scene;
};

class WcWorld : public LabList {
public:
    WcWorld(char *name);                                        /* 413190 */
    void AddScene(WcScene *s);                                  /* 4131f0 */
private:
    LabList *scenes;        /* +08 */
    char *name;             /* +0c */
};

/* An instance of an object (or a light) placed in a scene. */
class WcElement : public LabLink {
public:
    WcElement();                                                /* 413290 */

    int GetType() { return type; }
    void SetObject(char *o) { object = o; }                     /* 4132e0 */
    char *GetObject() { return object; }                        /* 4132f0 */
    void SetURL(char *u) { url = u; }                           /* 413300 */
    char *GetURL() { return url; }                              /* 413310 */
    void SetPosition(double x, double y, double z)              /* 413320 */
        { pos[0] = x; pos[1] = y; pos[2] = z; }
    void GetPosition(double *x, double *y, double *z)           /* 413350 */
        { *x = pos[0]; *y = pos[1]; *z = pos[2]; }
    void SetOrientation(double dx, double dy, double dz,
                        double ux, double uy, double uz)        /* 413380 */
    {
        dir[0] = dx; dir[1] = dy; dir[2] = dz;
        up[0] = ux; up[1] = uy; up[2] = uz;
    }
    void GetOrientation(double *dx, double *dy, double *dz,
                        double *ux, double *uy, double *uz)     /* 4133e0 */
    {
        *dx = dir[0]; *dy = dir[1]; *dz = dir[2];
        *ux = up[0]; *uy = up[1]; *uz = up[2];
    }

protected:
    int type;               /* +08 */
    char *object;           /* +0c */
    char *url;              /* +10 */
    double pos[3];          /* +18 */
    double dir[3];          /* +30 */
    double up[3];           /* +48 */
};

/* A light element: adds a light type and colour. */
class WcLight : public WcElement {
public:
    WcLight();                                                  /* 413440 */
    void SetLightType(int t) { lightType = t; }
    int GetLightType() { return lightType; }
    void SetColor(double r, double g, double b)
        { red = r; green = g; blue = b; }
    void GetColor(double *r, double *g, double *b)
        { *r = red; *g = green; *b = blue; }
private:
    int lightType;          /* +60 */
    double red, green, blue;/* +68 */
};

#endif /* WCMODEL_H */
