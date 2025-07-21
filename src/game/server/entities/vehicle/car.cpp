#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

#include "car.h"

CCar::CCar(CGameWorld *pGameWorld, vec2 Pos, int Team)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_CAR, Pos)
{
    m_Health = 16;
    m_Team = Team;
    m_Owner = -1;

    for (int i = 0; i < CAR_EXTRA_IDS; i++)
        m_IDs[i] = Server()->SnapNewID();

    GameWorld()->InsertEntity(this);
}

CCar::~CCar()
{
    for (int i = 0; i < CAR_EXTRA_IDS; i++)
        Server()->SnapFreeID(m_IDs[i]);

}

void CCar::Tick()
{
    if(m_Owner >= 0)
    {
        if(GameServer()->GetPlayerChar(m_Owner))
        {
            CCharacter *pChr = GameServer()->GetPlayerChar(m_Owner);
            pChr->GetCore()->m_Pos = vec2(m_Pos.x, m_Pos.y - 2.f);
            pChr->GetCore()->m_HookState = HOOK_IDLE;
            pChr->GetCore()->m_Vel = m_Vel;
            
            if(pChr->GetCore()->m_Direction < 0)
        		m_Vel.x = SaturatedAdd(-(float)CAR_MAX_SPEED, (float)CAR_MAX_SPEED, m_Vel.x, -(float)CAR_ACCEL);
        	else if(pChr->GetCore()->m_Direction > 0)
        		m_Vel.x = SaturatedAdd(-(float)CAR_MAX_SPEED, (float)CAR_MAX_SPEED, m_Vel.x, (float)CAR_ACCEL);
            else
                m_Vel.x *= 0.85f;

            m_Vel.y += 0.5f;
            vec2 NewPos = m_Pos;
        	GameServer()->Collision()->MoveBox(&NewPos, &m_Vel, vec2(31.0f * 3, 16.0f), 0);
            m_Pos = NewPos;
        }
        else
            m_Owner = -1;
    }
    else
    {
        CCharacter *pChr = GameWorld()->ClosestCharacter(m_Pos, 32.f, NULL);
        if(pChr && !pChr->m_OnVehicle)
        {
            pChr->m_OnVehicle = true;
            m_Owner = pChr->GetPlayer()->GetCID();
            m_Pos.y -= 16;

        }
    }
}

void CCar::Reset()
{
    GameWorld()->DestroyEntity(this);
}

void CCar::Snap(int SnappingClient)
{
    if (NetworkClipped(SnappingClient))
		return;
    
    {
        CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	    if (!pP)
	    	return;

	    pP->m_X = (int)m_Pos.x;
	    pP->m_Y = (int)m_Pos.y - 32.f;
	    pP->m_Type = POWERUP_ARMOR;
	    pP->m_Subtype = 0;
    }

    {
        CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_IDs[0], sizeof(CNetObj_Pickup)));
	    if (!pP)
	    	return;
    
	    pP->m_X = (int)m_Pos.x - 32.f;
	    pP->m_Y = (int)m_Pos.y;
	    pP->m_Type = POWERUP_HEALTH;
	    pP->m_Subtype = 0;
    }

    {
        CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_IDs[1], sizeof(CNetObj_Pickup)));
	    if (!pP)
	    	return;
    
	    pP->m_X = (int)m_Pos.x + 32.f;
	    pP->m_Y = (int)m_Pos.y;
	    pP->m_Type = POWERUP_HEALTH;
	    pP->m_Subtype = 0;
    }
}