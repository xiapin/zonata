#include "ecg-base.h"
#include <fstream>
#include <iostream>
#include <math.h>

#include "ecg-list.h"

#define ENTER       10
#define ESCAPE      27

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

int Curses_Win::CW_Exceptoin()
{
    if (m_row >= 20 && m_col >= 90) {
        return 0;		
	}

    touchwin(stdscr);
    refresh();	
    endwin();
    std::cout << "ERROR: Window initialization failed. Please set the minimum window size to 20x90." << std::endl;
    return 1;
}

int Curses_Win::CW_CreateBkd(std::string bkStr)
{
    bkgd(COLOR_PAIR(1));

    m_menu = subwin(stdscr, 1, m_col, 0, 0);
	m_shadow = subwin(stdscr, m_row - 8, m_wSize, 6, 10);

    wbkgd(m_menu, COLOR_PAIR(2));
    waddstr(m_menu, bkStr.c_str());
    wattron(m_menu, COLOR_PAIR(3));
	wattroff(m_menu, COLOR_PAIR(3));
	wbkgd(m_shadow, COLOR_PAIR(3));

    refresh();

    return 0;
}

int Curses_Win::CW_Init(std::string bkStr)
{
    initscr();
    getmaxyx(stdscr, m_row, m_col);
	m_lSize = m_row - 10;
	m_wSize = m_col - 16;

    if (CW_Exceptoin()) {
        return 1;
    }

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLUE, COLOR_WHITE);
    init_pair(3, COLOR_BLACK, COLOR_BLACK);

    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);

    CW_CreateBkd(bkStr);
    refresh();

    return 0;
}

void Curses_Win::CW_FlushContent(std::string selected)
{
    if (!m_cursesContent) {
        mvprintw(3, 8, "Content Not Set Yet!");
        abort();
    }

    if (selected.empty()) {
        return;
    }

    m_vContent = std::move(m_cursesContent->GetContent(selected));
    m_totalPages = ceil(m_vContent.size()/(m_row - 10.0)) - 1;
}

int Curses_Win::CW_Draw(std::string selected)
{
    CW_FlushContent(selected);

    CW_DrawPage(0);
    std::cout << "No content\n" << std::endl;
    return -1;
}

int Curses_Win::CW_BindWin(Curses_Win *win, bool sub)
{
    if (sub) {
        m_subWin = win;
    } else {
        m_belongWin = win;
    }

    return 0;
}

int Curses_Win::CW_Scroll(unsigned curPage, unsigned size)
{
    unsigned selected = 0;
    mvprintw(3, 8, "Remain pages(%d/%d)", curPage, m_totalPages); // TODO
    int rowNum = size < m_lSize ? size : m_lSize;

    while (1) {
        int key = getch();

        if (key == KEY_DOWN) {
            wbkgd(m_win[selected + 1], COLOR_PAIR(2));
            wnoutrefresh(m_win[selected + 1]);

            selected = (selected + 1) % rowNum;

            wbkgd(m_win[selected + 1], COLOR_PAIR(1));
            wnoutrefresh(m_win[selected + 1]);
            doupdate();
        } else if (key == KEY_UP) {
            wbkgd(m_win[selected + 1], COLOR_PAIR(2));
            wnoutrefresh(m_win[selected + 1]);

            selected = (selected + rowNum - 1) % rowNum;
            wbkgd(m_win[selected + 1], COLOR_PAIR(1));
            wnoutrefresh(m_win[selected + 1]);
            doupdate();
        } else if (key == KEY_PPAGE || key == KEY_NPAGE) {
            CW_Delete(m_win, m_lSize);
            touchwin(stdscr);
            refresh();
            curPage = (key == KEY_PPAGE) ? (curPage - 1) : (curPage + 1);
            if (curPage > m_totalPages || curPage < 0)
                curPage = 0;

            return CW_DrawPage(curPage);
        } else if (key == ENTER && m_subWin) {
            m_subWin->CW_BindWin(this, false);
            return m_subWin->CW_Draw(m_vContent.at(curPage * m_lSize + selected));
        } else if (key == ESCAPE) {
            if (m_belongWin)
                return m_belongWin->CW_Draw("");
            CW_Delete(m_win, m_lSize);
            exit(0);
        } else {
            mvprintw(3, 8, "Uknow key:%d  ", key);
        }
    }

    return 0;
}

int Curses_Win::CW_DrawPage(unsigned int curPage)
{
    unsigned int i, j, vecStart;

    m_win = (WINDOW **)malloc((m_lSize + 1) * sizeof(WINDOW *));
    if (!m_win) {
        abort();
    }
    m_win[0] = newwin(m_row - 8, m_wSize, 5, 8); // start_col = 8

    wbkgd(m_win[0], COLOR_PAIR(2));
	wborder(m_win[0], ' ', ' ', 0, 0,' ',' ',' ',' ');

    for (i = 1, j = 5; i <= m_lSize; i++, j++) {
        m_win[i] = subwin(m_win[0], 1, m_wSize - 2, j + 1, 9); // start_col + 1
    }

    vecStart = curPage * m_lSize;
    for (i = 1; i < m_lSize + 1 && i < m_vContent.size() - vecStart + 1; i++) {
        wprintw(m_win[i], m_vContent.at(vecStart + i - 1).c_str());
    }
    wbkgd(m_win[1], COLOR_PAIR(1));
    wrefresh(m_win[0]);

    return CW_Scroll(curPage, m_vContent.size() - (curPage * m_lSize));
}

int Curses_Win::CW_Delete(WINDOW **win, int count)
{
    int i = 0;
    for (i = 0; i < count; i++) {
        delwin(win[i]);
    }

    free(win);
    return 0;
}

}; // namespace Ecg

// Test Code
namespace Ecg {

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

};

// #ifdef ECG_BASE_TEST
#if 1
int main(int argc, char **argv)
{
    std::vector<std::string> v;
    for (auto i = 0; i < 100; i++) {
        v.emplace_back(std::to_string(i));
    }

    Ecg::Win_CgrpRoot *cgrpRoot = new Ecg::Win_CgrpRoot;
    Ecg::Win_CgrpSub *cgrpSub = new Ecg::Win_CgrpSub;

    Ecg::Curses_Win cw(cgrpRoot);
    Ecg::Curses_Win *subWin = new Ecg::Curses_Win(cgrpSub);
    Ecg::Curses_Win *subsubWin = new Ecg::Curses_Win(cgrpSub);

    if (cw.CW_Init(" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx ")) {
        std::cout << "Curses init failed\n";
        return 1;
    }
    subWin->CW_Init(" This is subWin"); // sub window bkg is invalid.

    subsubWin->CW_Init("");
    subWin->CW_BindWin(subsubWin, true);

    cw.CW_BindWin(subWin, true);
    cw.CW_Draw("null");

    while (1) {
        getchar();
    }

    return 0;
}
#endif