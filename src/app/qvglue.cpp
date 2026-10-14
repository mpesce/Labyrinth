/*
 * qvglue.cpp -- reading VRML 1.0 with QvLib.
 *
 * LabLoadVRML is reconstructed from LABYRINTH.EXE 0x415f70.  The traversal
 * below is Labyrinth 2.0a2's (0x416080-0x416990): QvLib's sample traversal
 * with announce() reporting each node through Application::Alert -- i.e. a
 * message box per node -- and the shape nodes dumping the traversal state
 * to stdout.  VRML scenes were parsed but not yet drawn in 2.0a2.
 *
 * Labyrinth shipped QvLib 0.0; this build uses QvLib 1.0, so QvLOD replaces
 * QvLevelOfDetail and QvAsciiText/QvFontStyle are new.
 *
 * When LabVRMLRender is true (the default -- see qvbuild.cpp) the traversal
 * builds Reality Lab geometry instead; set VRMLTraversal=original in the
 * [Main] section of LAB.INI to get the 2.0a2 behaviour back.
 */
#include <stdio.h>
#include <string.h>
#include <QvDB.h>
#include <QvInput.h>
#include <QvState.h>
#include <QvNode.h>
#include <QvElement.h>
#include <QvNodes.h>
#include <QvUnknownNode.h>
#include <QvReadError.h>
#include <QvChildList.h>
#include "drawwin.h"
#include "mainwin.h"
#include "qvbuild.h"

extern "C" const char *LabIniFile(void);
extern "C" void LabError(const char *fmt, ...);

RLFrame qvScene;                                                /* 4b8bb4 */
static int indent;                                              /* 4b8bf0 */
int LabVRMLRender = 1;

static void announce(const char *className)                     /* 416080 */
{
    char buf[200];
    sprintf(buf, "Traversing a %s.", className);
    labApp->Alert(labMainWindow, buf);
}

int LabLoadVRML(RLFrame scene, const char *file)                /* 415f70 */
{
    char mode[32];
    GetPrivateProfileString("Main", "VRMLTraversal", "render", mode, sizeof(mode), LabIniFile());
    LabVRMLRender = _stricmp(mode, "original") != 0;
    /* (new) skip unknown nodes and fields rather than give up on the world */
    QvReadError::lenient = !GetPrivateProfileInt("Main", "VRMLStrict", 0, LabIniFile());
    QvReadError::clear();

    QvDB::init();
    QvInput in;
    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        /* the original printed this and exited */
        printf("Things are bad.  File %s did not open.\n", file);
        QvBuildForget();
        return -101;
    }

    /* (new) say so rather than fail on a VRML 2.0 world */
    char header[16] = "";
    if (fgets(header, sizeof(header), fp) && strncmp(header, "#VRML V2.0", 10) == 0) {
        fclose(fp);
        QvBuildForget();
        LabError("This is a VRML 2.0 world.  Labyrinth reads VRML 1.0.");
        return 0;
    }
    int isVRML = strncmp(header, "#VRML V1.0", 10) == 0;
    rewind(fp);

    in.setFilePointer(fp);
    QvNode *root;
    if (!QvDB::read(&in, root) || root == NULL) {
        printf("Read was bad\n");
        fclose(fp);
        QvBuildForget();
        if (isVRML) {
            /* (new) the original went on to try it as a Labyrinth world */
            const char *why = QvReadError::getError();
            LabError("This VRML world could not be read:\n\n%s",
                     *why ? why : "The file ended before the world did.");
            return 0;
        }
        return -102;
    }
    if (QvReadError::lenient) {
        /* (new) worlds with more than one node at the top level */
        QvNode *next;
        QvGroup *group = NULL;
        while (QvDB::read(&in, next) && next != NULL) {
            if (group == NULL) {
                group = new QvGroup;
                group->getChildren()->append(root);
                root = group;
            }
            group->getChildren()->append(next);
        }
    }
    fclose(fp);

    qvScene = scene;
    {
        QvState state;
        if (LabVRMLRender)
            QvBuildBegin(scene, file);
        root->ref();
        root->traverse(&state);
        if (LabVRMLRender)
            QvBuildEnd();
        root->unref();
    }
    qvScene = 0;
    return 0;
}

/* ---- traversal ---------------------------------------------------------- */

#define ANNOUNCE(className) announce(QV__QUOTE(className))

