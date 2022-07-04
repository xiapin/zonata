#include <string>
#include <vector>
#include <map>

#pragma once

namespace Ecg {

class Ecg_Control {
public:
    static void Show(std::string &unUsed);
    static void Help(std::string &unUsed);
    static void Set(std::string &optArg);
    static void Get(std::string &optArg);
    static void Create(std::string &optArg);
    static void Delete(std::string &optArg);

    Ecg_Control() {}
    ~Ecg_Control() {}
private:
    static int Ecg_CgrpSet(std::string &cgrp, std::string &value);

    static std::map<std::string, std::vector<std::string>>
                Ecg_CgrpGet(std::string &cgrp);

    static int Ecg_CgrpCreate(std::string cgrp);

    static int Ecg_CgrpDelete(std::string cgrpName);
};

}; // namespace Ecg