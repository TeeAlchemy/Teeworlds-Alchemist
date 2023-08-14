/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/vmath.h>

#include <engine/shared/config.h>

#include <game/client/render.h>
#include <generated/protocol.h>

#include "chat.h"
#include "voting.h"


void CVoting::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CVoting *pSelf = (CVoting *)pUserData;
	if(str_comp_nocase(pResult->GetString(0), "yes") == 0)
		pSelf->Vote(VOTE_CHOICE_YES);
	else if(str_comp_nocase(pResult->GetString(0), "no") == 0)
		pSelf->Vote(VOTE_CHOICE_NO);
}

void CVoting::Callvote(const char *pType, const char *pValue, const char *pReason, bool ForceVote)
{
	CNetMsg_Cl_CallVote Msg = {0};
	Msg.m_Type = pType;
	Msg.m_Value = pValue;
	Msg.m_Reason = pReason;
	Msg.m_Force = ForceVote;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

void CVoting::CallvoteSpectate(int ClientID, const char *pReason, bool ForceVote)
{
	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%d", ClientID);
	Callvote("spectate", aBuf, pReason, ForceVote);
}

void CVoting::CallvoteKick(int ClientID, const char *pReason, bool ForceVote)
{
	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%d", ClientID);
	Callvote("kick", aBuf, pReason, ForceVote);
}

void CVoting::CallvoteOption(int OptionID, const char *pReason, bool ForceVote)
{
	CVoteOptionClient *pOption = m_pFirst;
	while(pOption && OptionID >= 0)
	{
		if(OptionID == 0)
		{
			Callvote("option", pOption->m_aDescription, pReason, ForceVote);
			break;
		}

		OptionID--;
		pOption = pOption->m_pNext;
	}
}

void CVoting::RconRemoveVoteOption(int OptionID)
{
	CVoteOptionClient *pOption = m_pFirst;
	while(pOption && OptionID >= 0)
	{
		if(OptionID == 0)
		{
			char aBuf[VOTE_DESC_LENGTH+32];
			str_format(aBuf, sizeof(aBuf), "remove_vote \"%s\"", pOption->m_aDescription);
			Client()->Rcon(aBuf);
			break;
		}

		OptionID--;
		pOption = pOption->m_pNext;
	}
}

void CVoting::RconAddVoteOption(const char *pDescription, const char *pCommand)
{
	char aBuf[VOTE_DESC_LENGTH+VOTE_CMD_LENGTH+32];
	str_format(aBuf, sizeof(aBuf), "add_vote \"%s\" \"%s\"", pDescription, pCommand);
	Client()->Rcon(aBuf);
}

void CVoting::Vote(int Choice)
{
	CNetMsg_Cl_Vote Msg = { Choice };
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

CVoting::CVoting()
{
	ClearOptions();
	Clear();
}

void CVoting::AddOption(const char *pDescription)
{
	CVoteOptionClient *pOption;
	if(m_pRecycleFirst)
	{
		pOption = m_pRecycleFirst;
		m_pRecycleFirst = m_pRecycleFirst->m_pNext;
		if(m_pRecycleFirst)
			m_pRecycleFirst->m_pPrev = 0;
		else
			m_pRecycleLast = 0;
	}
	else
		pOption = (CVoteOptionClient *)m_Heap.Allocate(sizeof(CVoteOptionClient));

	pOption->m_pNext = 0;
	pOption->m_pPrev = m_pLast;
	if(pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	m_pLast = pOption;
	if(!m_pFirst)
		m_pFirst = pOption;

	int Depth = 0;
	for(;*pDescription == '#'; pDescription++, Depth++);
	pOption->m_Depth = Depth ? Depth : pOption->m_pPrev ? pOption->m_pPrev->m_Depth : 0;

	pOption->m_IsSubheader = Depth;

	if(!*pDescription)
		pOption->m_Depth = 0;

	str_copy(pOption->m_aDescription, pDescription, sizeof(pOption->m_aDescription));

	if(Config()->m_Debug)
		dbg_msg("voting", "added option '%s' with depth='%d'", pDescription, pOption->m_Depth);

	++m_NumVoteOptions;
}

void CVoting::Clear()
{
	m_Closetime = 0;
	m_aDescription[0] = 0;
	m_aReason[0] = 0;
	m_Yes = m_No = m_Pass = m_Total = 0;
	m_Voted = 0;
	m_CallvoteBlockTick = 0;
}

void CVoting::ClearOptions()
{
	m_Heap.Reset();

	m_NumVoteOptions = 0;
	m_pFirst = 0;
	m_pLast = 0;

	m_pRecycleFirst = 0;
	m_pRecycleLast = 0;
}

void CVoting::OnReset()
{
	if(Client()->State() == IClient::STATE_LOADING)	// do not reset active vote while connecting
		return;

	Clear();
}

void CVoting::OnStateChange(int NewState, int OldState)
{
	if(OldState == IClient::STATE_ONLINE || OldState == IClient::STATE_OFFLINE)
		Clear();
}

void CVoting::OnConsoleInit()
{
	Console()->Register("vote", "r['yes'|'no']", CFGFLAG_CLIENT, ConVote, this, "Vote yes/no");
}

void CVoting::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_VOTESET)
	{
		CNetMsg_Sv_VoteSet *pMsg = (CNetMsg_Sv_VoteSet *)pRawMsg;
		int BlockTick = m_CallvoteBlockTick;
		char aBuf[128];
		if(pMsg->m_Timeout)
		{
			OnReset();
			str_copy(m_aReason, pMsg->m_pReason, sizeof(m_aReason));
			m_Closetime = time_get() + time_freq() * pMsg->m_Timeout;
			if(pMsg->m_ClientID != -1)
			{
				char aLabel[64];
				m_pClient->GetPlayerLabel(aLabel, sizeof(aLabel), pMsg->m_ClientID, m_pClient->m_aClients[pMsg->m_ClientID].m_aName);
				switch(pMsg->m_Type)
				{
				case VOTE_START_OP:
					str_format(aBuf, sizeof(aBuf), Localize("'%s' called vote to change server option '%s' (%s)"), aLabel, pMsg->m_pDescription, pMsg->m_pReason);
					str_copy(m_aDescription, pMsg->m_pDescription, sizeof(m_aDescription));
					m_pClient->m_pChat->AddLine(aBuf);
					break;
				case VOTE_START_KICK:
					{
						char aName[4];
						if(!Config()->m_ClShowsocial)
							str_copy(aName, pMsg->m_pDescription, sizeof(aName));
						str_format(aBuf, sizeof(aBuf), Localize("'%s' called for vote to kick '%s' (%s)"), aLabel, Config()->m_ClShowsocial ? pMsg->m_pDescription : aName, pMsg->m_pReason);
						str_format(m_aDescription, sizeof(m_aDescription), "Kick '%s'", Config()->m_ClShowsocial ? pMsg->m_pDescription : aName);
						m_pClient->m_pChat->AddLine(aBuf);
						break;
					}
				case VOTE_START_SPEC:
					{
						char aName[4];
						if(!Config()->m_ClShowsocial)
							str_copy(aName, pMsg->m_pDescription, sizeof(aName));
						str_format(aBuf, sizeof(aBuf), Localize("'%s' called for vote to move '%s' to spectators (%s)"), aLabel, Config()->m_ClShowsocial ? pMsg->m_pDescription : aName, pMsg->m_pReason);
						str_format(m_aDescription, sizeof(m_aDescription), "Move '%s' to spectators", Config()->m_ClShowsocial ? pMsg->m_pDescription : aName);
						m_pClient->m_pChat->AddLine(aBuf);
					}
				}
				if(pMsg->m_ClientID == m_pClient->m_LocalClientID)
					m_CallvoteBlockTick = Client()->GameTick()+Client()->GameTickSpeed()*VOTE_COOLDOWN;
			}
		}
		else
		{
			switch(pMsg->m_Type)
			{
			case VOTE_START_OP:
				str_format(aBuf, sizeof(aBuf), Localize("Admin forced server option '%s' (%s)"), pMsg->m_pDescription, pMsg->m_pReason);
				m_pClient->m_pChat->AddLine(aBuf);
				break;
			case VOTE_START_SPEC:
				str_format(aBuf, sizeof(aBuf), Localize("Admin moved '%s' to spectator (%s)"), pMsg->m_pDescription, pMsg->m_pReason);
				m_pClient->m_pChat->AddLine(aBuf);
				break;
			case VOTE_END_ABORT:
				OnReset();
				m_pClient->m_pChat->AddLine(Localize("Vote aborted"));
				break;
			case VOTE_END_PASS:
				OnReset();
				m_pClient->m_pChat->AddLine(pMsg->m_ClientID == -1 ? Localize("Admin forced vote yes") : Localize("Vote passed"));
				break;
			case VOTE_END_FAIL:
				OnReset();
				m_pClient->m_pChat->AddLine(pMsg->m_ClientID == -1 ? Localize("Admin forced vote no") : Localize("Vote failed"));
				m_CallvoteBlockTick = BlockTick;
			}
		}
	}
	else if(MsgType == NETMSGTYPE_SV_VOTESTATUS)
	{
		CNetMsg_Sv_VoteStatus *pMsg = (CNetMsg_Sv_VoteStatus *)pRawMsg;
		m_Yes = pMsg->m_Yes;
		m_No = pMsg->m_No;
		m_Pass = pMsg->m_Pass;
		m_Total = pMsg->m_Total;
	}
	else if(MsgType == NETMSGTYPE_SV_VOTECLEAROPTIONS)
	{
		ClearOptions();
	}
	else if(MsgType == NETMSGTYPE_SV_VOTEOPTIONADD)
	{
		CNetMsg_Sv_VoteOptionAdd *pMsg = (CNetMsg_Sv_VoteOptionAdd *)pRawMsg;
		AddOption(pMsg->m_pDescription);
	}
	else if(MsgType == NETMSGTYPE_SV_VOTEOPTIONREMOVE)
	{
		CNetMsg_Sv_VoteOptionRemove *pMsg = (CNetMsg_Sv_VoteOptionRemove *)pRawMsg;

		for(CVoteOptionClient *pOption = m_pFirst; pOption; pOption = pOption->m_pNext)
		{
			if(str_comp(pOption->m_aDescription, pMsg->m_pDescription) == 0)
			{
				// remove it from the list
				if(m_pFirst == pOption)
					m_pFirst = m_pFirst->m_pNext;
				if(m_pLast == pOption)
					m_pLast = m_pLast->m_pPrev;
				if(pOption->m_pPrev)
					pOption->m_pPrev->m_pNext = pOption->m_pNext;
				if(pOption->m_pNext)
					pOption->m_pNext->m_pPrev = pOption->m_pPrev;
				--m_NumVoteOptions;

				// add it to recycle list
				pOption->m_pNext = 0;
				pOption->m_pPrev = m_pRecycleLast;
				if(pOption->m_pPrev)
					pOption->m_pPrev->m_pNext = pOption;
				m_pRecycleLast = pOption;
				if(!m_pRecycleFirst)
					m_pRecycleLast = pOption;

				break;
			}
		}
	}
}

void CVoting::RenderBars(CUIRect Bars)
{
	Bars.Draw(vec4(0.8f,0.8f,0.8f,0.5f), Bars.h/3);

	CUIRect Splitter = Bars;
	Splitter.x = Splitter.x+Splitter.w/2;
	Splitter.w = Splitter.h/2.0f;
	Splitter.x -= Splitter.w/2;
	Splitter.Draw(vec4(0.4f,0.4f,0.4f,0.5f), Splitter.h/4);

	if(m_Total)
	{
		CUIRect PassArea = Bars;
		if(m_Yes)
		{
			CUIRect YesArea = Bars;
			YesArea.w *= m_Yes/(float)m_Total;
			YesArea.Draw(vec4(0.2f,0.9f,0.2f,0.85f), Bars.h/3);

			PassArea.x += YesArea.w;
			PassArea.w -= YesArea.w;
		}

		if(m_No)
		{
			CUIRect NoArea = Bars;
			NoArea.w *= m_No/(float)m_Total;
			NoArea.x = (Bars.x + Bars.w)-NoArea.w;
			NoArea.Draw(vec4(0.9f,0.2f,0.2f,0.85f), Bars.h/3);

			PassArea.w -= NoArea.w;
		}
	}
}
