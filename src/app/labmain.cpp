/*
 * labmain.cpp -- program entry, start-up and error reporting.
 * Reconstructed from LABYRINTH.EXE: 0x401000-0x4011a0 (main.obj),
 * 0x412890-0x412910 (error helpers) and WinMain at 0x424590.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "labwin.h"
#include "mainwin.h"
#include "drawwin.h"
#include "labwww.h"
#include "../rl/rl.h"

LabApp *labApp;                                                 /* 4b3000 */
LabMainWindow *labMainWindow;                                   /* 4b3008 */
FILE *labTraceFile;                                             /* 493000 */

static char errorBuf[1024];                                     /* 4a5d10 */
static char logBuf[1024];                                       /* 4a5910 */

/*
 * (new) the caption was "PRERELEASE VERSION - DO NOT REDISTRIBUTE!", and the
 * text "Copyright (c) 1995, The Community Company." followed by "Portions
 * Copyright (c) 1993 - 1995, RenderMorphics, Ltd.", whose Reality Lab is no
 * longer used.
 */
static void ShowPrereleaseNotice(void)                          /* 401000 */
{
    MessageBox(NULL,
               "Copyright (c) 2026, Mark Pesce and Tony Parisi",
               "PRERELEASE VERSION",
               MB_TASKMODAL);
}

/* Report an error on stderr and in a message box. */
extern "C" void LabError(const char *fmt, ...)                  /* 412890 */
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(errorBuf, fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s\n", errorBuf);
    labApp->Alert(labMainWindow, errorBuf);
}

/* Write a message to stdout. */
extern "C" void LabLog(const char *fmt, ...)                    /* 4128e0 */
{
    va_list ap;
    va_start(ap, fmt);
    vsprintf(logBuf, fmt, ap);
    va_end(ap);
    fprintf(stdout, "%s", logBuf);
}

/*
 * LAB.INI: the original passed the bare name to GetPrivateProfileString,
 * which means the Windows directory.  Prefer a LAB.INI beside the program.
 */
extern "C" const char *LabIniFile(void)
{
    static char path[MAX_PATH];
    if (!path[0]) {
        GetModuleFileName(NULL, path, MAX_PATH);
        char *slash = strrchr(path, '\\');
        strcpy(slash ? slash + 1 : path, "LAB.INI");
        if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
            strcpy(path, "LAB.INI");
    }
    return path;
}

extern "C" char *LabStrdup(const char *s)                       /* 412910 */
{
    char *p = (char *)malloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

static void LabMain(char *cmdline)                              /* 401020 */
{
    char home[100];
    char *file;

    /* (skipped when running under the rendering test hook, see rlrender.cpp) */
    if (!GetEnvironmentVariable("LABYRINTH_SNAPSHOT", home, sizeof(home)))
        ShowPrereleaseNotice();
    GetPrivateProfileString("Main", "TwoSided", "no", home, sizeof(home), LabIniFile());
    RLSetTwoSided(_stricmp(home, "yes") == 0);
    /* (new) multisample antialiasing, samples per pixel */
    RLSetAntialias(GetPrivateProfileInt("Main", "Antialias", 4, LabIniFile()));
    labApp = new LabApp;
    /* (new) twice the original's 400 x 300, which was sized for a 640 x 480
     * screen; the window can be resized */
    labMainWindow = new LabMainWindow(labApp, "Labyrinth VRML WWW Browser",
                                      100, 100, 800, 600);
    {
        /* (nothing is written to it) the original made it in the current
         * directory; keep it with the other scratch files instead */
        char trace[MAX_PATH];
        GetTempPath(MAX_PATH, trace);
        strcat(trace, "Labyrinth\\");
        CreateDirectory(trace, NULL);
        strcat(trace, "lab.err");
        labTraceFile = fopen(trace, "w");
    }
    labMainWindow->Show();

    /* a path with spaces arrives in quotes */
    if (cmdline[0] == '"') {
        cmdline++;
        char *q = strchr(cmdline, '"');
        if (q)
            *q = '\0';
    }

    LabDrawingWindow *view = labMainWindow->GetView();
    if (strlen(cmdline) < 5) {
        GetPrivateProfileString("Main", "Autoload Home", "no", home, sizeof(home), LabIniFile());
        if (strcmp(home, "yes") != 0)
            goto loop;
        GetPrivateProfileString("Main", "Home", "home.wrl", home, sizeof(home), LabIniFile());
        view->SetCursor(LabCursorWait);
        LabFetchWorld(home);
        file = LAB_WORLD_FILE;
    } else if (strstr(cmdline, "://")) {
        /* (new) a URL on the command line */
        view->SetCursor(LabCursorWait);
        LabFetchWorld(cmdline);
        file = LAB_WORLD_FILE;
    } else {
        view->SetCursor(LabCursorWait);
        LabSetBaseFile(cmdline);
        file = cmdline;
    }
    view->LoadWorld(file);
    view->SetCursor(LabCursorArrow);

loop:
    /* Render continuously; handle one message whenever one is waiting. */
    for (;;) {
        do {
            RLTick();
            labMainWindow->Idle();
        } while (!labApp->MessagePending());
        labApp->DispatchOne();
    }
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show) /* 424590 */
{
    labInstance = inst;
    LabMain(cmdline);
    return 1;
}
