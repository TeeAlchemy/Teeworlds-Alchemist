// (c) ST-Chara(Daiqian Yang) 2024 - 2024
#include <engine/shared/json.h>
#include <string>

#include "chatglm.h"

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

CChatGLM::CChatGLM(IEngine *pEngine)
{
    m_pEngine = pEngine;
}

CChatGLM::~CChatGLM()
{
}

void CChatGLM::Send(CGameContext *pGameServer, const char *pName, const char *pContent)
{
    CJsonStringWriter JsonWriter;
    {
        JsonWriter.BeginObject();
        {
            JsonWriter.WriteStrValue("model", "glm-4");
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
    pHttp = HttpPostJson("https://open.bigmodel.cn/api/paas/v4/chat/completions", JsonWriter.GetOutputString().c_str());

    pHttp->Header("Authorization: Bearer 259f9de5c3b0ea40766f6cb7e9060cd4.hjpS8hMuMXZDJBa6");
    pHttp->Header("Content-Type: application/json");

    pHttp->LogProgress(HTTPLOG::FAILURE);
    pHttp->IpResolve(IPRESOLVE::V4);

    m_pEngine->AddJob(std::make_unique<CJob_ChatGLM>(std::move(pHttp), pGameServer));
}

void CChatGLM::CJob_ChatGLM::Run()
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
        m_pGameServer->SendChatTarget(-1, "很抱歉，我不能回答你这个问题");
        return;
    }
    const json_value &Json = *pJson;
    const json_value &Message = Json["choices"][0]["message"]["content"];

    std::string Content = Message.u.string.ptr;
    Content = replace_comma_with_newline(Content);
    Content = replace_dot_with_newline(Content);
    m_pGameServer->SendMotd(-1, Content.c_str());
}