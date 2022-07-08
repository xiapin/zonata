#include "ecg-monitor.h"
#include "ecg-list.h"
#include <iostream> // TODO
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/eventfd.h>

namespace Ecg {

int Epoll_Type::Epoll_AddEvent(Poll_Data *data, EPOLL_EVENTS events)
{
    struct epoll_event event = {0};

    if (data == NULL) {
        return -1;
    }

    if (m_epollFd == 0) {
        m_epollFd = epoll_create(m_maxEvent);
    }

    // event.events = EPOLLPRI | EPOLLIN;
    event.events = events;
    event.data.ptr = (void *)data;
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, data->GetFd(), &event);

    return 0;
}

int Epoll_Type::Epoll_DelEvent(Poll_Data &data)
{
    return 0;
}

int Epoll_Type::Epoll_Loop()
{
    char buf[128] = { 0 };

    if (m_epollEvent == NULL) {
        m_epollEvent = (struct epoll_event *)malloc(sizeof(struct epoll_event) * m_maxEvent);

        if (m_epollEvent == NULL) {
            return -1;
        }
        memset(m_epollEvent, 0, sizeof(struct epoll_event) * m_maxEvent);
    }

    while (1) {
        int count = epoll_wait(m_epollFd, m_epollEvent, m_maxEvent, -1);
        if (count < 0) {
            std::cout << "epoll error: " << strerror(errno) << std::endl;
            // break;
            continue;
        }

        for (int i = 0; i < count; i++) {
            Poll_Data *pData = (Poll_Data *)((m_epollEvent + i)->data.ptr);

            read(pData->GetFd(), buf, sizeof(buf));
            std::cout << pData->GetCgrpPath() + " Psi trigger, type: " << pData->GetPsiType() <<
            " level: " << pData->GetPressureLevel() << std::endl;
        }
    }

    return 0;
}

void Ecg_Monitor::ScanMonitorRoot()
{
    Ecg::Ecg_list El;

    auto cgrpListMap = El.GetCgrpListMap(); // first: /sys/fs/cgroup/cpu,cpuact...
    auto contListMap = El.GetAllContainers(); // first: docker, podman...

    std::map<std::string, std::vector<std::string>>::iterator it, item;

    m_monitorRoot.clear();
    for (item = cgrpListMap.begin(); it != cgrpListMap.end(); item++) {
        if (strstr(item->first.c_str(), "cpu")) {
            // std::cout << item->first << std::endl;
            for (it = contListMap.begin(); it != contListMap.end(); it++) {
                m_monitorRoot.push_back(item->first + "/" + it->first);
            }
            break;
        }
    }
}

int Ecg_Monitor::MonitorCgroup(std::string cgrpPath, PSI_TYPE type, PRESSURE_LEVEL level)
{
    char tmp[128] = {0};

    std::cout << "Add " + cgrpPath + " to monitor...\n";
    int fd = open(cgrpPath.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        std::cout << "open " + cgrpPath + " error!\n";
        return -1;
    }

    psiTriggerData *trigData = m_psiCfg->GetPsiCfg(type, level);
    if (trigData == NULL) {
        std::cout << "get trigger data error\n";
        close(fd);
        return -1;
    }
    snprintf(tmp, sizeof(tmp), "%s %d %d", trigData->someOrFull,
            trigData->stallAmountUs, trigData->timeWindowUs);
    if (write(fd, tmp, sizeof(tmp)) < 0) {
        std::cout << "write trigger data error!\n";
        close(fd);
        return -1;
    }

    Poll_Data *pollData = new Poll_Data(fd, type, level, cgrpPath);
    return m_ep->Epoll_AddEvent(pollData, EPOLLPRI);;
}

int Ecg_Monitor::Init()
{
    if (m_monitorRoot.empty()) {
        ScanMonitorRoot();
        if (m_monitorRoot.empty()) {
            return 0; // no available cgroup.
        }
    }

    m_ep = new Epoll_Type(1024);
    m_psiCfg = new PsiConfig();

    m_psiCfg->Init();
    ScanMonitorRoot();

    if (m_monitorRoot.empty()) {
        std::cout << "No available child cgroup monitor.\n";
        return -1;
    }

    return 0;
}

