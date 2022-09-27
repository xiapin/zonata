#include "ecg-monitor.h"
#include "ecg-list.h"
#include "ecg-base.h"
#include <iostream> // TODO
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/eventfd.h>
#include <sys/wait.h>

extern char** environ;

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

int Epoll_Type::Epoll_DelEvent(Poll_Data *data)
{
    struct epoll_event event = {0};

    if (data == NULL) {
        return -1;
    }

    std::cout << "Remove " + data->GetCgrpPath() +" to monitor group!\n";
    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, data->GetFd(), &event);
    delete data;

    return 0;
}

int Epoll_Type::Epoll_Loop()
{
    char buf[128] = { 0 };

    if (m_epollFd == 0) {
        std::cout << "No avalibel resource to monitor!\n";
        return 0;
    }

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
            if (!Fs_Utils::FileExist(pData->GetCgrpPath())) {
                Epoll_DelEvent(pData);
                continue;
            }

            read(pData->GetFd(), buf, sizeof(buf));
            std::cout << pData->GetCgrpPath() + " Psi trigger, type: " << pData->GetPsiType() <<
            " level: " << pData->GetPressureLevel() << std::endl;
        }
    }

    return 0;
}

void Ecg_Monitor::ScanMonitorRoot()
{
    auto cgrpRoot = Ecg_list::GetCgroupRoots(); // /sys/fs/cgroup/cpu,cpuact...
    auto contListMap = Ecg_list::GetAllContainers(); // first: docker, podman...
    std::map<std::string, std::vector<std::string>>::iterator it;

    m_monitorRoot.clear();
    if (Common_Utils::IsCgroupV2()) {
        for (it = contListMap.begin(); it != contListMap.end(); it++) {
            m_monitorRoot.emplace_back(it->first);
            m_monitorRoot.insert(m_monitorRoot.end(), it->second.begin(), it->second.end());
        }
        return;
    }

    std::string cpuSubsys;
    for (auto item : cgrpRoot) {
        if (strstr(item.c_str(), "cpu")) {
            cpuSubsys = std::move(item);
            break;
        }
    }

     for (it = contListMap.begin(); it != contListMap.end(); it++) {
        m_monitorRoot.push_back(cpuSubsys + "/" + it->first);
        for (auto conts : it->second) {
            m_monitorRoot.push_back(cpuSubsys + "/" + conts);
        }
    }
}

int Ecg_Monitor::MonitorCgroup(std::string cgrpPath, PSI_TYPE type, PRESSURE_LEVEL level)
{
    char tmp[128] = {0};

    std::cout << "Add " + cgrpPath + " to monitor...\n";
    int fd = open(cgrpPath.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        std::cout << "open " + cgrpPath + " error:" + strerror(errno) << std::endl;;
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
        close(fd);
        return -1;
    }

    Poll_Data *pollData = new Poll_Data(fd, type, level, cgrpPath);
    return m_ep->Epoll_AddEvent(pollData, EPOLLPRI);;
}

int Ecg_Monitor::BpfResultProc(char *buf, int size)
{
    int create = 0;
    char path[128] = {0};
    std::string cgrpRoot = "/sys/fs/cgroup/";

    sscanf(buf, "%d\t%s", &create, path);
    if (create) {
        MonitorCgroup(cgrpRoot + path + "/memory.pressure", PSI_TYPE_MEM, PRESSURE_HIGH);
        MonitorCgroup(cgrpRoot + path + "/io.pressure", PSI_TYPE_IO, PRESSURE_HIGH);
        MonitorCgroup(cgrpRoot + path + "/cpu.pressure", PSI_TYPE_CPU, PRESSURE_HIGH);
    }

    return 0;
}

void Ecg_Monitor::NewGroupListener()
{
#define BPF_PROG    "/root/samples/build/cgdetect" // TODO
    int ret;
    int wstatus = 0;
    int ppfd[2];
    char buf[256] = {0};

    pipe(ppfd); // 0-read 1-write
    pid_t pid = fork();
    if (pid < 0) {
        return;
    }

    if (pid == 0) {
        close(ppfd[0]);
        dup2(ppfd[1], STDERR_FILENO);
        execve(BPF_PROG, NULL, environ);

        _exit(127);
    } else {
        close(ppfd[1]);

        while (1) {
            ret = read(ppfd[0], buf, sizeof(buf));
            if (ret <= 0)
                break;
            BpfResultProc(buf, sizeof(buf));
        }
    }

WAIT:
    ret = waitpid(pid, &wstatus, 0);
    if (ret == -1) {
        if (errno == EINTR) {
            goto WAIT;
        }
    } else if (ret != pid) {
        goto WAIT;
    }
    return;
}

int Ecg_Monitor::Init()
{
    if (m_monitorRoot.empty()) {
        ScanMonitorRoot();
        if (m_monitorRoot.empty()) {
            return 0; // no available cgroup.
        }
    }

    m_ep = new Epoll_Type(10240);
    m_psiCfg = new PsiConfig();

    m_psiCfg->Init();
    // ScanMonitorRoot();

    if (m_monitorRoot.empty()) {
        std::cout << "No available child cgroup monitor.\n";
        return -1;
    }

    m_bpfThread = new std::thread(&Ecg_Monitor::NewGroupListener, this);
    m_bpfThread->detach();

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
        std::cout << "Open " + controlFile + " error: " + strerror(errno) << std::endl;
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
    // if (argc != 3)
    //     return 1;
    Ecg::Ecg_Monitor ecgMonitor;

    if (ecgMonitor.Init()) {
        std::cout << "Init failed\n" ;
        return 0;
    }

    // memory/docker/<container_id>/memory.usage_in_bytes 104857600
    // memory/docker/<container_id>/memory.oom_control 1
    // memory/docker/<container_id>/memory.pressure_level level(low/medium/critical),mode(default/hierarchy/local)
    // memory/docker/<container_id>/memory.memsw.usage_in_bytes 104857600
    
    // ecgMonitor.AddEventMonitor(argv[1], argv[2]);
    ecgMonitor.AddPSIMonitor();
    ecgMonitor.StartMonitor();

    return 0;
}