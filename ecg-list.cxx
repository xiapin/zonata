#include "ecg-base.h"
#include "ecg-list.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <string.h>
#include <dirent.h>

#define CGROUP_PREFIX       "cgroup"
#define CGRP_PREFIX_LEN     (7)

namespace Ecg
{

std::string Ecg_list::m_cgrpRootDir = "";
unsigned Ecg_list::m_cgrpRootDirLen = 0;

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
}

std::map<std::string, std::vector<std::string>>
Ecg_list::GetCgrpListMap()
{
    std::map<std::string, std::vector<std::string>> m;

    auto cgrps = GetCgroupRoots();
    for (auto item : cgrps) {
        m[item] = ScanSpecificCgroup(item);
    }
    return m;
}

void Ecg_list::ShowAllCgroups()
{
    auto cgrpListMap = GetCgrpListMap();

    std::map<std::string, std::vector<std::string>>::iterator it;
    for (it = cgrpListMap.begin(); it != cgrpListMap.end(); it++) {
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
    auto cgrpListMap = GetCgrpListMap();
    std::vector<std::string> childGrpRoot;
    std::map<std::string, std::vector<std::string>>::iterator it;
    std::map<std::string, std::vector<std::string>> contListMap;

    for (it = cgrpListMap.begin(); it != cgrpListMap.end(); it++) {
        if (strstr(it->first.c_str(), "files")) {
            ScanContainersRoot(it->first, childGrpRoot);
            for (auto item : childGrpRoot) {
                std::vector<std::string> childGrp;
                ScanContainersRoot(it->first + "/" + item, childGrp);
                contListMap[item] = childGrp;
            }
            break;
        }
    }

    return contListMap;
}

class Win_CgrpRoot : public Curses_Content {
public:
    Win_CgrpRoot() {}
    virtual ~Win_CgrpRoot() {}

    std::vector<std::string> GetContent(std::string selected)
    {
        Ecg::Ecg_list ecgList;
        return ecgList.GetCgroupRoots();
    }
};

class Win_CgrpSub : public Curses_Content {
public:
    Win_CgrpSub() {}
    virtual ~Win_CgrpSub() {}

    std::vector<std::string> GetContent(std::string selected)
    {
        Ecg::Ecg_list ecglist;
       return ecglist.ScanSpecificCgroup(selected);
    }
};

class Win_CgrpCont : public Curses_Content {
public:
    Win_CgrpCont() {}
    virtual ~Win_CgrpCont() {}

    std::vector<std::string> GetContent(std::string cgrp)
    {
        std::ifstream f(cgrp, std::ios::in);
        if (!f.is_open()) {
            return {};
        }

        std::vector<std::string> fileList;
        DIR *pDir;
        struct dirent *d;
        if (!(pDir = opendir(cgrp.c_str()))) {
            std::cout << "open " + cgrp + " error" << std::endl;
            return {};
        }

        while ((d = readdir(pDir)) != 0) {
            if (d->d_type == DT_DIR) {
                continue;;
            }

            // map[d->d_name] = Ecg::Fs_Utils::readFileLine(cgrp + "/" + d->d_name);
            fileList.push_back(cgrp + "/" + d->d_name);
        }

        closedir(pDir);
        f.close();
        return fileList;
    }
};

class Win_FileCont : public Curses_Content {
public:
    Win_FileCont() {}
    virtual ~Win_FileCont() {}

    std::vector<std::string> GetContent(std::string controlFile)
    {
        return Ecg::Fs_Utils::readFileLine(controlFile);
    }
};

} // namespace Ecg

#ifdef ECG_LIST_EXE
int main(int argc, char **argv)
{
    Ecg::Win_CgrpRoot *cgrpRoot = new Ecg::Win_CgrpRoot;
    Ecg::Win_CgrpSub *cgrpSub = new Ecg::Win_CgrpSub;
    Ecg::Win_CgrpCont *cgrpCont = new Ecg::Win_CgrpCont;
    Ecg::Win_FileCont *fileCont = new Ecg::Win_FileCont;

    Ecg::Curses_Win cw(cgrpRoot);
    Ecg::Curses_Win *subWin = new Ecg::Curses_Win(cgrpSub);
    Ecg::Curses_Win *subsubWin = new Ecg::Curses_Win(cgrpCont);
    Ecg::Curses_Win *contWin = new Ecg::Curses_Win(fileCont);

    if (cw.CW_Init()) {
        std::cout << "Curses init failed\n";
        return 1;
    }
    cw.CW_CreateBkd("        SubPage<Enter>  UP<pageup>  Down<pagedown> Back<Esc>");

    subWin->CW_Init();
    subsubWin->CW_Init();
    contWin->CW_Init();

    cw.CW_BindWin(subWin, true);
    subWin->CW_BindWin(subsubWin, true);
    subsubWin->CW_BindWin(contWin, true);

    cw.CW_Draw("null");

    while (1) {
        getchar();
    }

    return 0;
}
#endif