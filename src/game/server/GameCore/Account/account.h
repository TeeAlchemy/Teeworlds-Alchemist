/* Copyright(C) 2022 - 2024 ST-Chara */
#ifndef GAME_SERVER_GAMECORE_ACCOUNT_H
#define GAME_SERVER_GAMECORE_ACCOUNT_H

#include <game/server/GameCore/TWorldComponent.h>
#include <game/server/gamecontext.h>

#include <base/tl/array.h>

    enum TYPE {
        REG = 0,
        LOG,
        SYNC,
        SAVE,
    };

struct FaBao
{
    // Base
    int m_Type;
    CGameContext *m_pGameServer;
    int m_ClientID;
    CPlayer::SAccData m_AccData;

    // Save / Sync
    int m_Table;
    char m_Language[64];
    SPlayerItemData m_aItems[NUM_ITEM];
};

class CAccount : public TWorldComponent
{
public:
    ~CAccount(){};

    virtual void OnInit();
    bool Register(int ClientID, const char *Username, const char *Password);
    bool Login(int ClientID, const char *Username, const char *Password);
    void SyncAccountData(int ClientID, int Table);
    void SaveAccountData(int ClientID, int Table, CPlayer::SAccData AccData);

public:
    struct AccountPool
    {
        array<FaBao *> m_pFaBao;
    };

    AccountPool *m_pPool;
    static void HandleThread(void *user);
};

#endif