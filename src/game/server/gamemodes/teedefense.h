/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_TD_H
#define GAME_SERVER_GAMEMODES_TD_H
#include <game/server/gamecontroller.h>

class CGameControllerTeeDefense : public IGameController
{
public:
	CGameControllerTeeDefense(class CGameContext *pGameServer);

	void StartRound() override;
	void EndRound() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void Tick() override;

	void DoWincheck() override;

	bool CanSpawn(int Team, vec2 *pPos) override;

	void InitBots() override;

	bool CheckTeamBalance() override { return true; }

	//Zomb2
	int m_Wave;
	int m_Zombie[13];//not sure about the amount of zombies
	int m_ZombLeft;

	void StartWave(int Wave);
	void CheckZombie();
	int RandZomb();
	bool EndWave();
	void DoZombMessage(int Which);
	void DoLifeMessage(int Life);
	void SetWaveAlg(int modulus, int wavedrittel);
	int GetZombieReihenfolge(int wavedrittel);

	void OnZombieKill(int ClientID);
};
#endif
