#ifndef ENGINE_SHARED_CHATGLM_H
#define ENGINE_SHARED_CHATGLM
#include <engine/engine.h>
#include <engine/shared/http.h>
#include <engine/shared/jsonwriter.h>

#include "gamecontext.h"

class CChatAI
{
private:
    IEngine *m_pEngine;

public:
    CChatAI(IEngine *pEngine);
    ~CChatAI();

    void Send(CGameContext *pGameServer, const char *pName, const char *pContent);

    class CJob_ChatAI : public IJob
    {
        std::unique_ptr<CHttpRequest> m_pHttp;
        CGameContext *m_pGameServer;
        void Run();

    public:
        CJob_ChatAI(std::unique_ptr<CHttpRequest> &&pHttp, CGameContext *pGameServer)
        {
            m_pHttp = std::move(pHttp);
            m_pGameServer = pGameServer;
        }
        virtual ~CJob_ChatAI() = default;
    };
};

#endif // !