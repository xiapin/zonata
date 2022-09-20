#include "ecg-top.h"
#include "ecg-list.h"
#include "ecg-base.h"

#include <algorithm>
#include <iostream>

namespace Ecg
{

class CPU_Stat {
public:
    CPU_Stat() {}
    ~CPU_Stat() {}

    std::string m_contName;
    std::string m_usageUsec;
    std::string m_usageUser;
    std::string m_usageSystem;
};

class Top_Cpu_Usage : public Top_Base {
public:
    std::vector<std::string> Top()
    {
        char tmp[256] = {0};
        std::vector<std::string> topVec;

        auto allCont = Ecg_list::GetAllContainers();
        std::map<std::string, std::vector<std::string>>::iterator iter;

        m_allContStat.clear();
        for (iter = allCont.begin(); iter != allCont.end(); iter++) {
            m_allContStat.emplace_back(GetCpuStat(iter->first));

            for (auto item : iter->second) {
                m_allContStat.emplace_back(GetCpuStat(item));
            }
        }

        sort(m_allContStat.begin(), m_allContStat.end(), cmp);

        snprintf(tmp, sizeof(tmp), "%-12.12s | %-12.12s | %-12.12s | %-12.12s",
                "Name", "Usage_usec", "User_usec", "Sys_usec");
        topVec.emplace_back(tmp);
        for (auto item : m_allContStat) {
            snprintf(tmp, sizeof(tmp), "%-12.12s | %-12.12s | %-12.12s | %-12.12s",
                    item.m_contName.c_str(), item.m_usageUsec.c_str(),
                    item.m_usageUser.c_str(), item.m_usageSystem.c_str());
            topVec.emplace_back(tmp);
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
    std::vector<CPU_Stat> m_allContStat;

    static bool cmp(CPU_Stat &a, CPU_Stat &b)
    {
        return a.m_usageUsec >= b.m_usageUsec;
    }

    static CPU_Stat GetCpuStat(std::string cgrp)
    {
        if (Common_Utils::IsCgroupV2())
            return GetCpuStatV2(cgrp);

        return GetCpuStatV1(cgrp);
    }

    static CPU_Stat GetCpuStatV2(std::string cgrp)
    {
        CPU_Stat cpuStat;
        auto stats = Fs_Utils::readFileLine(cgrp + "/cpu.stat");
        if (stats.size() < 3)
            return cpuStat;

        size_t pos = cgrp.rfind("/");
        if (pos == cgrp.npos) {
            cpuStat.m_contName = cgrp;
        } else {
            cpuStat.m_contName = cgrp.substr(pos + 1, 16);
        }

        // TODO: Change int to string
        cpuStat.m_usageUsec = Common_Utils::GetSplitString(stats.at(0), ' ', true);
        cpuStat.m_usageUser = Common_Utils::GetSplitString(stats.at(1), ' ', true);
        cpuStat.m_usageSystem = Common_Utils::GetSplitString(stats.at(2), ' ', true);

        return cpuStat;
    }

    static CPU_Stat GetCpuStatV1(std::string cgrp)
    {
        CPU_Stat cpuStat;
        std::string cpuacct = Ecg_list::GetCgrpMountPoint() + "/cpuacct";

        size_t pos = cgrp.rfind("/");
        if (pos == cgrp.npos) {
            cpuStat.m_contName = cgrp;
        } else {
            cpuStat.m_contName = cgrp.substr(pos + 1, 8);
        }

        cpuStat.m_usageUsec = Fs_Utils::readFileLine(cpuacct + "/" + cgrp + "/cpuacct.usage").at(0);
        cpuStat.m_usageUser = Fs_Utils::readFileLine(cpuacct + "/" + cgrp + "/cpuacct.usage_user").at(0);
        cpuStat.m_usageSystem = Fs_Utils::readFileLine(cpuacct + "/" + cgrp + "/cpuacct.usage_sys").at(0);

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