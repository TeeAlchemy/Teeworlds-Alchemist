#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include <base/system.h>

int main()
{
    dbg_logger_stdout();

    char Need = '|';
	int Find = 0;
	for (char c : "test|sa|aw")
	{
        if (Need == c)
			Find++;
    }

    dbg_msg("Sdad", "%d", Find);

    std::string data;
    std::cin >> data;
    std::istringstream iss(data);
    int number;
    std::vector<int> numbers;

    while (iss >> number) {
        numbers.push_back(number);
        iss.ignore(std::numeric_limits<std::streamsize>::max(), '|');
    }

    for (int num : numbers)
    {
        dbg_msg("ada", "Test %d", num);
    }

    return 0;
}