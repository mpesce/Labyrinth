# Labyrinth 2.0a2, reconstructed

Labyrinth was the first VRML browser, written in 1994–95 by Mark Pesce and Tony Parisi.
This folder holds C/C++ source rebuilt from the only surviving copy,
`LABYRINTH.EXE` (version 2.0a2, built with Microsoft Visual C++ 2.0 on 3 March 1995).
The rebuilt source compiles with Visual Studio 2022 and runs on Windows 11 as a
64-bit program; a 32-bit build is kept as well.

The original `LABYRINTH.EXE` only "barely ran" on modern Windows. The reason is that its
statically linked RenderMorphics Reality Lab could only drive 8-bit palettized displays.

This is not a re-creation of the 1995 look. It is the same program brought forward: the same
core functionality, implemented with what Windows and graphics hardware offer today.
Labyrinth now draws at the display's full resolution with antialiasing, follows each
monitor's scaling, and uses current Windows fonts and dialogs. Every departure from the
1995 behaviour is marked in the source and listed under
[Changes from the 1995 program](#changes-from-the-1995-program).

## What is in the program

The 1995 executable was linked from five parts. Each is handled differently here.

| 1995 component | Address range | Here |
|---|---|---|
| Labyrinth application: window framework, main window, drawing window, scene model, GIF reader, world loader | 0x401000–0x401200, 0x40ced0–0x416080, 0x4228e0–0x4245e0 | **Decompiled** into `src/app`, function by function. Each function is tagged with its original address, e.g. `/* 40efd0 */` |
| Tony Parisi's world compiler for the Labyrinth scene language (pcyacc grammar + lexer + actions) | inside 0x40ced0–0x416080 | **Decompiled** into `src/wc`. The grammar `wc.y` was recovered from the pcyacc tables, and Bison's automaton for it matches the original's 142 states |
| QvLib 0.0, SGI's VRML 1.0 parser | 0x416080–0x4221c0 | **Replaced** by QvLib 1.0 source (`src/qvlib`). It is the same library, a few months newer |
| CERN libwww 2.1x | 0x4011f0–0x40ced0 | **Replaced** by a WinINet fetcher (`src/app/labwww.cpp`). Labyrinth used libwww only to copy a URL into a local file. The fetcher keeps that interface and adds HTTPS and redirects |
| RenderMorphics Reality Lab (8-bit only) | 0x4245e0–0x487ba0 | **Re-implemented** (`src/rl`): the 65 Reality Lab entry points Labyrinth calls, on OpenGL (1.1, plus framebuffer objects for antialiasing where the driver has them). Reality Lab's own math is reproduced: row-vector matrices, rotation and orientation formulas, frame hierarchy, projection, picking, the XOF and PPM loaders |

The C runtime (0x487c00 onward) was recognised but not decompiled.

### VRML

Version 2.0a2 parsed VRML 1.0, but it did not yet draw it. It ran QvLib's sample
traversal, which pops up a message box ("Traversing a Separator.") for every node.
Labyrinth worlds were drawn through Tony's scene language. This build adds a VRML 1.0
renderer (`src/app/qvbuild.cpp`), so VRML worlds appear on screen. To see the original
2.0a2 behaviour, set `VRMLTraversal=original` in LAB.INI.

The VRML renderer supports:

- Separator / TransformSeparator / Group / Switch / LOD state handling
- all transforms
- Material, including emissive and per-face/per-vertex bindings
- IndexedFaceSet, with crease-angle smoothing, given normals, ShapeHints and concave faces
- IndexedLineSet and PointSet
- Cube, Sphere, Cone and Cylinder, with `parts`
- Texture2 (GIF, PPM, JPEG, PNG; cut-out alpha; wrapS/wrapT; mipmapped), Texture2Transform and TextureCoordinate2
- AsciiText and FontStyle, drawn as TrueType outline polygons
- Directional, point and spot lights, and a headlight when the world has no lights
- Perspective and orthographic cameras
- WWWAnchor: click to follow; `.wrl` links load in place, other links open the system browser
- WWWInline: fetched and gunzipped, and resolved relative to the inlining world
- gzipped `.wrl` files

## Layout

```
LABYRINTH.EXE      the 1995 original
src/               the reconstructed source (CMake project)
  app/             Labyrinth itself (decompiled), plus the new VRML renderer,
                   gzip decoder, image loader and WinINet fetcher
  wc/              the world compiler: wc.y (recovered grammar), lexer, actions,
                   pre-generated wcparse.cpp
  qvlib/           QvLib 1.0 (VRML 1.0 parser), lightly ported to Windows
  rl/              Reality Lab API on OpenGL
  res/             menus, dialogs, icon and cursors (taken from the EXE), manifest;
                   labyrinth-1995.ico is the original 32x32 icon, labyrinth.ico the
                   same picture at sizes up to 256 (tools/scripts/makeicon.ps1)
samples/           test worlds: test.wrl (scene language), vrml1.wrl, vrml1gz.wrl,
                   text.wrl, ushm.wrl (the original Holocaust Museum world -- see below)
dist/              ready-to-run copy: Labyrinth.exe (64-bit), Labyrinth32.exe (32-bit),
                   LAB.INI, samples
tools/scripts/     the analysis and test scripts (Ghidra export/naming, grammar
                   recovery, window driver, image montage)
work/              Ghidra project and exports, test output
ref/               reference sources consulted (libwww 1993, QvLib 1.0, geomview, ...)
```

In the GitHub repository, `dist/` holds only LAB.INI. The ready-to-run copy is
attached to the [releases](https://github.com/mpesce/Labyrinth/releases) as a zip.
`work/` and `ref/` are not in the repository, and the Ghidra scripts in
`tools/scripts` need a Ghidra project of their own.

## Building

You need Visual Studio 2022 with the C++ desktop workload, and CMake 3.21 or later.
Bison is optional: `src/wc/wcparse.cpp` is pre-generated, and it is regenerated only if
`bison` or `win_bison` is found.

From the `src` folder:

```
cmake --preset x64 && cmake --build --preset x64     # -> build/Release/Labyrinth.exe (64-bit)
cmake --preset x86 && cmake --build --preset x86     # -> build32/Release/Labyrinth.exe (32-bit)
```

Without presets: `cmake -S src -B build -A x64` (or `-A Win32`), then
`cmake --build build --config Release`. The two builds render identically.

The executable links the C runtime statically and needs no redistributable. It uses
only system DLLs: opengl32, glu32, dwmapi, wininet, windowscodecs and the usual Win32 set.
It needs Windows 10 version 1703 or later (per-monitor DPI awareness).

## Running

```
Labyrinth.exe                     (empty, or the Home world if Autoload Home=yes)
Labyrinth.exe world.wrl           a local world, VRML 1.0 or the Labyrinth scene language
Labyrinth.exe https://.../x.wrl   a world on the Web
```

Worlds can also be dropped onto the window, or opened with **File ▸ Open URL / Open Local**.

| Action | Control |
|---|---|
| Move | Hold the mouse button on the navigation pad. Above or below its centre goes forward or back, left or right turns, and the distance from the centre sets the speed. With Shift held, the pad slides the camera up or down instead |
| Follow a link | Click an object; the status line shows its URL when hovered |
| Move an object | Shift-drag |
| Tilt an object | Ctrl-drag |
| Return to the world's camera | `H`, or **View ▸ Restore** |
| Change the rendering style | `R`, or **View ▸ Wireframe / Flat / Gouraud / Phong** (Phong is drawn as Gouraud for now). Flat differs from Gouraud only on curved surfaces |

### LAB.INI

The `[Main]` section is read from LAB.INI next to the program. If there is none there,
the Windows directory is used, as in 1995.

| Key | Default | Meaning |
|---|---|---|
| `Autoload Home` | `no` | Load `Home` at start-up |
| `Home` | `home.wrl` | Home world (file or URL) |
| `BROWSER` | (none) | Program to run for non-world links. Unset means the system default browser (the original defaulted to `MOSAIC.EXE`) |
| `TwoSided` | `no` | Draw the back faces of Labyrinth-language worlds (Reality Lab culled them) |
| `Antialias` | `4` | Multisample antialiasing, in samples per pixel. `0` turns it off |
| `VRMLTraversal` | `render` | `original` restores 2.0a2's message-box-per-node traversal |
| `VRMLStrict` | `0` | `1` makes unknown VRML nodes/fields fatal, as QvLib was. By default they are skipped, as most browsers of the day did |

Downloaded files and scratch files go in `%TEMP%\Labyrinth`.

### Test hook

If the environment variable `LABYRINTH_SNAPSHOT=file.bmp` is set, Labyrinth renders,
saves frame `LABYRINTH_SNAPSHOT_FRAMES` (default 30) of the view to that file, and
exits. The picture is the size of the view in display pixels, so it depends on the
display's scale. It also skips the pre-release notice. The regression renders were made this way,
and `tools/scripts/labdrive.ps1` drives a running copy with menu commands, clicks and
drags.

## Changes from the 1995 program

Each change is marked in the source with `(new)`, `(fixed)` or "the original ...".

**Necessary replacements**
- Reality Lab is replaced by an OpenGL re-implementation, so any display depth works.
- libwww is replaced by WinINet. Relative URLs resolve against the current document;
  the original always resolved them against `http://www.butterfly.net/lab/ushm.wrl`.
  Failed transfers are reported.
- QvLib 0.0 is replaced by QvLib 1.0 (`LevelOfDetail` became `LOD`).
- LAB.INI is looked for beside the program first.

**Brought forward**
- Per-monitor DPI awareness. The window, the panels, the status line and the navigation
  pad are laid out in the display's own pixels from the 1995 sizes, and are laid out
  again when the window moves to a monitor with another scale.
- The 3D view is drawn at the display's full resolution, with 4x multisample
  antialiasing (`Antialias` in LAB.INI). Lines and points are widened to match the
  display's scale.
- Textures are mipmapped and anisotropically filtered, and honour VRML's
  `wrapS`/`wrapT CLAMP`. Spheres, cones and cylinders have 64 segments instead of 24.
- The frame rate follows the display's, timed by the desktop compositor. It used to wait
  in `SwapBuffers`, which kept a CPU core busy on some drivers. Nothing is drawn while the
  window is minimised.
- Current Windows look: the dialogs use Segoe UI, the status line uses the Windows
  status-bar font, the window uses the standard face colour rather than the grey of a
  1995 MDI workspace, and the icon comes at sizes up to 256 pixels.
- The default window is 800 x 600 at 100% scale, twice the original's 400 x 300, which
  was sized for a 640 x 480 screen. It has a minimum size so that the layout still fits.
- The View menu marks the rendering style in use with a radio bullet, whether it was
  chosen from the menu or with `R`. **Dither** is greyed: it chose dithering for 8-bit
  palette displays, and in true colour there is nothing to dither.
- The pre-release notice shown at start-up is titled just "PRERELEASE VERSION" (the
  1995 title went on "- DO NOT REDISTRIBUTE!") and reads "Copyright (c) 2026, Mark
  Pesce and Tony Parisi". It used to credit The Community Company and, for Reality
  Lab, which is no longer used, RenderMorphics. The About box and the program's file
  properties carry the same copyright, and the About box no longer has its Reality Lab
  line.
- **HotSpots ▸ U.S. Holocaust Museum Tour** fetches the world from
  `http://hyperreal.org/~mpesce/ushm/ushm.wrl`; the 1995 address at eit.com is gone.
- **Cyberbanana**, the very first VRML world, now heads the HotSpots menu's list of
  worlds (`http://hyperreal.org/~mpesce/banana.wrl`).
- A Labyrinth-format world with no lights of its own gets the same headlight as a VRML
  world. Before, it had only a dim 20% ambient light, so the Cyberbanana showed flat
  and dark olive. Worlds that bring their own lights look as they did.
- **WebEarth 0.1** (`http://hyperreal.org/~mpesce/wearth.wrl`) follows the Holocaust
  Museum tour on the HotSpots menu.
- Four worlds added at the foot of the HotSpots menu: **Cybersamhain**
  (`http://hyperreal.org/~mpesce/samhain/wrl/samhain.wrl`), **Tardis**, the Doctor
  Who teleport from Daniel's Story (`http://hyperreal.org/~mpesce/ushm/doctor.wrl`),
  **Cybermall** (`http://hyperreal.org/~mpesce/mall/GBC.wrl`), and **Circle by
  C. Scott Young**, a VRML 1.0 world (`http://hyperreal.org/~mpesce/coven.wrl`).
  **Internet Underground Music Archive** and **WaxWeb** are greyed, as their worlds are
  gone.
- The Help menu opens documents at `http://hyperreal.org/~mpesce/`, in place of the
  dead 1995 addresses at eit.com and net.org: **Navigation** the Labyrinth * MOTIF
  user's guide (`labnav.html`), **Creating worlds** the EIT introduction to VRML
  (`eitvrml.html`), and **Scripting** the June 1994 VRML working specification 1.3.1
  (`script.html`).

**Bugs fixed** (all latent in 1995)
- The message loop blocked in `GetMessage` and stalled rendering. It now drains
  messages with `PeekMessage`.
- The window lists freed their embedded links with `delete`, a heap corruption hidden by
  exiting through `exit(0)`.
- With two or more scenes, `WcWorld::AddScene` linked a scene into a second list, which
  made the end-of-compile dump loop forever.
- Every world load added another ambient light, so worlds grew brighter each time.
- Shift-drag gave `RLViewportTransform` camera-relative coordinates where it expects
  world coordinates, so objects jumped. Ctrl-drag pushed direction vectors through the
  viewport as if they were points. Ctrl-drag now turns the object about the camera's
  axes.
- Deselecting removed the frame's *second* visual, which it took to be the marker.
- On a resize the side panel was given the window's width less 80 pixels instead of
  80 pixels.
- A polygon written inline without a `Normal` used the world compiler's "not given" mark
  (-1, -1, -1) as its normal, so every such face was lit the same and a curved object
  had no shading. The normal is now worked out from the polygon's vertices.
- Smaller fixes: a division by an uninitialised variable in the navigation pad; vertex
  overrun and NULL-file cases in the world compiler; an end-of-file loop in the lexer;
  GIF errors calling `exit()`.

**Additions**
- The VRML 1.0 renderer described above. Each VRML shape gets its own frame, so it can
  be picked and dragged.
- Tolerant VRML parsing, multiple top-level nodes, and a clear message for VRML 2.0 files.
- gzipped files, recognised by their contents rather than their names: worlds, inlines,
  `.xof` shapes and textures. JPEG/PNG textures; a URL on the command line; HTTPS.
- Non-world links open the system browser when no `BROWSER` is configured.
- Reality Lab extensions used by the above: two-sided meshes, emissive materials,
  line/point faces, cut-out textures, light removal.
- An embedded manifest (per-monitor DPI aware, visual styles), and the original icon and
  cursors.

## Known limits

- **Daniel's Story** (the U.S. Holocaust Museum tour) is complete again at
  `http://hyperreal.org/~mpesce/ushm/`, with its `.xof` shape files, diary pages and
  the Doctor Who teleport, and **HotSpots ▸ U.S. Holocaust Museum Tour** opens it.
  `samples/ushm.wrl` is the 1995 world file, recovered from a 1999 Wayback Machine
  capture; it still names `www.butterfly.net`, which is gone, so it loads as an empty
  world.
- **HotSpots ▸ Virtual SOMA** points at a 1995 server (eit.com) that is gone and was
  not archived.
- **Edit** and **HotSpots ▸ Add/View** are greyed out, as they were in 2.0a2.
  **HOTWIRED - Coming Soon!** stays greyed as it was; IUMA and WaxWeb are greyed now.
- VRML 2.0 / VRML 97 is not supported (2.0a2 predates it). Material transparency is
  not drawn; cut-out texture alpha is.

## Credits and licences

Labyrinth was written by Mark Pesce and Tony Parisi. The scene-language compiler and its
grammar are Tony Parisi's. QvLib 1.0 is Silicon Graphics' VRML 1.0 parser; this copy
comes from the geomview repository, whose headers carry geomview's LGPL notice. Reality
Lab and libwww are not included; only Labyrinth's calls into them were reconstructed.
