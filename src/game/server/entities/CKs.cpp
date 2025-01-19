/* (c) FlowerFell-Sans. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                 */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "CKs.h"

#include <game/server/GameCore/Account/account.h>

CKs::CKs(CGameWorld *pGameWorld, int Type, vec2 Pos)
	: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, Pos, PhysSize)
{
	m_Type = Type;
	m_Pos = Pos;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CKs::Reset()
{
}

void CKs::Tick()
{
	if (m_Health == 0)
		m_Health = GetMaxHealth();

	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(GetPos(), 20.0f, 0);
	if (pChr && pChr->IsAlive() && !pChr->GetPlayer()->GetZomb())
	{		
		if (pChr->m_LatestInput.m_Fire & 1 && pChr->GetActiveWeapon() == WEAPON_HAMMER && pChr->m_MiningTick <= 0)
		{
			int Tool = ITYPE_PICKAXE;
			if (m_Type == ITEM_LOG)
				Tool = ITYPE_AXE;

			pChr->m_InMining = true;
			GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP);
			Picking(GameServer()->ItemHelper()->GetDmg(pChr->GetPlayer()->m_AccData.m_Holding[Tool]), pChr->GetPlayer());
		}
	}
}

void CKs::Picking(int Time, CPlayer *Player)
{
	m_Health -= Time;
	int CID = Player->GetCID();
	if (m_Health <= 0)
	{
		Player->m_AccData.m_aItems[m_Type].m_Num++;
		GameServer()->Chat(CID, "You picked up a {}", GameServer()->ItemHelper()->GetItemName(m_Type));
		m_Health = GetMaxHealth();
		GameServer()->TW()->Account()->SaveAccountData(CID, CGameContext::TABLE_ITEM, Player->m_AccData);
		GameServer()->ClearVotes(CID);
	}

	GameServer()->Broadcast(CID, "{}- Picking: {} - {}- {}/{} left. Keep hit! -{}- Using: {} | Dmg : {} -", "\n\n\n\n", 
							GameServer()->ItemHelper()->GetItemName(m_Type), "\n", m_Health, GetMaxHealth(), "\n", m_Type == ITEM_LOG ? GameServer()->ItemHelper()->GetItemName(Player->m_AccData.m_Holding[ITYPE_AXE]) : GameServer()->ItemHelper()->GetItemName(Player->m_AccData.m_Holding[ITYPE_PICKAXE]), GameServer()->ItemHelper()->GetDmg(Player->m_AccData.m_Holding[ITYPE_PICKAXE]));
	Player->GetCharacter()->m_MiningTick = 25;
}

void CKs::TickPaused()
{
}

void CKs::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if (!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	if (m_Type == ITEM_LOG)
		pP->m_Type = POWERUP_HEALTH;
	else
		pP->m_Type = POWERUP_ARMOR;
	pP->m_Subtype = 0;
}

int CKs::GetMaxHealth()
{
	return GameServer()->ItemHelper()->GetMaxHealth(m_Type);
}