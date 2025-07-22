#ifndef GAME_SERVER_ENTITIES_BUILD_INDICATOR_H
#define GAME_SERVER_ENTITIES_BUILD_INDICATOR_H

#include <game/server/entity.h>

class CBuildIndicator : public CEntity
{
public:
    CBuildIndicator(CGameWorld *pGameWorld);

    virtual void Tick();
    virtual void Snap(int SnappingClient);
    virtual void Reset();

    void Show(int ClientID);
    void Hide(int ClientID);

    int GetTeamLaser(int Team) { return Team ? LASERTYPE_FREEZE : LASERTYPE_SHOTGUN; }
    int GetTeamArmor(int Team) { return Team ? POWERUP_ARMOR_LASER : POWERUP_ARMOR_SHOTGUN; }

private:
    int m_Snap[MAX_CLIENTS];
    int m_IDs[12];
    float m_Angle;
};

#endif