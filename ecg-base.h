#include <string>
#include <vector>
#include <getopt.h>

#pragma once

namespace Ecg {

class Fs_Utils {
public:
    // read file, return multiple lines
    static std::vector<std::string> readFileLine(const std::string& path);
    // read file to single string
    static std::string readFile(const std::string& path);

    Fs_Utils() {}
    ~Fs_Utils() {}
};

class Common_Utils {
public:
    // Gets the part where two strings overlap
    static std::string Getoverlap(std::string str1, std::string str2);
    static void PrintVectorString(std::string prefix,
                std::vector<std::string> &v);

    Common_Utils() {}
    ~Common_Utils() {}
};

typedef void (* OptHandlerDF)(std::string optArg);

class Opt_Parser {
public:
    int Regist_Handler(char shortOpt, OptHandlerDF optHandler);
    int Start_Parse(int argc, char **argv);

    Opt_Parser(std::string &shortOpt) :
        m_shortOpts(optStr) {}
    ~Opt_Parser() {}

private:
    std::string m_shortOpts;
    std::map<char, OptHandlerDF> m_handlers;
};

};