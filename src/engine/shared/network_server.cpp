/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/hash_ctxt.h>
#include <base/system.h>

#include "config.h"
#include "netban.h"
#include "network.h"
#include <engine/message.h>
#include <engine/shared/protocol.h>

const int DummyMapCrc = 0x6c760ac4;
unsigned char g_aDummyMapData[] = {
	0x44, 0x41, 0x54, 0x41, 0x04, 0x00, 0x00, 0x00, 0x22, 0x01, 0x00, 0x00,
	0x14, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x1c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00,
	0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0a, 0xc3, 0x52,
	0xff, 0x7f, 0x00, 0x00, 0x20, 0x5c, 0xf6, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x05, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00,
	0x7c, 0x00, 0x00, 0x00, 0x78, 0x9c, 0x63, 0x64, 0x60, 0x60, 0x60, 0x44,
	0xc2, 0x00, 0x00, 0x38, 0x00, 0x05};

static SECURITY_TOKEN ToSecurityToken(const unsigned char *pData)
{
	return (int)pData[0] | (pData[1] << 8) | (pData[2] << 16) | (pData[3] << 24);
}

bool CNetServer::Open(NETADDR BindAddr, CNetBan *pNetBan, int MaxClients, int MaxClientsPerIP)
{
	// zero out the whole structure
	mem_zero(this, sizeof(*this));

	// open socket
	m_Socket = net_udp_create(BindAddr, 0);
	if(!m_Socket)
		return false;

	m_Address = BindAddr;
	m_pNetBan = pNetBan;

	m_MaxClients = clamp(MaxClients, 1, (int)NET_MAX_CLIENTS);
	m_MaxClientsPerIP = MaxClientsPerIP;

	m_NumConAttempts = 0;
	m_TimeNumConAttempts = time_get();

	m_VConnNum = 0;
	m_VConnFirst = 0;

	secure_random_fill(m_aSecurityTokenSeed, sizeof(m_aSecurityTokenSeed));

	for(auto &Slot : m_aSlots)
		Slot.m_Connection.Init(m_Socket, true);

	for(int i = 0; i < NET_MAX_CLIENTS; i++)
	{
		m_aSlots[i].m_Connection.Init(m_Socket, true);
		m_SlotTakenByBot[i] = false;
	}


	return true;
}

int CNetServer::SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser)
{
	m_pfnNewClient = pfnNewClient;
	m_pfnDelClient = pfnDelClient;
	m_pUser = pUser;
	return 0;
}

int CNetServer::SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_NEWCLIENT_NOAUTH pfnNewClientNoAuth, NETFUNC_CLIENTREJOIN pfnClientRejoin, NETFUNC_DELCLIENT pfnDelClient, void *pUser)
{
	m_pfnNewClient = pfnNewClient;
	m_pfnNewClientNoAuth = pfnNewClientNoAuth;
	m_pfnClientRejoin = pfnClientRejoin;
	m_pfnDelClient = pfnDelClient;
	m_pUser = pUser;
	return 0;
}

int CNetServer::Close()
{
	if(!m_Socket)
		return 0;
	return net_udp_close(m_Socket);
}

int CNetServer::Drop(int ClientID, const char *pReason)
{
	// TODO: insert lots of checks here

	if(m_pfnDelClient)
		m_pfnDelClient(ClientID, pReason, m_pUser);

	m_aSlots[ClientID].m_Connection.Disconnect(pReason);

	m_SlotTakenByBot[ClientID] = false;

	return 0;
}

int CNetServer::Update()
{
	for(int i = 0; i < MaxClients(); i++)
	{
		m_aSlots[i].m_Connection.Update();
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR &&
			(!m_aSlots[i].m_Connection.m_TimeoutProtected ||
				!m_aSlots[i].m_Connection.m_TimeoutSituation))
		{
			Drop(i, m_aSlots[i].m_Connection.ErrorString());
		}
	}

	return 0;
}

