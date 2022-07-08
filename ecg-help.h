#include <map>
#include <string>

#pragma once

namespace Ecg {

class Ecg_Help {
public:
    Ecg_Help() {}
    ~Ecg_Help() {}

    static std::string GetManual(std::string controlFile);
    std::string SetLanguage();

private:
    int m_langType;
};

}; // namespace Ecg