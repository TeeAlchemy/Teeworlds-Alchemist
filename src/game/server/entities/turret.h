#ifndef GAME_SERVER_ENTITIES_TURRET_H
#define GAME_SERVER_ENTITIES_TURRET_H

#include <game/server/entity.h>

static const int s_TurretNumID = 6;
class CTurret : public CEntity
{
public:
    CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner);
    ~CTurret();

    void Tick() override;
    void Snap(int SnappingClient) override;
    void Reset() override;

    int GetOwner() { return m_Owner; }
    int GetLevel();

private:
    int m_Type;
    int m_Owner;
    int m_IDs[s_TurretNumID];
};

#endif