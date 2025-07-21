#include <game/server/gamecontext.h>
#include "projectile.h"
#include "character.h"
#include "flying-pickup.h"
#include "fakelaser.h"
#include "buildings.h"

/*
placeholder("Node");placeholder("Can provide electricity and transport buildings.")
placeholder("Shield");placeholder("Protect you and your buildings from bullets.")
placeholder("Workbench");
placeholder("ShotgunBox");placeholder("Provide shotgun for you.")
placeholder("GrenadeBox");placeholder("Provide grenade for you.")
placeholder("LaserBox");placeholder("Provide laser for you.")
placeholder("TurretGun");placeholder("Auto shoot the nearest enemy with a pistol.")
placeholder("TurretShotgun");placeholder("Auto shoot the nearest enemy with a shotgun.")
placeholder("HealthBox");placeholder("How lovely, shoot heart for you.")
placeholder("ArmorBox");placeholder("How strong, shoot armor for you.")
placeholder("Gatherer");placeholder("Build it at a resource point can auto collect.")
*/
CBuildingInfo::CBuildingInfo(CGameContext *pGameServer)
{
    m_pGameServer = pGameServer;

    Register(BUILDING_NODE, {"Node", "Can provide electricity and transport buildings.", 40, 500.f, 25.f, 75.f, {10, 2, 10, 0, 0, 0, 0}});
    Register(BUILDING_SHIELD, {"Shield", "Protect you and your buildings from bullets.", 45, 250.f / 2.f, 50.f, 50.f, {10, 10, 0, 0, 0, 0, 0}});
    Register(BUILDING_WORKBENCH, {"Workbench", "YOU SHOULDN'T BE HERE", 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0}});
    Register(BUILDING_SHOTGUN, {"ShotgunBox", "Provide shotgun for you.", 25, 25.f, 50.f, 50.f, {10, 5, 15, 40, 0, 0, 0}});
    Register(BUILDING_GRENADE, {"GrenadeBox", "Provide grenade for you.", 25, 25.f, 50.f, 50.f, {10, 25, 15, 50, 0, 0, 0}});
    Register(BUILDING_LASER, {"LaserBox", "Provide laser for you.", 30, 25.f, 50.f, 50.f, {10, 50, 30, 0, 0, 4, 1}});
    Register(BUILDING_TURRET_GUN, {"TurretGun", "Auto shoot the nearest enemy with a pistol.", 30, 45.f, 50.f, 140.f, {40, 5, 15, 40, 20, 3, 5}});
    Register(BUILDING_TURRET_SHOTGUN, {"TurretShotgun", "Auto shoot the nearest enemy with a shotgun.", 30, 45.f, 50.f, 140.f, {40, 25, 15, 50, 20, 5, 10}});
    Register(BUILDING_HEALTH, {"HealthBox", "How lovely, shoot heart for you.", 50, 25.f, 50.f, 50.f, {50, 40, 30, 40, 20, 1, 5}});
    Register(BUILDING_ARMOR, {"ArmorBox", "How strong, shoot armor for you.", 50, 25.f, 50.f, 50.f, {50, 40, 30, 40, 20, 1, 5}});
    Register(BUILDING_GATHERER, {"Gatherer", "Build it at a resource point can auto collect.", 80, 32.f, 25.f, 75.f, {70, 40, 70, 60, 20, 5, 10}});
}

void CBuildingInfo::Register(int ID, SBuildingInfo Data)
{
    m_aBuildingsInfo[ID] = Data;
}