SECURITY_TOKEN CNetServer::GetGlobalToken()
{
	static NETADDR NullAddr = {0};
	return GetToken(NullAddr);
}
SECURITY_TOKEN CNetServer::GetToken(const NETADDR &Addr)
{
	SHA256_CTX Sha256;
	sha256_init(&Sha256);
	sha256_update(&Sha256, (unsigned char *)m_aSecurityTokenSeed, sizeof(m_aSecurityTokenSeed));
	sha256_update(&Sha256, (unsigned char *)&Addr, 20); // omit port, bad idea!

	SECURITY_TOKEN SecurityToken = ToSecurityToken(sha256_finish(&Sha256).data);

	if(SecurityToken == NET_SECURITY_TOKEN_UNKNOWN ||
		SecurityToken == NET_SECURITY_TOKEN_UNSUPPORTED)
		SecurityToken = 1;

	return SecurityToken;
}

void CNetServer::SendControl(NETADDR &Addr, int ControlMsg, const void *pExtra, int ExtraSize, SECURITY_TOKEN SecurityToken)
{
	CNetBase::SendControlMsg(m_Socket, &Addr, 0, ControlMsg, pExtra, ExtraSize, SecurityToken);
}

int CNetServer::NumClientsWithAddr(NETADDR Addr)
{
	int FoundAddr = 0;
	for(int i = 0; i < MaxClients(); ++i)
	{
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE ||
			(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR &&
				(!m_aSlots[i].m_Connection.m_TimeoutProtected ||
					!m_aSlots[i].m_Connection.m_TimeoutSituation)))
			continue;

		if(!net_addr_comp_noport(&Addr, m_aSlots[i].m_Connection.PeerAddress()))
			FoundAddr++;
	}

	return FoundAddr;
}

bool CNetServer::Connlimit(NETADDR Addr)
{
	int64_t Now = time_get();
	int Oldest = 0;

	for(int i = 0; i < NET_CONNLIMIT_IPS; ++i)
	{
		if(!net_addr_comp(&m_aSpamConns[i].m_Addr, &Addr))
		{
			if(m_aSpamConns[i].m_Time > Now - time_freq() * g_Config.m_SvConnlimitTime)
			{
				if(m_aSpamConns[i].m_Conns >= g_Config.m_SvConnlimit)
					return true;
			}
			else
			{
				m_aSpamConns[i].m_Time = Now;
				m_aSpamConns[i].m_Conns = 0;
			}
			m_aSpamConns[i].m_Conns++;
			return false;
		}

		if(m_aSpamConns[i].m_Time < m_aSpamConns[Oldest].m_Time)
			Oldest = i;
	}

	m_aSpamConns[Oldest].m_Addr = Addr;
	m_aSpamConns[Oldest].m_Time = Now;
	m_aSpamConns[Oldest].m_Conns = 1;
	return false;
}

