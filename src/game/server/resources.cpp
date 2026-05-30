#include "resources.h"

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <teeother/components/localization.h>

#include <string>

/*
placeholder("Wood");placeholder("Coal");placeholder("Copper");placeholder("Iron");placeholder("Gold");placeholder("Diamond");placeholder("Enegry");
placeholder("You received:\n{}");placeholder("You submitted:\n{}");placeholder("Your team received:\n{}");
placeholder("You have collected the resources within the resource point\nBring them back to the base!\n{}");
*/

const char *GetResourceName(int ID)
{
    switch (ID)
    {
    case RESOURCE_WOOD:
        return "Wood";

    case RESOURCE_COAL:
        return "Coal";

    case RESOURCE_COPPER:
        return "Copper";

    case RESOURCE_IRON:
        return "Iron";

    case RESOURCE_GOLD:
        return "Gold";

    case RESOURCE_DIAMOND:
        return "Diamond";

    case RESOURCE_ENEGRY:
        return "Enegry";

    default:
        return "None";
    }
    return "WTF";
}

bool HasResourceAmount(const int aAmounts[NUM_RESOURCE])
{
    for (int i = 0; i < NUM_RESOURCE; i++)
    {
        if (aAmounts[i] > 0)
            return true;
    }
    return false;
}

static std::string BuildResourceListString(CGameContext *pGameServer, int ClientID, const int aAmounts[NUM_RESOURCE])
{
    const char *pLanguage = pGameServer->GetClientLanguage(ClientID);
    CLocalization *pLocalization = pGameServer->Server()->Localization();
    std::string List;

    for (int i = 0; i < NUM_RESOURCE; i++)
    {
        if (aAmounts[i] <= 0)
            continue;

        if (!List.empty())
            List += "\n";

        List += pLocalization->Format(pLanguage, "➳ {} x{}", pLocalization->Localize(pLanguage, GetResourceName(i)), aAmounts[i]);
    }

    return List;
}

static void NotifyResourceGainImpl(CGameContext *pGameServer, int ClientID, vec2 Pos, const int aAmounts[NUM_RESOURCE], int Flags,
    const char *pBroadcastKey, const char *pChatKey)
{
    if (!HasResourceAmount(aAmounts))
        return;

    if ((Flags & RESOURCE_NOTIFY_SOUND) && ClientID >= 0)
        pGameServer->CreateSound(Pos, SOUND_PICKUP_HEALTH, CmaskOne(ClientID));

    if (ClientID >= 0)
    {
        const std::string ResourceList = BuildResourceListString(pGameServer, ClientID, aAmounts);
        if (!ResourceList.empty())
        {
            if (Flags & RESOURCE_NOTIFY_BROADCAST)
                pGameServer->Broadcast(ClientID, pBroadcastKey, ResourceList);

            if ((Flags & RESOURCE_NOTIFY_CHAT) && pChatKey)
                pGameServer->Chat(ClientID, pChatKey, ResourceList);
        }
    }

    if ((Flags & RESOURCE_NOTIFY_VOTE) && ClientID >= 0 && pGameServer->GetPlayer(ClientID) && pGameServer->GetPlayer(ClientID)->m_BuildMenuOpen)
        pGameServer->RefreshBuildMenu(ClientID);
}

void NotifyResourceGain(CGameContext *pGameServer, int ClientID, vec2 Pos, const int aAmounts[NUM_RESOURCE], int Flags,
    const char *pBroadcastKey, const char *pChatKey)
{
    if (!pChatKey)
        pChatKey = pBroadcastKey;

    NotifyResourceGainImpl(pGameServer, ClientID, Pos, aAmounts, Flags, pBroadcastKey, pChatKey);
}

void NotifyTeamResourceGain(CGameContext *pGameServer, int Team, vec2 Pos, const int aAmounts[NUM_RESOURCE], int Flags,
    const char *pBroadcastKey, const char *pChatKey)
{
    if (!HasResourceAmount(aAmounts))
        return;

    if (!pChatKey)
        pChatKey = pBroadcastKey;

    if (Flags & RESOURCE_NOTIFY_SOUND)
        pGameServer->CreateSound(Pos, SOUND_PICKUP_HEALTH);

    const int ClientFlags = Flags & ~RESOURCE_NOTIFY_SOUND;
    for (int ClientID = 0; ClientID < MAX_CLIENTS; ClientID++)
    {
        CPlayer *pPlayer = pGameServer->GetPlayer(ClientID);
        if (!pPlayer || pPlayer->GetTeam() != Team)
            continue;

        NotifyResourceGainImpl(pGameServer, ClientID, Pos, aAmounts, ClientFlags, pBroadcastKey, pChatKey);
    }
}
