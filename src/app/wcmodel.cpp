/*
 * wcmodel.cpp -- world compiler data model (see wcmodel.h).
 * Reconstructed from LABYRINTH.EXE 0x4129b0-0x413440.
 */
#include "wcmodel.h"

WcObject::WcObject(char *nm)                                    /* 4129b0 */
{
    name = nm;
    polys = 0;
    red = green = blue = 0.0;
    specular = power = 0.0;
    texture = 0;
    orientX = orientY = 0.0;
    mapping = 0;
    url = 0;
}

WcObject::~WcObject()                                           /* 412a00 */
{
    if (polys) {
        polys->DeleteLinks();
        delete polys;
    }
}

void WcObject::AddPoly(WcPoly *p)                               /* 412a70 */
{
    if (!polys)
        polys = new LabList;
    polys->Append(p);
}

WcPoly *WcObject::FirstPoly()                                   /* 412b10 */
{
    return polys ? (WcPoly *)polys->First() : 0;
}

WcPoly *WcObject::NextPoly(WcPoly *p)                           /* 412b20 */
{
    return polys ? (WcPoly *)LabList::Next(p) : 0;
}

WcPoly::WcPoly() : WcObject(0)                                  /* 412c80 */
{
    nverts = 0;
    /* -1 marks "not specified for this face" */
    red = green = blue = -1.0;
    specular = power = -1.0;
    normal[0] = normal[1] = normal[2] = -1.0;
}

WcCamera::WcCamera()                                            /* 412e60 */
{
    SetPosition(0.0, 0.0, -10.0);
    SetOrientation(0.0, 0.0, 1.0, 0.0, 1.0, 0.0);
    SetField(-0.5, 0.5, -0.5, 0.5);
}

WcScene::WcScene(char *nm) : WcObject(nm)                       /* 413070 */
{
    elements = 0;
}

void WcScene::AddElement(WcElement *e)                          /* 4130d0 */
{
    if (!elements)
        elements = new LabList;
    elements->Append(e);
}

WcElement *WcScene::FirstElement()                              /* 413160 */
{
    return elements ? (WcElement *)elements->First() : 0;
}

WcElement *WcScene::NextElement(WcElement *e)                   /* 413170 */
{
    return elements ? (WcElement *)LabList::Next(e) : 0;
}

WcWorld::WcWorld(char *nm)                                      /* 413190 */
{
    name = nm;
    scenes = 0;
}

void WcWorld::AddScene(WcScene *s)                              /* 4131f0 */
{
    if (!scenes)
        scenes = new LabList;
    scenes->Append(new WcSceneRef(s));
}

WcElement::WcElement()                                          /* 413290 */
{
    type = WcElementObject;
    object = 0;
    url = 0;
    pos[0] = pos[1] = pos[2] = 0.0;
    dir[0] = dir[1] = dir[2] = 0.0;
    up[0] = up[1] = up[2] = 0.0;
}

WcLight::WcLight()                                              /* 413440 */
{
    type = WcElementLight;
    lightType = 0;
    red = green = blue = 0.0;
}
