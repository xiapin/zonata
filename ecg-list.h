#include <string>
#include <vector>
#include <map>

#pragma once

namespace Ecg {

class Ecg_list {
public:
    static std::vector<std::string> GetCgroupRoots();
    static std::vector<std::string> ScanSpecificCgroup(std::string CgrpSubsysRoot);

    // first: container root, second: child containers
    static std::map<std::string, std::vector<std::string>> GetCgrpListMap();
    static void ShowAllCgroups();
    // first: cgroup root, second: child cgroups
    static std::map<std::string, std::vector<std::string>> GetAllContainers();

    Ecg_list();
    ~Ecg_list() {}

private:
    static void ScanChildGrp(std::string CgrpParent, std::vector<std::string> &v);
    // for container's root, scan from cgroup/files
    static void ScanContainersRoot(std::string CgrpSubsys, std::vector<std::string> &v);

    static std::string m_cgrpRootDir; // cgroup mount point.
    static unsigned m_cgrpRootDirLen;
};

}; // namespace Ecg