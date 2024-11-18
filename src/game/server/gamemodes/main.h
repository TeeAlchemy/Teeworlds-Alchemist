/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H
#include <game/server/gamecontroller.h>

class CGameControllerMain : public IGameController
{
public:
	CGameControllerMain(class CGameContext *pGameServer);

	void StartRound() override;
	void EndRound() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;

	void DoWincheck() override;

	bool CanSpawn(int Team, vec2 *pPos) override;

	void InitBots() override;
};
#endif
