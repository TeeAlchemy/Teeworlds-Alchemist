#include <stdio.h>
#include <math.h>

#define PI 3.14159265358979323846

// 物理参数
const float G = 9.8f;    // 重力加速度 (m/s^2)
const float L = 10.0f;   // 绳子的长度 (m)
const float dt = 0.01f;  // 时间步长 (s)
const float t_max = 20.0f; // 模拟的总时间 (s)

// vec2结构体
typedef struct {
    float x;
    float y;
} vec2;

// 摆动结构体
typedef struct {
    vec2 anchor;    // 绳子的固定点位置
    vec2 position;  // 球的位置
    float theta;    // 绳子与垂直方向的夹角 (弧度)
    float omega;    // 角速度 (rad/s)
} Pendulum;

// 初始化摆动状态
void init_pendulum(Pendulum *pendulum, float initial_theta) {
    pendulum->anchor = vec2{0.0f, 0.0f}; // 初始固定点在原点
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
    pendulum->position.x = pendulum->anchor.x + L * sin(pendulum->theta);
    pendulum->position.y = pendulum->anchor.y - L * cos(pendulum->theta); // 注意y轴方向向下为正
}

// 给球施加一个特定方向的力
void apply_force(Pendulum *pendulum, vec2 force) {
    // 计算力在绳子方向的分量，即力矩
    float torque = L * (force.x * sin(pendulum->theta) - force.y * cos(pendulum->theta));
    
    // 力矩等于转动惯量乘以角加速度，对于点质量，转动惯量为 m * L^2
    // 由于 m * L^2 在计算加速度时可以约去，所以直接用力矩除以 L 来得到角加速度
    float alpha = torque / L;
    
    // 更新角速度
    pendulum->omega += alpha * dt;
}

// 更新原点的位置
void move_anchor(Pendulum *pendulum, vec2 displacement) {
    pendulum->anchor.x += displacement.x;
    pendulum->anchor.y += displacement.y;
}

// 输出信息
void dbg_msg(const char* test, const char* format, float t, float x, float y) {
    printf("%s: ", test);
    printf(format, t, x, y);
    printf("\n");
}

int main() {
    Pendulum pendulum;
    init_pendulum(&pendulum, PI / 4); // 初始角度为45度

    // 施加一个特定方向的力
    vec2 force = {10.0f, 0.0f}; // 向右的力
    apply_force(&pendulum, force);

    // 移动原点
    vec2 displacement = {5.0f, 3.0f}; // 向右和向上移动
    move_anchor(&pendulum, displacement);

    for (float t = 0; t < t_max; t += dt) {
        update_pendulum(&pendulum);

        // 获取球的位置
        vec2 pos = pendulum.position;

        // 输出球的位置
        dbg_msg("Test", "Time: %f s, Position: (%f, %f)", t, pos.x, pos.y);
    }

    return 0;
}
