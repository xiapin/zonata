#include "ecg-base.h"
#include "ecg-list.h"
#include <iostream>
#include <vector>
#include <string.h>
#include <dirent.h>

#define CGROUP_PREFIX       "cgroup"
#define CGRP_PREFIX_LEN     (7)

namespace Ecg
{

std::vector<std::string>
Ecg_list::GetCgroupRoots()
{
    std::vector<std::string> mounts, cgrps;
    char *line = nullptr;
    size_t len = 0;
    int read = 0;

    FILE *pF = fopen("/proc/mounts", "r");
    if (pF == nullptr) {
        std::cerr << "fopen mounts error!\n";
        return cgrps;
    }

    while ((read = ::getline(&line, &len, pF)) != -1) {
        if (!strncmp(line, CGROUP_PREFIX, CGRP_PREFIX_LEN - 1) &&
            !strstr(line, "systemd") ) {
            mounts.emplace_back(line);
        }
    }

    for (auto item : mounts) {
        size_t pos = item.find(" ", CGRP_PREFIX_LEN);
        if (pos == item.npos) {
            continue;
        }

        cgrps.emplace_back(item.substr(CGRP_PREFIX_LEN, pos - CGRP_PREFIX_LEN));
    }

    fclose(pF);
    return cgrps;
}

Ecg_list::Ecg_list()
{
    auto cgrps = GetCgroupRoots();

    m_cgrpRootDir = Ecg::Common_Utils::Getoverlap(cgrps.at(0), cgrps.at(1));
    m_cgrpRootDirLen = m_cgrpRootDir.length();

    for (auto item : cgrps) {
        m_cgrpListMap[item] = ScanSpecificCgroup(item);
    }
}

std::map<std::string, std::vector<std::string>>
Ecg_list::GetCgrpListMap()
{
    return m_cgrpListMap;
}

void Ecg_list::ShowAllCgroups()
{
    std::map<std::string, std::vector<std::string>>::iterator it;
    for (it = m_cgrpListMap.begin(); it != m_cgrpListMap.end(); it++) {
        // std::cout << it->first.substr(m_cgrpRootDirLen) + ":" << std::endl;
        std::cout << it->first << std::endl;
        for (auto child : it->second) {
            // std::cout << child.replace(child.find("/"), 1, ":") << std::endl;
            std::cout << child << std::endl;
        }
    }
}

void Ecg_list::ScanChildGrp(std::string CgrpParent, std::vector<std::string> &v)
{
    DIR *pDir;
    struct dirent *d;
    if (!(pDir = opendir(CgrpParent.c_str()))) {
        std::cout << "open " + CgrpParent + " error" << std::endl;
        return;
    }

    while ((d = readdir(pDir)) != 0) {
        if (strcmp(d->d_name, ".") && strcmp(d->d_name, "..")) {
            if (d->d_type == DT_DIR) {
                // v.push_back(CgrpParent.substr(m_cgrpRootDirLen) + "/" + d->d_name);
                v.push_back(CgrpParent + "/" + d->d_name); // do not skip root
                ScanChildGrp(CgrpParent + "/" + d->d_name, v);
            }
        }
    }

    closedir(pDir);
}

std::vector<std::string>
Ecg_list::ScanSpecificCgroup
(std::string CgrpSubsysRoot)
{
    std::vector<std::string> childGrps;

    ScanChildGrp(CgrpSubsysRoot, childGrps);

    return childGrps;
}

void Ecg_list::ScanContainersRoot
(std::string CgrpSubsys, std::vector<std::string> &v)
{
    DIR *pDir;
    struct dirent *d;
    if (!(pDir = opendir(CgrpSubsys.c_str()))) {
        std::cout << "open " + CgrpSubsys + " error" << std::endl;
        return;
    }

    while ((d = readdir(pDir)) != 0) {
        if (d->d_type == DT_DIR && strcmp(d->d_name, ".") && strcmp(d->d_name, "..")) {
            v.push_back(d->d_name);
        }
    }

    closedir(pDir);
}

std::map<std::string, std::vector<std::string>>
Ecg_list::GetAllContainers()
{
    std::vector<std::string> childGrpRoot;
    std::map<std::string, std::vector<std::string>>::iterator it;

    for (it = m_cgrpListMap.begin(); it != m_cgrpListMap.end(); it++) {
        if (strstr(it->first.c_str(), "files")) {
            ScanContainersRoot(it->first, childGrpRoot);
            for (auto item : childGrpRoot) {
                std::vector<std::string> childGrp;
                ScanContainersRoot(it->first + "/" + item, childGrp);
                m_contListMap[item] = childGrp;
            }
            break;
        }
    }

    // std::cout << "Container root:" << std::endl;
    // for (it = m_contListMap.begin(); it != m_contListMap.end(); it++) {
    //     std::cout << it->first << std::endl;
    //     for (auto item : it->second) {
    //         std::cout << item << std::endl;
    //     }
    // }

    return m_contListMap;
}

} // namespace Ecg

#ifdef ECG_LIST_EXE
int main(int argc, char **argv)
{
    Ecg::Ecg_list ecg_list;

    // ecg_list.ShowAllCgroups();
    // ecg_list.GetAllContainers();

    tool_ncurses ncurses;

    ncurses.info_ncurses(ecg_list.GetCgrpListMap());

    return 0;
}
#endif