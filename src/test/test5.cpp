#include <stdio.h>
#include <math.h>

#define PI 3.14159265358979323846

// 物理参数
const float G = 9.8f;    // 重力加速度 (m/s^2)
const float L = 10.0f;   // 绳子的自然长度 (m)
const float dt = 0.01f;  // 时间步长 (s)
const float t_max = 20.0f; // 模拟的总时间 (s)
const float SPRING_CONSTANT = 5.0f; // 弹簧常数
const float DAMPING = 0.1f; // 阻尼系数

// vec2结构体
typedef struct {
    float x;
    float y;
} vec2;

// 摆动结构体
typedef struct {
    vec2 anchor;    // 绳子的固定点位置
    vec2 position;  // 球的位置
    vec2 velocity;  // 球的速度
} Pendulum;

// 初始化摆动状态
void init_pendulum(Pendulum *pendulum, vec2 initial_position) {
    pendulum->anchor = vec2{0.0f, 0.0f}; // 初始固定点在原点
    pendulum->position = initial_position;
    pendulum->velocity = vec2{0.0f, 0.0f};
}

// 更新摆动状态
void update_pendulum(Pendulum *pendulum) {
    // 计算球到固定点的向量
    vec2 to_anchor = vec2{
        pendulum->anchor.x - pendulum->position.x,
        pendulum->anchor.y - pendulum->position.y
    };
    float distance = sqrt(to_anchor.x * to_anchor.x + to_anchor.y * to_anchor.y);

    // 计算弹簧力
    float spring_force_magnitude = SPRING_CONSTANT * (distance - L);
    vec2 spring_force_direction = vec2{
        to_anchor.x / distance,
        to_anchor.y / distance
    };
    vec2 spring_force = vec2{
        spring_force_direction.x * spring_force_magnitude,
        spring_force_direction.y * spring_force_magnitude
    };

    // 计算阻尼力
    vec2 damping_force = vec2{
        -DAMPING * pendulum->velocity.x,
        -DAMPING * pendulum->velocity.y
    };

    // 计算总力
    vec2 total_force = vec2{
        spring_force.x + damping_force.x,
        spring_force.y + damping_force.y
    };

    // 使用牛顿第二定律更新速度和位置
    vec2 acceleration = vec2{
        total_force.x / 1.0f, // 假设球的质量为1kg
        total_force.y / 1.0f
    };
    pendulum->velocity.x += acceleration.x * dt;
    pendulum->velocity.y += acceleration.y * dt;
    pendulum->position.x += pendulum->velocity.x * dt;
    pendulum->position.y += pendulum->velocity.y * dt;
}

// 输出信息
void dbg_msg(const char* test, const char* format, float t, float x, float y) {
    printf("%s: ", test);
    printf(format, t, x, y);
    printf("\n");
}

int main() {
    Pendulum pendulum;
    init_pendulum(&pendulum, vec2{L * 0.5f, 0.0f}); // 初始位置在绳子自然长度的中间

    for (float t = 0; t < t_max; t += dt) {
        update_pendulum(&pendulum);

        // 获取球的位置
        vec2 pos = pendulum.position;

        // 输出球的位置
        dbg_msg("Pendulum", "Time: %f s, Position: (%f, %f)", t, pos.x, pos.y);
    }

    return 0;
}
