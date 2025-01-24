#include <engine/shared/config.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "turret.h"

CTurret::CTurret(CGameWorld *pGameWorld, vec2 Pos, int Owner) : CEntity(pGameWorld, CGameWorld::ENTTYPE_TURRET, Pos)
{
    m_Owner = Owner;
    m_Type = GameServer()->GetPlayer(Owner)->GetHolding(ITYPE_TURRET);

    for (int i = 0; i < s_TurretNumID; i++)
        m_IDs[i] = Server()->SnapNewID();
    
    GameWorld()->InsertEntity(this);
}

CTurret::~CTurret()
{
    for (int i = 0; i < s_TurretNumID; i++)
        Server()->SnapFreeID(m_IDs[i]);
}

void CTurret::Tick()
{

}

void CTurret::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient))
        return;
    
    CNetObj_DDNetLaser *pLaser = Server()->SnapNewItem<CNetObj_DDNetLaser>(GetID());
    if(!pLaser)
        return;

    pLaser->m_FromX = (int)GetPos().x;
    pLaser->m_FromY = (int)GetPos().y;
    pLaser->m_ToX = (int)GetPos().x;
    pLaser->m_ToY = (int)GetPos().y;
    pLaser->m_Owner = GetOwner();
    pLaser->m_StartTick = Server()->Tick();
    pLaser->m_Flags = LASERFLAG_NO_PREDICT;
    pLaser->m_Subtype = m_Type - ITEM_TURRET_BEGINNER;

    float BaseRadius = float(g_Config.m_SvTurretRadius);
    float Radius = BaseRadius + float(GetLevel() * BaseRadius);
    float AngleStep = 2.0f * pi / s_TurretNumID;

    for (int i = 0; i < s_TurretNumID; i++)
    {
	    vec2 PartPosStart = m_Pos + vec2(Radius * cos(AngleStep*i), Radius * sin(AngleStep*i));
	    vec2 PartPosEnd = m_Pos + vec2(Radius * cos(AngleStep*(i+1)), Radius * sin(AngleStep*(i+1)));
	    CNetObj_Laser *pObj = Server()->SnapNewItem<CNetObj_Laser>(m_IDs[i]);
	    if(!pObj)
	    	return;
	    pObj->m_X = (int)PartPosStart.x;
	    pObj->m_Y = (int)PartPosStart.y;
	    pObj->m_FromX = (int)PartPosEnd.x;
	    pObj->m_FromY = (int)PartPosEnd.y;
	    pObj->m_StartTick = Server()->Tick();
	}
}

void CTurret::Reset()
{
    
}

int CTurret::GetLevel()
{
    return m_Type - ITEM_TURRET_BEGINNER;
}