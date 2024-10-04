#include "wave.h"

#include <fstream>
#include <string>
#include <sstream>

CWave::CWave(CGameContext* pGameServer) : m_JsonZombieNames{"Zaby", "Zoomer", "Zooker", "Zamer", "Zunner", "Zaster", "Zotter", "Zenade", "Flombie", "Zinja", "Zele", "Zinvis", "Zeater"}
{
    m_pGameServer = pGameServer;
    m_Endless = 0;
    m_ZombAlive = 0;
    m_ZombLeft = 0;
    m_Wave = 0;
    distribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
}

CWave::~CWave()
{

}

void CWave::Reset()
{
    m_Wave = 0;
    m_Endless = 0;
    ClearZombies();
}

void CWave::ClearZombies()
{
    m_vWave.clear();
    m_ZombLeft = 0;
    m_ZombAlive = 0;
}

void CWave::ReadFile(const std::string& filename)
{
    ReadWave(filename);
}

void CWave::ReadWave(const std::string& filename)
{
    using nlohmann::json;
    std::ifstream i(filename);
    if(i.is_open())
    {
        i >> m_Json;
        i.close();
    }
    else
    {
        std::stringstream ss;
        ss << "could not open file '" << filename << "'!";
        GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "json", ss.str().c_str());
    }
}

void CWave::StartWave()
{
    m_Wave++;
    //Read current wave
    using nlohmann::json;
    std::string wave_str= std::to_string(m_Wave);
    json j = m_Json["Waves"][wave_str];

    if(!j.is_object())
    {
        GetEndlessWave();
    }
    else
    {
        //Fill in wave vector
        ClearZombies();
        for(int i = 0; i < 13; ++i)
        {
            int value = j.value(m_JsonZombieNames[i], 0);
            m_vWave.push_back(value);
            m_ZombLeft+= value;
        }
        m_ZombAlive = m_ZombLeft;
    }
    DoWaveStartMessage();
}

void CWave::GetEndlessWave()
{
    m_Endless++;
    ClearZombies();
    for(int i = 0; i < 13; ++i)
    {
        int value = m_Endless*5;
        m_vWave.push_back(value);
        m_ZombLeft+=value;
    }
    m_ZombAlive = m_ZombLeft;
}

int CWave::GetRandZombie()
{
    if(m_ZombLeft == 0)
        return -1;
    float uniform = distribution(generator);
    int zombsum = 0;
    for(size_t i = 0; i < m_vWave.size(); ++i)
    {
        zombsum+=m_vWave[i];
        if(zombsum >= uniform){
            m_ZombLeft--;
            m_vWave[i]--;
            return (int)i;
        }
    }
    return -1;//this should not happen
}

void CWave::OnZombieKill()
{
    m_ZombAlive--;
    DoZombMessage();
}

void CWave::DoZombMessage()
{
    std::stringstream ss;
    if(m_ZombAlive < 0)
        return;
    else if(m_ZombAlive == 0)
    {
        ss << "Wave " << m_Wave << " defeated!";
    }
	else if(m_ZombAlive <= 5 || !(m_ZombAlive%10))
	{

        ss << "Wave " << m_Wave << ": " << m_ZombAlive << " zombie"
            << (m_ZombAlive == 1 ? " is" : "s are") << " left";
	}
	else
	{
        return;
	}
	GameServer()->SendChat(-1, CHAT_ALL, -1, ss.str().c_str());
}

void CWave::DoWaveStartMessage()
{
    std::stringstream ss;
    ss << "Wave " << m_Wave << " started with " << m_ZombAlive << " Zombies!";
    GameServer()->SendBroadcast(ss.str().c_str(), -1);
}

void CWave::DoLifeMessage(int Life)
{
    char aBuf[64];
	if(Life > 1 && (Life <= 5 || !(Life%10)))
	{
		if(Life <= 10)
		{
			str_format(aBuf, sizeof(aBuf), "Only %d lifes left!", Life);
			GameServer()->SendBroadcast(aBuf, -1);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "%d lifes left!", Life);
			GameServer()->SendBroadcast(aBuf, -1);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
		}
	}
	else if(Life == 1)
	{
		str_format(aBuf, sizeof(aBuf), "!!!Only 1 life left!!!");
		GameServer()->SendBroadcast(aBuf, -1);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}
}

