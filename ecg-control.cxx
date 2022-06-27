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

    // TODO: dir check
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
(std::string parentCgrp, std::string name)
{
    if (access(parentCgrp.c_str(), F_OK)) {
        std::cout << parentCgrp + " not exist\n";
        return -1;
    }

    return mkdir((parentCgrp + "/" + name).c_str(), 0664);
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

}; // namespace Ecg

static const char *short_opts = "hc:o:l:s:p:";

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cout << "usage " << argv[0] << " cgroupfile value\n";
        return 1;
    }

    std::string cgrp = argv[1];
    std::string newValue = argv[2];

    // Ecg::Ecg_Control::Ecg_CgrpSet(cgrp, newValue);

    auto m = Ecg::Ecg_Control::Ecg_CgrpGet(cgrp);
    std::map<std::string, std::vector<std::string>>::iterator it;
    for (it = m.begin(); it != m.end(); it++) {
        std::cout << it->first + ":";
        Ecg::Common_Utils::PrintVectorString("\t" , it->second);
    }
    std::cout << std::endl;

    // Ecg::Ecg_Control::Ecg_CgrpDelete(cgrp);

    return 0;
}