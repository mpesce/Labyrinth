/*
 * rl.h -- Reality Lab API, as used by Labyrinth.
 *
 * Labyrinth 2.0a2 statically linked RenderMorphics' Reality Lab (fixed point,
 * 8-bit palette only).  That library is not available, so this header
 * re-declares exactly the subset of the API that Labyrinth calls (65 entry
 * points, recovered from the 1995 binary), and rlcore.cpp and rlrender.cpp
 * re-implement it on top of OpenGL.
 *
 * Calling conventions follow the original: every RLValue argument is a
 * double, vectors handed back to the caller are float triples, objects are
 * opaque handles, and functions return RLError (0 == RLNoError) unless they
 * create something, in which case they return the new handle (NULL on error).
 *
 * The comment after each declaration gives the address of the original
 * routine in LABYRINTH.EXE.
 */
#ifndef RL_H
#define RL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef double RLValue;
typedef int RLError;
typedef unsigned long RLColour;          /* 0xFFrrggbb */

typedef struct RLObjectRec *RLObject;
typedef RLObject RLFrame, RLMesh, RLFace, RLLight, RLDevice, RLViewport,
                 RLTexture, RLMaterial, RLWrap, RLVisual;

typedef struct { float x, y, z; } RLVector;
typedef struct { RLVector min, max; } RLBox;

/* Error codes (the order of the strings in the original error table) */
enum {
    RLNoError = 0,
    RLBadObject,            /* "object expected in argument" */
    RLBadType,              /* "bad argument type passed" */
    RLBadAlloc,             /* "out of memory" */
    RLFaceUsed,             /* "face already used in a mesh" */
    RLNotFound,             /* "object not found in specified place" */
    RLNotDoneYet,           /* "unimplemented" */
    RLFileNotFound,         /* "file cannot be opened" */
    RLBadFile,              /* "data file is corrupt" */
    RLBadDevice,            /* "device is not compatible with renderer" */
    RLBadValue              /* "bad argument value" */
};

/* Light types */
typedef enum {
    RLLightAmbient,
    RLLightPoint,
    RLLightSpot,
    RLLightDirectional,
    RLLightParallelPoint
} RLLightType;

/* Render qualities used by the View menu */
typedef enum {
    RLRenderWireframe = 0,
    RLRenderUnlitFlat = 1,
    RLRenderFlat = 2,
    RLRenderGouraud = 3,
    RLRenderPhong = 4
} RLRenderQuality;

/* Combine types for RLFrameAddRotation */
typedef enum {
    RLCombineReplace,
    RLCombineBefore,
    RLCombineAfter
} RLCombineType;

/* Texture wrap types */
typedef enum {
    RLWrapFlat,
    RLWrapCylinder,
    RLWrapSphere,
    RLWrapChrome
} RLWrapType;

/* The image structure handed to RLCreateTexture (same layout as the later
 * D3DRMIMAGE).  For 8-bit images 'palette' points at palette_size entries of
 * {red, green, blue, flags}. */
typedef struct {
    unsigned char red, green, blue, flags;
} RLPaletteEntry;

typedef struct {
    int width, height;
    int aspectx, aspecty;
    int depth;              /* bits per pixel: 8 or 32 */
    int rgb;                /* nonzero: direct colour, zero: palettised */
    int bytes_per_line;
    void *buffer1;
    void *buffer2;
    unsigned long red_mask, green_mask, blue_mask, alpha_mask;
    int palette_size;
    RLPaletteEntry *palette;
} RLImage;

/* ---- general ---------------------------------------------------------- */
void      RLTick(void);                                             /* 4245e0 */
RLError   RLObjectDestroy(RLObject);                                /* 426560 */
RLError   RLObjectSetAppData(RLObject, void *data);                 /* 426f10 */
void     *RLObjectGetAppData(RLObject);                             /* 42ed40 */
RLError   RLMalloc(void **p, int size);                             /* 426a10 */
void      RLFree(void *p);                                          /* 426af0 */
RLColour  RLCreateColourRGB(RLValue r, RLValue g, RLValue b);       /* 42f930 */
RLError   RLGetLastError(void);                                     /* 42f880 */

