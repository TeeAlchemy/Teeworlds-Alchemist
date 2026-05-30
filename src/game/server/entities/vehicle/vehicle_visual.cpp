#include "vehicle_visual.h"

#include <base/math.h>
#include <engine/server.h>
#include <game/generated/protocol.h>

#include "vehicle_util.h"

namespace VehicleVisual
{
static vec2 At(vec2 Pos, float X, float Y)
{
	return vec2(Pos.x + VehicleScale(X), Pos.y + VehicleScale(Y));
}

static vec2 TiltAt(vec2 Pos, float X, float Y, float Angle)
{
	const float ScaledX = VehicleScale(X);
	const float ScaledY = VehicleScale(Y);
	const float Cos = cosf(Angle);
	const float Sin = sinf(Angle);
	return vec2(Pos.x + ScaledX * Cos - ScaledY * Sin, Pos.y + ScaledX * Sin + ScaledY * Cos);
}

static vec2 TiltDelta(vec2 Pos, vec2 Delta, float Angle)
{
	const float Cos = cosf(Angle);
	const float Sin = sinf(Angle);
	return vec2(Pos.x + Delta.x * Cos - Delta.y * Sin, Pos.y + Delta.x * Sin + Delta.y * Cos);
}
static vec2 FacingAt(vec2 Pos, float X, float Y, vec2 Facing)
{
	vec2 Dir = Facing;
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	else
		Dir = normalize(Dir);

	return TiltAt(Pos, X, Y, atan2f(Dir.y, Dir.x));
}

static float HelicopterBodyTilt(vec2 Vel, float MaxSpeed)
{
	const float MaxTilt = 0.22f;
	const float PitchTilt = 0.14f;
	const float NormX = clamp(Vel.x / MaxSpeed, -1.f, 1.f);
	const float NormY = clamp(Vel.y / MaxSpeed, -1.f, 1.f);
	return -NormX * MaxTilt + NormY * PitchTilt;
}

bool SnapPickup(IServer *pServer, int Id, vec2 Pos, int Type, int Subtype)
{
	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(pServer->SnapNewItem(NETOBJTYPE_PICKUP, Id, sizeof(CNetObj_Pickup)));
	if (!pP)
		return false;

	pP->m_X = (int)Pos.x;
	pP->m_Y = (int)Pos.y;
	pP->m_Type = Type;
	pP->m_Subtype = Subtype;
	return true;
}

bool SnapLaser(IServer *pServer, int Id, vec2 From, vec2 To, int StartTick)
{
	CNetObj_Laser *pL = static_cast<CNetObj_Laser *>(pServer->SnapNewItem(NETOBJTYPE_LASER, Id, sizeof(CNetObj_Laser)));
	if (!pL)
		return false;

	pL->m_FromX = (int)From.x;
	pL->m_FromY = (int)From.y;
	pL->m_X = (int)To.x;
	pL->m_Y = (int)To.y;
	pL->m_StartTick = pServer->Tick();
	return true;
}

void SnapCar(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, vec2 Pos, int StartTick)
{
	if (NumParts < 4 || !pPartIds || !pLaserIds)
		return;

	SnapPickup(pServer, BodyId, At(Pos, 0.f, -18.f), POWERUP_ARMOR);
	SnapPickup(pServer, pPartIds[0], At(Pos, 0.f, -34.f), POWERUP_WEAPON, WEAPON_GUN);
	SnapPickup(pServer, pPartIds[1], At(Pos, -42.f, 6.f), POWERUP_HEALTH);
	SnapPickup(pServer, pPartIds[2], At(Pos, 42.f, 6.f), POWERUP_HEALTH);
	SnapPickup(pServer, pPartIds[3], At(Pos, 0.f, 2.f), POWERUP_ARMOR_NINJA);
	SnapLaser(pServer, pLaserIds[0], At(Pos, -46.f, -10.f), At(Pos, 46.f, -10.f), StartTick);
}

void SnapAircraft(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, vec2 Pos, int StartTick)
{
	if (NumParts < 3 || !pPartIds || !pLaserIds)
		return;

	SnapPickup(pServer, BodyId, At(Pos, 0.f, 0.f), POWERUP_WEAPON, WEAPON_GRENADE);
	SnapPickup(pServer, pPartIds[0], At(Pos, -46.f, 6.f), POWERUP_ARMOR_LASER);
	SnapPickup(pServer, pPartIds[1], At(Pos, 46.f, 6.f), POWERUP_ARMOR_LASER);
	SnapPickup(pServer, pPartIds[2], At(Pos, -8.f, -22.f), POWERUP_ARMOR);
	SnapLaser(pServer, pLaserIds[0], At(Pos, -52.f, 6.f), At(Pos, 52.f, 6.f), StartTick);
	SnapLaser(pServer, pLaserIds[1], At(Pos, 8.f, 0.f), At(Pos, 44.f, 0.f), StartTick);
}

void SnapJet(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, vec2 Pos, vec2 FacingDir, int StartTick)
{
	if (NumParts < 4 || !pPartIds || !pLaserIds)
		return;

	SnapPickup(pServer, BodyId, FacingAt(Pos, 10.f, 0.f, FacingDir), POWERUP_WEAPON, WEAPON_RIFLE);
	SnapPickup(pServer, pPartIds[0], FacingAt(Pos, -14.f, -6.f, FacingDir), POWERUP_ARMOR);
	SnapPickup(pServer, pPartIds[1], FacingAt(Pos, -18.f, 12.f, FacingDir), POWERUP_ARMOR_SHOTGUN);
	SnapPickup(pServer, pPartIds[2], FacingAt(Pos, 28.f, 12.f, FacingDir), POWERUP_ARMOR_SHOTGUN);
	SnapPickup(pServer, pPartIds[3], FacingAt(Pos, -38.f, 4.f, FacingDir), POWERUP_NINJA);
	SnapLaser(pServer, pLaserIds[0], FacingAt(Pos, -30.f, 0.f, FacingDir), FacingAt(Pos, 46.f, 0.f, FacingDir), StartTick);
	SnapLaser(pServer, pLaserIds[1], FacingAt(Pos, -38.f, 4.f, FacingDir), FacingAt(Pos, -50.f, 14.f, FacingDir), StartTick);
}

void SnapHelicopter(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, int NumLasers, vec2 Pos, vec2 RotorHub, vec2 Vel, float MainRotorAngle, float TailRotorAngle, int StartTick)
{
	if (NumParts < 4 || NumLasers < 4 || !pPartIds || !pLaserIds)
		return;

	const float BodyTilt = HelicopterBodyTilt(Vel, 14.f);
	const vec2 Hub = TiltDelta(Pos, RotorHub - Pos, BodyTilt);
	const float MainBladeLen = VehicleScale(50.f);
	const float TailBladeLen = VehicleScale(10.f);
	const float RotorTilt = 0.28f + 0.07f * fabs(sinf(MainRotorAngle * 2.f));

	auto RotorTip = [&](float Angle) {
		return Hub + vec2(cosf(Angle), sinf(Angle) * RotorTilt) * MainBladeLen;
	};

	const vec2 TailHub = TiltAt(Pos, 36.f, 4.f, BodyTilt);
	const vec2 TailBlade = vec2(cosf(TailRotorAngle), sinf(TailRotorAngle) * 0.45f) * TailBladeLen;

	SnapPickup(pServer, BodyId, TiltAt(Pos, 0.f, 0.f, BodyTilt), POWERUP_WEAPON, WEAPON_SHOTGUN);
	SnapPickup(pServer, pPartIds[0], TiltAt(Pos, -24.f, 18.f, BodyTilt), POWERUP_HEALTH);
	SnapPickup(pServer, pPartIds[1], TiltAt(Pos, 24.f, 18.f, BodyTilt), POWERUP_HEALTH);
	SnapPickup(pServer, pPartIds[2], TailHub, POWERUP_ARMOR);
	SnapPickup(pServer, pPartIds[3], Hub, POWERUP_ARMOR_NINJA);

	SnapLaser(pServer, pLaserIds[0], RotorTip(MainRotorAngle + pi), RotorTip(MainRotorAngle), StartTick);
	SnapLaser(pServer, pLaserIds[1], RotorTip(MainRotorAngle + pi + pi / 2.f), RotorTip(MainRotorAngle + pi / 2.f), StartTick);
	SnapLaser(pServer, pLaserIds[2], TiltAt(Pos, 4.f, -2.f, BodyTilt), TailHub, StartTick);
	SnapLaser(pServer, pLaserIds[3], TailHub - TailBlade, TailHub + TailBlade, StartTick);
}

static float TankTrackSegmentY(int Slot, int Phase)
{
	const float aOffsets[] = {6.f, 14.f, 22.f};
	return VehicleScale(aOffsets[(Slot + Phase) % 3]);
}

void SnapTank(IServer *pServer, int BodyId, const int *pPartIds, int NumParts, const int *pLaserIds, int NumLasers, int BarrelId, vec2 Pos, vec2 BarrelDir, int StartTick)
{
	if (NumParts < 8 || NumLasers < 4 || !pPartIds || !pLaserIds)
		return;

	vec2 Dir = BarrelDir;
	if (length(Dir) < 1e-3f)
		Dir = vec2(1.f, 0.f);
	Dir = normalize(Dir);

	const vec2 Turret = At(Pos, 0.f, -30.f);
	const vec2 Tip = Turret + Dir * VehicleScale(42.f);
	const int TreadPhase = StartTick % 3;

	SnapPickup(pServer, BodyId, At(Pos, 0.f, -14.f), POWERUP_ARMOR);
	SnapPickup(pServer, pPartIds[6], Turret, POWERUP_ARMOR_GRENADE);
	SnapPickup(pServer, pPartIds[7], At(Pos, 0.f, 6.f), POWERUP_ARMOR_LASER);

	SnapPickup(pServer, pPartIds[0], At(Pos, -40.f, TankTrackSegmentY(0, TreadPhase)), POWERUP_HEALTH);
	SnapPickup(pServer, pPartIds[1], At(Pos, -40.f, TankTrackSegmentY(1, TreadPhase)), POWERUP_ARMOR_NINJA);
	SnapPickup(pServer, pPartIds[2], At(Pos, 40.f, TankTrackSegmentY(0, TreadPhase + 1)), POWERUP_HEALTH);
	SnapPickup(pServer, pPartIds[3], At(Pos, 40.f, TankTrackSegmentY(1, TreadPhase + 1)), POWERUP_ARMOR_NINJA);
	SnapPickup(pServer, pPartIds[4], At(Pos, -40.f, TankTrackSegmentY(2, TreadPhase)), POWERUP_ARMOR);
	SnapPickup(pServer, pPartIds[5], At(Pos, 40.f, TankTrackSegmentY(2, TreadPhase + 1)), POWERUP_ARMOR);

	const vec2 LeftTop = At(Pos, -44.f, 4.f);
	const vec2 LeftBottom = At(Pos, -44.f, 26.f);
	const vec2 RightTop = At(Pos, 44.f, 4.f);
	const vec2 RightBottom = At(Pos, 44.f, 26.f);
	SnapLaser(pServer, pLaserIds[0], LeftTop, LeftBottom, StartTick);
	SnapLaser(pServer, pLaserIds[1], RightTop, RightBottom, StartTick);
	SnapLaser(pServer, pLaserIds[2], At(Pos, -44.f, 10.f + TreadPhase * 4.f), At(Pos, -44.f, 18.f + TreadPhase * 4.f), StartTick);
	SnapLaser(pServer, pLaserIds[3], At(Pos, 44.f, 10.f + TreadPhase * 4.f), At(Pos, 44.f, 18.f + TreadPhase * 4.f), StartTick);

	SnapLaser(pServer, BarrelId, Turret, Tip, StartTick);
}

} // namespace VehicleVisual
