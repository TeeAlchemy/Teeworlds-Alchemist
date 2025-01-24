/* Copyright(C) 2022 - 2024 ST-Chara */
#include "account.h"
#include <engine/server/database/connection_pool.h>
#include <engine/server/database/sql_string_helpers.h>
#include <engine/shared/config.h>

#include <game/server/GameCore/TWorldController.h>
#include <game/server/player.h>

#include <thread>
void CAccount::OnInit()
{
}

static void register_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    int ClientID = Data->m_ClientID;
    CPlayer *P = Data->m_pGameServer->GetPlayer(ClientID);
    if (!P)
    {
        delete Data;
        return;
    }

    CSqlConnection *pConn = CConnectionPool::GetConnPool()->GetOneConn();

    auto Username = CSqlString<64>(Data->m_AccData.m_aUsername);
    auto Password = CSqlString<64>(Data->m_AccData.m_aPassword);

    if (pConn)
    {
        char aBuf[512];
        str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s';", Username.ClrStr());
        pConn->Query(aBuf);
        if (pConn->m_pResult->next())
            Data->m_pGameServer->Chat(ClientID, "This username is already in use.");
        else
        {
            str_format(aBuf, sizeof(aBuf), "INSERT INTO tw_Accounts(Username, Password) VALUES ('%s', '%s');", Username.ClrStr(), Password.ClrStr());
            pConn->Execute(aBuf);
            Data->m_pGameServer->Chat(ClientID, "Account was created successfully.");
            Data->m_pGameServer->TW()->Account()->Login(ClientID, Username.ClrStr(), Password.ClrStr());
        }
    }

    CConnectionPool::GetConnPool()->ReleaseOneConn(pConn);
    delete Data;
}

bool CAccount::Register(int ClientID, const char *Username, const char *Password)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    str_copy(data->m_AccData.m_aUsername, Username, sizeof data->m_AccData.m_aUsername);
    str_copy(data->m_AccData.m_aPassword, Password, sizeof data->m_AccData.m_aPassword);

    std::thread(&register_thread, data).detach();
    return true;
}

static void login_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    int ClientID = Data->m_ClientID;
    CPlayer *P = Data->m_pGameServer->GetPlayer(ClientID);
    if (!P)
    {
        delete Data;
        return;
    }

    auto Username = CSqlString<64>(Data->m_AccData.m_aUsername);
    auto Password = CSqlString<64>(Data->m_AccData.m_aPassword);

    CSqlConnection *pConn = CConnectionPool::GetConnPool()->GetOneConn();

    if (pConn)
    {
        char aBuf[512];
        str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s';", Username.ClrStr());
        pConn->Query(aBuf);
        if (pConn->m_pResult->next())
        {
            str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s' AND Password = '%s';", Username.ClrStr(), Password.ClrStr());
            pConn->Query(aBuf);
            if (pConn->m_pResult->next())
            {
                P->m_AccData.m_UserID = pConn->m_pResult->getInt("UserID");
                P->m_AccData.m_Holding[ITYPE_SWORD] = pConn->m_pResult->getInt("Sword");
                P->m_AccData.m_Holding[ITYPE_AXE] = pConn->m_pResult->getInt("Axe");
                P->m_AccData.m_Holding[ITYPE_PICKAXE] = pConn->m_pResult->getInt("Pickaxe");

                str_copy(P->m_AccData.m_aUsername, pConn->m_pResult->getString("Username").c_str(), sizeof(P->m_AccData.m_aUsername));
                str_copy(P->m_AccData.m_aPassword, pConn->m_pResult->getString("Password").c_str(), sizeof(P->m_AccData.m_aPassword));
                P->SetLanguage(pConn->m_pResult->getString("Language").c_str());

                Data->m_pGameServer->Chat(ClientID, "You are now logged in.");
                Data->m_pGameServer->Broadcast(ClientID, "Welcome {}!", Data->m_pGameServer->Server()->ClientName(ClientID));
                P->m_InitAcc = true;
            }
            else
                Data->m_pGameServer->Chat(ClientID, "The password you entered is wrong.");
        }
        else
        {
            Data->m_pGameServer->Chat(ClientID, "This Account does not exists.");
            Data->m_pGameServer->Chat(ClientID, "Please register first. (/register <user> <pass>)");
        }
    }

    CConnectionPool::GetConnPool()->ReleaseOneConn(pConn);
    delete Data;
}
bool CAccount::Login(int ClientID, const char *Username, const char *Password)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    str_copy(data->m_AccData.m_aUsername, Username, sizeof data->m_AccData.m_aUsername);
    str_copy(data->m_AccData.m_aPassword, Password, sizeof data->m_AccData.m_aPassword);

    std::thread(&login_thread, data).detach();
    return true;
}

