#include <string>
#include <vector>
#include <map>

#pragma once

namespace Ecg {

class Ecg_list {
public:
    std::vector<std::string> GetCgroupRoots();
    std::vector<std::string> ScanSpecificCgroup(std::string CgrpSubsysRoot);

    // first: container root, second: child containers
    std::map<std::string, std::vector<std::string>> Init();
    void ShowAllCgroups();
    // first: cgroup root, second: child cgroups
    std::map<std::string, std::vector<std::string>> GetAllContainers();

    Ecg_list() {}
    ~Ecg_list() {}

private:
    void ScanChildGrp(std::string CgrpParent, std::vector<std::string> &v);
    // for container's root, scan from cgroup/files
    void ScanContainersRoot(std::string CgrpSubsys, std::vector<std::string> &v);

    std::string m_cgrpRootDir; // cgroup mount point.
    unsigned m_cgrpRootDirLen;
    std::map<std::string, std::vector<std::string>> m_cgrpListMap;
    std::map<std::string, std::vector<std::string>> m_contListMap;
};

}; // namespace Ecg