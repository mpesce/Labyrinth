/*
 * qvbuild.h -- turning a VRML 1.0 scene graph into Reality Lab objects.
 *
 * This is new code, not part of Labyrinth 2.0a2 (whose VRML support stopped
 * at parsing).  It is called from the QvLib traversal methods in qvglue.cpp
 * and builds the same kind of RL scene that the Labyrinth world compiler
 * does: frames with meshes and lights under the scene frame, with any
 * WWWAnchor attached as a hot link the drawing window can follow.
 */
#ifndef QVBUILD_H
#define QVBUILD_H

#include "../rl/rl.h"

class QvNode;
class QvState;
class QvLOD;
class QvAsciiText;
class QvCone;
class QvCube;
class QvCylinder;
class QvIndexedFaceSet;
class QvIndexedLineSet;
class QvPointSet;
class QvSphere;
class QvWWWAnchor;
class QvWWWInline;
class QvInfo;
class QvUnknownNode;

void QvBuildBegin(RLFrame scene, const char *file);
void QvBuildEnd(void);
void QvBuildForget(void);       /* a non-VRML world is being loaded */
void QvBuildHeadlight(RLFrame scene);   /* ... and it has no lights */

void QvBuildPush(void);
void QvBuildPop(void);
void QvBuildPushTransform(void);
void QvBuildPopTransform(void);
void QvBuildProperty(QvNode *node, QvState *state);

void QvBuildQvLOD(QvLOD *node, QvState *state);
void QvBuildQvAsciiText(QvAsciiText *node, QvState *state);
void QvBuildQvCone(QvCone *node, QvState *state);
void QvBuildQvCube(QvCube *node, QvState *state);
void QvBuildQvCylinder(QvCylinder *node, QvState *state);
void QvBuildQvIndexedFaceSet(QvIndexedFaceSet *node, QvState *state);
void QvBuildQvIndexedLineSet(QvIndexedLineSet *node, QvState *state);
void QvBuildQvPointSet(QvPointSet *node, QvState *state);
void QvBuildQvSphere(QvSphere *node, QvState *state);
void QvBuildQvWWWAnchor(QvWWWAnchor *node, QvState *state);
void QvBuildQvWWWInline(QvWWWInline *node, QvState *state);
void QvBuildQvInfo(QvInfo *node, QvState *state);
void QvBuildQvUnknownNode(QvUnknownNode *node, QvState *state);

#endif /* QVBUILD_H */
