#include "iostream"
#include "ecg-qos.h"
#include "ecg-base.h"
#include "ecg-list.h"

#include <vector>
#include <linux/perf_event.h>    /* Definition of PERF_* constants */
#include <linux/hw_breakpoint.h> /* Definition of HW_* constants */
#include <sys/syscall.h>         /* Definition of SYS_* constants */
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sysinfo.h> // for cpuinfo

namespace Ecg {

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

// TODO: add to common_utils
static bool CPUOnline(int cpu)
{
    if (cpu == 0)
        return true;
    std::string online = Fs_Utils::readFile("/sys/devices/system/cpu/cpu"
            + std::to_string(cpu) + "online");

    return online.compare("0");
}

long long Qos::Qos_GetCPUCycles(unsigned timeout)
{
    long long cycles = 0;
    long long count = 0;
    struct perf_event_attr pe = { 0 };
    struct timespec tv = {.tv_sec = timeout, .tv_nsec = 0};
    std::vector<int> perfFds;
    int i = 0;

    if (Qos_PreparePerfEventGrp()) {
        std::cout << "Qos_PreparePerfEventGrp failed!\n";
        return -1;
    }

    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    for (i = 0; i < get_nprocs_conf(); i++) {
        if (!CPUOnline(i)) {
            continue;
        }

        int fd = perf_event_open(&pe, m_perfEventFd, i, -1, PERF_FLAG_PID_CGROUP|PERF_FLAG_FD_CLOEXEC);
        if (fd < 0) {
            std::cout << "perf_event_open failed, May not support!\n";
            return -1;
        }
        perfFds.emplace_back(fd);
    }

    for (i = 0; i < perfFds.size(); i++) {
        ioctl(perfFds.at(i), PERF_EVENT_IOC_ENABLE, 0);
    }

    nanosleep(&tv, NULL);
    for (i = 0; i < perfFds.size(); i++) {
        ioctl(perfFds.at(i), PERF_EVENT_IOC_DISABLE, 0);
    }

    for (i = 0; i < perfFds.size(); i++) {
        read(perfFds.at(i), &count, sizeof(count));
        cycles += count;
        close(perfFds.at(i));
    }

    return cycles;
}

long long Qos::Qos_GetInstrumentions(unsigned timeout)
{
    long long inst = 0;

    return inst;
}

long long Qos::Qos_GetCacheMisses(unsigned timeout)
{
    long long misses = 0;

    return misses;
}

int Qos::Qos_PreparePerfEventGrp()
{
    auto cgrpTasks = Fs_Utils::readFileLine(m_cgrp + "/tasks");
    if (!cgrpTasks.size()) {
        std::cout << "Cgroup " + m_cgrp + " invalid, no tasks!\n" << std::endl;
        return -1;
    }

    if (Common_Utils::IsCgroupV2()) {
        std::cout << "Cgroup v2, to be added!\n" << std::endl;
        return -1;
    }

//     size_t pos = m_cgrp.rfind("/");
//     std::string grpName = "/sys/fs/cgroup/perf_event/" + m_cgrp.substr(pos, m_cgrp.length());

//     if (Fs_Utils::GetFileType(grpName) == DIRECTORY) {
//         goto open;
//     }

//     // create perf_event group
//     mkdir(grpName.c_str(), 0664);
//     for (auto item : cgrpTasks) {
//         Fs_Utils::WriteFile(grpName + "/tasks", item, true);
//     }

//     m_perfEventGrp = grpName;

// open:
    m_perfEventFd = open(m_cgrp.c_str(), O_RDONLY);

    return 0;
}

void Qos::Qos_DestroyPerfEventGrp()
{
    if (m_perfEventGrp.empty()) {
        return;
    }

    close(m_perfEventFd);
    rmdir(m_perfEventGrp.c_str());
}

};

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: %s + cgrp\n", argv[0]);
        return 1;
    }

    std::string cgrp = argv[1];
    Ecg::Qos EcgQos(cgrp);

    printf("cgrp %s cycles:%lld\n", cgrp.c_str(), EcgQos.Qos_GetCPUCycles(3));

    return 0;
}