static void sync_accdata_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    int ClientID = Data->m_ClientID;
    CPlayer *P = Data->m_pGameServer->GetPlayer(ClientID);
    if (!P || !P->m_AccData.m_UserID)
    {
        delete Data;
        return;
    }

    int UserID = P->m_AccData.m_UserID;

    CSqlConnection *pConn = CConnectionPool::GetConnPool()->GetOneConn();

    if (pConn)
    {
        try
        {
            char aBuf[512];
            str_format(aBuf, sizeof(aBuf), "SELECT Username from tw_Accounts WHERE UserID = %d;", UserID);
            pConn->Query(aBuf);
            if (pConn->m_pResult->next())
            {
                switch (Data->m_Table)
                {
                case CGameContext::TABLE_ACCOUNT:
                {
                    P->m_AccData.m_UserID = pConn->m_pResult->getInt("UserID");
                    P->m_AccData.m_Holding[ITYPE_SWORD] = pConn->m_pResult->getInt("Sword");
                    P->m_AccData.m_Holding[ITYPE_AXE] = pConn->m_pResult->getInt("Axe");
                    P->m_AccData.m_Holding[ITYPE_PICKAXE] = pConn->m_pResult->getInt("Pickaxe");

                    str_copy(P->m_AccData.m_aUsername, pConn->m_pResult->getString("Username").c_str(), sizeof(P->m_AccData.m_aUsername));
                    str_copy(P->m_AccData.m_aPassword, pConn->m_pResult->getString("Password").c_str(), sizeof(P->m_AccData.m_aPassword));
                    P->SetLanguage(pConn->m_pResult->getString("Language").c_str());
                }
                break;

                case CGameContext::TABLE_ITEM:
                {
                    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Items WHERE UserID = %d;", UserID);
                    pConn->Query(aBuf);
                    while (pConn->m_pResult->next())
                    {
                        int ItemID = pConn->m_pResult->getInt("ItemID");
                        P->m_AccData.m_aItems[ItemID].m_Num = pConn->m_pResult->getInt("Num");
                        P->m_AccData.m_aItems[ItemID].m_aExtra = pConn->m_pResult->getString("Extra");
                    }
                }
                break;

                default:
                    break;
                }
            }
            else
                dbg_msg("SyncAccountData", "Error when saving account data.");
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }


    CConnectionPool::GetConnPool()->ReleaseOneConn(pConn);
    delete Data;
}

void CAccount::SyncAccountData(int ClientID, int Table)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    data->m_Table = Table;
    str_copy(data->m_Language, GameServer()->GetPlayer(ClientID)->GetLanguage(), sizeof(data->m_Language));

    std::thread(&sync_accdata_thread, data).detach();
}

static void save_accdata_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    int UserID = Data->m_AccData.m_UserID;
    if (!UserID)
    {
        delete Data;
        return;
    }

    CSqlConnection *pConn = CConnectionPool::GetConnPool()->GetOneConn();

    auto Username = CSqlString<64>(Data->m_AccData.m_aUsername);
    auto Password = CSqlString<64>(Data->m_AccData.m_aPassword);

    if (pConn)
    {
        try
        {
            char aBuf[512];
            str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE UserID = %d;", UserID);
            pConn->Query(aBuf);
            if (pConn->m_pResult->next())
            {
                switch (Data->m_Table)
                {
                case CGameContext::TABLE_ACCOUNT:
                {
                    str_format(aBuf, sizeof(aBuf), "UPDATE tw_Accounts SET "
                                                   "Username='%s',Password='%s',Language='%s',Sword=%d,Axe=%d,Pickaxe=%d "
                                                   "WHERE UserID=%d;",
                               Username.ClrStr(), Password.ClrStr(), Data->m_Language, Data->m_AccData.m_Holding[ITYPE_SWORD], Data->m_AccData.m_Holding[ITYPE_AXE], Data->m_AccData.m_Holding[ITYPE_PICKAXE], UserID);
                    pConn->Execute(aBuf);
                }
                break;

                case CGameContext::TABLE_ITEM:
                {
                    for (int i = 0; i < NUM_ITEM; i++)
                    {
                        str_format(aBuf, sizeof(aBuf), "SELECT * FROM tw_Items WHERE UserID=%d AND ItemID=%d;", UserID, i);
                        pConn->Query(aBuf);

                        if (pConn->m_pResult->next())
                        {
                            if (Data->m_aItems[i].m_Num)
                                str_format(aBuf, sizeof(aBuf), "UPDATE tw_Items SET Num = %d, Extra = '%s' WHERE UserID=%d AND ItemID=%d;", Data->m_aItems[i].m_Num, Data->m_aItems[i].m_aExtra.c_str(), UserID, i); // if yes, update it.
                            else
                                str_format(aBuf, sizeof(aBuf), "DELETE FROM tw_Items WHERE UserID=%d AND ItemID=%d;", UserID, i); // So delete it
                            pConn->Execute(aBuf);                                                             // Execute
                        }
                        else if (Data->m_aItems[i].m_Num)
                        {
                            str_format(aBuf, sizeof(aBuf), "INSERT INTO tw_Items(UserID, ItemID, Num, Extra) VALUES (%d, %d, %d, '%s')", UserID, i, Data->m_aItems[i].m_Num, Data->m_aItems[i].m_aExtra.c_str()); // if not, insert it.
                            pConn->Execute(aBuf);
                        }
                    }
                }
                break;

                default:
                    break;
                }
            }
            else
                dbg_msg("SaveAccountData", "Error when saving account data.");
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Saving Account Data (%d) : %s", e.getErrorCode(), e.what());
        }
    }

    CConnectionPool::GetConnPool()->ReleaseOneConn(pConn);
    delete Data;
}

void CAccount::SaveAccountData(int ClientID, int Table, CPlayer::SAccData AccData)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    data->m_Table = Table;
    data->m_AccData = AccData;
    for (int i = 0; i < NUM_ITYPE; i++)
        data->m_AccData.m_Holding[i] = GameServer()->GetPlayer(ClientID)->m_AccData.m_Holding[i];
    for (int i = 0; i < NUM_ITEM; i++)
        data->m_aItems[i] = GameServer()->GetPlayer(ClientID)->m_AccData.m_aItems[i];
    str_copy(data->m_Language, GameServer()->GetPlayer(ClientID)->GetLanguage(), sizeof(data->m_Language));

    std::thread(&save_accdata_thread, data).detach();
}