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
	m_LockedPlayer = -1;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CKs::Reset()
{
}

void CKs::HandleLock(CCharacter *pChr)
{
	if (!GameServer()->GetPlayerChar(m_LockedPlayer))
		m_LockedPlayer = -1;

	if (!pChr)
		return;
	
	if (m_LockedPlayer == -1 && !pChr->m_LockedCK && pChr->GetPlayer()->PressTab())
	{
		pChr->m_LockedCK = true;
		m_LockedPlayer = pChr->GetPlayer()->GetCID();
		pChr->m_LockPos = GetPos();
	}
}

void CKs::Tick()
{
	if (m_Health == 0)
		m_Health = GetMaxHealth();

	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(GetPos(), 20.0f, 0);
	if (pChr && pChr->IsAlive() && !pChr->GetPlayer()->GetZomb())
	{
		HandleLock(pChr);

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
	int CID = Player->GetCID();
	int Extra = ITYPE_PICKAXE; // 3 use, awa
	if (m_Type == ITEM_LOG)
		Extra = ITYPE_AXE;

	Extra = GameServer()->ItemHelper()->GetCard(Player->GetExtraHolding(Extra), ITEM_CARD_DAMAGE);
	if (Extra == ITYPE_AXE)
		Extra = Time * 2; // TODO: Config
	else
		Extra = Time * Extra; // TODO: Config

	m_Health -= Time + Extra;
	
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
	
	Extra = GameServer()->ItemHelper()->GetCard(Player->GetExtraHolding(Extra), ITEM_CARD_QUICKLY_FIRE);
	Player->GetCharacter()->m_MiningTick = 25 - Extra; // TODO: Config
}

void CKs::TickPaused()
{
}

void CKs::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = Server()->SnapNewItem<CNetObj_Pickup>(GetID());
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