// TODO: Write RegisterBuildings();
CBuilding::CBuilding(CGameWorld *pGameWorld, int Team, vec2 Pos, int Type)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_BUILDINGS, Pos)
{
    m_Team = Team;
    m_StartTick = Server()->Tick();
    m_BuildingType = Type;
    m_VeteranTTLBonus = 1.f;
    m_Power = false;
    m_Health = GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Type].m_Health;
    m_Radius = GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Type].m_Radius;
    m_Width = GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Type].m_Width;
    m_Height = GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[Type].m_Height;

    mem_zero(m_RegenTick, 64);

    switch (m_BuildingType)
    {
    case BUILDING_SHIELD:
        m_NumIDs = NUMID_SHIELD;
        break;

    case BUILDING_SHOTGUN:
    case BUILDING_GRENADE:
    case BUILDING_LASER:
        m_NumIDs = NUMID_WEAPONBOX;
        break;

    case BUILDING_TURRET_GUN:
        m_NumIDs = NUMID_TURRET;
        m_MaxAimRange = 700;
        m_ReloadTime = 12;
        break;

    case BUILDING_TURRET_SHOTGUN:
        m_NumIDs = NUMID_TURRET;
        m_MaxAimRange = 500;
        m_ReloadTime = 30;
        break;

    case BUILDING_HEALTH:
    case BUILDING_ARMOR:
        m_NumIDs = NUMID_PICKUP;
        break;

    case BUILDING_GATHERER:
        m_NumIDs = NUMID_GATHERER;
        break;

    case BUILDING_NODE:
        m_NumIDs = NUMID_NODE;
        break;

    /*case BUILDING_POWER:
        m_Health = 30;
        m_NumIDs = NUMID_POWER;
        m_Radius = 25.f;
        m_Width = 25.f;
        m_Height = 75.f;
        break;*/
    default:
        break;
    }

    for (int i = 0; i < m_NumIDs; i++)
        m_IDs[i] = Server()->SnapNewID();

    m_TeamID = Server()->SnapNewID();
    m_LowPowerID = Server()->SnapNewID();

    GameWorld()->InsertEntity(this);
}

void CBuilding::Tick()
{
    if (m_BuildingType == BUILDING_NODE)
    {
        for (CBuilding *pBuilding = (CBuilding *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_BUILDINGS); pBuilding; pBuilding = (CBuilding *)pBuilding->TypeNext())
        {
            if (pBuilding == this)
                continue;

            // TODO: Done this.
            /*if (pBuilding->GetType() != BUILDING_NODE)
                continue;

            vec2 NodePos = {GetPos().x, GetPos().y - 100.f};
            vec2 TargetPos = {pBuilding->GetPos().x, pBuilding->GetPos().y - 100.f};

            // too far
            if(distance(NodePos, TargetPos) > m_Radius)
                continue;

            // block
            if (GameServer()->Collision()->IntersectLine(NodePos, TargetPos, NULL, NULL))
                continue;

            // foe
            if (GetTeam() != pBuilding->GetTeam())
                continue;

            new CFakeLaser(GameWorld(), NodePos, TargetPos, GetTeamLaser());
            */

            vec2 NodePos = {GetPos().x, GetPos().y - 100.f};

            if (distance(NodePos, pBuilding->GetPos()) > m_Radius)
                continue;

            /* No need for check this. (I mean, through the wall, 5G, or smth, im kidding, but, hm, you know that, i guess, yes, that what I mean(WHAT AM I TALKING ABOUT WHAT THE HELL SORRY FOR THAT LOT BUT I MEAN YOU KNOW I JUST AHHHH IM JUST KIDDING OK FINE THAT ALLLLLLLLL)(:D))
            if (GameServer()->Collision()->IntersectLine(NodePos, pBuilding->GetPos(), NULL, NULL))
                continue;*/

            if (GetTeam() != pBuilding->GetTeam())
                continue;

            pBuilding->m_Power = true;
        }
    }
}

