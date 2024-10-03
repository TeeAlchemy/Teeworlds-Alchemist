#include "pathfinder.h"

CLabyrinthAI::CLabyrinthAI(const CLabyrinth &vLabyrinth)
{
    m_xLabyrinth = vLabyrinth;
    getBeginning();
    getEnding();
}

// 初始化迷宫数组
void CLabyrinthAI::setLabyrinth(const CLabyrinth &vLabyrinth)
{
    m_xLabyrinth = vLabyrinth;
}

// 查找起点
void CLabyrinthAI::getBeginning()
{
    uint nRow = 0;
    for (; nRow < m_xLabyrinth.size(); nRow++)
    {
        CRow xRow = m_xLabyrinth[nRow];
        uint nCol = 0;
        for (; nCol < xRow.size(); nCol++)
        {
            int n = xRow[nCol].m_Index;
            if (n == Beginning)
            {
                m_ptBeginning = ivec2(nCol, nRow);
                break;
            }
        }
    }
}

// 查找终点
void CLabyrinthAI::getEnding()
{
    uint nRow = 0;
    for (; nRow < m_xLabyrinth.size(); nRow++)
    {
        CRow xRow = m_xLabyrinth[nRow];
        uint nCol = 0;
        for (; nCol < xRow.size(); nCol++)
        {
            int n = xRow[nCol].m_Index;
            if (n == Ending)
            {
                m_ptEnding = ivec2(nCol, nRow);
                break;
            }
        }
    }
}

// 调用核心算法函数，输出获得的路线
void CLabyrinthAI::AI()
{
    findRoute(m_ptBeginning);
    if (!m_vRoute.empty())
    {
        char aBuf[256];
        CRoute::iterator it = m_vRoute.begin();
        for (; it != m_vRoute.end(); it++)
        {
            ivec2 pt = *it;
            str_format(aBuf, sizeof(aBuf), "(%d,%d)", pt.x, pt.y);
            if (it != m_vRoute.end() - 1)
            {
                str_append(aBuf, "->", sizeof(aBuf));
            }
            else
            {
                dbg_msg("AI", "Output: %s", aBuf);
            }
        }
    }
    else
    {
        // 如果没有找到路线到达终点
        dbg_msg("AI", "Sorry cannot file any ways to get ending.");
    }
}


bool CLabyrinthAI::isRepeat(const ivec2 Pos)
{
    bool bRes = false;
    CRoute::iterator it = m_vRoute.begin();
    for (; it != m_vRoute.end(); it++)
    {
        ivec2 pt0 = *it;
        if (pt0 == Pos)
        {
            bRes = true;
            break;
        }
    }
    return bRes;
}

void CLabyrinthAI::advance(const ivec2 ptTo)
{
    m_vRoute.push_back(ptTo);
}

void CLabyrinthAI::back()
{
    m_vRoute.pop_back();
}

bool CLabyrinthAI::isBeginning(const ivec2 pt)
{
    return m_ptBeginning == pt;
}

bool CLabyrinthAI::isEnding(const ivec2 pt)
{
    return m_ptEnding == pt;
}

ivec2 CLabyrinthAI::canUp(const ivec2 ptCurrent) // 接受当前位置
{
    ivec2 ptRes = ivec2(-1, -1);
    ivec2 Pos = ptCurrent;
    if (Pos.y > 0)
    {
        ivec2 ptNext = ivec2(Pos.x, Pos.y - 1); // 上移后位置
        // 检查上移后位置是否已经走过，以免寻路过程中绕圈子进入死循环
        if (!isRepeat(ptNext))
        {
            // 获得迷宫二维数组中上移后位置的属性（起点、终点、可走、障碍）
            int nAttr = m_xLabyrinth[ptNext.x][ptNext.y].m_Index;
            // 如果上移后位置为可走或到达终点，则设定返回值为上移后的位置
            if (nAttr == CanGo || nAttr == Ending)
            {
                ptRes = ptNext;
            }
        }
    }
    return ptRes; // 如果上移后位置不可走则返回非法的位置
}

