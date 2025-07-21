#ifndef GAME_SERVER_ENTITIES_WORKBENCH_H
#define GAME_SERVER_ENTITIES_WORKBENCH_H

#include "buildings.h"
#include <game/server/entity.h>

class CWorkbench : public CBuilding
{
public:
    CWorkbench(CGameWorld *pGameWorld, int Team, vec2 V1, vec2 V2);
    ~CWorkbench() {};

    virtual void Tick();
    virtual void Reset();
    virtual void Snap(int SnappingClient);

    void Init();

private:
    float m_Step;
    vec2 m_Vertex[2];
};


#endif