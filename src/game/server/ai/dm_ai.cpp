#include <engine/shared/config.h>

#include <game/server/ai.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

#include "dm_ai.h"


CAIdm::CAIdm(CGameContext *pGameServer, CPlayer *pPlayer)
: CAI(pGameServer, pPlayer)
{
	m_SkipMoveUpdate = 0;
}


void CAIdm::OnCharacterSpawn(CCharacter *pChr)
{
	CAI::OnCharacterSpawn(pChr);
	
	pChr->GiveWeapon(WEAPON_RIFLE, -1);
	pChr->SetWeapon(WEAPON_RIFLE);
	m_WaypointDir = vec2(0, 0);
}


void CAIdm::DoBehavior()
{
	// reset jump and attack
	m_Jump = 0;
	m_Attack = 0;
	
	HeadToMovingDirection();

	SeekClosestEnemyInSight();

	// if we see a player
	if (m_EnemiesInSight > 0)
	{
		ShootAtClosestEnemy();
		ReactToPlayer();
	}
	else
		m_AttackTimer = 0;


	//if (SeekClosestEnemy())
	if (SeekRandomEnemy())
	{
		m_TargetPos = m_PlayerPos;
				
		if (m_EnemiesInSight > 1)
		{
			// distance to the player
			if (m_PlayerPos.x < m_Pos.x)
				m_TargetPos.x = m_PlayerPos.x + WeaponShootRange()/2*(0.5f+frandom()*1.0f);
			else
				m_TargetPos.x = m_PlayerPos.x - WeaponShootRange()/2*(0.5f+frandom()*1.0f);
		}
	}

	
	if (UpdateWaypoint())
	{
		MoveTowardsWaypoint(20);
		HookMove();
		AirJump();
		
		// jump if waypoint is above us
		if (abs(m_WaypointPos.x - m_Pos.x) < 60 && m_WaypointPos.y < m_Pos.y - 100 && frandom()*20 < 4)
			m_Jump = 1;
	}
	else
	{
		m_Hook = 0;
	}
	
	
	DoJumping();
	Unstuck();

	RandomlyStopShooting();
	
	// next reaction in
	m_ReactionTime = 0;
	
}