int CNetServer::TryAcceptClient(NETADDR &Addr, SECURITY_TOKEN SecurityToken, bool VanillaAuth, bool Sixup, SECURITY_TOKEN Token)
{
	if(Sixup)
	{
		const char aMsg[] = "0.7 connections are not accepted at this time";
		CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aMsg, sizeof(aMsg), SecurityToken, Sixup);
		return -1; // failed to add client?
	}

	if(Connlimit(Addr))
	{
		const char aMsg[] = "Too many connections in a short time";
		CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aMsg, sizeof(aMsg), SecurityToken, Sixup);
		return -1; // failed to add client
	}

	// check for sv_max_clients_per_ip
	if(NumClientsWithAddr(Addr) + 1 > m_MaxClientsPerIP)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Only %d players with the same IP are allowed", m_MaxClientsPerIP);
		CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aBuf, str_length(aBuf) + 1, SecurityToken, Sixup);
		return -1; // failed to add client
	}

	int Slot = -1;
	for(int i = 0; i < MaxClients(); i++)
	{
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE && !m_SlotTakenByBot[i])
		{
			Slot = i;
			break;
		}
	}

	if(Slot == -1)
	{
		for(int i = 0; i < MaxClients(); i++)
		{
			if(m_SlotTakenByBot[i])
			{
				Drop(i, "Making room for a real player.");
				m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr);
				if(m_pfnNewClient)
					m_pfnNewClient(i, m_pUser, Sixup);
				break;
			}
		}

		const char aFullMsg[] = "This server is full";
		CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aFullMsg, sizeof(aFullMsg), SecurityToken, Sixup);

		return -1; // failed to add client
	}

	// init connection slot
	m_aSlots[Slot].m_Connection.DirectInit(Addr, SecurityToken, Token, Sixup);

	if(VanillaAuth)
	{
		// client sequence is unknown if the auth was done
		// connection-less
		m_aSlots[Slot].m_Connection.SetUnknownSeq();
		// correct sequence
		m_aSlots[Slot].m_Connection.SetSequence(6);
	}

	if(g_Config.m_Debug)
	{
		char aAddrStr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr), true);
		dbg_msg("security", "client accepted %s", aAddrStr);
	}

	if(VanillaAuth)
		m_pfnNewClientNoAuth(Slot, m_pUser);
	else
		m_pfnNewClient(Slot, m_pUser, Sixup);

	return Slot; // done
}

void CNetServer::SendMsgs(NETADDR &Addr, const CMsgPacker *apMsgs[], int Num)
{
	CNetPacketConstruct Construct;
	mem_zero(&Construct, sizeof(Construct));
	unsigned char *pChunkData = &Construct.m_aChunkData[Construct.m_DataSize];

	for(int i = 0; i < Num; i++)
	{
		const CMsgPacker *pMsg = apMsgs[i];
		CNetChunkHeader Header;
		Header.m_Flags = NET_CHUNKFLAG_VITAL;
		Header.m_Size = pMsg->Size();
		Header.m_Sequence = i + 1;
		pChunkData = Header.Pack(pChunkData);
		mem_copy(pChunkData, pMsg->Data(), pMsg->Size());
		*pChunkData <<= 1;
		*pChunkData |= 1;
		pChunkData += pMsg->Size();
		Construct.m_NumChunks++;
	}

	//
	Construct.m_DataSize = (int)(pChunkData - Construct.m_aChunkData);
	CNetBase::SendPacket(m_Socket, &Addr, &Construct, NET_SECURITY_TOKEN_UNSUPPORTED);
}

