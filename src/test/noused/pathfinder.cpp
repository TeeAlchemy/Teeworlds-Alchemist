#include "pathfinder.h"
//用VC 6.0貌似不需要给main传参数，那我就偷一下懒
int main()
{
    dbg_logger_stdout();

    //定义迷宫数组，定义成C风格的二维数组方便查看

    int vLabyrinthArray[][4] = {
        {1,0,-1,1}
        , {1,0,0,1}
        , {0,0,1,1}
        , {0,1,1,0}
        , {0,1,1,1}
        , {-2,1,0,0}
    };

    //以下代码为将C风格的二维数组导入成C++风格的二维数组
    int nRowNum = sizeof(vLabyrinthArray) / sizeof(vLabyrinthArray[0]);
    int nColNum = sizeof(vLabyrinthArray[0]) / sizeof(int);


    CLabyrinth vLabyrinth;
    for(int x = 0; x < nRowNum; x++){
        CRow xRow;
        for(int y = 0; y < nColNum; y++){
            CTile n;
            n.m_Index = vLabyrinthArray[x][y];
            xRow.push_back(n);
        }
        vLabyrinth.push_back(xRow);
    }

    //实例化CLabyrinthAI
    CLabyrinthAI xAI(vLabyrinth);
    //打出路线
    xAI.AI();

    return 0;
}