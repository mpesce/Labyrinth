/*
 * resource.h -- resource identifiers (as found in LABYRINTH.EXE).
 */
#ifndef RESOURCE_H
#define RESOURCE_H

#define IDM_LABYRINTH           1       /* menu bar */
#define IDA_LABYRINTH           1       /* accelerators (never loaded) */
#define IDI_LABYRINTH           101
#define IDC_LABLINK             100     /* cursor over a hot link */
#define IDC_LABROTATE           101     /* cursor while rotating */

#define IDD_ABOUT               100
#define IDC_ABOUT_NAME          101
#define IDC_ABOUT_VERSION       102
#define IDC_ABOUT_COPYRIGHT     103
#define IDC_ABOUT_REALITYLAB    104     /* (new) the line is struck */

#define IDD_PROMPT              200
#define IDC_PROMPT_TEXT         201
#define IDC_PROMPT_EDIT         202

/* File */
#define IDM_OPEN_URL            101
#define IDM_OPEN_LOCAL          102
#define IDM_SAVE_CAMERA         103
#define IDM_EXIT                120
/* Edit */
#define IDM_CUT                 201
#define IDM_COPY                202
#define IDM_PASTE               203
#define IDM_DELETE              204
/* View */
#define IDM_WIREFRAME           301
#define IDM_FLAT                302
#define IDM_GOURAUD             303
#define IDM_PHONG               304
#define IDM_RESTORE             305
#define IDM_DITHER              306
/* HotSpots */
#define IDM_ADD_HOTSPOT         40001
#define IDM_VIEW_HOTSPOTS       40002
#define IDM_HOTWIRED            403
#define IDM_SOMA                404
#define IDM_USHM                405
#define IDM_WAXWEB              406     /* 40007 in the 1995 resources */
#define IDM_IUMA                407     /* 40006 in the 1995 resources */
#define IDM_SAMHAIN             408     /* (new) */
#define IDM_TARDIS              409     /* (new) */
#define IDM_BANANA              410     /* (new) */
#define IDM_MALL                411     /* (new) */
#define IDM_WEARTH              412     /* (new) */
#define IDM_COVEN               413     /* (new) */
/* Help */
#define IDM_HELP_NAVIGATION     901
#define IDM_HELP_CREATING       902
#define IDM_HELP_SCRIPTING      903
#define IDM_ABOUT               904

#endif /* RESOURCE_H */