// connection-less msg packet without token-support
void CNetServer::OnPreConnMsg(NETADDR &Addr, CNetPacketConstruct &Packet)
{
	bool IsCtrl = Packet.m_Flags & NET_PACKETFLAG_CONTROL;
	int CtrlMsg = m_RecvUnpacker.m_Data.m_aChunkData[0];

	// log flooding
	//TODO: remove
	if(g_Config.m_Debug)
	{
		int64_t Now = time_get();

		if(Now - m_TimeNumConAttempts > time_freq())
			// reset
			m_NumConAttempts = 0;

		m_NumConAttempts++;

		if(m_NumConAttempts > 100)
		{
			dbg_msg("security", "flooding detected");

			m_TimeNumConAttempts = Now;
			m_NumConAttempts = 0;
		}
	}

	if(IsCtrl && CtrlMsg == NET_CTRLMSG_CONNECT)
	{
		if(g_Config.m_SvVanillaAntiSpoof && g_Config.m_Password[0] == '\0')
		{
			bool Flooding = false;

			if(g_Config.m_SvVanConnPerSecond)
			{
				// detect flooding
				Flooding = m_VConnNum > g_Config.m_SvVanConnPerSecond;
				const int64_t Now = time_get();

				if(Now <= m_VConnFirst + time_freq())
				{
					m_VConnNum++;
				}
				else
				{
					m_VConnNum = 1;
					m_VConnFirst = Now;
				}
			}

			if(g_Config.m_Debug && Flooding)
			{
				dbg_msg("security", "vanilla connection flooding detected");
			}

			// simulate accept
			SendControl(Addr, NET_CTRLMSG_CONNECTACCEPT, NULL, 0, NET_SECURITY_TOKEN_UNSUPPORTED);

			// Begin vanilla compatible token handshake
			// The idea is to pack a security token in the gametick
			// parameter of NETMSG_SNAPEMPTY. The Client then will
			// return the token/gametick in NETMSG_INPUT, allowing
			// us to validate the token.
			// https://github.com/eeeee/ddnet/commit/b8e40a244af4e242dc568aa34854c5754c75a39a

			// Before we can send NETMSG_SNAPEMPTY, the client needs
			// to load a map, otherwise it might crash. The map
			// should be as small as is possible and directly available
			// to the client. Therefore a dummy map is sent in the same
			// packet. To reduce the traffic we'll fallback to a default
			// map if there are too many connection attempts at once.

			// send mapchange + map data + con_ready + 3 x empty snap (with token)
			CMsgPacker MapChangeMsg(NETMSG_MAP_CHANGE);

			if(Flooding)
			{
				// Fallback to dm1
				MapChangeMsg.AddString("dm1", 0);
				MapChangeMsg.AddInt(0xf2159e6e);
				MapChangeMsg.AddInt(5805);
			}
			else
			{
				// dummy map
				MapChangeMsg.AddString("dummy", 0);
				MapChangeMsg.AddInt(DummyMapCrc);
				MapChangeMsg.AddInt(sizeof(g_aDummyMapData));
			}

			CMsgPacker MapDataMsg(NETMSG_MAP_DATA);

			if(Flooding)
			{
				// send empty map data to keep 0.6.4 support
				MapDataMsg.AddInt(1); // last chunk
				MapDataMsg.AddInt(0); // crc
				MapDataMsg.AddInt(0); // chunk index
				MapDataMsg.AddInt(0); // map size
				MapDataMsg.AddRaw(NULL, 0); // map data
			}
			else
			{
				// send dummy map data
				MapDataMsg.AddInt(1); // last chunk
				MapDataMsg.AddInt(DummyMapCrc); // crc
				MapDataMsg.AddInt(0); // chunk index
				MapDataMsg.AddInt(sizeof(g_aDummyMapData)); // map size
				MapDataMsg.AddRaw(g_aDummyMapData, sizeof(g_aDummyMapData)); // map data
			}

			CMsgPacker ConReadyMsg(NETMSG_CON_READY);

			CMsgPacker SnapEmptyMsg(NETMSG_SNAPEMPTY);
			SECURITY_TOKEN SecurityToken = GetVanillaToken(Addr);
			SnapEmptyMsg.AddInt(SecurityToken);
			SnapEmptyMsg.AddInt(SecurityToken + 1);

			// send all chunks/msgs in one packet
			const CMsgPacker *apMsgs[] = {&MapChangeMsg, &MapDataMsg, &ConReadyMsg,
				&SnapEmptyMsg, &SnapEmptyMsg, &SnapEmptyMsg};
			SendMsgs(Addr, apMsgs, 6);
		}
		else
		{
			// accept client directly
			SendControl(Addr, NET_CTRLMSG_CONNECTACCEPT, NULL, 0, NET_SECURITY_TOKEN_UNSUPPORTED);

			TryAcceptClient(Addr, NET_SECURITY_TOKEN_UNSUPPORTED);
		}
	}
	else if(!IsCtrl && g_Config.m_SvVanillaAntiSpoof && g_Config.m_Password[0] == '\0')
	{
		CNetChunkHeader h;

		unsigned char *pData = Packet.m_aChunkData;
		pData = h.Unpack(pData);
		CUnpacker Unpacker;
		Unpacker.Reset(pData, h.m_Size);
		int Msg = Unpacker.GetInt() >> 1;

		if(Msg == NETMSG_INPUT)
		{
			SECURITY_TOKEN SecurityToken = Unpacker.GetInt();
			if(SecurityToken == GetVanillaToken(Addr))
			{
				if(g_Config.m_Debug)
					dbg_msg("security", "new client (vanilla handshake)");
				// try to accept client skipping auth state
				TryAcceptClient(Addr, NET_SECURITY_TOKEN_UNSUPPORTED, true);
			}
			else if(g_Config.m_Debug)
				dbg_msg("security", "invalid token (vanilla handshake)");
		}
		else
		{
			if(g_Config.m_Debug)
			{
				dbg_msg("security", "invalid preconn msg %d", Msg);
			}
		}
	}
}

