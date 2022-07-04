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

int Opt_Parser::Regist_Handler(char shortOpt, OptHandlerDF optHandler)
{
    m_handlers[shortOpt] = optHandler;
    return 0;
}

int Opt_Parser::Start_Parse(int argc, char **argv)
{
    int ch;
    std::map<char, OptHandlerDF>::iterator it;
    std::string optArg;

    while ((ch = getopt(argc, argv, m_shortOpts.c_str())) != -1) {
        it = m_handlers.find(ch);
        if (it == m_handlers.end()) {
            continue;
        }

        if (optarg != NULL) {
            optArg = std::move(optarg);
        }

        it->second(optArg);
    }

    return 0;
}

}; // namespace Ecg


#ifdef ECG_BASE_TEST
static void A(std::string &arg)
{
    std::cout << "A " << arg << std::endl;
}

static void B(std::string &arg)
{
    std::cout << "B " << arg << std::endl;
}

static void C(std::string &arg)
{
    std::cout << "C " << std::endl;
}

int main(int argc, char **argv)
{
    // std::string path = "/sys/fs/cgroup/cpuacct/cpuacct.usage";

    // std::cout << Ecg::Fs_Utils::readFile(path);
    // for (auto item : Ecg::Fs_Utils::readFileLine(path)) {
    //     std::cout << item << std::endl;
    // }

    std::string shortOpt = "a:b:c";
    Ecg::Opt_Parser optParser(shortOpt);

    optParser.Regist_Handler('a', A);
    optParser.Regist_Handler('b', B);
    optParser.Regist_Handler('c', C);

    optParser.Start_Parse(argc, argv);

    return 0;
}
#endif