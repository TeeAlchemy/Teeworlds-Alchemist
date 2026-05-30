#include <game/server/gamecontext.h>
#include <engine/shared/config.h>

#include "workbench.h"

const float CWorkbench::ms_FlagHitRadius = 28.f;

CWorkbench::CWorkbench(CGameWorld *pGameWorld, int Team, vec2 V1, vec2 V2)
    : CBuilding(pGameWorld, Team, vec2((V2.x + V1.x) / 2, V2.y), BUILDING_WORKBENCH)
{
    m_Vertex[0] = V1;
    m_Vertex[1] = V2;

    m_Width = V2.x - V1.x;
    m_Height = V2.y - V1.y;

    Init();
}

void CWorkbench::Tick()
{
}

void CWorkbench::Reset()
{
}

vec2 CWorkbench::GetFlagPos() const
{
    return vec2(GetPos().x, m_Vertex[0].y + abs(g_Config.m_SvWorkbenchesHealth - GetHealth()) * m_Step);
}

bool CWorkbench::IsDamageableAt(vec2 HitPos, float HitRadius) const
{
    return distance(HitPos, GetFlagPos()) <= ms_FlagHitRadius + HitRadius;
}

vec2 CWorkbench::GetDamageCenter() const
{
    return GetFlagPos();
}

void CWorkbench::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient))
        return;

    CNetObj_Flag *pFlag = static_cast<CNetObj_Flag *>(Server()->SnapNewItem(NETOBJTYPE_FLAG, GetID(), sizeof(CNetObj_Flag)));
    if (pFlag)
    {
        pFlag->m_X = round_to_int(GetFlagPos().x);
        pFlag->m_Y = round_to_int(GetFlagPos().y);
        pFlag->m_Team = GetTeam();
    }
}

void CWorkbench::Init()
{
    SetHealth(g_Config.m_SvWorkbenchesHealth);
    m_Step = fabs(m_Vertex[0].y - GetPos().y) / GetHealth();
}