void CNetServer::OnConnCtrlMsg(NETADDR &Addr, int ClientID, int ControlMsg, const CNetPacketConstruct &Packet)
{
	if(ControlMsg == NET_CTRLMSG_CONNECT)
	{
		// got connection attempt inside of valid session
		// the client probably wants to reconnect
		bool SupportsToken = Packet.m_DataSize >=
					     (int)(1 + sizeof(SECURITY_TOKEN_MAGIC) + sizeof(SECURITY_TOKEN)) &&
				     !mem_comp(&Packet.m_aChunkData[1], SECURITY_TOKEN_MAGIC, sizeof(SECURITY_TOKEN_MAGIC));

		if(SupportsToken)
		{
			// response connection request with token
			SECURITY_TOKEN Token = GetToken(Addr);
			SendControl(Addr, NET_CTRLMSG_CONNECTACCEPT, SECURITY_TOKEN_MAGIC, sizeof(SECURITY_TOKEN_MAGIC), Token);
		}

		if(g_Config.m_Debug)
			dbg_msg("security", "client %d wants to reconnect", ClientID);
	}
	else if(ControlMsg == NET_CTRLMSG_ACCEPT && Packet.m_DataSize == 1 + sizeof(SECURITY_TOKEN))
	{
		SECURITY_TOKEN Token = ToSecurityToken(&Packet.m_aChunkData[1]);
		if(Token == GetToken(Addr))
		{
			// correct token
			// try to accept client
			if(g_Config.m_Debug)
				dbg_msg("security", "client %d reconnect", ClientID);

			// reset netconn and process rejoin
			m_aSlots[ClientID].m_Connection.Reset(true);
			m_pfnClientRejoin(ClientID, m_pUser);
		}
	}
}

void CNetServer::OnTokenCtrlMsg(NETADDR &Addr, int ControlMsg, const CNetPacketConstruct &Packet)
{
	if(ClientExists(Addr))
		return; // silently ignore

	if(Addr.type == NETTYPE_IPV4)
	{
		// websocket client doesn't send token
		// direct accept
		SendControl(Addr, NET_CTRLMSG_CONNECTACCEPT, SECURITY_TOKEN_MAGIC, sizeof(SECURITY_TOKEN_MAGIC), NET_SECURITY_TOKEN_UNSUPPORTED);
		TryAcceptClient(Addr, NET_SECURITY_TOKEN_UNSUPPORTED);
	}
	else if(ControlMsg == NET_CTRLMSG_CONNECT)
	{
		// response connection request with token
		SECURITY_TOKEN Token = GetToken(Addr);
		SendControl(Addr, NET_CTRLMSG_CONNECTACCEPT, SECURITY_TOKEN_MAGIC, sizeof(SECURITY_TOKEN_MAGIC), Token);
	}
	else if(ControlMsg == NET_CTRLMSG_ACCEPT)
	{
		SECURITY_TOKEN Token = ToSecurityToken(&Packet.m_aChunkData[1]);
		if(Token == GetToken(Addr))
		{
			// correct token
			// try to accept client
			if(g_Config.m_Debug)
				dbg_msg("security", "new client (ddnet token)");
			TryAcceptClient(Addr, Token);
		}
		else
		{
			// invalid token
			if(g_Config.m_Debug)
				dbg_msg("security", "invalid token");
		}
	}
}

