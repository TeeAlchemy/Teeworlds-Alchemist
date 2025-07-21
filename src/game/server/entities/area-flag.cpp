/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/mapitems.h>

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

#include <engine/shared/config.h>

#include "character.h"
#include "area-flag.h"

#include <string>

/*
CAreaFlag from [Commander&Killer Remake].
or I mean ControlPoint mod
annnd Oil mod
both unfinished (
*/

CAreaFlag::CAreaFlag(CGameWorld *pWorld, vec2 Pos0, vec2 Pos1, int MaxProgress, int Level)
    : CEntity(pWorld, CGameWorld::ENTTYPE_AREA_FLAG, Pos0)
{
    m_LowerPos = Pos0;
    m_UpperPos = Pos1;
    m_LaserSnap[0] = Server()->SnapNewID();
    m_LaserSnap[1] = Server()->SnapNewID();
    m_MaxProgress = MaxProgress;
    m_Level = Level;
    m_ProduceTeam = -1;
    m_ProduceTick = 50;
    mem_zero(m_pArea, 1024 * 1024 + 1024);
    Reset();
    GameWorld()->InsertEntity(this);

    InitArea();
}

CAreaFlag::~CAreaFlag()
{
    Server()->SnapFreeID(m_LaserSnap[0]);
    Server()->SnapFreeID(m_LaserSnap[1]);
}

void CAreaFlag::Reset()
{
    m_StepX = fabs(m_LowerPos.x - m_UpperPos.x) / m_MaxProgress;
    m_StepY = fabs(m_LowerPos.y - m_UpperPos.y) / m_MaxProgress;
    m_Progress = 0;
}

void CAreaFlag::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient, m_LowerPos) && NetworkClipped(SnappingClient, m_UpperPos))
        return;

    CNetObj_Flag *pFlag = static_cast<CNetObj_Flag *>(Server()->SnapNewItem(NETOBJTYPE_FLAG, GetID(), sizeof(CNetObj_Flag)));
    if (pFlag)
    {
        if (m_LowerPos.x < m_UpperPos.x)
            pFlag->m_X = round_to_int(m_LowerPos.x + abs(m_Progress) * m_StepX) + 14.f;
        else
            pFlag->m_X = round_to_int(m_LowerPos.x + (-abs(m_Progress)) * m_StepX) + 14.f;
        if (m_LowerPos.y < m_UpperPos.y)
            pFlag->m_Y = round_to_int(m_LowerPos.y + abs(m_Progress) * m_StepY);
        else
            pFlag->m_Y = round_to_int(m_LowerPos.y + (-abs(m_Progress)) * m_StepY);

        pFlag->m_Team = (m_Progress < 0) ? TEAM_RED : TEAM_BLUE;
    }
    CNetObj_Laser *pLaser0 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_LaserSnap[0], sizeof(CNetObj_Laser)));
    if (pLaser0)
    {
        pLaser0->m_FromX = m_LowerPos.x;
        pLaser0->m_FromY = m_LowerPos.y;
        pLaser0->m_X = m_UpperPos.x;
        pLaser0->m_Y = m_UpperPos.y;
        pLaser0->m_StartTick = Server()->Tick();
    }

    CNetObj_Laser *pLaser1 = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_LaserSnap[1], sizeof(CNetObj_Laser)));
    if (pLaser1)
    {
        pLaser1->m_FromX = m_UpperPos.x;
        pLaser1->m_FromY = m_UpperPos.y;
        pLaser1->m_X = m_UpperPos.x;
        pLaser1->m_Y = m_LowerPos.y;
        pLaser1->m_StartTick = Server()->Tick();
    }
}

void CAreaFlag::Tick()
{
}

