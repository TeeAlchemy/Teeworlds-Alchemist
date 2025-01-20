#include "nlohmann_json.h"
#include <iostream>

using json = nlohmann::json;

int main() {
    json j = {
        {"name", "彭"},
        {"age", 43},
        {"city", "北平"}
    };

    std::cout << "原始的JSON对象: " << j << std::endl;

    j.erase("age");

    std::cout << "修改后的JSON对象: " << j << std::endl;

    json j1;
    j1["people"].push_back({{"test", "hello"}});

    std::cout << j1.dump();

    j1["people"].erase(j1["people"].find("test"));

    std::cout << j1.dump();

    nlohmann::json Json = R"(
    {"Extra":{"Cards":[{"id":30,"num":2}, {"id":31,"num":2}]}}
    )"_json;
    int i = 0;
    std::cout << Json.dump();
	if (Json["Extra"].contains("Cards") && !Json["Extra"]["Cards"].empty())
	{
		for (const auto &j : Json["Extra"]["Cards"])
		{
			if (j["id"] == 31)
            {
				Json["Extra"]["Cards"].erase(i);
                break;
            }
            i++;   
        }
	}

    std::cout << Json.dump();

    return 0;
}
