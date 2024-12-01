#include <base/vmath.h>
#include <base/system.h>

#define PI 3.14159265358979323846

// 物理参数
const float G = 9.8f;    // 重力加速度 (m/s^2)
const float L = 10.0f;   // 绳子的长度 (m)
const float dt = 0.01f;  // 时间步长 (s)
const float t_max = 20.0f; // 模拟的总时间 (s)

// 摆动结构体
typedef struct {
    vec2 position; // 球的位置
    float theta;   // 绳子与垂直方向的夹角 (弧度)
    float omega;   // 角速度 (rad/s)
} Pendulum;

// 初始化摆动状态
void init_pendulum(Pendulum *pendulum, float initial_theta) {
    pendulum->theta = initial_theta;
    pendulum->omega = 0.0f;
    pendulum->position = vec2{L * sin(initial_theta), -L * cos(initial_theta)};
}

// 更新摆动状态
void update_pendulum(Pendulum *pendulum) {
    // 摆动运动的微分方程
    float acceleration = -G / L * sin(pendulum->theta);

    // 使用欧拉方法更新角速度和角度
    pendulum->omega += acceleration * dt;
    pendulum->theta += pendulum->omega * dt;

    // 更新球的位置
    pendulum->position.x = L * sin(pendulum->theta);
    pendulum->position.y = -L * cos(pendulum->theta); // 注意y轴方向向下为正
}

int main() {
    dbg_logger_stdout();
    Pendulum pendulum;
    init_pendulum(&pendulum, PI / 4); // 初始角度为45度

    for (float t = 0; t < t_max; t += dt) {
        update_pendulum(&pendulum);

        // 获取球的位置
        vec2 pos = pendulum.position;

        // 输出球的位置
        dbg_msg("Test", "Time: %f s, Position: (%f, %f)", t, pos.x, pos.y);
    }

    return 0;
}