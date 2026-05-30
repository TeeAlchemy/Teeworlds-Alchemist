#ifndef GAME_SERVER_ENTITIES_DOOR_SWITCH_H
#define GAME_SERVER_ENTITIES_DOOR_SWITCH_H

#include <game/server/entity.h>

#include "../door.h"

class CDoorSwitch : public CEntity
{
public:
	CDoorSwitch(CGameWorld *pGameWorld, vec2 Pos, CDoor *pDoor, bool Lights);

	void Tick() override;
	void Snap(int SnappingClient) override;

private:
	CDoor *m_pDoor;
	int m_ID1;
	int m_ID2;
	bool m_Lights;
};

#endif
