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
    static std::string GetCgrpMountPoint();

    Ecg_list();
    ~Ecg_list() {}

private:
    static std::map<std::string, std::vector<std::string>> GetAllContainers_v2();
    // for container's root, scan from cgroup/files
    static void ScanContainersRoot
        (std::string CgrpSubsys, std::string prefix, std::vector<std::string> &v);

    static std::string m_cgrpRootDir; // cgroup mount point.
    static std::vector<std::string> m_cgrpRootSys; // root subsystem
};

}; // namespace Ecg