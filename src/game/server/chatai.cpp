// (c) ST-Chara(Daiqian Yang) 2024 - 2024
#include <engine/shared/json.h>
#include <engine/shared/config.h>
#include <string>

#include "chatai.h"

std::string replace_comma_with_newline(const std::string& input) {
    std::string result = input;
    size_t pos = 0;
    while((pos = result.find("，", pos)) != std::string::npos) {
        result.replace(pos, 3, "\n");
        pos += 2;
    }
    return result;
}

std::string replace_dot_with_newline(const std::string& input) {
    std::string result = input;
    size_t pos = 0;
    while((pos = result.find("。", pos)) != std::string::npos) {
        result.replace(pos, 3, "\n");
        pos += 2;
    }
    return result;
}

CChatAI::CChatAI(IEngine *pEngine)
{
    m_pEngine = pEngine;
}

CChatAI::~CChatAI()
{
}

void CChatAI::Send(CGameContext *pGameServer, const char *pName, const char *pContent)
{
    if (g_Config.m_SvChatAI)
        return;

    CJsonStringWriter JsonWriter;
    {
        JsonWriter.BeginObject();
        {
            JsonWriter.WriteStrValue("model", g_Config.m_SvChatAIModule);
            // "messages": []
            JsonWriter.BeginArray("messages");
            {
                JsonWriter.BeginObject();
                {
                    JsonWriter.WriteStrValue("role", "user");
                    JsonWriter.WriteStrValue("content", pContent);
                }
                JsonWriter.EndObject();
            }
            JsonWriter.EndArray();
        }
        JsonWriter.EndObject(); // }
    }

    std::unique_ptr<CHttpRequest> pHttp;
    // For example: https://api.chatanywhere.tech/v1/chat/completions
    pHttp = HttpPostJson(g_Config.m_SvChatAIUrl, JsonWriter.GetOutputString().c_str());

    char aAuth[128];
    str_format(aAuth, sizeof(aAuth), "Authorization: Bearer %s", g_Config.m_SvChatAIAPI);
    pHttp->Header(aAuth);
    pHttp->Header("Content-Type: application/json");

    pHttp->LogProgress(HTTPLOG::FAILURE);
    pHttp->IpResolve(IPRESOLVE::V4);

    m_pEngine->AddJob(std::make_unique<CJob_ChatAI>(std::move(pHttp), pGameServer));
}

void CChatAI::CJob_ChatAI::Run()
{
    IEngine::RunJobBlocking(m_pHttp.get());

    if (m_pHttp->Status() != HTTP_DONE)
    {
        dbg_msg("ChatGLM", "Error! Status: %d", m_pHttp->Status());
        return;
    }
    json_value *pJson = m_pHttp->ResultJson();
    if (!pJson)
    {
        m_pGameServer->Chat(-1, "Sorry, I can't answer your question.");
        return;
    }
    const json_value &Json = *pJson;
    const json_value &Message = Json["choices"][0]["message"]["content"];

    std::string Content = Message.u.string.ptr;
    Content = replace_comma_with_newline(Content);
    Content = replace_dot_with_newline(Content);
    m_pGameServer->Motd(-1, Content.c_str());
}