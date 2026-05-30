#ifndef GAME_SERVER_ENTITIES_H
#define GAME_SERVER_ENTITIES_H

#include <game/server/resources.h>
#include <game/server/entity.h>

// DAMN IT'S NODES
// WHYYYYYY
#define MAX_BUILDINGS 512

enum
{
    // Wait its actrully an copy
    BUILDING_NODE = 0, // Not an copy
                       // BUILDING_POWER, // This one is :D
    BUILDING_SHIELD,
    BUILDING_WORKBENCH, // but undefined

    BUILDING_SHOTGUN,
    BUILDING_GRENADE,
    BUILDING_LASER,

    BUILDING_TURRET_GUN,
    BUILDING_TURRET_SHOTGUN,

    BUILDING_HEALTH,
    BUILDING_ARMOR,
    BUILDING_WEAPON_PACK,

    BUILDING_GATHERER,

    BUILDING_AIRCRAFT,

    BUILDING_HELICOPTER,
    BUILDING_JET,
    BUILDING_TANK,
    BUILDING_CAR,

    NUM_BUILDING,
};

class CBuildingInfo
{
    CGameContext *m_pGameServer;

public:
    CBuildingInfo(CGameContext *pGameServer);

    struct SBuildingInfo
    {
        char m_aName[32];
        char m_aDesc[64];
        int m_Health;
        float m_Radius;
        float m_Width;
        float m_Height;
        int m_Formula[NUM_RESOURCE];
    };

    void Register(int ID, SBuildingInfo Data);
    SBuildingInfo m_aBuildingsInfo[NUM_BUILDING];

private:
    CGameContext *GameServer() { return m_pGameServer; }
};

class CBuilding : public CEntity
{
public:
    enum
    {
        NUMID_SHIELD = 12,
        NUMID_WEAPONBOX = 4,
        NUMID_TURRET = 5,
        NUMID_PICKUP = 5,
        NUMID_GATHERER = 1,
        NUMID_NODE = 1,
        NUMID_POWER = 4,
    };

public:
    CBuilding(CGameWorld *pGameWorld, int Team, vec2 Pos, int Type);
    ~CBuilding();

    virtual void Tick();
    virtual void TickDefered();
    virtual void Reset();
    virtual void Snap(int SnappingClient);

    virtual vec2 GetDamageCenter() const { return GetPos(); }
    virtual bool IsDamageableAt(vec2 HitPos, float HitRadius = 0.f) const { return true; }
    bool TakeDamage(int Dmg, int From, int Weapon);
    bool TakeDamageAt(vec2 HitPos, int Dmg, int From, int Weapon, float HitRadius = 0.f);
    bool IncreaseHealth(int Amount);
    bool Powered() { return m_Power; }

    float Width() { return m_Width; }
    float Height() { return m_Height; }

    int GetType() { return m_BuildingType; }
    int GetTeam() { return m_Team; }
    int GetTeamLaser() { return GetTeam() ? LASERTYPE_FREEZE : LASERTYPE_SHOTGUN; }
    int GetTeamArmor() { return GetTeam() ? POWERUP_ARMOR_LASER : POWERUP_ARMOR_SHOTGUN; }
    int GetHealth() const { return m_Health; }
    void SetHealth(int Health) { m_Health = Health; }

    float m_Width;
    float m_Height;
    float m_Angle;
    float m_VeteranTTLBonus;

    CCharacter *GetNearest(int MaxDist, int Team);

    // TODO: Do this after the Mod Jam
    struct SBuildingDIY
    {
        int m_Health;
        int m_Weapon;
        int m_MaxArmor;
        int m_MinReloadTime;
    };

private:
    int m_Health;
    int m_Team;
    int m_BuildingType;
    int m_Radius;
    int m_Hit;
    int m_Armor;
    int m_StartTick;
    int m_NumIDs;
    int m_IDs[12];
    int m_LowPowerID;
    int m_DamageTaken;
    int m_DamageTakenTick;
    int m_Decay;
    int m_ReloadTime;
    int m_MaxAimRange;
    int m_VeteranShots;
    int m_RegenTick[MAX_CLIENTS];
    int m_SupplyTick;
    bool m_Power;
};

#endif