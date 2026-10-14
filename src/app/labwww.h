/*
 * labwww.h -- fetching documents from the Web.
 *
 * Labyrinth 2.0a2 linked the CERN WWW library (libwww 2.1x, early 1994) and
 * used it for one thing: copying the document at a URL into a local file
 * (labyrnth.wrl, texture.ppm or shape.xof) through a file-writing stream
 * (VIEWER\HTWOP.C, "WebObject").  This module keeps that interface and does
 * the transfer with WinINet, which adds HTTPS and redirects.
 *
 * Differences from the original:
 *  - Relative URLs are resolved against the document currently being viewed
 *    (initially the directory Labyrinth runs from); the original always
 *    resolved them against http://www.butterfly.net/lab/ushm.wrl.
 *  - The local copies live in %TEMP%\Labyrinth rather than the current
 *    directory.
 *  - A failed transfer is reported instead of silently leaving an empty file.
 */
#ifndef LABWWW_H
#define LABWWW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Local copies of fetched documents */
extern char *LAB_WORLD_FILE;            /* "labyrnth.wrl" */
extern char *LAB_TEXTURE_FILE;          /* "texture.ppm" */
extern char *LAB_SHAPE_FILE;            /* "shape.xof" */

void LabInitWWW(void);                                          /* 40dca0 */
int LabFetchWorld(const char *url);                             /* 40ddc0 */
int LabFetchTexture(const char *url);                           /* 40de40 */
int LabFetchShape(const char *url);                             /* 40ded0 */

/* Note that a local world file is being viewed, so that relative references
 * inside it resolve against it. */
void LabSetBaseFile(const char *path);

/* Hand a URL to the external WWW browser (411890). */
void LabLaunchBrowser(const char *url);

/* For the VRML builder (new): fetch a document into a cache file of its own
 * and return that file's name (NULL on failure); resolve a URL against the
 * current base; read or replace the base. */
const char *LabFetchFile(const char *url);
int LabResolveURL(const char *url, char *out, int outlen);
const char *LabGetBaseURL(void);
void LabSetBaseURL(const char *url);
const char *LabLastFetchedURL(void);

#ifdef __cplusplus
}
#endif

#endif /* LABWWW_H */
