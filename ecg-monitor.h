#include <sys/epoll.h>
#include <string>
#include <vector>
#pragma once

#define PSI_ROOT    "/proc/pressure/"
#define PSI_SOME  "some"
#define PSI_FULL  "full"
#define MAX_PULL_COUNT  24

namespace Ecg {

typedef enum {
    PSI_TYPE_CPU        = 0,
    PSI_TYPE_MEM,
    PSI_TYPE_IO,

    PSI_TYPE_CNT,
} PSI_TYPE;

typedef enum {
    PRESSURE_NORMAL     = 0,
    PRESSURE_MID,
    PRESSURE_HIGH,

    PRESSURE_LEVEL_CNT,
} PRESSURE_LEVEL;

typedef struct {
    char someOrFull[8];
    unsigned int stallAmountUs;
    unsigned int timeWindowUs;
} psiTriggerData;

typedef struct {
    PSI_TYPE    type;
    PRESSURE_LEVEL  level;
} Private_Data_T;

typedef int (*Poll_Handler)(Private_Data_T, std::string &);

class Poll_Data {
public:
    int GetFd() { return m_fd; };
    int GetPsiType() { return m_psiType; }
    int GetPressureLevel() { return m_level; }
    std::string GetCgrpPath() { return m_cgrpPath; }

    Poll_Data(int fd, PSI_TYPE type, PRESSURE_LEVEL level, std::string cgrpPath) :
            m_fd(fd), m_psiType(type), m_level(level), m_cgrpPath(cgrpPath) { };
    ~Poll_Data() {};
private:
    int m_fd;
    PSI_TYPE m_psiType;
    PRESSURE_LEVEL m_level;
    std::string m_cgrpPath;
};

class Epoll_Type {
public:
    Epoll_Type(int maxEvent) :
    m_maxEvent(maxEvent), m_epollFd(0), m_epollEvent(NULL) {  };
    ~Epoll_Type() { if (m_epollEvent) free(m_epollEvent); };

    int Epoll_AddEvent(Poll_Data *data);
    int Epoll_DelEvent(Poll_Data &data);
    int Epoll_Loop();

private:
    int m_maxEvent;
    int m_epollFd;
    struct epoll_event *m_epollEvent;
};

class PsiConfig {
public:
    void Init();
    psiTriggerData *GetPsiCfg(PSI_TYPE psiType, PRESSURE_LEVEL level)
    {
        // TODO:size check
        return m_Cfg[psiType] + level;
    }

    PsiConfig() {}
    ~PsiConfig() {}

private:
    psiTriggerData **m_Cfg; 
};

class Ecg_Monitor {
public:
    int StartPSIMonitor(); // base on psi control file
    int StartEventMonitor(std::string controlFile, std::string args); // base on event control file

    Ecg_Monitor() { }
    ~Ecg_Monitor() { }

private:
    void ScanMonitorRoot();
    int MonitorCgroup(std::string cgrpPath, PSI_TYPE type, PRESSURE_LEVEL level);
    std::vector<std::string> m_monitorRoot;
    Ecg::Epoll_Type *m_ep;
    Ecg::PsiConfig *m_psiCfg;
};

}; // namespace Ecg