int CNetServer::OnSixupCtrlMsg(NETADDR &Addr, CNetChunk *pChunk, int ControlMsg, const CNetPacketConstruct &Packet, SECURITY_TOKEN &ResponseToken, SECURITY_TOKEN Token)
{
	if(m_RecvUnpacker.m_Data.m_DataSize < 5 || ClientExists(Addr))
		return 0; // silently ignore

	mem_copy(&ResponseToken, Packet.m_aChunkData + 1, 4);

	if(ControlMsg == 5)
	{
		if(m_RecvUnpacker.m_Data.m_DataSize >= 512)
		{
			SendTokenSixup(Addr, ResponseToken);
			return 0;
		}

		// Is this behaviour safe to rely on?
		pChunk->m_Flags = 0;
		pChunk->m_ClientID = -1;
		pChunk->m_Address = Addr;
		pChunk->m_DataSize = 0;
		return 1;
	}
	else if(ControlMsg == NET_CTRLMSG_CONNECT)
	{
		SECURITY_TOKEN MyToken = GetToken(Addr);
		unsigned char aToken[4];
		mem_copy(aToken, &MyToken, 4);

		CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CONNECTACCEPT, aToken, sizeof(aToken), ResponseToken, true);
		if(Token == MyToken)
			TryAcceptClient(Addr, ResponseToken, false, true, Token);
	}

	return 0;
}

int CNetServer::GetClientSlot(const NETADDR &Addr)
{
	int Slot = -1;

	for(int i = 0; i < MaxClients(); i++)
	{
		if(!m_SlotTakenByBot[i] && 
			m_aSlots[i].m_Connection.State() != NET_CONNSTATE_OFFLINE &&
			m_aSlots[i].m_Connection.State() != NET_CONNSTATE_ERROR &&
			net_addr_comp(m_aSlots[i].m_Connection.PeerAddress(), &Addr) == 0)

		{
			Slot = i;
		}
	}

	return Slot;
}

static bool IsDDNetControlMsg(const CNetPacketConstruct *pPacket)
{
	if(!(pPacket->m_Flags & NET_PACKETFLAG_CONTROL) || pPacket->m_DataSize < 1)
	{
		return false;
	}
	if(pPacket->m_aChunkData[0] == NET_CTRLMSG_CONNECT && pPacket->m_DataSize >= (int)(1 + sizeof(SECURITY_TOKEN_MAGIC) + sizeof(SECURITY_TOKEN)) && mem_comp(&pPacket->m_aChunkData[1], SECURITY_TOKEN_MAGIC, sizeof(SECURITY_TOKEN_MAGIC)) == 0)
	{
		// DDNet CONNECT
		return true;
	}
	if(pPacket->m_aChunkData[0] == NET_CTRLMSG_ACCEPT && pPacket->m_DataSize >= 1 + (int)sizeof(SECURITY_TOKEN))
	{
		// DDNet ACCEPT
		return true;
	}
	return false;
}

