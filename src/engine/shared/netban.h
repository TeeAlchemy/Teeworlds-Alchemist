/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_NETBAN_H
#define ENGINE_SHARED_NETBAN_H

#include <base/system.h>


inline int NetComp(const NETADDR *pAddr1, const NETADDR *pAddr2)
{
	return net_addr_comp(pAddr1, pAddr2, false);
}

class CNetRange
{
public:
	NETADDR m_LB;
	NETADDR m_UB;

	bool IsValid() const { return m_LB.type == m_UB.type && mem_comp(m_LB.ip, m_UB.ip, m_LB.type==NETTYPE_IPV4 ? NETADDR_SIZE_IPV4 : NETADDR_SIZE_IPV6) < 0; }
};

inline int NetComp(const CNetRange *pRange1, const CNetRange *pRange2)
{
	return net_addr_comp(&pRange1->m_LB, &pRange2->m_LB, false) || net_addr_comp(&pRange1->m_UB, &pRange2->m_UB, false);
}


class CNetBan
{
protected:
	bool NetMatch(const NETADDR *pAddr1, const NETADDR *pAddr2) const
	{
		return NetComp(pAddr1, pAddr2) == 0;
	}

	bool NetMatch(const CNetRange *pRange, const NETADDR *pAddr, int Start, int Length) const
	{
		return pRange->m_LB.type == pAddr->type && (Start == 0 || mem_comp(&pRange->m_LB.ip[0], &pAddr->ip[0], Start) == 0) &&
			mem_comp(&pRange->m_LB.ip[Start], &pAddr->ip[Start], Length-Start) <= 0 && mem_comp(&pRange->m_UB.ip[Start], &pAddr->ip[Start], Length-Start) >= 0;
	}

	bool NetMatch(const CNetRange *pRange, const NETADDR *pAddr) const
	{
		return NetMatch(pRange, pAddr, 0,  pRange->m_LB.type==NETTYPE_IPV4 ? 4 : 16);
	}

	const char *NetToString(const NETADDR *pData, char *pBuffer, unsigned BufferSize) const
	{
		char aAddrStr[NETADDR_MAXSTRSIZE];
		net_addr_str(pData, aAddrStr, sizeof(aAddrStr), false);
		str_format(pBuffer, BufferSize, "'%s'", aAddrStr);
		return pBuffer;
	}

	const char *NetToString(const CNetRange *pData, char *pBuffer, unsigned BufferSize) const
	{
		char aAddrStr1[NETADDR_MAXSTRSIZE], aAddrStr2[NETADDR_MAXSTRSIZE];
		net_addr_str(&pData->m_LB, aAddrStr1, sizeof(aAddrStr1), false);
		net_addr_str(&pData->m_UB, aAddrStr2, sizeof(aAddrStr2), false);
		str_format(pBuffer, BufferSize, "'%s' - '%s'", aAddrStr1, aAddrStr2);
		return pBuffer;
	}

	class CNetHash
	{
	public:
		int m_Hash;
		int m_HashIndex;	// matching parts for ranges, 0 for addr

		CNetHash() {}	
		CNetHash(const NETADDR *pAddr);
		CNetHash(const CNetRange *pRange);

		static int MakeHashArray(const NETADDR *pAddr, CNetHash aHash[17]);
	};

	struct CBanInfo
	{
		enum
		{
			EXPIRES_NEVER=-1,
			REASON_LENGTH=64,
		};
		int m_Expires;
		int m_LastInfoQuery;
		char m_aReason[REASON_LENGTH];		
	};

	template<class T> struct CBan
	{
		T m_Data;
		CBanInfo m_Info;
		CNetHash m_NetHash;

		// hash list
		CBan *m_pHashNext;
		CBan *m_pHashPrev;

		// used or free list
		CBan *m_pNext;
		CBan *m_pPrev;
	};

	template<class T, int HashCount> class CBanPool
	{
	public:
		typedef T CDataType;

		CBan<CDataType> *Add(const CDataType *pData, const CBanInfo *pInfo, const CNetHash *pNetHash);
		int Remove(CBan<CDataType> *pBan);
		void Update(CBan<CDataType> *pBan, const CBanInfo *pInfo);
		void Reset();
	
		int Num() const { return m_CountUsed; }
		bool IsFull() const { return m_CountUsed == MAX_BANS; }

		CBan<CDataType> *First() const { return m_pFirstUsed; }
		CBan<CDataType> *First(const CNetHash *pNetHash) const { return m_paaHashList[pNetHash->m_HashIndex][pNetHash->m_Hash]; }
		CBan<CDataType> *Find(const CDataType *pData, const CNetHash *pNetHash) const
		{
			for(CBan<CDataType> *pBan = m_paaHashList[pNetHash->m_HashIndex][pNetHash->m_Hash]; pBan; pBan = pBan->m_pHashNext)
			{
				if(NetComp(&pBan->m_Data, pData) == 0)
					return pBan;
			}

			return 0;
		}
		CBan<CDataType> *Get(int Index) const;

	private:
		enum
		{
			MAX_BANS=1024,
		};

		CBan<CDataType> *m_paaHashList[HashCount][256];
		CBan<CDataType> m_aBans[MAX_BANS];
		CBan<CDataType> *m_pFirstFree;
		CBan<CDataType> *m_pFirstUsed;
		int m_CountUsed;
	};

	typedef CBanPool<NETADDR, 1> CBanAddrPool;
	typedef CBanPool<CNetRange, 16> CBanRangePool;
	typedef CBan<NETADDR> CBanAddr;
	typedef CBan<CNetRange> CBanRange;
	
	template<class T> void MakeBanInfo(CBan<T> *pBan, char *pBuf, unsigned BuffSize, int Type, int *pLastInfoQuery=0);
	template<class T> int Ban(T *pBanPool, const typename T::CDataType *pData, int Seconds, const char *pReason);
	template<class T> int Unban(T *pBanPool, const typename T::CDataType *pData);

	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	CBanAddrPool m_BanAddrPool;
	CBanRangePool m_BanRangePool;
	NETADDR m_LocalhostIPV4, m_LocalhostIPV6;

public:
	enum
	{
		MSGTYPE_PLAYER=0,
		MSGTYPE_LIST,
		MSGTYPE_BANADD,
		MSGTYPE_BANREM,
	};

	class IConsole *Console() const { return m_pConsole; }
	class IStorage *Storage() const { return m_pStorage; }

	virtual ~CNetBan() {}
	void Init(class IConsole *pConsole, class IStorage *pStorage);
	void Update();

	virtual int BanAddr(const NETADDR *pAddr, int Seconds, const char *pReason);
	virtual int BanRange(const CNetRange *pRange, int Seconds, const char *pReason);
	int UnbanByAddr(const NETADDR *pAddr);
	int UnbanByRange(const CNetRange *pRange);
	int UnbanByIndex(int Index);
	void UnbanAll();
	template<class T> bool IsBannable(const T *pData);
	bool IsBanned(const NETADDR *pAddr, char *pBuf, unsigned BufferSize, int *pLastInfoQuery);

	static void ConBan(class IConsole::IResult *pResult, void *pUser);
	static void ConUnban(class IConsole::IResult *pResult, void *pUser);
	static void ConUnbanAll(class IConsole::IResult *pResult, void *pUser);
	static void ConBans(class IConsole::IResult *pResult, void *pUser);
	static void ConBansSave(class IConsole::IResult *pResult, void *pUser);
};

#endif
