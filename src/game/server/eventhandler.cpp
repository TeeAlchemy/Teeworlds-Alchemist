/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "eventhandler.h"

#include "entity.h"
#include "gamecontext.h"

#include <base/system.h>
#include <base/vmath.h>

//////////////////////////////////////////////////
// Event handler
//////////////////////////////////////////////////
CEventHandler::CEventHandler()
{
	m_pGameServer = 0;
	Clear();
}

void CEventHandler::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
}

void *CEventHandler::Create(int Type, int Size, CClientMask Mask, int MapID)
{
	if(m_NumEvents == MAX_EVENTS)
		return 0;
	if(m_CurrentOffset + Size >= MAX_DATASIZE)
		return 0;

	void *p = &m_aData[m_CurrentOffset];
	m_aOffsets[m_NumEvents] = m_CurrentOffset;
	m_aTypes[m_NumEvents] = Type;
	m_aSizes[m_NumEvents] = Size;
	m_aClientMasks[m_NumEvents] = Mask;
	m_aMapID[m_NumEvents] = MapID;
	m_CurrentOffset += Size;
	m_NumEvents++;
	return p;
}

void CEventHandler::Clear()
{
	m_NumEvents = 0;
	m_CurrentOffset = 0;
}

void CEventHandler::Snap(int SnappingClient)
{
	for(int i = 0; i < m_NumEvents; i++)
	{
		if(SnappingClient == -1 || m_aClientMasks[i].test(SnappingClient))
		{
			CNetEvent_Common *pEvent = (CNetEvent_Common *)&m_aData[m_aOffsets[i]];
			if (m_aMapID[i] != -1 && m_aMapID[i] != GameServer()->Server()->ClientMapID(SnappingClient))
				continue;
			if (SnappingClient != -1 || distance(GameServer()->m_apPlayers[SnappingClient]->m_ViewPos, vec2(pEvent->m_X, pEvent->m_Y)) < 1500.0f)
			{
				int Type = m_aTypes[i];
				int Size = m_aSizes[i];
				const char *pData = &m_aData[m_aOffsets[i]];

				void *pItem = GameServer()->Server()->SnapNewItem(Type, i, Size);
				if(pItem)
					mem_copy(pItem, pData, Size);
			}
		}
	}
}