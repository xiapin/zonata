#include "ecg-top.h"
#include "ecg-list.h"
#include "ecg-base.h"

#include <algorithm>
#include <iostream>

namespace Ecg
{

class CPU_Stat {
public:
    CPU_Stat()
    {
        m_usageUsec = 0;
        m_usageUser = 0;
        m_usageSystem = 0;
    }
    CPU_Stat(std::string name, uint64_t usage, uint64_t user, uint64_t sys)
        : m_contName(name), m_usageUsec(usage),
            m_usageUser(user), m_usageSystem(sys) {}
    ~CPU_Stat() {}

    std::string m_contName;
    uint64_t m_usageUsec;
    uint64_t m_usageUser;
    uint64_t m_usageSystem;
};

class Top_Cpu_Usage : public Top_Base {
public:
    std::vector<std::string> Top()
    {
        Ecg_list ecgList;
        std::vector<std::string> topVec;

        auto allCont = ecgList.GetAllContainers();
        std::map<std::string, std::vector<std::string>>::iterator iter;
        for (iter = allCont.begin(); iter != allCont.end(); iter++) {
            m_allConts.emplace_back(GetCpuStatV2(iter->first));

            for (auto item : iter->second) {
                m_allConts.emplace_back(GetCpuStatV2(item));
            }
        } // TODO:m_allConts update, current duplicate.

        sort(m_allConts.begin(), m_allConts.end(), cmp);

        topVec.emplace_back("Name\t\t\tusage_usec\t\tuser_usec\t\tsystem_usec");
        for (auto item : m_allConts) {
            topVec.emplace_back(item.m_contName + "\t" +
                        std::to_string(item.m_usageUsec) + "\t\t" +
                        std::to_string(item.m_usageUser) + "\t\t" +
                        std::to_string(item.m_usageUser));
        }

        return topVec;
    }

    bool Match(ECG_TOP_TYPE topType)
    {
        return topType == TOP_CPU_USAGE;
    }

    bool Match(std::string topType)
    {
        return !topType.compare("cpu_usage");
    }

private:
    std::vector<CPU_Stat> m_allConts;

    static bool cmp(CPU_Stat &a, CPU_Stat &b)
    {
        return a.m_usageUsec >= b.m_usageUsec;
    }

    static CPU_Stat GetCpuStatV2(std::string cgrp)
    {
        CPU_Stat cpuStat;
        auto stats = Fs_Utils::readFileLine(cgrp + "/cpu.stat");
        if (stats.size() <= 3)
            return cpuStat;

        size_t pos = cgrp.rfind("/");
        if (pos == cgrp.npos) {
            cpuStat.m_contName = cgrp;
        } else {
            cpuStat.m_contName = cgrp.substr(pos + 1, 16);
        }
        cpuStat.m_usageUsec = Common_Utils::GetSplitInteger(stats.at(0), ' ');
        cpuStat.m_usageUser = Common_Utils::GetSplitInteger(stats.at(1), ' ');
        cpuStat.m_usageSystem = Common_Utils::GetSplitInteger(stats.at(2), ' ');

        return cpuStat;
    }
};

class Win_TopRoot : public Curses_Content {
public:
    virtual ~Win_TopRoot() {}

    std::vector<std::string> GetContent(std::string selected)
    {
        std::vector<std::string> v;

        v.emplace_back("cpu_usage");
        v.emplace_back("slab");
        v.emplace_back("swap");
        v.emplace_back("mem_usage");
        v.emplace_back("pids");
        v.emplace_back("files");

        return v;
    }
};

class Win_TopSub : public Curses_Content {
public:
    Win_TopSub()
    {
        Top_Cpu_Usage *cpuTop = new Top_Cpu_Usage;

        m_topBases.emplace_back(cpuTop);
    }
    virtual ~Win_TopSub()
    {
        for (auto it : m_topBases) {
            delete it;
        }
        m_topBases.clear();
    }

    std::vector<std::string> GetContent(std::string selected)
    {
        for (auto item : m_topBases) {
            if (item->Match(selected)) {
                return item->Top();
            }
        }

        return {};
    }

private:
    std::vector<Top_Base *> m_topBases;
};

}; // namespace Ecg

int main()
{
    // Ecg::Top_Cpu_Usage topCPU;

    // topCPU.Top();

    Ecg::Win_TopRoot *topRoot = new Ecg::Win_TopRoot();
    Ecg::Win_TopSub *topSub = new Ecg::Win_TopSub();

    Ecg::Curses_Win WinRoot(topRoot);
    Ecg::Curses_Win *WinSub = new Ecg::Curses_Win(topSub);

    WinSub->CW_Init();
    WinRoot.CW_BindWin(WinSub, true);
    WinRoot.CW_Init();
    WinRoot.CW_CreateBkd("        Choise<Enter>  UP<pageup>  Down<pagedown> Back<Esc>");
    WinRoot.CW_Draw("null");

    return 0;
}