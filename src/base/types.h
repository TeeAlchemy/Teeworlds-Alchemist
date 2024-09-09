#ifndef BASE_TYPES_H
#define BASE_TYPES_H

#include <ctime>

enum class TRISTATE
{
	NONE,
	SOME,
	ALL,
};

typedef void *IOHANDLE;

typedef int (*FS_LISTDIR_CALLBACK)(const char *name, int is_dir, int dir_type, void *user);

typedef struct
{
	const char *m_pName;
	time_t m_TimeCreated; // seconds since UNIX Epoch
	time_t m_TimeModified; // seconds since UNIX Epoch
} CFsFileInfo;

typedef int (*FS_LISTDIR_CALLBACK_FILEINFO)(const CFsFileInfo *info, int is_dir, int dir_type, void *user);

/**
 * @ingroup Network-General
 */
typedef struct NETSOCKET_INTERNAL *NETSOCKET;

enum
{
	/**
	 * The maximum bytes necessary to encode one Unicode codepoint with UTF-8.
	 */
	UTF8_BYTE_LENGTH = 4,

	IO_MAX_PATH_LENGTH = 512,

	NETADDR_MAXSTRSIZE = 1 + (8 * 4 + 7) + 1 + 1 + 5 + 1, // [XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX]:XXXXX

	NETTYPE_LINK_BROADCAST = 4,

	NETTYPE_INVALID = 0,
	NETTYPE_IPV4 = 1,
	NETTYPE_IPV6 = 2,
	NETTYPE_WEBSOCKET_IPV4 = 8,
	/**
	 * 0.7 address. This is a flag in NETADDR to avoid introducing a parameter to every networking function
	 * to differenciate between 0.6 and 0.7 connections.
	 */
	NETTYPE_TW7 = 16,

	NETTYPE_ALL = NETTYPE_IPV4 | NETTYPE_IPV6 | NETTYPE_WEBSOCKET_IPV4,
	NETTYPE_MASK = NETTYPE_ALL | NETTYPE_LINK_BROADCAST | NETTYPE_TW7,
};

/**
 * @ingroup Network-General
 */
typedef struct NETADDR
{
	unsigned int type;
	unsigned char ip[16];
	unsigned short port;

	bool operator==(const NETADDR &other) const;
	bool operator!=(const NETADDR &other) const { return !(*this == other); }
} NETADDR;

struct THREAD_RUN
{
	void (*threadfunc)(void *);
	void *u;
};

// Reset color
#define COLOR_RESET "\033[0m"

// Basic colors
#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

// Bright colors
#define COLOR_BLACK_BRIGHT "\033[90m"
#define COLOR_RED_BRIGHT "\033[91m"
#define COLOR_GREEN_BRIGHT "\033[92m"
#define COLOR_YELLOW_BRIGHT "\033[93m"
#define COLOR_BLUE_BRIGHT "\033[94m"
#define COLOR_MAGENTA_BRIGHT "\033[95m"
#define COLOR_CYAN_BRIGHT "\033[96m"
#define COLOR_WHITE_BRIGHT "\033[97m"

// Background colors
#define COLOR_BLACK_BG "\033[40m"
#define COLOR_RED_BG "\033[41m"
#define COLOR_GREEN_BG "\033[42m"
#define COLOR_YELLOW_BG "\033[43m"
#define COLOR_BLUE_BG "\033[44m"
#define COLOR_MAGENTA_BG "\033[45m"
#define COLOR_CYAN_BG "\033[46m"
#define COLOR_WHITE_BG "\033[47m"

// Bright background colors
#define COLOR_BLACK_BG_BRIGHT "\033[100m"
#define COLOR_RED_BG_BRIGHT "\033[101m"
#define COLOR_GREEN_BG_BRIGHT "\033[102m"
#define COLOR_YELLOW_BG_BRIGHT "\033[103m"
#define COLOR_BLUE_BG_BRIGHT "\033[104m"
#define COLOR_MAGENTA_BG_BRIGHT "\033[105m"
#define COLOR_CYAN_BG_BRIGHT "\033[106m"
#define COLOR_WHITE_BG_BRIGHT "\033[107m"


#endif // BASE_TYPES_H
