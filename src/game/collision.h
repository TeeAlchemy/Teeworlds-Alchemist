/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include "pathfinding.h"

class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	int GetTile(int x, int y);

	int m_WaypointCount;
	int m_ConnectionCount;
	
	void ClearWaypoints();
	void AddWaypoint(vec2 Position, bool InnerCorner = false);
	CWaypoint *GetWaypointAt(int x, int y);
	void ConnectWaypoints();
	CWaypoint *GetClosestWaypoint(vec2 Pos);

	CWaypoint *m_apWaypoint[MAX_WAYPOINTS];
	CWaypoint *m_pCenterWaypoint;
	
	CWaypointPath *m_pPath;

public:
	enum
	{
		COLFLAG_SOLID = 1,
		COLFLAG_DEATH = 2,
		COLFLAG_NOHOOK = 4,
	};

	bool IsTileSolid(int x, int y, bool IncludeDeath = false);

	void GenerateWaypoints();
	bool GenerateSomeMoreWaypoints();
	int WaypointCount() { return m_WaypointCount; }
	int ConnectionCount() { return m_ConnectionCount; }
	
	void SetWaypointCenter(vec2 Position);
	void AddWeight(vec2 Pos, int Weight);
	bool FindWaypointPath(vec2 TargetPos);
	
	//CWaypointPath *AStar(vec2 Start, vec2 End);
	bool AStar(vec2 Start, vec2 End);
	
	CWaypointPath *GetPath(){ return m_pPath; }
	void ForgetAboutThePath(){ m_pPath = 0; }

	// for testing
	vec2 m_aPath[99];

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y, bool IncludeDeath = false) { return IsTileSolid(round(x), round(y), IncludeDeath); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, bool IncludeDeath = false);
	int FastIntersectLine(vec2 Pos0, vec2 Pos1);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);
};

#endif
