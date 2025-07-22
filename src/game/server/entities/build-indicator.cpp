#include <game/server/gamecontext.h>
#include "build-indicator.h"

CBuildIndicator::CBuildIndicator(CGameWorld *pGameWorld)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_BUILDINDICATOR, vec2(0.f, 0.f))
{
    mem_zero(m_Snap, MAX_CLIENTS);

    for (int i = 0; i < 12; i++)
        m_IDs[i] = Server()->SnapNewID();

    GameWorld()->InsertEntity(this);
}

void CBuildIndicator::Tick()
{
}

void CBuildIndicator::Snap(int SnappingClient)
{
    int BuildingType = GameServer()->GetPlayer(SnappingClient)->m_SelectBuilding;
    if (!m_Snap[SnappingClient] || BuildingType == -1 || !GameServer()->GetPlayerChar(SnappingClient))
        return;
    

    CBuildingInfo::SBuildingInfo Info = GameServer()->m_pBuildingsInfo->m_aBuildingsInfo[BuildingType];
    int Team = GameServer()->GetPlayer(SnappingClient)->GetTeam();

    switch (BuildingType)
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
            pLaser->m_Type = GetTeamLaser(Team);
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
        float Time = (Server()->Tick() - GameServer()->m_pController->RoundStartTick()) / (float)Server()->TickSpeed();
        float Angle = fmodf(Time * pi / 2, 2.0f * pi);
        float ShiftedAngle = Angle + pi;
        float AngleStep = 2.0f * pi / CBuilding::NUMID_SHIELD;
        CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(GetID());
        if (pLaser)
        {
            int Len = Info.m_Radius;
            int Type = LASERTYPE_PLASMA;
            pLaser->m_FromX = round_to_int(GetPos().x + Len * cos(Angle));
            pLaser->m_FromY = round_to_int(GetPos().y + Len * sin(Angle));
            pLaser->m_ToX = round_to_int(GetPos().x + Len * cos(ShiftedAngle));
            pLaser->m_ToY = round_to_int(GetPos().y + Len * sin(ShiftedAngle));
            pLaser->m_Type = Type;
        }

        for (int i = 0; i < CBuilding::NUMID_SHIELD; i++)
        {
            vec2 PartPosStart = m_Pos + vec2(Info.m_Radius * cos(AngleStep * i), Info.m_Radius * sin(AngleStep * i));
            vec2 PartPosEnd = m_Pos + vec2(Info.m_Radius * cos(AngleStep * (i + 1)), Info.m_Radius * sin(AngleStep * (i + 1)));

            CNetObj_DDNetLaser *pObj = Server()->SnapNewItem<CNetObj_DDNetLaser>(m_IDs[i]);
            if (!pObj)
                return;

            pObj->m_ToX = (int)PartPosStart.x;
            pObj->m_ToY = (int)PartPosStart.y;
            pObj->m_FromX = (int)PartPosEnd.x;
            pObj->m_FromY = (int)PartPosEnd.y;
            pObj->m_Type = GetTeamLaser(Team);
        }
    }
    break;

    case BUILDING_SHOTGUN:
    case BUILDING_GRENADE:
    case BUILDING_LASER:
    {
        vec2 Vertices[4] = {
            vec2(GetPos().x - Info.m_Radius, GetPos().y - Info.m_Radius),
            vec2(GetPos().x + Info.m_Radius, GetPos().y - Info.m_Radius),
            vec2(GetPos().x + Info.m_Radius, GetPos().y + Info.m_Radius),
            vec2(GetPos().x - Info.m_Radius, GetPos().y + Info.m_Radius)};

        for (int i = 0; i < CBuilding::NUMID_WEAPONBOX; i++)
        {
            CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(m_IDs[i]);
            if (!pLaser)
                continue;
            pLaser->m_FromX = Vertices[i].x;
            pLaser->m_FromY = Vertices[i].y;
            pLaser->m_ToX = Vertices[i == 3 ? 0 : i + 1].x;
            pLaser->m_ToY = Vertices[i == 3 ? 0 : i + 1].y;
            pLaser->m_Type = GetTeamLaser(Team);
        }

        CNetObj_DDNetPickup *pWeapon = Server()->SnapNewItem<CNetObj_DDNetPickup>(GetID());
        if (pWeapon)
        {
            pWeapon->m_X = round_to_int(GetPos().x);
            pWeapon->m_Y = round_to_int(GetPos().y);
            if (BuildingType == BUILDING_SHOTGUN)
                pWeapon->m_Type = POWERUP_ARMOR_SHOTGUN;
            else
                pWeapon->m_Type = BuildingType == BUILDING_GRENADE ? POWERUP_ARMOR_GRENADE : POWERUP_ARMOR_LASER;
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

        int FrameType = BuildingType == BUILDING_TURRET_GUN ? POWERUP_ARMOR : POWERUP_ARMOR_SHOTGUN;
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
            pFrame->m_FromX = round_to_int(GetPos().x + 45.f * cos(0.f));
            pFrame->m_FromY = round_to_int(GetPos().y + 45.f * sin(0.f) - 75.f);
            pFrame->m_Type = GetTeamLaser(Team);
            pFrame->m_Subtype = -1;
        }
    }
    break;

    case BUILDING_HEALTH:
    case BUILDING_ARMOR:
    {
        vec2 Vertices[4] = {
            vec2(GetPos().x - Info.m_Radius, GetPos().y - Info.m_Radius),
            vec2(GetPos().x + Info.m_Radius, GetPos().y - Info.m_Radius),
            vec2(GetPos().x + Info.m_Radius, GetPos().y + Info.m_Radius),
            vec2(GetPos().x - Info.m_Radius, GetPos().y + Info.m_Radius)};

        for (int i = 0; i < CBuilding::NUMID_PICKUP; i++)
        {
            CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(m_IDs[i]);
            if (!pLaser)
                continue;
            pLaser->m_FromX = Vertices[i].x;
            pLaser->m_FromY = Vertices[i].y;
            pLaser->m_ToX = Vertices[i == 3 ? 0 : i + 1].x;
            pLaser->m_ToY = Vertices[i == 3 ? 0 : i + 1].y;
            pLaser->m_Type = GetTeamLaser(Team);
        }

        CNetObj_DDNetPickup *pWeapon = Server()->SnapNewItem<CNetObj_DDNetPickup>(GetID());
        if (pWeapon)
        {
            pWeapon->m_X = round_to_int(GetPos().x);
            pWeapon->m_Y = round_to_int(GetPos().y);
            if (BuildingType == BUILDING_HEALTH)
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
            pLaser->m_Type = GetTeamLaser(Team);
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

void CBuildIndicator::Reset()
{
    
}