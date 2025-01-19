#include "tower-main.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <new>
#include <engine/shared/config.h>

#define PickupPhysSizeS 14
#define M_PI 3.14159265358979323846
CTowerMain::CTowerMain(CGameWorld *pGameWorld, vec2 StandPos)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_TOWERMAIN, StandPos, PickupPhysSizeS)
{
    m_Pos = StandPos;

    for (unsigned i = 0; i < sizeof(m_aIDs) / sizeof(int); i++)
        m_aIDs[i] = Server()->SnapNewID();

    for (int i = 0; i < s_TowerNumSide; i++)
    {
        m_alIDs[i] = Server()->SnapNewID();
    }

    m_FlagID = Server()->SnapNewID();

    GameWorld()->InsertEntity(this);

    m_Health = g_Config.m_SvMaxTowerHealth;
}

CTowerMain::~CTowerMain()
{
    for (unsigned i = 0; i < sizeof(m_aIDs) / sizeof(int); i++)
    {
        if (m_aIDs[i] >= 0)
        {
            Server()->SnapFreeID(m_aIDs[i]);
            m_aIDs[i] = -1;
        }
    }
    Server()->SnapFreeID(m_FlagID);

    for (int i = 0; i < s_TowerNumSide; i++)
    {
        Server()->SnapFreeID(m_alIDs[i]);
    }
}

void CTowerMain::Tick()
{
    for (CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
    {
        if (!pChr->IsAlive())
            return;

        float Len = distance(pChr->GetPos(), m_Pos);
        if (Len < pChr->GetProximityRadius() + s_TowerSize)
        {
            int ClientID = pChr->GetPlayer()->GetCID();
            if (pChr->GetPlayer()->GetZomb())
            {
                TakeDamage(1);
                if (pChr && pChr->GetPlayer())
                    pChr->Die(ClientID, WEAPON_GAME, false);
                continue;
            }

            if (Server()->Tick() % Server()->TickSpeed() == 0)
            {
                pChr->IncreaseHealth(3);
                GameServer()->Broadcast(ClientID, "{}Health: {int:Health}/{int:MaxHealth}", "\n\n\n\n\n\n\n", m_Health, g_Config.m_SvMaxTowerHealth);
            }
        }
    }
}
void CTowerMain::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
}

void CTowerMain::Snap(int SnappingClient)
{
    int aSize = sizeof(m_aIDs) / sizeof(int);

    for (int i = 0; i < aSize; i++)
    {
        CNetObj_Projectile *pEff = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aIDs[i], sizeof(CNetObj_Projectile)));
        if (!pEff)
            continue;

        pEff->m_X = (int)(cos(M_PI / 9 * i * 4) * (16.0 + 5 / 110) + m_Pos.x);
        pEff->m_Y = (int)(sin(M_PI / 9 * i * 4) * (16.0 + 5 / 110) + m_Pos.y);

        pEff->m_StartTick = Server()->Tick() - 2;
        pEff->m_Type = WEAPON_SHOTGUN;
    }

    float AngleStep = 2.0f * M_PI / s_TowerNumSide;

    for (int i = 0; i < s_TowerNumSide; i++)
    {
        vec2 PartPosStart = m_Pos + vec2(s_TowerSize * cos(AngleStep * i), s_TowerSize * sin(AngleStep * i));
        vec2 PartPosEnd = m_Pos + vec2(s_TowerSize * cos(AngleStep * (i + 1)), s_TowerSize * sin(AngleStep * (i + 1)));

        CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_alIDs[i], sizeof(CNetObj_Laser)));
        if (!pObj)
            return;

        pObj->m_X = (int)PartPosStart.x;
        pObj->m_Y = (int)PartPosStart.y;
        pObj->m_FromX = (int)PartPosEnd.x;
        pObj->m_FromY = (int)PartPosEnd.y;
        pObj->m_StartTick = Server()->Tick();
    }
}

void CTowerMain::TakeDamage(int Dmg)
{
    if (m_Health <= 0)
        return;
    m_Health -= Dmg;
}