/* ---- vectors ---------------------------------------------------------- */
RLVector *RLVectorNormalise(RLVector *v);                           /* 429ec0 */
void      RLVectorCrossProduct(RLVector *d, RLVector *a, RLVector *b); /* 42b360 */

/* ---- frames ----------------------------------------------------------- */
RLFrame   RLCreateFrame(RLFrame parent);                            /* 425a60 */
RLError   RLFrameSetPosition(RLFrame, RLFrame ref,
                             RLValue x, RLValue y, RLValue z);      /* 425b20 */
RLError   RLFrameSetOrientation(RLFrame, RLFrame ref,
                                RLValue dx, RLValue dy, RLValue dz,
                                RLValue ux, RLValue uy, RLValue uz);/* 425c50 */
RLError   RLFrameAddRotation(RLFrame, RLCombineType,
                             RLValue x, RLValue y, RLValue z,
                             RLValue theta);                        /* 425ea0 */
RLError   RLFrameGetPosition(RLFrame, RLFrame ref, RLVector *pos);  /* 426050 */
RLError   RLFrameGetOrientation(RLFrame, RLFrame ref,
                                RLVector *dir, RLVector *up);       /* 425f40 */
RLError   RLFrameDeleteChild(RLFrame, RLFrame child);               /* 426140 */
RLError   RLFrameAddVisual(RLFrame, RLVisual);                      /* 4261c0 */
RLError   RLFrameDeleteVisual(RLFrame, RLVisual);                   /* 426240 */
RLError   RLFrameAddLight(RLFrame, RLLight);                        /* 4262f0 */
RLError   RLFrameGetChildren(RLFrame, int *count, RLFrame **list);  /* 4263f0 */
RLError   RLFrameGetVisuals(RLFrame, int *count, RLVisual **list);  /* 4264a0 */

/* ---- lights, materials, textures, wraps --------------------------------- */
RLLight   RLCreateLightRGB(RLLightType, RLValue r, RLValue g, RLValue b); /* 426950 */
RLMaterial RLCreateMaterial(RLValue specular, RLValue power);       /* 42ba20 */
RLTexture RLCreateTexture(RLImage *image);                          /* 42d370 */
RLTexture RLLoadTexture(const char *ppmfile);                       /* 42b3f0 */
RLError   RLTextureSetColours(RLTexture, int colours);              /* 42d3e0 */
RLWrap    RLCreateWrap(RLWrapType, RLFrame ref,
                       RLValue ox, RLValue oy, RLValue oz,
                       RLValue dx, RLValue dy, RLValue dz,
                       RLValue ux, RLValue uy, RLValue uz,
                       RLValue ou, RLValue ov,
                       RLValue su, RLValue sv);                     /* 42d460 */
RLError   RLWrapApply(RLWrap, RLObject meshOrFace);                 /* 42d8c0 */

/* ---- meshes and faces --------------------------------------------------- */
RLMesh    RLCreateMesh(void);                                       /* 428ba0 */
RLMesh    RLLoadMesh(const char *xoffile);                          /* 42e3c0 */
int       RLMeshAddVertex(RLMesh, RLValue x, RLValue y, RLValue z); /* 429330 */
int       RLMeshAddNormal(RLMesh, RLValue x, RLValue y, RLValue z); /* 429400 */
RLFace    RLMeshCreateFace(RLMesh);                                 /* 4292a0 */
RLError   RLMeshAddFaces(RLMesh, int vcount, RLVector *vertices,
                         int ncount, RLVector *normals, int *data,
                         int *fcount, RLFace **faces);              /* 428330 */
RLError   RLMeshSetTextureCoordinates(RLMesh, int index,
                                      RLValue u, RLValue v);        /* 429210 */
