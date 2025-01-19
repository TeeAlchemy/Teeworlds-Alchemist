/* Copyright(C) 2022 - 2024 ST-Chara */
#include "account.h"
#include <thread>

void CAccount::OnInit()
{
    m_pPool = new AccountPool;
    m_pPool->m_pDB = GameServer()->DB();

    std::thread(&HandleThread, m_pPool).detach();
}

static void register_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    int ClientID = Data->m_ClientID;
    lock_wait(Data->m_pGameServer->DB()->SQL_Lock);
    char aBuf[512];
    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s';", Data->m_AccData.m_aUsername);
    sql::ResultSet *Result;
    if (Data->m_pGameServer->DB()->Connect())
    {
        try
        {
            Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                Data->m_pGameServer->Chat(ClientID, "This username is already in use.");
                Data->m_pGameServer->DB()->FreeData(Result);
                lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                return;
            }
            else
            {
                str_format(aBuf, sizeof(aBuf), "INSERT INTO tw_Accounts(Username, Password) VALUES ('%s', '%s');", Data->m_AccData.m_aUsername, Data->m_AccData.m_aPassword);
                Data->m_pGameServer->DB()->Execute(aBuf);
                Data->m_pGameServer->Chat(ClientID, "Account was created successfully.");
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    Data->m_pGameServer->DB()->FreeData(Result);

    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
}

bool CAccount::Register(int ClientID, const char *Username, const char *Password)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    str_copy(data->m_AccData.m_aUsername, Username, sizeof data->m_AccData.m_aUsername);
    str_copy(data->m_AccData.m_aPassword, Password, sizeof data->m_AccData.m_aPassword);
    data->m_Type = TYPE::REG;
    m_pPool->m_pFaBao.add(data);
    return true;
}

static void login_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    lock_wait(Data->m_pGameServer->DB()->SQL_Lock);
    int ClientID = Data->m_ClientID;
    CPlayer *P = Data->m_pGameServer->GetPlayer(ClientID);
    if (!P)
        return;

    char aBuf[512];
    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s';", Data->m_AccData.m_aUsername);
    sql::ResultSet *Result;
    if (Data->m_pGameServer->DB()->Connect())
    {
        try
        {
            Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE Username = '%s' AND Password = '%s';", Data->m_AccData.m_aUsername, Data->m_AccData.m_aPassword);
                Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
                if (Result->next())
                {
                    P->m_AccData.m_UserID = Result->getInt("UserID");
                    P->m_AccData.m_Holding[ITYPE_SWORD] = Result->getInt("Sword");
                    P->m_AccData.m_Holding[ITYPE_AXE] = Result->getInt("Axe");
                    P->m_AccData.m_Holding[ITYPE_PICKAXE] = Result->getInt("Pickaxe");

                    str_copy(P->m_AccData.m_aUsername, Result->getString("Username").c_str(), sizeof(P->m_AccData.m_aUsername));
                    str_copy(P->m_AccData.m_aPassword, Result->getString("Password").c_str(), sizeof(P->m_AccData.m_aPassword));
                    P->SetLanguage(Result->getString("Language").c_str());

                    Data->m_pGameServer->Chat(ClientID, "You are now logged in.");
                    Data->m_pGameServer->Broadcast(ClientID, "Welcome {}!", Data->m_pGameServer->Server()->ClientName(ClientID));
                    P->m_InitAcc = true;
                }
                else
                {
                    Data->m_pGameServer->Chat(ClientID, "The password you entered is wrong.");
                    Data->m_pGameServer->DB()->FreeData(Result);
                    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                    return;
                }
            }
            else
            {
                Data->m_pGameServer->Chat(ClientID, "This Account does not exists.");
                Data->m_pGameServer->Chat(ClientID, "Please register first. (/register <user> <pass>)");
                Data->m_pGameServer->DB()->FreeData(Result);
                lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                return;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    Data->m_pGameServer->DB()->FreeData(Result);

    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
}
bool CAccount::Login(int ClientID, const char *Username, const char *Password)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    str_copy(data->m_AccData.m_aUsername, Username, sizeof data->m_AccData.m_aUsername);
    str_copy(data->m_AccData.m_aPassword, Password, sizeof data->m_AccData.m_aPassword);
    data->m_Type = TYPE::LOG;

    m_pPool->m_pFaBao.add(data);
    return true;
}

