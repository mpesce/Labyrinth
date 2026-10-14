/*
 * labwww.cpp -- fetching documents from the Web (see labwww.h).
 *
 * Replaces the CERN libwww calls of the original (LabInitWWW at 0x40dca0,
 * the fetch routines at 0x40ddc0-0x40ded0, the WebObject stream of
 * VIEWER\HTWOP.C at 0x40df60-0x40e110) and the browser launcher (0x411890).
 */
#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "labwww.h"

extern "C" void LabError(const char *fmt, ...);
extern "C" const char *LabIniFile(void);

static char worldPath[MAX_PATH];
static char texturePath[MAX_PATH];
static char shapePath[MAX_PATH];
char *LAB_WORLD_FILE = worldPath;
char *LAB_TEXTURE_FILE = texturePath;
char *LAB_SHAPE_FILE = shapePath;

static HINTERNET inet;
static char baseURL[INTERNET_MAX_URL_LENGTH];

/* The address resolved by the most recent fetch */
static char lastURL[INTERNET_MAX_URL_LENGTH];

static void fileURL(const char *path, char *url)
{
    char full[MAX_PATH];
    DWORD len = INTERNET_MAX_URL_LENGTH;
    if (!GetFullPathName(path, MAX_PATH, full, NULL))
        strcpy(full, path);
    if (UrlCreateFromPath(full, url, &len, 0) != S_OK)
        strcpy(url, full);
}

void LabInitWWW(void)                                           /* 40dca0 */
{
    char dir[MAX_PATH];
    char exe[MAX_PATH];

    /* local copies go in %TEMP%\Labyrinth */
    GetTempPath(MAX_PATH, dir);
    strcat(dir, "Labyrinth\\");
    CreateDirectory(dir, NULL);
    sprintf(worldPath, "%slabyrnth.wrl", dir);
    sprintf(texturePath, "%stexture.ppm", dir);
    sprintf(shapePath, "%sshape.xof", dir);

    /* relative names start out relative to the program's own directory */
    GetModuleFileName(NULL, exe, MAX_PATH);
    fileURL(exe, baseURL);

    inet = InternetOpen("Labyrinth/2.0a2", INTERNET_OPEN_TYPE_PRECONFIG,
                        NULL, NULL, 0);
}

void LabSetBaseFile(const char *path)
{
    fileURL(path, baseURL);
}

/* Is this an absolute URL (scheme:...)?  A drive letter is not a scheme. */
static int hasScheme(const char *s)
{
    const char *p = s;
    while (*p && (isalnum((unsigned char)*p) || *p == '+' || *p == '-' || *p == '.'))
        p++;
    return *p == ':' && p - s > 1;
}

static int resolve(const char *url, char *out)
{
    DWORD len = INTERNET_MAX_URL_LENGTH;
    if (hasScheme(url)) {
        strncpy(out, url, INTERNET_MAX_URL_LENGTH - 1);
        out[INTERNET_MAX_URL_LENGTH - 1] = '\0';
        return 1;
    }
    if (url[0] && url[1] == ':') {          /* C:\... */
        fileURL(url, out);
        return 1;
    }
    return UrlCombine(baseURL, url, out, &len, 0) == S_OK;
}

static int copyLocal(const char *url, const char *dest)
{
    char path[MAX_PATH];
    DWORD len = MAX_PATH;
    if (PathCreateFromUrl(url, path, &len, 0) != S_OK)
        return 0;
    if (lstrcmpi(path, dest) == 0)
        return 1;
    return CopyFile(path, dest, FALSE);
}

