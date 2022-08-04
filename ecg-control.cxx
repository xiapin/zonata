#include "ecg-list.h"
#include "ecg-base.h"
#include "ecg-control.h"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>

namespace Ecg {

int Ecg_Control::Ecg_CgrpSet(std::string &cgrp, std::string &value)
{
    if (access(cgrp.c_str(), F_OK)) {
        std::cout << cgrp + " not exist\n";
        return -1;
    }

    std::ofstream f(cgrp, std::ios::out);
    if (!f.is_open() || f.bad()) {
        std::cout << "Open " + cgrp + " error\n";
        return -1;
    }

    f << value;

    f.close();
    return 0;
}

std::map<std::string, std::vector<std::string>>
Ecg_Control::Ecg_CgrpGet(std::string &cgrp)
{
    std::ifstream f(cgrp, std::ios::in);
    if (!f.is_open()) {
        return {};
    }

    std::map<std::string, std::vector<std::string>> map;
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

        map[d->d_name] = Ecg::Fs_Utils::readFileLine(cgrp + "/" + d->d_name);
    }

    return map;
}

int Ecg_Control::Ecg_CgrpCreate
(std::string cgrp)
{
    if (access(cgrp.c_str(), F_OK)) {
        std::cout << cgrp + " not exist\n";
        return -1;
    }

    return mkdir(cgrp.c_str(), 0664);
}

// Delete childgroup
int Ecg_Control::Ecg_CgrpDelete(std::string cgrpName)
{
    if (access(cgrpName.c_str(), F_OK)) {
        std::cout << cgrpName + " not exist\n";
        return -1;
    }

    DIR *pDir;
    struct dirent *d;
    if (!(pDir = opendir(cgrpName.c_str()))) {
        std::cout << "open " + cgrpName + " error" << std::endl;
        return {};
    }

    while ((d = readdir(pDir)) != 0) {
        if (d->d_type == DT_DIR && strcmp(d->d_name, ".") && strcmp(d->d_name, "..")) {
            Ecg_CgrpDelete(cgrpName + "/" + d->d_name);
        }
    }

    return rmdir(cgrpName.c_str());
}

void Ecg_Control::Show(std::string &unUsed)
{
    Ecg::Ecg_list ecgList;

    ecgList.ShowAllCgroups();
}

void Ecg_Control::Help(std::string &unUsed)
{
    std::cout << "usage: ecg-control [<flags>]\n\n"
                "-l show all cgroups\n"
                "-s cgrpfile=new value, set cgroup controller, eg: -s /cgroup/files/docker/files.limit=1\n"
                "-g cgrp, get cgroup content, eg: -g /cgroup/files/docker\n"
                "-c cgrp, create new cgroup, eg: -c /cgroup/files/test\n"
                "-d cgrp, delete exist cgroup and child cgroup, eg: -d /cgroup/files/test\n";
}

void Ecg_Control::Set(std::string &optArg)
{
    std::cout << "Set " << optArg << std::endl;

    int pos = optArg.find('=');
    if (pos == optArg.npos) {
        std::cerr << "syntax error, use -s cgrp_control=value.\n";
        return;
    }
    std::string cgrp = optArg.substr(0, pos);
    std::string value = optArg.substr(pos, optArg.length());

    std::cout << "cgrp: " + cgrp + " value: " + value << std::endl;
    return;
}

void Ecg_Control::Get(std::string &optArg)
{
    auto m = Ecg::Ecg_Control::Ecg_CgrpGet(optArg);
    std::map<std::string, std::vector<std::string>>::iterator it;
    for (it = m.begin(); it != m.end(); it++) {
        std::cout << it->first + ":\n";
        Ecg::Common_Utils::PrintVectorString("\t" , it->second);
    }
    std::cout << std::endl;
}

void Ecg_Control::Create(std::string &optArg)
{
    std::cout << "Create " << optArg << std::endl;
    Ecg_CgrpCreate(optArg);
}

void Ecg_Control::Delete(std::string &optArg)
{
    std::cout << "Delete " << optArg << std::endl;
    Ecg_CgrpDelete(optArg);
}

}; // namespace Ecg

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cout << "usage " << argv[0] << " cgroupfile value\n";
        return 1;
    }

    std::string shortOpt = "hs:g:c:d:l";
    Ecg::Opt_Parser optParser(shortOpt);

    optParser.Regist_Handler('l', Ecg::Ecg_Control::Show);
    optParser.Regist_Handler('h', Ecg::Ecg_Control::Help); 
    optParser.Regist_Handler('s', Ecg::Ecg_Control::Set);
    optParser.Regist_Handler('g', Ecg::Ecg_Control::Get);
    optParser.Regist_Handler('c', Ecg::Ecg_Control::Create);
    optParser.Regist_Handler('d', Ecg::Ecg_Control::Delete);

    optParser.Start_Parse(argc, argv);

    return 0;
}