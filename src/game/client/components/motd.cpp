/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>

#include <generated/protocol.h>
#include <generated/client_data.h>
#include <game/client/gameclient.h>

#include "menus.h"
#include "motd.h"

void CMotd::Clear()
{
	m_ServerMotdTime = 0;
}

bool CMotd::IsActive()
{
	// dont render modt if the menu is active
	if(m_pClient->m_pMenus->IsActive())
		return false;
	return time_get() < m_ServerMotdTime;
}

void CMotd::OnStateChange(int NewState, int OldState)
{
	if(OldState == IClient::STATE_ONLINE || OldState == IClient::STATE_OFFLINE)
		Clear();
}

void CMotd::OnRender()
{
	if(!IsActive())
		return;

	const float Height = 400.0f * 3.0f;
	const float Width = Height * Graphics()->ScreenAspect();

	Graphics()->MapScreen(0, 0, Width, Height);

	const int MaxLines = 24;
	const float TextSize = 32.0f;

	float h = MaxLines * TextSize + 2 * 25.0f;
	float w = 650.0f;
	float x = Width/2 - w/2;
	float y = 150.0f;
	CUIRect Rect = {x, y, w, h};

	Graphics()->BlendNormal();
	Rect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.5f), 30.0f);

	Rect.Margin(25.0f, &Rect);

	m_ServerMotdCursor.m_Flags = TEXTFLAG_ALLOW_NEWLINE | TEXTFLAG_WORD_WRAP | TEXTFLAG_ELLIPSIS;
	m_ServerMotdCursor.m_FontSize = TextSize;
	m_ServerMotdCursor.m_MaxWidth = Rect.w;
	m_ServerMotdCursor.m_MaxLines = MaxLines;

	m_ServerMotdCursor.Reset(m_ServerMotdTime);
	m_ServerMotdCursor.MoveTo(Rect.x, Rect.y);
	TextRender()->TextOutlined(&m_ServerMotdCursor, m_aServerMotd, -1);
}

void CMotd::OnMessage(int MsgType, void *pRawMsg)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	if(MsgType == NETMSGTYPE_SV_MOTD)
	{
		CNetMsg_Sv_Motd *pMsg = (CNetMsg_Sv_Motd *)pRawMsg;

		// process escaping
		str_copy(m_aServerMotd, pMsg->m_pMessage, sizeof(m_aServerMotd));
		for(int i = 0; m_aServerMotd[i]; i++)
		{
			if(m_aServerMotd[i] == '\\')
			{
				if(m_aServerMotd[i+1] == 'n')
				{
					m_aServerMotd[i] = ' ';
					m_aServerMotd[i+1] = '\n';
					i++;
				}
			}
		}

		if(m_aServerMotd[0] && Config()->m_ClMotdTime)
			m_ServerMotdTime = time_get()+time_freq()*Config()->m_ClMotdTime;
		else
			m_ServerMotdTime = 0;
	}
}

bool CMotd::OnInput(IInput::CEvent Event)
{
	if(IsActive() && Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		Clear();
		return true;
	}
	return false;
}