RLError   RLMeshSetColour(RLMesh, RLColour);                        /* 429970 */
RLError   RLMeshSetColourRGB(RLMesh, RLValue r, RLValue g, RLValue b); /* 4299d0 */
RLError   RLMeshSetTexture(RLMesh, RLTexture);                      /* 429a10 */
RLError   RLMeshSetMaterial(RLMesh, RLMaterial);                    /* 429ae0 */
RLError   RLMeshGetBox(RLMesh, RLBox *box);                         /* 429bd0 */
RLError   RLMeshGetFaces(RLMesh, int *count, RLFace **faces);       /* 429cb0 */
RLError   RLFaceAddVertexAndNormalIndexed(RLFace, int vertex, int normal); /* 4272a0 */
RLError   RLFaceSetColour(RLFace, RLColour);                        /* 4273d0 */
RLError   RLFaceSetColourRGB(RLFace, RLValue r, RLValue g, RLValue b); /* 427420 */
RLError   RLFaceSetTexture(RLFace, RLTexture);                      /* 427460 */

/* ---- devices and viewports ---------------------------------------------- */
RLDevice  RLWinCreateDevice(void *hwnd, int width, int height);     /* 424cf0 */
RLError   RLWinHandlePaint(RLDevice, void *hdc);                    /* 424dd0 */
RLError   RLWinHandleActivate(RLDevice, int active);                /* 424ed0 */
RLError   RLDeviceSetQuality(RLDevice, RLRenderQuality);            /* 424f40 */
RLRenderQuality RLDeviceGetQuality(RLDevice);                       /* 42ec90 */
RLError   RLDeviceSetDither(RLDevice, int dither);                  /* 424f80 */
int       RLDeviceGetDither(RLDevice);                              /* 424fe0 */
RLError   RLDeviceSetShades(RLDevice, int shades);                  /* 42eb90 */
int       RLDeviceGetShades(RLDevice);                              /* 42ec10 */
RLError   RLDeviceSetColours(RLDevice, int colours);                /* 425300 */
int       RLDeviceGetColours(RLDevice);                             /* 42ec50 */
int       RLDeviceGetWidth(RLDevice);                               /* 425280 */
int       RLDeviceGetHeight(RLDevice);                              /* 4252c0 */

RLViewport RLCreateViewport(RLDevice, RLFrame camera,
                            int x, int y, int width, int height);   /* 425080 */
RLError   RLViewportSetBack(RLViewport, RLValue back);              /* 425020 */
RLError   RLViewportSetPlane(RLViewport, RLValue left, RLValue right,
                             RLValue bottom, RLValue top);          /* 42eb20 */
RLError   RLViewportGetPlane(RLViewport, RLValue *left, RLValue *right,
                             RLValue *bottom, RLValue *top);        /* 42ecd0 */
RLError   RLViewportForceUpdate(RLViewport, int x1, int y1,
                                int x2, int y2);                    /* 42ea10 */
RLError   RLViewportPick(RLViewport, RLVisual *visual, RLFrame *frame,
                         int x, int y);                             /* 42ed70 */
/* world -> screen: dst is a homogeneous (x, y, z, w) quadruple */
RLError   RLViewportTransform(RLViewport, float dst[4], RLVector *src);   /* 42f810 */
/* screen -> world: src is a homogeneous (x, y, z, w) quadruple */
RLError   RLViewportInverseTransform(RLViewport, RLVector *dst, float src[4]); /* 42f7a0 */

/* ---- extensions (not in the 1995 library) ------------------------------- */

/* Multisample antialiasing: samples per pixel; 0 or 1 turns it off. */
void      RLSetAntialias(int samples);
/* Clamp texture coordinates to the edge instead of repeating the picture. */
RLError   RLTextureSetClamp(RLTexture, int clampS, int clampT);
/* Draw the back faces of polygons too (Reality Lab culled them). */
void      RLSetTwoSided(int on);
/* ... for one mesh only (VRML shapes of unknown vertex ordering) */
RLError   RLMeshSetTwoSided(RLMesh, int on);
/* Remove a light from a frame (as Direct3D RM's DeleteLight). */
RLError   RLFrameDeleteLight(RLFrame, RLLight);
/* A material that glows (VRML emissiveColor); specular may be zero here. */
RLMaterial RLCreateMaterialEmissive(RLValue specular, RLValue power,
                                    RLValue r, RLValue g, RLValue b);
/* Faces with two vertices are drawn as lines and faces with one vertex as
 * points (for VRML line and point sets); they cannot be picked. */

#ifdef __cplusplus
}
#endif

#endif /* RL_H */
