/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CONSOLE_H
#define ENGINE_CONSOLE_H

#include "kernel.h"

class IConsole : public IInterface
{
	MACRO_INTERFACE("console", 0)
public:

	//	TODO: rework/cleanup
	enum
	{
		OUTPUT_LEVEL_STANDARD=0,
		OUTPUT_LEVEL_ADDINFO,
		OUTPUT_LEVEL_DEBUG,

		ACCESS_LEVEL_ADMIN=0,
		ACCESS_LEVEL_MOD,

		TEMPCMD_NAME_LENGTH=48,
		TEMPCMD_HELP_LENGTH=128,
		TEMPCMD_PARAMS_LENGTH=96,

		TEMPMAP_NAME_LENGTH = 32,

		MAX_PRINT_CB=4,
	};

	// TODO: rework this interface to reduce the amount of virtual calls
	class IResult
	{
	protected:
		unsigned m_NumArgs;
	public:
		int m_Value;
		char m_aValue[128];
		IResult() { m_NumArgs = 0; }
		virtual ~IResult() {}

		virtual int GetInteger(unsigned Index) = 0;
		virtual float GetFloat(unsigned Index) = 0;
		virtual const char *GetString(unsigned Index) = 0;

		int NumArguments() const { return m_NumArgs; }
	};

	class CCommandInfo
	{
	protected:
		int m_AccessLevel;
	public:
		CCommandInfo(bool BasicAccess) { m_AccessLevel = BasicAccess ? ACCESS_LEVEL_MOD : ACCESS_LEVEL_ADMIN; }
		virtual ~CCommandInfo() {}
		const char *m_pName;
		const char *m_pHelp;
		const char *m_pParams;

		virtual const CCommandInfo *NextCommandInfo(int AccessLevel, int FlagMask) const = 0;

		int GetAccessLevel() const { return m_AccessLevel; }
	};

	typedef void (*FPrintCallback)(const char *pStr, void *pUser, bool Highlighted);
	typedef void (*FPossibleCallback)(int Index, const char *pCmd, void *pUser);
	typedef void (*FCommandCallback)(IResult *pResult, void *pUserData);
	typedef void (*FChainCommandCallback)(IResult *pResult, void *pUserData, FCommandCallback pfnCallback, void *pCallbackUserData);

	static void EmptyPossibleCommandCallback(int Index, const char *pCmd, void *pUser) {}

	virtual void Init() = 0;
	virtual const CCommandInfo *FirstCommandInfo(int AccessLevel, int Flagmask) const = 0;
	virtual const CCommandInfo *GetCommandInfo(const char *pName, int FlagMask, bool Temp) = 0;
	virtual int PossibleCommands(const char *pStr, int FlagMask, bool Temp, FPossibleCallback pfnCallback = EmptyPossibleCommandCallback, void *pUser = 0) = 0;
	virtual int PossibleMaps(const char *pStr, FPossibleCallback pfnCallback = EmptyPossibleCommandCallback, void *pUser = 0) = 0;
	virtual void ParseArguments(int NumArgs, const char **ppArguments) = 0;

	virtual void Register(const char *pName, const char *pParams, int Flags, FCommandCallback pfnFunc, void *pUser, const char *pHelp) = 0;
	virtual void RegisterTemp(const char *pName, const char *pParams, int Flags, const char *pHelp) = 0;
	virtual void DeregisterTemp(const char *pName) = 0;
	virtual void DeregisterTempAll() = 0;
	virtual void RegisterTempMap(const char *pName) = 0;
	virtual void DeregisterTempMap(const char *pName) = 0;
	virtual void DeregisterTempMapAll() = 0;
	virtual void Chain(const char *pName, FChainCommandCallback pfnChainFunc, void *pUser) = 0;
	virtual void StoreCommands(bool Store) = 0;

	virtual bool ArgStringIsValid(const char *pFormat) = 0;
	virtual bool LineIsValid(const char *pStr) = 0;
	virtual void ExecuteLine(const char *pStr) = 0;
	virtual void ExecuteLineFlag(const char *pStr, int FlagMask) = 0;
	virtual void ExecuteLineStroked(int Stroke, const char *pStr) = 0;
	virtual bool ExecuteFile(const char *pFilename) = 0;

	virtual int RegisterPrintCallback(int OutputLevel, FPrintCallback pfnPrintCallback, void *pUserData) = 0;
	virtual void SetPrintOutputLevel(int Index, int OutputLevel) = 0;
	virtual void Print(int Level, const char *pFrom, const char *pStr, bool Highlighted=false) = 0;

	virtual int ParseCommandArgs(const char *pArgs, const char *pFormat, FCommandCallback pfnCallback, void *pContext) = 0;

	virtual void SetAccessLevel(int AccessLevel) = 0;
};

extern IConsole *CreateConsole(int FlagMask);

#endif // FILE_ENGINE_CONSOLE_H