void CBuilding::TickDefered()
{
    if (m_Decay > 0)
        m_Decay--;

    if (!m_Power)
        m_StartTick++;

    switch (m_BuildingType)
    {
    case BUILDING_SHIELD:
    {
        // Low power.
        if (!m_Power)
        {
            m_Armor = 0;
            break;
        }

        if (Server()->Tick() % Server()->TickSpeed() == 0)
        {
            if (m_Armor < 30)
                m_Armor++;
        }

        if (Server()->Tick() % 5 == 0)
        {
            if (m_Armor > 0)
            {
                CProjectile *pProj[128];
                int Num = GameServer()->m_World.FindEntities(m_Pos, m_Radius * 2.f, (CEntity **)pProj, 128, CGameWorld::ENTTYPE_PROJECTILE);
                for (int i = 0; i < Num && m_Armor > 0; i++)
                {
                    if ((pProj[i]->GetOwner() >= 0) && GameServer()->m_apPlayers[pProj[i]->GetOwner()] && GameServer()->m_apPlayers[pProj[i]->GetOwner()]->GetTeam() == m_Team)
                        continue;

                    if (pProj[i]->GetOwner() < 0)
                        continue;

                    pProj[i]->m_InitPos = pProj[i]->GetRealPos();
                    pProj[i]->m_StartTick = Server()->Tick();
                    pProj[i]->m_Direction = normalize(pProj[i]->GetRealPos() - m_Pos);
                    pProj[i]->m_Team = GetTeam();

                    m_Armor = clamp(m_Armor - (pProj[i]->m_Weapon == WEAPON_GRENADE ? 4 : 2), 0, 30);
                    if (m_Armor == 0)
                        m_Armor = -4;
                }
            }
        }
    }
    break;
    case BUILDING_SHOTGUN:
    case BUILDING_GRENADE:
    case BUILDING_LASER:
    {
        // Low power.
        if (!m_Power)
            break;

        CCharacter *pChr[MAX_CLIENTS];
        int Num = GameServer()->m_World.FindEntities(m_Pos, m_Radius, (CEntity **)pChr, 128, CGameWorld::ENTTYPE_CHARACTER);
        for (int i = 0; i < Num; i++)
        {
            if (pChr[i]->GetPlayer()->GetTeam() != m_Team)
                continue;

            int WeaponType = WEAPON_SHOTGUN;
            if (m_BuildingType == BUILDING_GRENADE)
                WeaponType = WEAPON_GRENADE;
            else
                WeaponType = WEAPON_RIFLE;

            if (pChr[i]->GetWeaponAmmo(WeaponType) < 10)
            {
                pChr[i]->GiveWeapon(WeaponType, 10);
                GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);

                if (pChr[i]->GetPlayer())
                    GameServer()->SendWeaponPickup(pChr[i]->GetPlayer()->GetCID(), WeaponType);
            }
        }
    }
    break;

    case BUILDING_TURRET_GUN:
    {
        // Low power.
        if (!m_Power)
            break;

        vec2 RealPos = vec2(m_Pos.x, m_Pos.y - 74);
        vec2 TempPos = m_Pos;
        m_Pos = RealPos;

        CCharacter *pChr = GetNearest(m_MaxAimRange, m_Team ^ 1);
        m_Pos = TempPos;

        if (pChr)
        {
            m_Angle = angle(normalize(pChr->GetPos() - RealPos));
            if (m_Decay == 0)
            {
                m_VeteranShots++;
                if (m_VeteranShots == 100)
                {
                    m_ReloadTime /= 2;
                    m_MaxAimRange += 100;
                }
                else if (m_VeteranShots == 200)
                    m_MaxAimRange += 25;

                m_Decay = m_ReloadTime;
                float a = m_Angle;
                a += frandom() * 0.2f - 0.1f;

                new CProjectile(GameWorld(), WEAPON_GUN,
                                -1, RealPos,
                                vec2(cosf(a), sinf(a)),
                                (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GunLifetime),
                                1, false, 0, -1, WEAPON_GUN, GetTeam());
                GameServer()->CreateSound(RealPos, SOUND_GUN_FIRE);
            }
        }
    }
    break;

    case BUILDING_TURRET_SHOTGUN:
    {
        // Low power.
        if (!m_Power)
            break;

        vec2 RealPos = vec2(m_Pos.x, m_Pos.y - 66);
        vec2 TempPos = m_Pos;
        m_Pos = RealPos;

        CCharacter *pChr = GetNearest(m_MaxAimRange, m_Team ^ 1);
        m_Pos = TempPos;

        if (pChr)
        {
            m_Angle = angle(normalize(pChr->GetPos() - RealPos));
            if (m_Decay == 0)
            {
                m_VeteranShots++;
                if (m_VeteranShots == 100)
                {
                    m_ReloadTime = m_ReloadTime - ((m_ReloadTime / 10) * 2);
                    m_MaxAimRange += 100;
                    m_VeteranTTLBonus = 1.8f;
                }
                else if (m_VeteranShots == 200)
                {
                    m_ReloadTime = m_ReloadTime - ((m_ReloadTime / 10) * 2);
                    m_MaxAimRange += 25;
                }

                m_Decay = m_ReloadTime;

                int ShotSpread = 2;

                for (int i = -ShotSpread; i <= ShotSpread; i++)
                {
                    float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
                    float a = m_Angle;
                    a += Spreading[i + 2];
                    float v = 1 - (abs(i) / (float)ShotSpread);
                    float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

                    new CProjectile(GameWorld(), WEAPON_SHOTGUN,
                                    -1, RealPos,
                                    vec2(cosf(a), sinf(a)) * Speed,
                                    (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_ShotgunLifetime * m_VeteranTTLBonus),
                                    1, false, 0, -1, WEAPON_SHOTGUN, GetTeam());
                }
                GameServer()->CreateSound(RealPos, SOUND_SHOTGUN_FIRE);
            }
        }
    }
    break;

    case BUILDING_HEALTH:
    case BUILDING_ARMOR:
    {
        // Low power.
        if (!m_Power)
            break;

        CCharacter *pChr[MAX_CLIENTS];
        int Num = GameServer()->m_World.FindEntities(m_Pos, 340.f, (CEntity **)pChr, 64, CGameWorld::ENTTYPE_CHARACTER);
        for (int i = 0; i < Num; i++)
        {
            int CID = pChr[i]->GetPlayer()->GetCID();
            if (pChr[i]->GetPlayer()->GetTeam() != m_Team)
                continue;

            if (m_RegenTick[CID] > 0)
            {
                m_RegenTick[CID]--;
                continue;
            }

            new CFlyingPickup(GameWorld(), GetPos(), m_BuildingType == BUILDING_HEALTH ? POWERUP_HEALTH : POWERUP_ARMOR, -1, CID);
            m_RegenTick[CID] = 100;
        }
    }
    break;

    case BUILDING_GATHERER:
        break;

    case BUILDING_NODE:
    {
        vec2 NodePos = {GetPos().x, GetPos().y - 100.f};
        for (CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
        {
            if (pChr->GetPlayer()->GetTeam() != GetTeam())
                continue;

            if (distance(NodePos, pChr->GetPos()) > m_Radius)
                continue;

            pChr->m_CanBuild = true;
        }
    }
    break;

    default:
        break;
    }

    m_Power = false;
}