void CAreaFlag::TickDefered()
{
    int Progress = 0;
    int ClosestID = -1;
    float SmallestDistance = 999999999.f;
    for (CCharacter *p = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CCharacter *)p->TypeNext())
    {
        if (!InArea(p->GetPos()))
            continue;

        if (p->GetPlayer()->GetTeam() == TEAM_RED)
            Progress--;
        else if (p->GetPlayer()->GetTeam() == TEAM_BLUE) // I'm not sure if team = spec...(it was happened before)
            Progress++;

        if (distance(p->GetPos(), GetPos()) < SmallestDistance && p->GetPlayer()->GetTeam() == GetTeam())
        {
            SmallestDistance = distance(p->GetPos(), GetPos());
            ClosestID = p->GetPlayer()->GetCID();
        }

        if (GetTeam() == -1)
        {
            if (Server()->Tick() % 5 == 0)
            {
                GameServer()->CreateSound(m_LowerPos, SOUND_HOOK_ATTACH_GROUND);
                GameServer()->CreateSound(m_UpperPos, SOUND_HOOK_LOOP);
            }
            int Percent = (int)(((float)(abs(m_Progress)) / (float)m_MaxProgress) * 100);
            GameServer()->Broadcast(p->GetPlayer()->GetCID(), "Capturing the Resource Point! {}%", Percent);
        }
    }

    if (GameServer()->GetPlayerChar(ClosestID))
    {
        bool Send = false;
        for (int i = 0; i < NUM_RESOURCE; i++)
        {
            if (m_Product[i] > 0)
                Send = true;
            GameServer()->GetPlayerChar(ClosestID)->m_Resource[i] += m_Product[i];
            m_Product[i] = 0;
        }
        if (Send)
        {
            GameServer()->Broadcast(ClosestID, "You have collected the resources within the resource point\nBring them back to the base!");
            GameServer()->CreateSound(GetPos(), SOUND_CTF_RETURN);
            GameServer()->ClearVotes(ClosestID);
        }
    }

    if ((m_Progress + Progress) >= m_MaxProgress)
    {
        if (GetTeam() == -1)
        {
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                CPlayer *p = GameServer()->m_apPlayers[i];
                if (!p || !p->GetCharacter())
                    continue;

                if (p->GetTeam() == TEAM_RED)
                    GameServer()->CreateSound(p->GetCharacter()->GetPos(), SOUND_CTF_DROP, CmaskOne(i));
                if (p->GetTeam() == TEAM_BLUE)
                    GameServer()->CreateSound(p->GetCharacter()->GetPos(), SOUND_CTF_CAPTURE, CmaskOne(i));
            }

            GameServer()->Broadcast(-1, "The Blue Team has Captured a Resource Point!");
            GameServer()->Chat(-1, "The Blue Team has Captured a Resource Point!");
            m_ProduceTeam = TEAM_BLUE;
        }
        m_Progress = m_MaxProgress;
    }
    else if ((m_Progress + Progress) <= -m_MaxProgress)
    {
        if (GetTeam() == -1)
        {
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                CPlayer *p = GameServer()->m_apPlayers[i];
                if (!p || !p->GetCharacter())
                    continue;

                if (p->GetTeam() == TEAM_RED)
                    GameServer()->CreateSound(p->GetCharacter()->GetPos(), SOUND_CTF_CAPTURE, CmaskOne(i));
                if (p->GetTeam() == TEAM_BLUE)
                    GameServer()->CreateSound(p->GetCharacter()->GetPos(), SOUND_CTF_DROP, CmaskOne(i));
            }
            GameServer()->Broadcast(-1, "The Red Team has Captured a Resource Point!");
            GameServer()->Chat(-1, "The Red Team has Captured a Resource Point!");
            m_ProduceTeam = TEAM_RED;
        }
        m_Progress = -m_MaxProgress;
    }
    else
        m_Progress += Progress;

    HandleProduce();

    for (CBuilding *pBuilding = (CBuilding *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_BUILDINGS); pBuilding; pBuilding = (CBuilding *)pBuilding->TypeNext())
    {
        if (!pBuilding->Powered())
            continue;

        if (pBuilding->GetTeam() != GetTeam())
            continue;

        if (!InArea(pBuilding->GetPos()))
            continue;

        for (int i = 0; i < NUM_RESOURCE; i++)
        {
            GameServer()->m_pController->m_aTeamResources[GetTeam()][i] += m_Product[i];
            m_Product[i] = 0;
        }
    }
}

int CAreaFlag::GetTeam()
{
    if (m_Progress >= m_MaxProgress)
        return TEAM_BLUE;
    else if (m_Progress <= -m_MaxProgress)
        return TEAM_RED;
    return -1;
}

