#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>

#include "aircraft.h"
#include "vehicle_util.h"
#include "vehicle_visual.h"

CAircraft::CAircraft(CGameWorld *pGameWorld, vec2 Pos, int Team)
	: CVehicle(pGameWorld, CGameWorld::ENTTYPE_AIRCRAFT, Pos, Team, 50)
{
	for (int i = 0; i < AIRCRAFT_PART_IDS; i++)
		m_PartIds[i] = Server()->SnapNewID();
	for (int i = 0; i < AIRCRAFT_LASER_IDS; i++)
		m_LaserIds[i] = Server()->SnapNewID();
	GameWorld()->InsertEntity(this);
}

CAircraft::~CAircraft()
{
	for (int i = 0; i < AIRCRAFT_PART_IDS; i++)
		Server()->SnapFreeID(m_PartIds[i]);
	for (int i = 0; i < AIRCRAFT_LASER_IDS; i++)
		Server()->SnapFreeID(m_LaserIds[i]);
}

void CAircraft::TickDriver(CCharacter *pDriver)
{
	ApplyHorizontalInput(pDriver, AIRCRAFT_MAX_SPEED, AIRCRAFT_ACCEL, 0.92f);
	VehicleApplyFlyingVertical(GameServer(), m_Vel, pDriver, AIRCRAFT_MAX_SPEED, AIRCRAFT_ACCEL);
	VehicleApplyFriction(m_Vel, 0.96f);
	MoveBox(CollisionSize());
}

void CAircraft::TickIdle()
{
	VehicleApplyGravity(GameServer(), m_Vel);
	VehicleApplyFriction(m_Vel, 0.95f);
	MoveBox(CollisionSize());
}

void CAircraft::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	VehicleVisual::SnapAircraft(Server(), GetID(), m_PartIds, AIRCRAFT_PART_IDS, m_LaserIds, m_Pos, Server()->Tick());
	SnapHealthBar(SnappingClient);
}
