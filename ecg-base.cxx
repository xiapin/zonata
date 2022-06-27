#include "ecg-base.h"
#include <fstream>
#include <iostream>

namespace Ecg {

std::string
Fs_Utils::readFile
(const std::string& path)
{
    std::ifstream f(path, std::ios::in);
    if (!f.is_open()) {
        return {};
    }

    std::string s, tmp;
    while (std::getline(f, tmp, f.widen('\n'))) {
        s.append(tmp + "\n");
    }

    f.close();
    return s;
}

std::vector<std::string>
Fs_Utils::readFileLine
(const std::string& path)
{
    std::ifstream f(path, std::ios::in);
    if (!f.is_open()) {
        return {};
    }

    std::vector<std::string> v;
    std::string s;
    while (std::getline(f, s, f.widen('\n'))) {
        v.emplace_back(s);
    }

    f.close();
    return v;
}

std::string
Common_Utils::Getoverlap
(std::string str1, std::string str2)
{
    unsigned int i = 0;
    for (i = 0; i < str1.length() && i < str2.length(); i++) {
        if (str1.at(i) != str2.at(i)) {
            break;
        }
    }

    return str1.substr(0, i);
}

void Common_Utils::PrintVectorString
(std::string prefix, std::vector<std::string> &v)
{
    if (v.size() == 0)
        return;

    for (auto item : v) {
        std::cerr << prefix + item << std::endl;
    }
}

int Regist_Handler(char shortOpt, OptHandlerDF optHandler)
{
    return 0;
}

int Start_Parse(int argc, char **argv)
{
    return 0;
}

}; // namespace Ecg

// int main(void)
// {
//     std::string path = "/sys/fs/cgroup/cpuacct/cpuacct.usage";

//     std::cout << Ecg::Fs_Utils::readFile(path);
//     // for (auto item : Ecg::Fs_Utils::readFileLine(path)) {
//     //     std::cout << item << std::endl;
//     // }

//     return 0;
// }