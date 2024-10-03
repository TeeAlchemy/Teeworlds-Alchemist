// notice: too lazy to finish it.
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>

#include <base/vmath.h>
#include <base/system.h>
#include <cstdlib> // For rand() and srand()

using namespace std;

enum
{
    NONE = 0,
    LEFT = -1,
    RIGHT = 1,
    UP = -1,
    DOWN = 1,

    DX = 0,
    DY = 1
};

const int dpos[2][8] = {
    {RIGHT, RIGHT, NONE, LEFT, LEFT, LEFT, NONE, RIGHT},
    {NONE, UP, UP, UP, NONE, DOWN, DOWN, DOWN}
};

int H(ivec2 Pos, ivec2 TargetPos)
{
    return (abs(Pos.x - TargetPos.x) + abs(Pos.y - TargetPos.y)) * 10;
}

int G(ivec2 Pos1, ivec2 Pos2)
{
    return (sqrt((Pos1.x - Pos2.x) * (Pos1.x - Pos2.x) + (Pos1.y - Pos2.y) * (Pos1.y - Pos2.y))) * 10;
}

struct Node
{
    ivec2 Pos;
    int f_n, g_n, h_n;
};

bool isValid(ivec2 Pos, const vector<vector<int>>& map)
{
    return Pos.x >= 0 && Pos.x < (int)map.size() && Pos.y >= 0 && Pos.y < (int)map[0].size() && map[Pos.x][Pos.y];
}

int main()
{
    int StartX, StartY, EndX, EndY;
    cin >> StartX >> StartY >> EndX >> EndY;

    ivec2 StartPos(StartX, StartY);
    ivec2 EndPos(EndX, EndY);

    // Define map size and initialize it with no obstacles
    int mapSizeX = std::max(StartX, EndX) + 1;
    int mapSizeY = std::max(StartY, EndY) + 1;
    vector<vector<int>> map(mapSizeX, vector<int>(mapSizeY, true));

    // Initialize random seed
    srand(static_cast<unsigned int>(time(0)));

    // Example of adding obstacles
    int numObstacles = mapSizeX + mapSizeY; // Adjust the number of obstacles as needed
    for (int i = 0; i < numObstacles; ++i)
    {
        int obstacleX = rand() % mapSizeX;
        int obstacleY = rand() % mapSizeY;
        map[obstacleX][obstacleY] = false;
    }

    // Make sure start and end positions are not obstacles
    map[StartX][StartY] = true;
    map[EndX][EndY] = true;

    Node q = {StartPos, 0, 0, H(StartPos, EndPos)};
    bool pathFound = false;
    while (true)
    {
        if (q.Pos == EndPos)
        {
            cout << "Path found with cost: " << q.f_n << endl;
            pathFound = true;
            break;
        }

        Node node = {ivec2(0, 0), __INT_MAX__, 0, 0};
        bool validMoveFound = false;
        for (int k = 0; k < 8; ++k)
        {
            ivec2 NewPos(q.Pos.x + dpos[DX][k], q.Pos.y + dpos[DY][k]);

            if (!isValid(NewPos, map))
                continue; // Skip if not valid or is an obstacle

            validMoveFound = true;
            int g = G(NewPos, q.Pos) + q.g_n;
            int h = H(NewPos, EndPos);
            int f = h + g;

            if (node.f_n > f)
            {
                node = {NewPos, f, g, h};
                map[NewPos.x][NewPos.y] = 2;
            }
        }

        if (!validMoveFound)
        {
            cout << "No valid moves available. Path not found." << endl;
            break;
        }

        q = node;

        // Print the map
        for (int y = 0; y < mapSizeY; y++)
        {
            cout << "\n";
            for (int x = 0; x < mapSizeX; x++)
            {
                if (q.Pos == ivec2(x, y))
                    cout << " #";
                                else if (!map[x][y])
                    cout << " X";
                else
                    cout << " .";
            }
        }

        cout << endl << endl;
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Print final map with path
    for (int y = 0; y < mapSizeY; y++)
    {
        cout << "\n";
        for (int x = 0; x < mapSizeX; x++)
        {
            if (StartPos == ivec2(x, y) || EndPos == ivec2(x, y))
                cout << " S"; // Start or End
            else if (map[x][y] == 0)
                cout << " X";
            else if (map[x][y] == 2)
                cout << " *";
            else
                cout << " .";
        }
    }

    cout << endl << endl;

    return 0;
}