static void sync_accdata_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    int ClientID = Data->m_ClientID;
    CPlayer *P = Data->m_pGameServer->GetPlayer(ClientID);
    if (!P)
        return;
    int UserID = P->m_AccData.m_UserID;
    if (!UserID)
        return;

    lock_wait(Data->m_pGameServer->DB()->SQL_Lock);
    sql::ResultSet *Result;

    if (Data->m_pGameServer->DB()->Connect())
    {
        try
        {
            char aBuf[512];
            str_format(aBuf, sizeof(aBuf), "SELECT Username from tw_Accounts WHERE UserID = %d;", UserID);
            Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                switch (Data->m_Table)
                {
                case CGameContext::TABLE_ACCOUNT:
                {
                    P->m_AccData.m_UserID = Result->getInt("UserID");
                    P->m_AccData.m_Holding[ITYPE_SWORD] = Result->getInt("Sword");
                    P->m_AccData.m_Holding[ITYPE_AXE] = Result->getInt("Axe");
                    P->m_AccData.m_Holding[ITYPE_PICKAXE] = Result->getInt("Pickaxe");

                    str_copy(P->m_AccData.m_aUsername, Result->getString("Username").c_str(), sizeof(P->m_AccData.m_aUsername));
                    str_copy(P->m_AccData.m_aPassword, Result->getString("Password").c_str(), sizeof(P->m_AccData.m_aPassword));
                    P->SetLanguage(Result->getString("Language").c_str());
                }
                break;

                case CGameContext::TABLE_ITEM:
                {
                    str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Items WHERE UserID = %d;", UserID);
                    sql::ResultSet *res = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
                    while (res->next())
                    {
                        int ItemID = res->getInt("ItemID");
                        P->m_AccData.m_aItems[ItemID].m_Num = res->getInt("Num");
                        // str_copy(P->m_AccData.m_aItems[ItemID].m_aCards, res->getString("Cards").c_str(), sizeof(P->m_AccData.m_aItems[ItemID].m_aCards));
                    }
                }
                break;

                default:
                    break;
                }
            }
            else
            {
                dbg_msg("SyncAccountData", "Error when saving account data.");
                Data->m_pGameServer->DB()->FreeData(Result);
                lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                return;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Creating Account (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    Data->m_pGameServer->DB()->FreeData(Result);

    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
}

void CAccount::SyncAccountData(int ClientID, int Table)
{
    FaBao *data = new FaBao();
    data->m_pGameServer = GameServer();
    data->m_ClientID = ClientID;
    data->m_Table = Table;
    str_copy(data->m_Language, GameServer()->GetPlayer(ClientID)->GetLanguage(), sizeof(data->m_Language));
    data->m_Type = TYPE::SYNC;

    m_pPool->m_pFaBao.add(data);
}

static void save_accdata_thread(void *user)
{
    FaBao *Data = (FaBao *)user;
    int UserID = Data->m_AccData.m_UserID;
    if (!UserID)
        return;

    lock_wait(Data->m_pGameServer->DB()->SQL_Lock);
    sql::ResultSet *Result;
    if (Data->m_pGameServer->DB()->Connect())
    {
        try
        {
            char aBuf[512];
            str_format(aBuf, sizeof(aBuf), "SELECT * from tw_Accounts WHERE UserID = %d;", UserID);
            Result = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);
            if (Result->next())
            {
                switch (Data->m_Table)
                {
                case CGameContext::TABLE_ACCOUNT:
                {
                    str_format(aBuf, sizeof(aBuf), "UPDATE tw_Accounts SET "
                                                   "Username='%s',Password='%s',Language='%s',Sword=%d,Axe=%d,Pickaxe=%d "
                                                   "WHERE UserID=%d;",
                               Data->m_AccData.m_aUsername, Data->m_AccData.m_aPassword, Data->m_Language, Data->m_AccData.m_Holding[ITYPE_SWORD], Data->m_AccData.m_Holding[ITYPE_AXE], Data->m_AccData.m_Holding[ITYPE_PICKAXE], UserID);
                    Data->m_pGameServer->DB()->Execute(aBuf);
                }
                break;

                case CGameContext::TABLE_ITEM:
                {
                    for (int i = 0; i < NUM_ITEM; i++)
                    {
                        str_format(aBuf, sizeof(aBuf), "SELECT * FROM tw_Items WHERE UserID=%d AND ItemID=%d;", UserID, i);
                        sql::ResultSet *res = Data->m_pGameServer->DB()->ExecuteQuery(aBuf);

                        if (res->next())
                        {
                            if (Data->m_aItems[i].m_Num)
                                str_format(aBuf, sizeof(aBuf), "UPDATE tw_Items SET Num = %d WHERE UserID=%d AND ItemID=%d;", Data->m_aItems[i].m_Num, UserID, i); // if yes, update it.
                            else
                                str_format(aBuf, sizeof(aBuf), "DELETE FROM tw_Items WHERE UserID=%d AND ItemID=%d;", UserID, i); // So delete it
                            Data->m_pGameServer->DB()->Execute(aBuf);                                                             // Execute
                        }
                        else if (Data->m_aItems[i].m_Num)
                        {
                            str_format(aBuf, sizeof(aBuf), "INSERT INTO tw_Items(UserID, ItemID, Num) VALUES (%d, %d, %d);", UserID, i, Data->m_aItems[i].m_Num); // if not, insert it.
                            Data->m_pGameServer->DB()->Execute(aBuf);
                        }
                    }
                }
                break;

                default:
                    break;
                }
            }
            else
            {
                dbg_msg("SaveAccountData", "Error when saving account data.");
                Data->m_pGameServer->DB()->FreeData(Result);
                lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
                return;
            }
        }
        catch (sql::SQLException &e)
        {
            dbg_msg("SQLError", "Error when Saving Account Data (%d) : %s", e.getErrorCode(), e.what());
        }
    }
    Data->m_pGameServer->DB()->FreeData(Result);

    lock_unlock(Data->m_pGameServer->DB()->SQL_Lock);
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
    data->m_Type = TYPE::SAVE;

    m_pPool->m_pFaBao.add(data);
}

void CAccount::HandleThread(void *user)
{
    AccountPool *pPool = (AccountPool *)user;
    while (true)
    {
        if (!pPool->m_pFaBao.size())
            continue;
        switch (pPool->m_pFaBao[0]->m_Type)
        {
        case TYPE::REG:
            register_thread(pPool->m_pFaBao[0]);
            break;

        case TYPE::LOG:
            login_thread(pPool->m_pFaBao[0]);
            break;

        case TYPE::SYNC:
            sync_accdata_thread(pPool->m_pFaBao[0]);
            break;

        case TYPE::SAVE:
            save_accdata_thread(pPool->m_pFaBao[0]);
            break;

        default:
            // none
            break;
        }
        pPool->m_pFaBao.remove_index(0);
        thread_sleep(50);
    }
}