static int download(const char *url, const char *dest)
{
    if (!inet)
        return 0;
    HINTERNET h = InternetOpenUrl(inet, url, NULL, 0,
                                  INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_UI, 0);
    if (!h)
        return 0;

    DWORD status = 200, size = sizeof(status);
    if (strncmp(url, "http", 4) == 0 &&
        HttpQueryInfo(h, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                      &status, &size, NULL) && status >= 400) {
        InternetCloseHandle(h);
        return 0;
    }

    /* after redirects, relative references are relative to where we ended */
    DWORD ulen = sizeof(lastURL);
    InternetQueryOption(h, INTERNET_OPTION_URL, lastURL, &ulen);

    FILE *fp = fopen(dest, "wb");
    if (!fp) {
        InternetCloseHandle(h);
        return 0;
    }
    char buf[16384];
    DWORD n;
    int ok = 1;
    while (InternetReadFile(h, buf, sizeof(buf), &n)) {
        if (n == 0)
            break;
        if (fwrite(buf, 1, n, fp) != n) {
            ok = 0;
            break;
        }
    }
    fclose(fp);
    InternetCloseHandle(h);
    return ok;
}

static int fetch(const char *url, const char *dest)
{
    char abs[INTERNET_MAX_URL_LENGTH];
    if (!resolve(url, abs))
        return 0;
    strcpy(lastURL, abs);
    if (_strnicmp(abs, "file:", 5) == 0)
        return copyLocal(abs, dest);
    return download(abs, dest);
}

int LabFetchWorld(const char *url)                              /* 40ddc0 */
{
    if (!fetch(url, LAB_WORLD_FILE)) {
        /* leave an empty world rather than the previous one */
        FILE *fp = fopen(LAB_WORLD_FILE, "wb");
        if (fp)
            fclose(fp);
        LabError("Unable to access document.\n%s", url);
        return 0;
    }
    strcpy(baseURL, lastURL);
    return 1;
}

int LabFetchTexture(const char *url)                            /* 40de40 */
{
    if (!fetch(url, LAB_TEXTURE_FILE)) {
        fprintf(stderr, "Couldn't fetch texture map %s\n", url);
        return 0;
    }
    return 1;
}

int LabFetchShape(const char *url)                              /* 40ded0 */
{
    if (!fetch(url, LAB_SHAPE_FILE)) {
        fprintf(stderr, "Couldn't fetch shape %s\n", url);
        return 0;
    }
    return 1;
}

const char *LabFetchFile(const char *url)
{
    static char path[MAX_PATH];
    static int serial;
    char dir[MAX_PATH];
    strcpy(dir, worldPath);
    char *slash = strrchr(dir, '\\');
    if (slash)
        slash[1] = '\0';
    sprintf(path, "%sfetch%04d.tmp", dir, serial++ % 10000);
    if (!fetch(url, path))
        return NULL;
    return path;
}

int LabResolveURL(const char *url, char *out, int outlen)
{
    char abs[INTERNET_MAX_URL_LENGTH];
    if (!resolve(url, abs))
        return 0;
    strncpy(out, abs, outlen - 1);
    out[outlen - 1] = '\0';
    return 1;
}

const char *LabGetBaseURL(void)
{
    return baseURL;
}

/* The address the last fetch actually came from (after redirects). */
const char *LabLastFetchedURL(void)
{
    return lastURL;
}

void LabSetBaseURL(const char *url)
{
    strncpy(baseURL, url, sizeof(baseURL) - 1);
    baseURL[sizeof(baseURL) - 1] = '\0';
}

/*
 * The original read BROWSER from the [Main] section of LAB.INI (default
 * MOSAIC.EXE) and ran "<browser> <url>".  That is still honoured when
 * BROWSER is set; otherwise the system's default browser is used.
 */
void LabLaunchBrowser(const char *url)                          /* 411890 */
{
    char browser[200];
    char abs[INTERNET_MAX_URL_LENGTH];

    if (!resolve(url, abs))
        strcpy(abs, url);
    GetPrivateProfileString("Main", "BROWSER", "", browser, sizeof(browser), LabIniFile());
    if (browser[0] && _stricmp(browser, "MOSAIC.EXE") != 0) {
        char *cmd = (char *)malloc(strlen(browser) + strlen(abs) + 4);
        sprintf(cmd, "%s %s", browser, abs);
        WinExec(cmd, SW_SHOW);
        free(cmd);
    } else {
        ShellExecute(NULL, "open", abs, NULL, NULL, SW_SHOWNORMAL);
    }
}
