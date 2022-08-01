#include <string>
#include <vector>
#include <map>

#pragma once

namespace Ecg {

class Ecg_list {
public:
    static std::vector<std::string> GetCgroupRoots();
    // first: container root, second: child containers
    static std::map<std::string, std::vector<std::string>> GetCgrpListMap();
    static void ShowAllCgroups();
    // first: cgroup root, second: child cgroups
    static std::map<std::string, std::vector<std::string>> GetAllContainers();

    Ecg_list();
    ~Ecg_list() {}

private:
    // for container's root, scan from cgroup/files
    static void ScanContainersRoot(std::string CgrpSubsys, std::vector<std::string> &v);

    static std::string m_cgrpRootDir; // cgroup mount point.
    static unsigned m_cgrpRootDirLen;
    static bool m_cgroupV2;
};

}; // namespace Ecg