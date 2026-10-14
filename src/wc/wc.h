/*
 * wc.h -- the Labyrinth world compiler ("wc").
 *
 * wc reads a world written in Labyrinth's scripting language (the proto-VRML
 * designed by Tony Parisi) and builds the in-memory model of wcmodel.h.  It
 * was generated with pcyacc in 1994-95; the grammar has been recovered from
 * the parser tables in LABYRINTH.EXE and lives in wc.y.
 */
#ifndef WC_H
#define WC_H

#include <stdio.h>
#include "../app/wcmodel.h"

/* Token values (the order of the original %token declarations; 258 and 259
 * were declared but never used by the grammar). */
enum {
    WC_WORLD = 257, WC_RESERVED_258, WC_RESERVED_259,
    WC_OBJECT, WC_POLY, WC_VERTEX, WC_COLOR, WC_MATERIAL, WC_TEXTURE,
    WC_SHAPEFILE, WC_CAMERA, WC_FIELD, WC_NORMAL, WC_URL, WC_SCENE,
    WC_ELEMENT, WC_LIGHT, WC_POSITION, WC_ORIENTATION, WC_OPEN, WC_CLOSE,
    WC_BEGIN, WC_END, WC_IDENTIFIER, WC_INTEGER, WC_DOUBLE, WC_STRING
};

/* The parser value stack holds doubles, ints and strings. */
typedef union {
    double dval;
    int ival;
    char *sval;
} WcValue;

/* Results of the most recent compilation */
extern LabList *wcObjects;          /* WcObject list (4b7d0c) */
extern LabList *wcScenes;           /* WcScene list (4b7d10) */
extern WcWorld *wcWorld;            /* 4b7cf4 */
extern WcCamera *wcCamera;          /* created by the drawing window (4b7d14) */

void WcCompile(const char *filename);                           /* 414710 */

/* lexer (wclex.cpp) */
void WcLexInit(FILE *fp);                                       /* 413590 */
int WcLex(void);                                                /* 4135d0 */
int WcLineNumber(void);                                         /* 413de0 */
extern WcValue wclval;                                          /* 4a6b70 */

/* support used by the generated parser (wcact.cpp) */
int WcNextToken(void);                                          /* 414620 */
void WcError(const char *fmt, ...);                             /* 414640 */
const char *WcTokenName(int token);                             /* 415540 */
void WcDebug(const char *fmt, ...);                             /* 414610 */

/* grammar actions (wcact.cpp) */
void WcBeginWorld(char *name, int open);                        /* 414760 */
void WcEndWorld(int close);                                     /* 4147c0 */
void WcBeginObject(char *name, int open);                       /* 414800 */
void WcEndObject(int close);                                    /* 414860 */
void WcBeginPoly(int open);                                     /* 414910 */
void WcEndPoly(int close);                                      /* 414960 */
void WcAddVertex(double x, double y, double z);                 /* 4149d0 */
void WcSetColor(double r, double g, double b);                  /* 414a50 */
void WcSetMaterial(double specular, double power);              /* 414ad0 */
void WcSetTexture(char *file, double ox, double oy, char *map); /* 414b40 */
void WcSetNormal(double x, double y, double z);                 /* 414c80 */
void WcSetShapeURL(char *url);                                  /* 414ce0 */
void WcSetObjectURL(char *url);                                 /* 414d10 */
void WcBeginScene(char *name, int open);                        /* 414d40 */
void WcEndScene(int close);                                     /* 414da0 */
void WcBeginElement(int open);                                  /* 414e80 */
void WcEndElement(int close);                                   /* 414ed0 */
void WcSetElementObject(char *name);                            /* 414f30 */
void WcSetElementPosition(double x, double y, double z);        /* 414f60 */
void WcSetElementOrientation(double dx, double dy, double dz,
                             double ux, double uy, double uz);  /* 414fc0 */
void WcSetElementURL(char *url);                                /* 415060 */
void WcBeginLight(int open);                                    /* 415090 */
void WcEndLight(int close);                                     /* 4150e0 */
void WcSetLightObject(char *name, char *type,
                      double r, double g, double b);            /* 415140 */
void WcSetLightPosition(double x, double y, double z);          /* 4152d0 */
void WcSetLightOrientation(double dx, double dy, double dz,
                           double ux, double uy, double uz);    /* 415330 */
void WcSetCameraPosition(double x, double y, double z);         /* 4153d0 */
void WcSetCameraOrientation(double dx, double dy, double dz,
                            double ux, double uy, double uz);   /* 415430 */
void WcSetCameraField(double left, double right,
                      double top, double bottom);               /* 4154d0 */

int WcParse(void);                                              /* 413df0 */

#endif /* WC_H */