ivec2 CLabyrinthAI::canDown(const ivec2 ptCurrent)
{
    ivec2 ptRes = ivec2(-1, -1);
    ivec2 Pos = ptCurrent;
    if (Pos.y < m_xLabyrinth.size() - 1)
    {
        ivec2 ptNext = ivec2(Pos.x, Pos.y + 1);
        if (!isRepeat(ptNext))
        {
            int nAttr = m_xLabyrinth[ptNext.x][ptNext.y].m_Index;
            if (nAttr == CanGo || nAttr == Ending)
            {
                ptRes = ptNext;
            }
        }
    }
    return ptRes;
}

ivec2 CLabyrinthAI::canLeft(const ivec2 ptCurrent)
{
    ivec2 ptRes = ivec2(-1, -1);
    ivec2 Pos = ptCurrent;
    if (Pos.x > 0)
    {
        ivec2 ptNext = ivec2(Pos.x - 1, Pos.y);
        if (!isRepeat(ptNext))
        {
            int nAttr = m_xLabyrinth[ptNext.x][ptNext.y].m_Index;
            if (nAttr == CanGo || nAttr == Ending)
            {
                ptRes = ptNext;
            }
        }
    }
    return ptRes;
}

ivec2 CLabyrinthAI::canRight(const ivec2 ptCurrent)
{
    ivec2 ptRes = ivec2(-1, -1);
    ivec2 Pos = ptCurrent;
    if (Pos.x < m_xLabyrinth[0].size() - 1)
    {
        ivec2 ptNext = ivec2(Pos.x + 1, Pos.y);
        if (!isRepeat(ptNext))
        {
            int nAttr = m_xLabyrinth[ptNext.x][ptNext.y].m_Index;
            if (nAttr == CanGo || nAttr == Ending)
            {
                ptRes = ptNext;
            }
        }
    }
    return ptRes;
}

int CLabyrinthAI::findRoute(const ivec2 ptCurrent)
{
    int nRes = NotFoundEnding; // 默认返回值为没有找到终点
    ivec2 ptNext = ivec2(-1, -1);

    advance(ptCurrent); // 将当前位置加入路线数组

    // 判断当前位置是否是终点，如果是终点则不进行下面的判断，将返回值设置为找到终点
    if (isEnding(ptCurrent))
    {
        nRes = FoundEnding;
    }
    else
    { // 按上左下右的顺序判断有无可走路径
        // 尝试向上
        ptNext = canUp(ptCurrent); // 获取向上走后的位置
        // 判断向上走后的位置是否是合法位置，若不合法，则表明上走到了迷宫的边缘，或者上面没有可走路径
        if (allRight(ptNext))
        {
            // 上述判断成功，则将向上移动后的位置传入给自己，进行递归。当该函数退出，查看返回值是否为找到终点。若找到终点则立刻返回FoundEnding
            if (findRoute(ptNext) == FoundEnding)
            {
                nRes = FoundEnding;
                return nRes;
            }
        }
        // 下列尝试四周位置是否可走的代码与上述大体相同，就不多说了
        // 尝试向左
        ptNext = canLeft(ptCurrent);
        if (allRight(ptNext))
        {
            if (findRoute(ptNext) == FoundEnding)
            {
                nRes = FoundEnding;
                return nRes;
            }
        }
        // 尝试向下
        ptNext = canDown(ptCurrent);
        if (allRight(ptNext))
        {
            if (findRoute(ptNext) == FoundEnding)
            {
                nRes = FoundEnding;
                return nRes;
            }
        }
        // 尝试向右
        ptNext = canRight(ptCurrent);
        if (allRight(ptNext))
        {
            if (findRoute(ptNext) == FoundEnding)
            {
                nRes = FoundEnding;
                return nRes;
            }
        }
    }

    // 检测是否到达终点，若没有到达终点，则立刻从路线表中删除该位置
    if (nRes != FoundEnding)
    {
        back();
    }

    return nRes;
}

bool CLabyrinthAI::allRight(const ivec2 pt)
{
    return pt.x >= 0 && pt.y >= 0;
}