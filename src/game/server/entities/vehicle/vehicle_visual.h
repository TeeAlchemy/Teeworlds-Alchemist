#ifndef GAME_SERVER_ENTITIES_VEHICLE_VEHICLE_VISUAL_H
#define GAME_SERVER_ENTITIES_VEHICLE_VEHICLE_VISUAL_H

#include <base/vmath.h>

class IServer;

namespace VehicleVisual
{
bool SnapPickup(IServer *pServer, int Id, vec2 Pos, int Type, int Subtype = 0);
bool SnapLaser(IServer *pServer, int Id, vec2 From, vec2 To, int StartTick);

void SnapCar(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, vec2 Pos, int StartTick);
void SnapAircraft(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, vec2 Pos, int StartTick);
void SnapJet(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, vec2 Pos, vec2 FacingDir, int StartTick);
void SnapHelicopter(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, int NumLasers, vec2 Pos, vec2 RotorHub, vec2 Vel, float MainRotorAngle, float TailRotorAngle, int StartTick);
void SnapTank(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, int NumLasers, int BarrelId, vec2 Pos, vec2 BarrelDir, int StartTick);
}

#endif
