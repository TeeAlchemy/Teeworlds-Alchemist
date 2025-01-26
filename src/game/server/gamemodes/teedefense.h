/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_TD_H
#define GAME_SERVER_GAMEMODES_TD_H
#include <game/server/gamecontroller.h>

class CGameControllerTeeDefense : public IGameController
{
public:
	CGameControllerTeeDefense(class CGameContext *pGameServer);
	void Tick() override;
	void Snap(int SnappingClient) override;

	void StartRound() override;
	void EndRound() override;
	void InitBots() override;
	void DoWincheck() override;
	bool CheckTeamBalance() override { return true; }

	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnPlayerConnect(class CPlayer *pPlayer) override;

	bool OnEntity(int Index, vec2 Pos) override;
	bool CanSpawn(int Team, vec2 *pPos) override;

	void SetWave(int Wave) override;
	void SetTowerHealth(int Health) override;
	void ResetBots();
private:
	//Zomb2
	int m_ZombStart;
	int m_Wave;
	int m_Zombie[NUM_ZOMB];//not sure about the amount of zombies
	int m_ZombLeft;

	void StartWave(int Wave);
	void CheckZombie();
	int RandZomb();
	bool EndWave();
	void DoZombMessage(int Which);
	void SetWaveAlg(int modulus, int wavedrittel);
	int GetZombieReihenfolge(int wavedrittel);

	void OnZombieKill(int ClientID);

	class CTowerMain *m_pTower;
};
#endif