/*
	TODO: chopp up this function into smaller working parts
*/
int CNetServer::Recv(CNetChunk *pChunk, SECURITY_TOKEN *pResponseToken)
{
	while(true)
	{
		NETADDR Addr;

		// check for a chunk
		if(m_RecvUnpacker.FetchChunk(pChunk))
			return 1;

		// TODO: empty the recvinfo
		unsigned char *pData;
		int Bytes = net_udp_recv(m_Socket, &Addr, &pData);

		// no more packets for now
		if(Bytes <= 0)
			break;

		// check if we just should drop the packet
		char aBuf[128];
		if(NetBan() && NetBan()->IsBanned(&Addr, aBuf, sizeof(aBuf)))
		{
			// banned, reply with a message
			CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aBuf, str_length(aBuf) + 1, NET_SECURITY_TOKEN_UNSUPPORTED);
			continue;
		}

		SECURITY_TOKEN Token;
		bool Sixup = false;
		*pResponseToken = NET_SECURITY_TOKEN_UNKNOWN;
		if(CNetBase::UnpackPacket(pData, Bytes, &m_RecvUnpacker.m_Data, Sixup, &Token, pResponseToken) == 0)
		{
			if(m_RecvUnpacker.m_Data.m_Flags & NET_PACKETFLAG_CONNLESS)
			{
				if(Sixup && Token != GetToken(Addr) && Token != GetGlobalToken())
					continue;

				pChunk->m_Flags = NETSENDFLAG_CONNLESS;
				pChunk->m_ClientID = -1;
				pChunk->m_Address = Addr;
				pChunk->m_DataSize = m_RecvUnpacker.m_Data.m_DataSize;
				pChunk->m_pData = m_RecvUnpacker.m_Data.m_aChunkData;
				if(m_RecvUnpacker.m_Data.m_Flags & NET_PACKETFLAG_EXTENDED)
				{
					pChunk->m_Flags |= NETSENDFLAG_EXTENDED;
					mem_copy(pChunk->m_aExtraData, m_RecvUnpacker.m_Data.m_aExtraData, sizeof(pChunk->m_aExtraData));
				}
				return 1;
			}
			else
			{
				// drop invalid ctrl packets
				if(m_RecvUnpacker.m_Data.m_Flags & NET_PACKETFLAG_CONTROL &&
					m_RecvUnpacker.m_Data.m_DataSize == 0)
					continue;

				// normal packet, find matching slot
				int Slot = GetClientSlot(Addr);

				if(!Sixup && Slot != -1 && m_aSlots[Slot].m_Connection.m_Sixup)
				{
					Sixup = true;
					if(CNetBase::UnpackPacket(pData, Bytes, &m_RecvUnpacker.m_Data, Sixup, &Token))
						continue;
				}

				if(Slot != -1)
				{
					// found

					// control
					if(m_RecvUnpacker.m_Data.m_Flags & NET_PACKETFLAG_CONTROL)
						OnConnCtrlMsg(Addr, Slot, m_RecvUnpacker.m_Data.m_aChunkData[0], m_RecvUnpacker.m_Data);

					if(m_aSlots[Slot].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr, Token))
					{
						if(m_RecvUnpacker.m_Data.m_DataSize)
							m_RecvUnpacker.Start(&Addr, &m_aSlots[Slot].m_Connection, Slot);
					}
				}
				else
				{
					// not found, client that wants to connect

					if(Sixup)
					{
						// got 0.7 control msg
						if(OnSixupCtrlMsg(Addr, pChunk, m_RecvUnpacker.m_Data.m_aChunkData[0], m_RecvUnpacker.m_Data, *pResponseToken, Token) == 1)
							return 1;
					}
					else if(IsDDNetControlMsg(&m_RecvUnpacker.m_Data))
						// got ddnet control msg
						OnTokenCtrlMsg(Addr, m_RecvUnpacker.m_Data.m_aChunkData[0], m_RecvUnpacker.m_Data);
					else
						// got connection-less ctrl or sys msg
						OnPreConnMsg(Addr, m_RecvUnpacker.m_Data);
				}
			}
		}
	}
	return 0;
}

