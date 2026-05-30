#ifndef GAME_SERVER_RESOURCES_H
#define GAME_SERVER_RESOURCES_H

#include <base/vmath.h>

enum
{
    RESOURCE_WOOD = 0,
    RESOURCE_COAL,
    RESOURCE_COPPER,
    RESOURCE_IRON,
    RESOURCE_GOLD,
    RESOURCE_DIAMOND,
    RESOURCE_ENEGRY,
    NUM_RESOURCE,
};

enum
{
    RESOURCE_NOTIFY_BROADCAST = 1 << 0,
    RESOURCE_NOTIFY_CHAT = 1 << 1,
    RESOURCE_NOTIFY_SOUND = 1 << 2,
    RESOURCE_NOTIFY_VOTE = 1 << 3,
    RESOURCE_NOTIFY_PERSONAL = RESOURCE_NOTIFY_BROADCAST | RESOURCE_NOTIFY_SOUND | RESOURCE_NOTIFY_VOTE,
    RESOURCE_NOTIFY_SUBMIT = RESOURCE_NOTIFY_BROADCAST | RESOURCE_NOTIFY_CHAT | RESOURCE_NOTIFY_SOUND | RESOURCE_NOTIFY_VOTE,
    RESOURCE_NOTIFY_TEAM = RESOURCE_NOTIFY_BROADCAST | RESOURCE_NOTIFY_CHAT | RESOURCE_NOTIFY_SOUND | RESOURCE_NOTIFY_VOTE,
};

class CGameContext;

const char *GetResourceName(int ID);
bool HasResourceAmount(const int aAmounts[NUM_RESOURCE]);
void NotifyResourceGain(CGameContext *pGameServer, int ClientID, vec2 Pos, const int aAmounts[NUM_RESOURCE], int Flags,
    const char *pBroadcastKey = "You received:\n{}", const char *pChatKey = nullptr);
void NotifyTeamResourceGain(CGameContext *pGameServer, int Team, vec2 Pos, const int aAmounts[NUM_RESOURCE], int Flags,
    const char *pBroadcastKey = "Your team received:\n{}", const char *pChatKey = nullptr);

#endif