void Ecg_Monitor::StartMonitor()
{
    m_ep->Epoll_Loop();
}

int Ecg_Monitor::AddPSIMonitor()
{
    for (auto it : m_monitorRoot) {
        MonitorCgroup(it + "/memory.pressure", PSI_TYPE_MEM, PRESSURE_HIGH);
        MonitorCgroup(it + "/io.pressure", PSI_TYPE_IO, PRESSURE_HIGH);
        MonitorCgroup(it + "/cpu.pressure", PSI_TYPE_CPU, PRESSURE_HIGH);
    }

    return 0;
}

int Ecg_Monitor::AddEventMonitor(std::string controlFile, std::string args)
{
    char line[LINE_MAX];
   
    int cFd = open(controlFile.c_str(), O_RDONLY);
    if (cFd < 0) {
        std::cout << "Open " + controlFile + " error\n";
        return -1;
    }

    int pos = controlFile.rfind("/");
    std::string eventFile = controlFile.substr(0, pos) + "/cgroup.event_control";
    int eFd = open(eventFile.c_str(), O_WRONLY);
    if (eFd < 0) {
        std::cout << "Open " + eventFile + " error\n";
        close(cFd);
        return -1;
    }

    int epFd = eventfd(0, 0);

    // std::string eventStr = std::to_string(epFd) + std::to_string(cFd) + args;
    snprintf(line, LINE_MAX, "%d %d %s", epFd, cFd, args.c_str());
    // if (write(epFd, eventStr.c_str(), eventStr.length()) < 0) {
    if (write(eFd, line, strlen(line) + 1) < 0) {
        std::cout << "Write " << line << " error\n";
        close(cFd);
        close(eFd);
        return -1;
    }

    Poll_Data *pollData = new Poll_Data(epFd, PSI_TYPE_MEM, PRESSURE_MID, controlFile);
    m_ep->Epoll_AddEvent(pollData, EPOLLIN);

    return 0;
}

// TODO: self define or from configure file
static psiTriggerData ioTrig[PRESSURE_LEVEL_CNT] = {
    {PSI_SOME, 70000, 1000000},
    {PSI_SOME, 100000, 1000000},
    {PSI_FULL, 70000, 1000000},
};

static psiTriggerData cpuTrig[PRESSURE_LEVEL_CNT] = {
    {PSI_SOME, 50000, 1000000},
    {PSI_SOME, 70000, 1000000},
    {PSI_SOME, 100000, 1000000},
};

static psiTriggerData memTrig[PRESSURE_LEVEL_CNT] = {
    {PSI_SOME, 70000, 1000000},
    {PSI_SOME, 100000, 1000000},
    {PSI_FULL, 70000, 1000000},
};

void PsiConfig::Init()
{
    m_Cfg = (psiTriggerData **)malloc(sizeof(void *) * PSI_TYPE_CNT);
    if (m_Cfg == NULL) {
        abort();
    }

    m_Cfg[PSI_TYPE_CPU] = cpuTrig;
    m_Cfg[PSI_TYPE_MEM] = memTrig;
    m_Cfg[PSI_TYPE_IO] = ioTrig;
}

};  // namespace Ecg

int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    Ecg::Ecg_Monitor ecgMonitor;

    if (ecgMonitor.Init()) {
        std::cout << "Init failed\n" ;
        return 0;
    }

    // memory/docker/<container_id>/memory.usage_in_bytes 104857600
    // memory/docker/<container_id>/memory.oom_control 1
    // memory/docker/<container_id>/memory.pressure_level level(low/medium/critical),mode(default/hierarchy/local)
    // memory/docker/<container_id>/memory.memsw.usage_in_bytes 104857600
    ecgMonitor.AddEventMonitor(argv[1], argv[2]);
    ecgMonitor.AddPSIMonitor();
    ecgMonitor.StartMonitor();

    return 0;
}