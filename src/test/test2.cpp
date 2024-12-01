#include <base/system.h>
#include <base/vmath.h>

float magnitude(const vec2 &v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

vec2 applyGravity(const vec2 &position, float gravity)
{
    return {0, -gravity};
}

float calculateTension(const vec2 &position, const vec2 &tensionPoint, float baseTension, float maxDistance, float tensionIncreaseRate)
{
    vec2 distanceVector = tensionPoint - position;
    float distance = magnitude(distanceVector);
    if (distance > maxDistance)
        return baseTension + (maxDistance - baseTension) * tensionIncreaseRate;
    else
        return baseTension + (distance - baseTension) * tensionIncreaseRate;
}

vec2 applyTension(const vec2 &position, const vec2 &tensionPoint, float baseTension, float maxDistance, float tensionIncreaseRate)
{
    float tension = calculateTension(position, tensionPoint, baseTension, maxDistance, tensionIncreaseRate);
    vec2 direction = tensionPoint - position;
    vec2 unitDirection = normalize(direction);
    return unitDirection * tension;
}

int main()
{
    dbg_logger_stdout();
    vec2 position(0.f, 0.f);
    vec2 tensionPoint(200.f, 200.f);

    float baseTension = 10.f;
    float maxDistance = 100.f;
    float tensionIncreaseRate = 2.5f;
    const float gravity = 0.5f;

    vec2 gravityEffect = applyGravity(position, gravity);
    vec2 tensionEffect = applyTension(position, tensionPoint, baseTension, maxDistance, tensionIncreaseRate);

    vec2 newPosition = position + gravityEffect + tensionEffect;

    dbg_msg("Test", "在重力和绳子拉力作用下的新坐标为: (%f, %f)", newPosition.x, newPosition.y);
    vec2 finalForce = gravityEffect + tensionEffect;
    dbg_msg("Test", "最终力度的方向为: (%f, %f) %f", finalForce.x, finalForce.y, normalize(finalForce));
    dbg_msg("Test", "最终力度的力度为: %f", magnitude(finalForce));
    return 0;
}
