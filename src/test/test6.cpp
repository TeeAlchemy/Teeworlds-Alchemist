#include <base/vmath.h>
#include <base/system.h>

// 定义物体结构体
struct Object {
    vec2 position; // 物体的位置
    vec2 velocity; // 物体的速度
    float mass;    // 物体的质量
};

// 计算绳子提供的弹力
vec2 calculate_rope_force(const vec2& obj_pos, const vec2& anchor_pos, float k, float rest_length) {
    vec2 displacement = vec2(obj_pos.x - anchor_pos.x, obj_pos.y - anchor_pos.y);
    float x = rest_length - displacement.x; // 假设绳子沿着x轴方向
    float y = rest_length - displacement.y; // 假设绳子沿着x轴方向
    vec2 force = vec2(k * x, k * y); // 绳子的弹力，只考虑x轴方向
    return force;
}

// 更新物体的位置和速度
void update_object(Object& obj, const vec2& force, float dt) {
    vec2 acceleration = vec2(force.x / obj.mass, force.y / obj.mass);
    obj.velocity.x += acceleration.x * dt;
    obj.velocity.y += acceleration.y * dt;
    obj.position.x += obj.velocity.x * dt;
    obj.position.y += obj.velocity.y * dt;
}

int main() {
    dbg_logger_stdout();
    // 物体的初始状态
    Object obj = {vec2(0.0f, 10.0f), vec2(0.0f, 0.0f), 1.0f};
    vec2 anchor = vec2(0.0f, 0.0f); // 绳子的固定点
    float k = 10.0f; // 绳子的弹性系数
    float rest_length = 5.0f; // 绳子的自然长度
    float dt = 0.01f; // 时间步长
    float g = 9.81f; // 重力加速度

    // 模拟运动
    for (int i = 0; i < 1000; ++i) {
        // 计算重力
        vec2 gravity_force = vec2(0.0f, -obj.mass * g);
        // 计算绳子弹力
        vec2 rope_force = calculate_rope_force(obj.position, anchor, k, rest_length);
        // 合力
        vec2 total_force = vec2(gravity_force.x + rope_force.x, gravity_force.y + rope_force.y);
        // 更新物体状态
        update_object(obj, total_force, dt);
        // 输出物体位置
        dbg_msg("Object", "Position: (%f, %f)", obj.position.x, obj.position.y);
    }

    return 0;
}
