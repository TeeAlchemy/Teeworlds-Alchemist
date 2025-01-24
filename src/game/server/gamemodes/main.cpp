/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "main.h"

CGameControllerMain::CGameControllerMain(class CGameContext *pGameServer)
	: IGameController(pGameServer)
{
}

void CGameControllerMain::InitBots()
{
	// Init bots;
	for (int i = 0; i < MAX_BOTS; i++)
		GameServer()->AddBot();
}

void CGameControllerMain::StartRound()
{
	// no.
}

void CGameControllerMain::EndRound()
{
	// no
}

void CGameControllerMain::OnCharacterSpawn(CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);
}

int CGameControllerMain::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	if (!pVictim || !pKiller)
		return 0;

	if(!pVictim->GetPlayer()->IsBot())
		GameServer()->Chat(pVictim->GetPlayer()->GetCID(), "You're dead");

	return 1;
}

void CGameControllerMain::Tick()
{

}

void CGameControllerMain::DoWincheck()
{
}

bool CGameControllerMain::CanSpawn(int Team, vec2 *pPos)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if (Team == TEAM_SPECTATORS)
		return false;

	EvaluateSpawnType(&Eval, Team);

	*pPos = Eval.m_Pos;
	return Eval.m_Got;
}