#pragma once

#include <game/server/entity.h>
#include <vector>

enum
{
    CAR_EXTRA_IDS = 2,
    CAR_MAX_SPEED = 25,
    CAR_SPEED = 2,
    CAR_ACCEL = 2,
    
};

class CCar : public CEntity
{
private:
    int m_Health;
    int m_Owner;
    int m_Team;
    int m_IDs[CAR_EXTRA_IDS];
    vec2 m_Vel;

public:
    CCar(CGameWorld *pGameWorld, vec2 Pos, int Team = -1);
    ~CCar();

    /* Vehicle Functions */
    void GravityVehicle();

    virtual void Tick();
    virtual void Snap(int SnappingClient);
    virtual void Reset();

    int GetHealth() { return m_Health; }
    int GetOwner() { return m_Owner; }
    int GetTeam() { return m_Team; }
};