int CAreaFlag::GetProduceTeam()
{
    return m_ProduceTeam;
}

void CAreaFlag::InitArea()
{
    Search(vec2(GetPos().x / 32.f - 1, GetPos().y / 32.f - 1), DUP);
}

void CAreaFlag::Search(vec2 StartPos, int DirType)
{
    switch (DirType)
    {
    case DUP:
        for (int y = round_to_int(StartPos.y);; y--)
        {
            if (m_pArea[GameServer()->Collision()->CalcTile(StartPos.x, y)])
                break;
            else if (GameServer()->Collision()->GetTileIndex(round_to_int(StartPos.x), y) == TILE_RESOURCE)
            {
                m_pArea[GameServer()->Collision()->CalcTile(StartPos.x, y)] = true;
                Search(vec2(StartPos.x - 1, y), DLEFT);
                Search(vec2(StartPos.x + 1, y), DRIGHT);
            }
            else
                break;
        }

        break;

    case DDOWN:
        for (int y = round_to_int(StartPos.y);; y++)
        {
            if (m_pArea[GameServer()->Collision()->CalcTile(StartPos.x, y)])
                break;
            else if (GameServer()->Collision()->GetTileIndex(round_to_int(StartPos.x), y) == TILE_RESOURCE)
            {
                m_pArea[GameServer()->Collision()->CalcTile(StartPos.x, y)] = true;
                Search(vec2(StartPos.x - 1, y), DLEFT);
                Search(vec2(StartPos.x + 1, y), DRIGHT);
            }
            else
                break;
        }

        break;

    case DLEFT:
        for (int x = round_to_int(StartPos.x);; x--)
        {
            if (m_pArea[GameServer()->Collision()->CalcTile(x, StartPos.y)])
                break;
            else if (GameServer()->Collision()->GetTileIndex(x, round_to_int(StartPos.y)) == TILE_RESOURCE)
            {
                m_pArea[GameServer()->Collision()->CalcTile(x, StartPos.y)] = true;
                Search(vec2(x, StartPos.y - 1), DUP);
                Search(vec2(x, StartPos.y + 1), DDOWN);
            }
            else
                break;
        }

        break;

    case DRIGHT:
        for (int x = round_to_int(StartPos.x);; x++)
        {
            if (m_pArea[GameServer()->Collision()->CalcTile(x, StartPos.y)])
                break;
            else if (GameServer()->Collision()->GetTileIndex(x, round_to_int(StartPos.y)) == TILE_RESOURCE)
            {
                m_pArea[GameServer()->Collision()->CalcTile(x, StartPos.y)] = true;
                Search(vec2(x, StartPos.y - 1), DUP);
                Search(vec2(x, StartPos.y + 1), DDOWN);
            }
            else
                break;
        }

        break;

    default:
        break;
    }
}

void CAreaFlag::HandleProduce()
{
    if (m_ProduceTick > 0 && GetTeam() != -1)
        m_ProduceTick--;

    if (m_ProduceTick <= 0)
    {
        switch (m_Level)
        {
        // 2-4s 获得 1-10个木头 1-5个煤炭 1-6个铜矿
        case LWOODCOALCOPPER:
            m_Product[RESOURCE_WOOD] += random_int(1, 10);
            m_Product[RESOURCE_COAL] += random_int(1, 5);
            m_Product[RESOURCE_COPPER] += random_int(1, 6);
            m_ProduceTick = random_int(100, 200);
            break;

        // 3-6s 获得 1-4个铁矿 1-3个金矿
        case LIRONGOLD:
            m_Product[RESOURCE_IRON] += random_int(1, 4);
            m_Product[RESOURCE_GOLD] += random_int(1, 3);
            m_ProduceTick = random_int(150, 300);
            break;

        // 6-11s 获得 1-2个钻石 1个能量
        case LDIAMONDENEGRY:
            m_Product[RESOURCE_DIAMOND] += random_int(1, 2);
            m_Product[RESOURCE_ENEGRY] += 1;
            m_ProduceTick = random_int(300, 550);
            break;

        default:
            break;
        }
    }
}

bool CAreaFlag::InArea(vec2 Pos)
{
    return m_pArea[GameServer()->Collision()->CalcTileRaw(Pos.x, Pos.y)];
}