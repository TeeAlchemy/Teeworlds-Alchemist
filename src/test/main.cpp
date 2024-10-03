#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>

#include <base/vmath.h>
#include <base/system.h>

using namespace std;

/* 参考: https://zhuanlan.zhihu.com/p/613981189 */
const int dx[8] = {0, -1, -1, -1, 0, 1, 1, 1}; // 右，右上，上，左上，左，左下，下，右下
const int dy[8] = {1, 1, 0, -1, -1, -1, 0, 1};

// Pos -> TargetPos 的曼哈顿距离
int H(ivec2 Pos, ivec2 TargetPos)
{
    return (abs(Pos.x - TargetPos.x) + abs(Pos.y - TargetPos.y)) * 10;
}

// Pos1 -> Pos2 的两点距离
int G(ivec2 Pos1, ivec2 Pos2)
{
    return (sqrt((Pos1.x - Pos2.x) * (Pos1.x - Pos2.x) + (Pos1.y - Pos2.y) * (Pos1.y - Pos2.y))) * 10;
}

struct Node
{
    int x, y;          // (x,y)
    int f_n, g_n, h_n; // f = g + h
};

int main()
{
    int StartX, StartY, EndX, EndY;
    std::cin >> StartX >> StartY >> EndX >> EndY;
    ivec2 MapSize(std::max(StartX, EndX), std::max(StartY, EndY));

    dbg_logger_stdout();
    Node q = {StartX, StartY, 50, 0, 50};
    while (true)
    {
        if (q.x == EndX && q.y == EndY)
        {
            cout << q.f_n << endl;
            break;
        }
        Node node = {0, 0, __INT_MAX__, 0, 0};
        for (int k = 0; k < 8; ++k)
        {
            ivec2 Pos1(dx[k] + q.x, dy[k] + q.y);

            int g = G(Pos1, ivec2(q.x, q.y)) + q.g_n; // 注意g累加
            int h = H(Pos1, ivec2(EndX, EndY));
            int f = h + g;

            if (node.f_n > f)
            { // 此步骤每次找到最小的总代价
                node = {Pos1.x, Pos1.y, f, g, h};
            }
        }
        q = node;
        // 打印结果
        //dbg_msg("AI", "(%d, %d) f: %d h: %d g: %d", node.x, node.y, node.f_n, node.h_n, node.g_n);
        for (int y = 0; y < MapSize.y; y++)
        {
            std::cout << "\n";
            for (int x = 0; x < MapSize.x; x++)
            {
                if(node.x-1 == x && node.y-1 == y)
                    std::cout << " #";
                else
                    std::cout << " *";
            }
        }
        std::cout << std::endl << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
