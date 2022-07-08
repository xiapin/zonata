#include "ecg-top.h"

namespace Ecg
{
    
class Top_Cpu_Usage : public Top_Base {
public:
    void Top();
    bool Match(ECG_TOP_TYPE topType);
    bool Match(std::string topType);
};

void Top_Cpu_Usage::Top()
{

}

bool Top_Cpu_Usage::Match(ECG_TOP_TYPE topType)
{
    return topType == TOP_CPU_USAGE;
}

bool Top_Cpu_Usage::Match(std::string topType)
{
    return topType.compare("cpu_usage");
}

}; // namespace Ecg

int main()
{
    Ecg::Top_Cpu_Usage topCPu;

    topCPu.Top();

    return 0;
}