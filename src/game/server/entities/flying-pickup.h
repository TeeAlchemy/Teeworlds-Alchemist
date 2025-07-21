#ifndef GAME_SERVER_ENTITIES_FLYINGPICKUP_H
#define GAME_SERVER_ENTITIES_FLYINGPICKUP_H

#include <game/server/entity.h>

class CFlyingPickup : public CEntity
{
public:
    CFlyingPickup(CGameWorld *pGameWorld, vec2 Pos, int Type, int Subtype, int TargetID);
    ~CFlyingPickup() {};

    virtual void Tick();
    virtual void Reset();
    virtual void Snap(int SnappingClient);

private:
    int m_Type;
    int m_Subtype;
    int m_TargetID;
    float m_InitialAmount;
    vec2 m_Vel;
};

#endif