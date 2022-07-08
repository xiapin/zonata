#include <string>
#include <vector>
#include <getopt.h>
#include <map>
#include <ncurses.h>

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

typedef void (* OptHandlerDF)(std::string &optArg);

class Opt_Parser {
public:
    int Regist_Handler(char shortOpt, OptHandlerDF optHandler);
    int Start_Parse(int argc, char **argv);

    Opt_Parser(std::string &shortOpt) :
        m_shortOpts(shortOpt) {}
    ~Opt_Parser() {}

private:
    std::string m_shortOpts;
    std::map<char, OptHandlerDF> m_handlers;
};

class Curses_Win {
public:
    int CW_Init(std::string bkStr);
    int CW_SetContent(std::vector<std::string> &v);
    // int CW_SetContent(std::string &str);
    int CW_SetContent(std::map<std::string, std::vector<std::string>> &m);
    int CW_Draw(std::string selected);
    int CW_BindWin(Curses_Win *win, bool sub);

    Curses_Win() {}
    ~Curses_Win() {}

private:
    int CW_Exceptoin();
    int CW_CreateBkd(std::string bkStr);

        // int CW_Draw(std::string str);
    int CW_Draw(unsigned curPage);
    int CW_Scroll(unsigned curPage, int size);
    int CW_Delete(WINDOW **win, int count);

    std::vector<std::string> m_vContent;
    std::map<std::string, std::vector<std::string>> m_mapContent;
    Curses_Win *m_subWin, *m_belongWin;
    WINDOW **m_win;
    WINDOW *m_menu;
    WINDOW *m_shadow;
    unsigned int m_row, m_col, m_lSize, m_wSize, m_totalPages;
};

};