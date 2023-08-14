/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_CONFIG_H
#define ENGINE_SHARED_CONFIG_H

#include <engine/config.h>
#include "protocol.h"

class CConfig
{
public:
	#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) int m_##Name;
	#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) char m_##Name[Len]; // Flawfinder: ignore
	#define MACRO_CONFIG_UTF8STR(Name,ScriptName,Size,Len,Def,Save,Desc) char m_##Name[Size]; // Flawfinder: ignore

	#include "config_variables.h"
	
	#undef MACRO_CONFIG_INT
	#undef MACRO_CONFIG_STR
	#undef MACRO_CONFIG_UTF8STR
};

enum
{
	CFGFLAG_SAVE=1,
	CFGFLAG_CLIENT=2,
	CFGFLAG_SERVER=4,
	CFGFLAG_STORE=8,
	CFGFLAG_MASTER=16,
	CFGFLAG_ECON=32,
	CFGFLAG_BASICACCESS=64,
};

class CConfigManager : public IConfigManager
{
	enum
	{
		MAX_CALLBACKS = 16
	};

	struct CCallback
	{
		SAVECALLBACKFUNC m_pfnFunc;
		void *m_pUserData;
	};

	class IStorage *m_pStorage;
	class IConsole *m_pConsole;
	IOHANDLE m_ConfigFile;
	int m_FlagMask;
	CCallback m_aCallbacks[MAX_CALLBACKS];
	int m_NumCallbacks;
	CConfig m_Values;

public:
	CConfigManager();

	virtual void Init(int FlagMask);
	virtual void Reset();
	virtual void RestoreStrings();
	virtual void Save(const char *pFilename);
	virtual CConfig *Values() { return &m_Values; }

	virtual void RegisterCallback(SAVECALLBACKFUNC pfnFunc, void *pUserData);

	virtual void WriteLine(const char *pLine);
};

#endif