CBuilding::~CBuilding()
{
    for (int i = 0; i < m_NumIDs; i++)
        Server()->SnapFreeID(m_IDs[i]);

    Server()->SnapFreeID(m_TeamID);
    Server()->SnapFreeID(m_LowPowerID);
}

void CBuilding::Reset()
{
    MarkForDestroy();
}

void CBuilding::Snap(int SnappingClient)
{
    switch (m_BuildingType)
    {
    case BUILDING_NODE:
    {
        CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(GetID());
        if (pLaser)
        {
            pLaser->m_FromX = round_to_int(GetPos().x);
            pLaser->m_FromY = round_to_int(GetPos().y);
            pLaser->m_ToX = round_to_int(GetPos().x);
            pLaser->m_ToY = round_to_int(GetPos().y - 75.f);
            pLaser->m_Type = GetTeamLaser();
        }

        CNetObj_DDNetPickup *pPickup = Server()->SnapNewItem<CNetObj_DDNetPickup>(m_IDs[0]);
        if (pPickup)
        {
            pPickup->m_X = round_to_int(GetPos().x);
            pPickup->m_Y = round_to_int(GetPos().y - 100.f);
            pPickup->m_Type = POWERUP_ARMOR_LASER;
            pPickup->m_Subtype = -1;
        }
    }
    break;

    case BUILDING_SHIELD:
    {
        float Time = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();
        float Angle = fmodf(Time * pi / 2, 2.0f * pi);
        float ShiftedAngle = Angle + pi;
        float AngleStep = 2.0f * pi / NUMID_SHIELD;
        CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(GetID());
        if (pLaser)
        {
            int Len = m_Radius / 30 * m_Health;
            int Type = LASERTYPE_PLASMA;
            if (m_Armor > 0)
            {
                Len = m_Radius / 30 * m_Armor;
                Type = LASERTYPE_DOOR;
            }
            pLaser->m_FromX = round_to_int(GetPos().x + Len * cos(Angle));
            pLaser->m_FromY = round_to_int(GetPos().y + Len * sin(Angle));
            pLaser->m_ToX = round_to_int(GetPos().x + Len * cos(ShiftedAngle));
            pLaser->m_ToY = round_to_int(GetPos().y + Len * sin(ShiftedAngle));
            pLaser->m_Type = Type;
        }

        for (int i = 0; i < NUMID_SHIELD; i++)
        {
            vec2 PartPosStart = m_Pos + vec2(m_Radius * cos(AngleStep * i), m_Radius * sin(AngleStep * i));
            vec2 PartPosEnd = m_Pos + vec2(m_Radius * cos(AngleStep * (i + 1)), m_Radius * sin(AngleStep * (i + 1)));

            CNetObj_DDNetLaser *pObj = Server()->SnapNewItem<CNetObj_DDNetLaser>(m_IDs[i]);
            if (!pObj)
                return;

            pObj->m_ToX = (int)PartPosStart.x;
            pObj->m_ToY = (int)PartPosStart.y;
            pObj->m_FromX = (int)PartPosEnd.x;
            pObj->m_FromY = (int)PartPosEnd.y;
            pObj->m_Type = GetTeamLaser();
        }
    }
    break;

    case BUILDING_SHOTGUN:
    case BUILDING_GRENADE:
    case BUILDING_LASER:
    {
        vec2 Vertices[4] = {
            vec2(GetPos().x - m_Radius, GetPos().y - m_Radius),
            vec2(GetPos().x + m_Radius, GetPos().y - m_Radius),
            vec2(GetPos().x + m_Radius, GetPos().y + m_Radius),
            vec2(GetPos().x - m_Radius, GetPos().y + m_Radius)};

        for (int i = 0; i < NUMID_WEAPONBOX; i++)
        {
            CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(m_IDs[i]);
            if (!pLaser)
                continue;
            pLaser->m_FromX = Vertices[i].x;
            pLaser->m_FromY = Vertices[i].y;
            pLaser->m_ToX = Vertices[i == 3 ? 0 : i + 1].x;
            pLaser->m_ToY = Vertices[i == 3 ? 0 : i + 1].y;
            pLaser->m_Type = GetTeamLaser();
        }

        CNetObj_DDNetPickup *pWeapon = Server()->SnapNewItem<CNetObj_DDNetPickup>(GetID());
        if (pWeapon)
        {
            pWeapon->m_X = round_to_int(GetPos().x);
            pWeapon->m_Y = round_to_int(GetPos().y);
            if (m_BuildingType == BUILDING_SHOTGUN)
                pWeapon->m_Type = POWERUP_ARMOR_SHOTGUN;
            else
                pWeapon->m_Type = m_BuildingType == BUILDING_GRENADE ? POWERUP_ARMOR_GRENADE : POWERUP_ARMOR_LASER;
            pWeapon->m_Subtype = -1;
        }
    }
    break;

    case BUILDING_TURRET_GUN:
    case BUILDING_TURRET_SHOTGUN:
    {
        vec2 Points[4] = {
            vec2(GetPos().x - 25.f, GetPos().y),
            vec2(GetPos().x + 25.f, GetPos().y),
            vec2(GetPos().x, GetPos().y - 15.f),
            vec2(GetPos().x, GetPos().y - 40.f)};

        int FrameType = m_BuildingType == BUILDING_TURRET_GUN ? POWERUP_ARMOR : POWERUP_ARMOR_SHOTGUN;
        int Types[4] = {FrameType, FrameType, POWERUP_HEALTH, POWERUP_HEALTH};

        for (int i = 0; i < 4; i++)
        {
            CNetObj_DDNetPickup *pFrame = Server()->SnapNewItem<CNetObj_DDNetPickup>(m_IDs[i]);
            if (!pFrame)
                continue;
            pFrame->m_X = round_to_int(Points[i].x);
            pFrame->m_Y = round_to_int(Points[i].y);
            pFrame->m_Type = Types[i];
            pFrame->m_Subtype = -1;
        }
        CNetObj_DDNetLaser *pFrame = Server()->SnapNewItem<CNetObj_DDNetLaser>(GetID());
        if (pFrame)
        {
            pFrame->m_ToX = round_to_int(GetPos().x);
            pFrame->m_ToY = round_to_int(GetPos().y - 75.f);
            pFrame->m_FromX = round_to_int(GetPos().x + 45.f * cos(m_Angle));
            pFrame->m_FromY = round_to_int(GetPos().y + 45.f * sin(m_Angle) - 75.f);
            pFrame->m_Type = GetTeamLaser();
            pFrame->m_Subtype = -1;
        }
    }
    break;

    case BUILDING_HEALTH:
    case BUILDING_ARMOR:
    {
        vec2 Vertices[4] = {
            vec2(GetPos().x - m_Radius, GetPos().y - m_Radius),
            vec2(GetPos().x + m_Radius, GetPos().y - m_Radius),
            vec2(GetPos().x + m_Radius, GetPos().y + m_Radius),
            vec2(GetPos().x - m_Radius, GetPos().y + m_Radius)};

        for (int i = 0; i < NUMID_PICKUP; i++)
        {
            CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(m_IDs[i]);
            if (!pLaser)
                continue;
            pLaser->m_FromX = Vertices[i].x;
            pLaser->m_FromY = Vertices[i].y;
            pLaser->m_ToX = Vertices[i == 3 ? 0 : i + 1].x;
            pLaser->m_ToY = Vertices[i == 3 ? 0 : i + 1].y;
            pLaser->m_Type = GetTeamLaser();
        }

        CNetObj_DDNetPickup *pWeapon = Server()->SnapNewItem<CNetObj_DDNetPickup>(GetID());
        if (pWeapon)
        {
            pWeapon->m_X = round_to_int(GetPos().x);
            pWeapon->m_Y = round_to_int(GetPos().y);
            if (m_BuildingType == BUILDING_HEALTH)
                pWeapon->m_Type = POWERUP_HEALTH;
            else
                pWeapon->m_Type = POWERUP_ARMOR;
            pWeapon->m_Subtype = -1;
        }
    }
    break;

    case BUILDING_GATHERER:
    {
        CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(GetID());
        if (pLaser)
        {
            pLaser->m_FromX = round_to_int(GetPos().x);
            pLaser->m_FromY = round_to_int(GetPos().y);
            pLaser->m_ToX = round_to_int(GetPos().x);
            pLaser->m_ToY = round_to_int(GetPos().y - 75.f);
            pLaser->m_Type = GetTeamLaser();
        }

        CNetObj_DDNetPickup *pPickup = Server()->SnapNewItem<CNetObj_DDNetPickup>(m_IDs[0]);
        if (pPickup)
        {
            pPickup->m_X = round_to_int(GetPos().x);
            pPickup->m_Y = round_to_int(GetPos().y - 100.f);
            pPickup->m_Type = POWERUP_ARMOR_NINJA;
            pPickup->m_Subtype = -1;
        }
    }
    break;

    default:
        break;
    }
}

