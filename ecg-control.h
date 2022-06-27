#include <string>
#include <vector>
#include <map>

#pragma once

namespace Ecg {

class Ecg_Control {
public:
    static int Ecg_CgrpSet(std::string &cgrp, std::string &value);

    static std::map<std::string, std::vector<std::string>>
                Ecg_CgrpGet(std::string &cgrp);

    static int Ecg_CgrpCreate(std::string parentCgrp,
                                std::string name);

    static int Ecg_CgrpDelete(std::string cgrpName);

    Ecg_Control() {}
    ~Ecg_Control() {}
};

}; // namespace Ecg