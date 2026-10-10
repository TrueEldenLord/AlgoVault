#pragma once
#include <string>
#include <vector>

struct Problem {
    int id;
    std::string name;
    std::string difficulty; // "Easy", "Medium", "Hard"
    bool solved = false;
    std::vector<std::string> topics;
};
