/*
 * wc.y -- grammar of the Labyrinth world description language
 * (Tony Parisi's proto-VRML, 1994-95).
 *
 * The original was compiled with pcyacc.  This grammar was recovered from
 * the parser tables embedded in LABYRINTH.EXE 2.0a2 (yyr1/yyr2/yychk/...):
 * the 65 rules below appear in their original order, the nonterminals were
 * named from the semantic actions they trigger, and the action bodies call
 * the same routines with the same $n arguments as the pcyacc action switch.
 *
 * A small example world:
 *
 *    World "Example" Begin
 *        Object "Square" Begin
 *            Color 0.2 0.4 1.0
 *            Poly { Vertex 0 0 0  Vertex 1 0 0  Vertex 1 1 0  Vertex 0 1 0 }
 *        End
 *        Scene "Main" Begin
 *            Element Begin
 *                Object "Square"
 *                Position 0 0 5
 *                URL "http://www.example.com/next.wrl"
 *            End
 *            Light Begin
 *                Object "Sun" Directional Color 1 1 1
 *                Orientation 0 -1 1  0 1 0
 *            End
 *        End
 *        Camera Begin
 *            Position 0 0 -10
 *            Orientation 0 0 1  0 1 0
 *            Field -0.5 0.5 0.5 -0.5
 *        End
 *    End
 */

%{
#include <stdio.h>
#include "wc.h"

#define YYSTYPE WcValue
#define yylval wclval
#define yylex WcNextToken
#define yyparse WcParse
#define yyerror wcerror

void wcerror(const char *msg);
%}

/* Token numbers are those of the original (see wc.h); 258 and 259 were
 * declared but never used. */
%token WORLD 257 RESERVED_258 258 RESERVED_259 259
%token OBJECT 260 POLY 261 VERTEX 262 COLOR 263 MATERIAL 264 TEXTURE 265
%token SHAPEFILE 266 CAMERA 267 FIELD 268 NORMAL 269 URL 270 SCENE 271
%token ELEMENT 272 LIGHT 273 POSITION 274 ORIENTATION 275
%token OPEN 276 CLOSE 277 BEGIN_ 278 END_ 279
%token IDENTIFIER 280 INTEGER 281 DOUBLE 282 STRING 283

%start world

%%

world
        : WORLD name open
                { WcBeginWorld($<sval>2, $<ival>3); }
          world_items close
                { WcEndWorld($<ival>6); YYACCEPT; }
        ;

world_items
        : /* empty */
        | world_items world_item
        ;

world_item
        : object
        | scene
        | camera
        ;

camera
        : CAMERA open camera_position camera_orientation camera_field close
        ;

camera_position
        : POSITION number number number
                { WcSetCameraPosition($<dval>2, $<dval>3, $<dval>4); }
        ;

camera_orientation
        : ORIENTATION number number number number number number
                { WcSetCameraOrientation($<dval>2, $<dval>3, $<dval>4,
                                         $<dval>5, $<dval>6, $<dval>7); }
        ;

camera_field
        : FIELD number number number number
                { WcSetCameraField($<dval>2, $<dval>3, $<dval>4, $<dval>5); }
        ;

object
        : OBJECT name open
                { WcBeginObject($<sval>2, $<ival>3); }
          object_items close
                { WcEndObject($<ival>6); }
        ;

object_items
        : /* empty */
        | object_items object_item
        ;

object_item
        : geometry
        | attribute
        | object_url
        ;

geometry
        : poly
        | shapefile
        ;

poly
        : POLY open
                { WcBeginPoly($<ival>2); }
          poly_items close
                { WcEndPoly($<ival>5); }
        ;

poly_items
        : /* empty */
        | poly_items poly_item
        ;

poly_item
        : vertex
        | attribute
        | normal
        ;

vertex
        : VERTEX number number number
                { WcAddVertex($<dval>2, $<dval>3, $<dval>4); }
        ;

attribute
        : COLOR number number number
                { WcSetColor($<dval>2, $<dval>3, $<dval>4); }
        | MATERIAL number number
                { WcSetMaterial($<dval>2, $<dval>3); }
        | TEXTURE STRING number number IDENTIFIER
                { WcSetTexture($<sval>2, $<dval>3, $<dval>4, $<sval>5); }
        ;

normal
        : NORMAL number number number
                { WcSetNormal($<dval>2, $<dval>3, $<dval>4); }
        ;

object_url
        : URL STRING
                { WcSetObjectURL($<sval>2); }
        ;

shapefile
        : SHAPEFILE STRING
                { WcSetShapeURL($<sval>2); }
        ;

scene
        : SCENE name open
                { WcBeginScene($<sval>2, $<ival>3); }
          scene_items close
                { WcEndScene($<ival>6); }
        ;

scene_items
        : /* empty */
        | scene_items scene_item
        ;

scene_item
        : element
        | light
        ;

element
        : ELEMENT open
                { WcBeginElement($<ival>2); }
          element_items close
                { WcEndElement($<ival>5); }
        ;

element_items
        : /* empty */
        | element_items element_item
        ;

element_item
        : OBJECT STRING
                { WcSetElementObject($<sval>2); }
        | POSITION number number number
                { WcSetElementPosition($<dval>2, $<dval>3, $<dval>4); }
        | ORIENTATION number number number number number number
                { WcSetElementOrientation($<dval>2, $<dval>3, $<dval>4,
                                          $<dval>5, $<dval>6, $<dval>7); }
        | URL STRING
                { WcSetElementURL($<sval>2); }
        ;

light
        : LIGHT open
                { WcBeginLight($<ival>2); }
          light_items close
                { WcEndLight($<ival>5); }
        ;

light_items
        : /* empty */
        | light_items light_item
        ;

light_item
        : OBJECT STRING IDENTIFIER COLOR number number number
                { WcSetLightObject($<sval>2, $<sval>3, $<dval>5, $<dval>6, $<dval>7); }
        | POSITION number number number
                { WcSetLightPosition($<dval>2, $<dval>3, $<dval>4); }
        | ORIENTATION number number number number number number
                { WcSetLightOrientation($<dval>2, $<dval>3, $<dval>4,
                                        $<dval>5, $<dval>6, $<dval>7); }
        ;

name
        : /* empty */
                { $<sval>$ = (char *)""; }
        | STRING
                { $<sval>$ = $<sval>1; }
        ;

open
        : OPEN          { $<ival>$ = $<ival>1; }
        | BEGIN_        { $<ival>$ = $<ival>1; }
        | '{'           { $<ival>$ = $<ival>1; }
        ;

close
        : CLOSE         { $<ival>$ = $<ival>1; }
        | END_          { $<ival>$ = $<ival>1; }
        | '}'           { $<ival>$ = $<ival>1; }
        ;

number
        : INTEGER       { $<dval>$ = (double)$<ival>1; }
        | DOUBLE        { $<dval>$ = $<dval>1; }
        ;

%%
