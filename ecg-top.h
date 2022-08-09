#include <string>
#include <vector>

#pragma once

namespace Ecg
{

typedef enum {
    TOP_CPU_USAGE = 0,
    TOP_SLAB,
    TOP_SWAP,
    TOP_MEM_USAGE,
    TOP_PIDS,
    TOP_FILES,
} ECG_TOP_TYPE;

class Top_Base
{
public:
    Top_Base() {}

    virtual ~Top_Base() {}
    virtual std::vector<std::string> Top() = 0;
    virtual bool Match(ECG_TOP_TYPE topType) = 0;
    virtual bool Match(std::string topType) = 0;
};

}; // namespace Ecg