bool CBuilding::TakeDamage(int Dmg, int From, int Weapon)
{
    if (From >= 0 && GameServer()->m_apPlayers[From] && GameServer()->m_apPlayers[From]->GetTeam() == m_Team)
        return false;

    if (m_Health <= 0)
        return false;

    m_DamageTaken++;

    // create healthmod indicator
    if (Server()->Tick() < m_DamageTakenTick + 25)
        GameServer()->CreateDamageInd(m_Pos, m_DamageTaken * 0.25f, Dmg);
    else
    {
        m_DamageTaken = 0;
        GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
    }

    if (Dmg)
        m_Health -= Dmg;

    m_DamageTakenTick = Server()->Tick();

    // do damage hit sound
    if (From >= 0 && GameServer()->m_apPlayers[From])
        GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, CmaskOne(From));

    // check for death
    if (m_Health <= 0)
    {
        // set attacker's face to happy (taunt!)
        if (From >= 0 && GameServer()->m_apPlayers[From])
        {
            CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
            if (pChr && GameServer()->m_apPlayers[From]->GetTeam() != m_Team)
                pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
        }

        GameServer()->CreateExplosion(GetPos(), From, Weapon, false);
        Reset();

        return false;
    }

    return true;
}

bool CBuilding::IncreaseHealth(int Amount)
{
    if (GetHealth() >= GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[GetType()].m_Health)
        return false;

    m_Health += Amount;

    if (GetHealth() >= GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[GetType()].m_Health)
        m_Health = GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[GetType()].m_Health;

    return true;
}

CCharacter *CBuilding::GetNearest(int MaxDist, int Team)
{
    CCharacter *CloseCharacters[MAX_CLIENTS];
    int Num = GameServer()->m_World.FindEntities(m_Pos, MaxDist, (CEntity **)CloseCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
    int Disti = -1;
    float Dist = 10000000.0f;
    for (int i = 0; i < Num; i++)
    {
        if (!CloseCharacters[i])
            continue;

        if (CloseCharacters[i]->GetPlayer()->GetTeam() != Team || GameServer()->Collision()->IntersectLine(m_Pos, CloseCharacters[i]->GetPos(), 0x0, 0x0))
            continue;

        float d = distance(CloseCharacters[i]->GetPos(), m_Pos);
        if (d < Dist)
        {
            Disti = i;
            Dist = d;
        }
    }

    if (Disti == -1)
        return 0;

    return CloseCharacters[Disti];
}