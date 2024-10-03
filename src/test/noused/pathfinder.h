#pragma once
#include <map>
#include <vector>

#include <base/vmath.h>
#include <base/system.h>

class CTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	unsigned char m_Skip;
	unsigned char m_Reserved;
};

typedef unsigned int uint;
typedef std::vector<CTile> CRow;
typedef std::vector<CRow> CLabyrinth;
typedef std::vector<ivec2> CRoute;

class CLabyrinthAI
{
protected:
    // 装有迷宫数据的二维数组
    CLabyrinth m_xLabyrinth;
    // 起点位置
    ivec2 m_ptBeginning;
    // 终点位置
    ivec2 m_ptEnding;
    // 记录路线的数组
    CRoute m_vRoute;

public:
    // 枚举表示起点、终点的值
    enum
    {
        Ending = -2,
        Beginning = -1,
        CanGo = 0,
        CanntGo = 1
        
    };
    // 枚举表示障碍物与可走区的值

    // 枚举是否找到终点
    enum
    {
        FoundEnding = 0,
        NotFoundEnding = 1
    };

protected:
    // 判断某个位置是否已在路线数组中，用于别走重复的路
    bool isRepeat(const ivec2 Pos);

    // 将某一位置加入路线数组
    void advance(const ivec2 ptTo);

    // 将路线数组最后一个位置弹出
    void back();

    // 判断某一位置是否是起点
    bool isBeginning(const ivec2 pt);

    // 判断某一位置是否是终点
    bool isEnding(const ivec2 pt);

    // 判断该位置是否合法
    bool allRight(const ivec2 pt);

    /*-----------------核心算法------------------------*/
    // 判断某一位置是否可以向上移动
    ivec2 canUp(const ivec2 ptCurrent); // 接受当前位置

    // 判断某一位置是否可以向下移动
    ivec2 canDown(const ivec2 ptCurrent);

    // 判断某一位置是否可以向左移动
    ivec2 canLeft(const ivec2 ptCurrent);

    // 判断某一位置是否可以向右移动
    ivec2 canRight(const ivec2 ptCurrent);
    /*
     *判断某一位置是否可以向四周移动，如果判断到某一位置可以移动，则递归进入该位置判断。
     *如果该位置没有任何位置可移动，则返会上级位置并且调用back函数。如果走到终点，
     *则立刻返回枚举值FoundEnding，上级位置检查到返回值为FoundEnding，也直接返回。
     */
    int findRoute(const ivec2 ptCurrent);
    /*-----------------核心算法------------------------*/

public:
    // 带有初始化迷宫数组构造函数
    CLabyrinthAI(const CLabyrinth &vLabyrinth);

    // 初始化迷宫数组
    void setLabyrinth(const CLabyrinth &vLabyrinth);

    // 查找起点
    void getBeginning();

    // 查找终点
    void getEnding();

    // 调用核心算法函数，输出获得的路线
    void AI();
};