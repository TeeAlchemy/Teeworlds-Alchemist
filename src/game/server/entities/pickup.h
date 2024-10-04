/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUP_H
#define GAME_SERVER_ENTITIES_PICKUP_H

#include <game/server/entity.h>

const int PickupPhysSize = 14;

class CPickup : public CEntity
{
public:
	CPickup(CGameWorld *pGameWorld, int Type, int SubType, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	bool IsWeapon();

	int GetType() { return m_Type; }
	int GetSubType() { return m_Subtype; }
	bool IsSpawned() { return m_SpawnTick == -1; }
 
private:
	int m_Type;
	int m_Subtype;
	int m_SpawnTick;
};

#endif