#define DEFAULT_TRAVERSE(className)                                     \
void className::traverse(QvState *state)                                \
{                                                                       \
    if (LabVRMLRender) { QvBuild##className(this, state); return; }     \
    ANNOUNCE(className);                                                \
}

void QvGroup::traverse(QvState *state)                          /* 4160c0 */
{
    if (!LabVRMLRender)
        ANNOUNCE(QvGroup);
    indent++;
    for (int i = 0; i < getNumChildren(); i++)
        getChild(i)->traverse(state);
    indent--;
}

void QvLOD::traverse(QvState *state)                            /* 416120 */
{
    if (LabVRMLRender) {
        QvBuildQvLOD(this, state);
        return;
    }
    ANNOUNCE(QvLOD);
    indent++;
    if (getNumChildren() > 0)
        getChild(0)->traverse(state);
    indent--;
}

void QvSeparator::traverse(QvState *state)                      /* 416160 */
{
    if (!LabVRMLRender)
        ANNOUNCE(QvSeparator);
    state->push();
    if (LabVRMLRender)
        QvBuildPush();
    indent++;
    for (int i = 0; i < getNumChildren(); i++)
        getChild(i)->traverse(state);
    indent--;
    if (LabVRMLRender)
        QvBuildPop();
    state->pop();
}

void QvSwitch::traverse(QvState *state)                         /* 4161c0 */
{
    if (!LabVRMLRender)
        ANNOUNCE(QvSwitch);
    indent++;
    int which = whichChild.value;
    if (which == QV_SWITCH_NONE)
        ;
    else if (which == QV_SWITCH_ALL)
        for (int i = 0; i < getNumChildren(); i++)
            getChild(i)->traverse(state);
    else if (which < getNumChildren())
        getChild(which)->traverse(state);
    indent--;
}

void QvTransformSeparator::traverse(QvState *state)             /* 416240 */
{
    if (!LabVRMLRender)
        ANNOUNCE(QvTransformSeparator);

    /* "push" just the transformation stack with a no-op marker */
    QvElement *markerElt = new QvElement;
    markerElt->data = this;
    markerElt->type = QvElement::NoOpTransform;
    state->addElement(QvState::TransformationIndex, markerElt);
    if (LabVRMLRender)
        QvBuildPushTransform();

    indent++;
    for (int i = 0; i < getNumChildren(); i++)
        getChild(i)->traverse(state);
    indent--;

    if (LabVRMLRender)
        QvBuildPopTransform();
    while (state->getTopElement(QvState::TransformationIndex) != markerElt)
        state->popElement(QvState::TransformationIndex);
}

/* Properties: push an element on the matching stack. */
#define DO_PROPERTY(className, stackIndex)                              \
void className::traverse(QvState *state)                                \
{                                                                       \
    if (!LabVRMLRender) ANNOUNCE(className);                            \
    QvElement *elt = new QvElement;                                     \
    elt->data = this;                                                   \
    state->addElement(QvState::stackIndex, elt);                        \
    if (LabVRMLRender) QvBuildProperty(this, state);                    \
}

#define DO_TYPED_PROPERTY(className, stackIndex, eltType)               \
void className::traverse(QvState *state)                                \
{                                                                       \
    if (!LabVRMLRender) ANNOUNCE(className);                            \
    QvElement *elt = new QvElement;                                     \
    elt->data = this;                                                   \
    elt->type = QvElement::eltType;                                     \
    state->addElement(QvState::stackIndex, elt);                        \
    if (LabVRMLRender) QvBuildProperty(this, state);                    \
}

DO_PROPERTY(QvCoordinate3, Coordinate3Index)                    /* 4162e0 */
DO_PROPERTY(QvFontStyle, FontStyleIndex)
DO_PROPERTY(QvMaterial, MaterialIndex)
DO_PROPERTY(QvMaterialBinding, MaterialBindingIndex)
DO_PROPERTY(QvNormal, NormalIndex)
DO_PROPERTY(QvNormalBinding, NormalBindingIndex)
DO_PROPERTY(QvShapeHints, ShapeHintsIndex)
DO_PROPERTY(QvTextureCoordinate2, TextureCoordinate2Index)
DO_PROPERTY(QvTexture2, Texture2Index)
DO_PROPERTY(QvTexture2Transform, Texture2TransformationIndex)

DO_TYPED_PROPERTY(QvDirectionalLight, LightIndex, DirectionalLight)
DO_TYPED_PROPERTY(QvPointLight, LightIndex, PointLight)
DO_TYPED_PROPERTY(QvSpotLight, LightIndex, SpotLight)

DO_TYPED_PROPERTY(QvOrthographicCamera, CameraIndex, OrthographicCamera)
DO_TYPED_PROPERTY(QvPerspectiveCamera, CameraIndex, PerspectiveCamera)

DO_TYPED_PROPERTY(QvTransform, TransformationIndex, Transform)
DO_TYPED_PROPERTY(QvRotation, TransformationIndex, Rotation)
DO_TYPED_PROPERTY(QvMatrixTransform, TransformationIndex, MatrixTransform)
DO_TYPED_PROPERTY(QvTranslation, TransformationIndex, Translation)
DO_TYPED_PROPERTY(QvScale, TransformationIndex, Scale)

/* Shapes: 2.0a2 dumped the traversal state between two rules. */
static void printProperties(QvState *state)                     /* 416840 */
{
    printf("--------------------------------------------------------------\n");
    state->print();
    printf("--------------------------------------------------------------\n");
}

#define DO_SHAPE(className)                                             \
void className::traverse(QvState *state)                                \
{                                                                       \
    if (LabVRMLRender) { QvBuild##className(this, state); return; }     \
    ANNOUNCE(className);                                                \
    printProperties(state);                                             \
}

DO_SHAPE(QvAsciiText)
DO_SHAPE(QvCone)                                                /* 416870 */
DO_SHAPE(QvCube)
DO_SHAPE(QvCylinder)
DO_SHAPE(QvIndexedFaceSet)
DO_SHAPE(QvIndexedLineSet)
DO_SHAPE(QvPointSet)
DO_SHAPE(QvSphere)

/* WWW nodes and the rest */
DEFAULT_TRAVERSE(QvWWWAnchor)                                   /* 416950 */
DEFAULT_TRAVERSE(QvWWWInline)                                   /* 416960 */
DEFAULT_TRAVERSE(QvInfo)                                        /* 416970 */
DEFAULT_TRAVERSE(QvUnknownNode)                                 /* 416980 */

#undef ANNOUNCE
#undef DEFAULT_TRAVERSE
#undef DO_PROPERTY
#undef DO_TYPED_PROPERTY
#undef DO_SHAPE
