#include <iostream>
#include <fstream>
#include "nlohmann_json.h"

// 使用nlohmann::json命名空间
using json = nlohmann::json;

// 假设玩家武器和模块的数据结构如下
struct Module {
    std::string name;
    int level;
};

struct Weapon {
    std::string name;
    std::vector<Module> modules;
};

// 将武器转换为JSON对象
json weaponToJson(const Weapon& weapon) {
    json j;
    j["name"] = weapon.name;
    for (const auto& module : weapon.modules) {
        j["modules"].push_back({{"name", module.name}, {"level", module.level}});
    }
    return j;
}

// 从JSON对象中读取武器数据
Weapon jsonToWeapon(const json& j) {
    Weapon weapon;
    weapon.name = j["name"];
    for (const auto& module : j["modules"]) {
        Module m;
        m.name = module["name"];
        m.level = module["level"];
        weapon.modules.push_back(m);
    }
    return weapon;
}

// 保存武器到JSON文件
void saveWeaponToJson(const Weapon& weapon, const std::string& filename) {
    std::ofstream file(filename);
    file << weaponToJson(weapon);
}

// 从JSON文件读取武器
Weapon loadWeaponFromJson(const std::string& filename) {
    std::ifstream file(filename);
    json j;
    file >> j;
    return jsonToWeapon(j);
}

int main() {
    // 创建一个武器实例并添加模块
    Weapon myWeapon;
    myWeapon.name = "Laser Rifle";
    myWeapon.modules.push_back({"Scope", 3});
    myWeapon.modules.push_back({"Silencer", 2});

    // 保存武器到JSON文件
    saveWeaponToJson(myWeapon, "weapon.json");

    // 从JSON文件加载武器
    Weapon loadedWeapon = loadWeaponFromJson("weapon.json");

    // 输出加载的武器信息
    std::cout << "Loaded Weapon: " << loadedWeapon.name << std::endl;
    for (const auto& module : loadedWeapon.modules) {
        std::cout << "Module: " << module.name << ", Level: " << module.level << std::endl;
    }

    json Json;
    Json["Extra"]["Cards"].push_back({{"id", 1}, {"num", 1}});

    std::cout << Json.dump();

    return 0;
}
