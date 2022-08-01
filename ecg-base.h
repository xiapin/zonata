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
    static std::vector<std::string> ScanChildDir(const std::string& path, bool recursion);

    Fs_Utils() {}
    ~Fs_Utils() {}

private:
    static void ScanChildRecursion(std::string parent, std::vector<std::string> &v, bool recursion);
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

class Curses_Content {
public:
    virtual ~Curses_Content() {}
    virtual std::vector<std::string> GetContent(std::string selected) = 0;
};

class Curses_Win {
public:
    int CW_Init();
    // Initialize once
    int CW_CreateBkd(std::string bkStr);
    int CW_Draw(std::string selected);
    int CW_BindWin(Curses_Win *win, bool sub);
    int CW_DrawPage(unsigned curPage);

    Curses_Win(Curses_Content *cc) : m_cursesContent(cc), m_subWin(NULL),
            m_belongWin(NULL), m_win(NULL), m_menu(NULL),
            m_shadow(NULL) {}
    ~Curses_Win() {}

private:
    int CW_Exceptoin();

    void CW_FlushContent(std::string selected);

    int CW_SetSubWinContent(std::string selected); // for subwindow
    int CW_Scroll(unsigned curPage, unsigned size);
    int CW_Delete(WINDOW **win, int count);

    Curses_Content *m_cursesContent;
    std::vector<std::string> m_vContent;
    std::map<std::string, std::vector<std::string>> m_mapContent;
    Curses_Win *m_subWin, *m_belongWin;
    WINDOW **m_win;
    WINDOW *m_menu;
    WINDOW *m_shadow;
    unsigned int m_row, m_col, m_lSize, m_wSize, m_totalPages;
};

};