int CNetServer::Send(CNetChunk *pChunk)
{
	if(pChunk->m_DataSize >= NET_MAX_PAYLOAD)
	{
		dbg_msg("netserver", "packet payload too big. %d. dropping packet", pChunk->m_DataSize);
		return -1;
	}

	if(pChunk->m_Flags & NETSENDFLAG_CONNLESS)
	{
		// send connectionless packet
		CNetBase::SendPacketConnless(m_Socket, &pChunk->m_Address, pChunk->m_pData, pChunk->m_DataSize,
			pChunk->m_Flags & NETSENDFLAG_EXTENDED, pChunk->m_aExtraData);
	}
	else
	{
		int Flags = 0;
		dbg_assert(pChunk->m_ClientID >= 0, "erroneous client id");
		dbg_assert(pChunk->m_ClientID < MAX_CLIENTS, "erroneous client id");

		// might crash, fatal error
		if (m_SlotTakenByBot[pChunk->m_ClientID])
			return -1;

		if(pChunk->m_Flags & NETSENDFLAG_VITAL)
			Flags = NET_CHUNKFLAG_VITAL;

		if(m_aSlots[pChunk->m_ClientID].m_Connection.QueueChunk(Flags, pChunk->m_DataSize, pChunk->m_pData) == 0)
		{
			if(pChunk->m_Flags & NETSENDFLAG_FLUSH)
				m_aSlots[pChunk->m_ClientID].m_Connection.Flush();
		}
		else
		{
			//Drop(pChunk->m_ClientID, "Error sending data");
		}
	}
	return 0;
}

void CNetServer::SendTokenSixup(NETADDR &Addr, SECURITY_TOKEN Token)
{
	SECURITY_TOKEN MyToken = GetToken(Addr);
	unsigned char aBuf[512] = {};
	mem_copy(aBuf, &MyToken, 4);
	int Size = (Token == NET_SECURITY_TOKEN_UNKNOWN) ? 512 : 4;
	CNetBase::SendControlMsg(m_Socket, &Addr, 0, 5, aBuf, Size, Token, true);
}

int CNetServer::SendConnlessSixup(CNetChunk *pChunk, SECURITY_TOKEN ResponseToken)
{
	if(pChunk->m_DataSize > NET_MAX_PACKETSIZE - 9)
		return -1;

	unsigned char aBuffer[NET_MAX_PACKETSIZE];
	aBuffer[0] = NET_PACKETFLAG_CONNLESS << 2 | 1;
	SECURITY_TOKEN Token = GetToken(pChunk->m_Address);
	mem_copy(aBuffer + 1, &ResponseToken, 4);
	mem_copy(aBuffer + 5, &Token, 4);
	mem_copy(aBuffer + 9, pChunk->m_pData, pChunk->m_DataSize);
	net_udp_send(m_Socket, &pChunk->m_Address, aBuffer, pChunk->m_DataSize + 9);

	return 0;
}

void CNetServer::SetMaxClientsPerIP(int Max)
{
	// clamp
	if(Max < 1)
		Max = 1;
	else if(Max > NET_MAX_CLIENTS)
		Max = NET_MAX_CLIENTS;

	m_MaxClientsPerIP = Max;
}

bool CNetServer::SetTimedOut(int ClientID, int OrigID)
{
	if(m_aSlots[ClientID].m_Connection.State() != NET_CONNSTATE_ERROR)
		return false;

	m_aSlots[ClientID].m_Connection.SetTimedOut(ClientAddr(OrigID), m_aSlots[OrigID].m_Connection.SeqSequence(), m_aSlots[OrigID].m_Connection.AckSequence(), m_aSlots[OrigID].m_Connection.SecurityToken(), m_aSlots[OrigID].m_Connection.ResendBuffer(), m_aSlots[OrigID].m_Connection.m_Sixup);
	m_aSlots[OrigID].m_Connection.Reset();
	return true;
}

void CNetServer::SetTimeoutProtected(int ClientID)
{
	m_aSlots[ClientID].m_Connection.m_TimeoutProtected = true;
}

int CNetServer::ResetErrorString(int ClientID)
{
	m_aSlots[ClientID].m_Connection.ResetErrorString();
	return 0;
}

const char *CNetServer::ErrorString(int ClientID)
{
	return m_aSlots[ClientID].m_Connection.ErrorString();
}
