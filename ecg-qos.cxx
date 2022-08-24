#include "iostream"
#include "ecg-qos.h"
#include "ecg-base.h"
#include "ecg-list.h"

#include <vector>
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

long long Qos::Qos_GroupEvents(perf_type_id perfType, long long timeoutUs, int config)
{
    long long Total = 0;
    long long count = 0;
    struct perf_event_attr pe = { 0 };
    struct timespec tv = {
	    .tv_sec = timeoutUs / 1000000,
	    .tv_nsec = (timeoutUs % 1000000) * 1000};
    std::vector<int> perfFds;
    size_t i = 0;

    if (Qos_PreparePerfEventGrp()) {
        std::cout << "Qos_PreparePerfEventGrp failed!\n";
        return -1;
    }

    pe.type = perfType;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = config;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    for (i = 0; i < (size_t)get_nprocs_conf(); i++) {
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
        Total += count;
        close(perfFds.at(i));
    }

    return Total;
}

int Qos::Qos_PreparePerfEventGrp()
{
    if (m_perfEventFd > 0) {
        return 0;
    }

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

long long Qos::Qos_GetInstrumentons(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_HARDWARE, timeoutUs, PERF_COUNT_HW_INSTRUCTIONS);
}

long long Qos::Qos_GetCpuCycles(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_HARDWARE, timeoutUs, PERF_COUNT_HW_CPU_CYCLES);
}

long long Qos::Qos_GetBranchMisses(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_HARDWARE, timeoutUs, PERF_COUNT_HW_CACHE_MISSES);
}

long long Qos::Qos_GetCacheMisses(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_HARDWARE, timeoutUs, PERF_COUNT_HW_BRANCH_MISSES);
}

long long Qos::Qos_GetDTLBLoads(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_HW_CACHE, timeoutUs,
                PERF_COUNT_HW_CACHE_DTLB | PERF_COUNT_HW_CACHE_OP_READ << 8 |
                PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
}

long long Qos::Qos_GetDTLBMisses(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_HW_CACHE, timeoutUs,
                PERF_COUNT_HW_CACHE_DTLB | PERF_COUNT_HW_CACHE_OP_READ << 8 |
                PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
}

long long Qos::Qos_GetL3Loads(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_HW_CACHE, timeoutUs,
                PERF_COUNT_HW_CACHE_LL | PERF_COUNT_HW_CACHE_OP_READ << 8 |
                PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
}

long long Qos::Qos_GetL3Misses(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_HW_CACHE, timeoutUs,
                PERF_COUNT_HW_CACHE_LL | PERF_COUNT_HW_CACHE_OP_READ << 8 |
                PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
}

long long Qos::Qos_GetAlignmentFaults(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_SOFTWARE, timeoutUs, PERF_COUNT_SW_ALIGNMENT_FAULTS);
}

long long Qos::Qos_GetContextSwitches(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_SOFTWARE, timeoutUs, PERF_COUNT_SW_CONTEXT_SWITCHES);
}

long long Qos::Qos_GetPageFaults(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_SOFTWARE, timeoutUs, PERF_COUNT_SW_PAGE_FAULTS);
}

long long Qos::Qos_GetTaskClock(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_SOFTWARE, timeoutUs, PERF_COUNT_SW_TASK_CLOCK);
}

long long Qos::Qos_GetCPUClock(long long timeoutUs)
{
    return Qos_GroupEvents(PERF_TYPE_SOFTWARE, timeoutUs, PERF_COUNT_SW_CPU_CLOCK);
}

};

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("usage: %s + cgrp + timeoutUs\n", argv[0]);
        return 1;
    }

    std::string cgrp = argv[1];
    Ecg::Qos EcgQos(cgrp);
    long long timeout = atoll(argv[2]);

    long long insts = EcgQos.Qos_GetInstrumentons(timeout);
    long long cycles = EcgQos.Qos_GetCpuCycles(timeout);
    long long CacheMisses = EcgQos.Qos_GetCacheMisses(timeout);
    long long branchMisses = EcgQos.Qos_GetBranchMisses(timeout);
    long long dTLBloads = EcgQos.Qos_GetDTLBLoads(timeout);
    long long dTLBMisses = EcgQos.Qos_GetDTLBMisses(timeout);
    long long L3Misses = EcgQos.Qos_GetL3Misses(timeout);

    long long cs = EcgQos.Qos_GetContextSwitches(timeout);
    long long faults = EcgQos.Qos_GetPageFaults(timeout);
    long long tc = EcgQos.Qos_GetTaskClock(timeout);
    long long cc = EcgQos.Qos_GetCPUClock(timeout);

    printf("cgrp:%s in %lldus\n"
            "IPC:%lld%% Cachemiss:%lld BranchMiss:%lld\n"
            "dTLB-loads:%lld dTLB-Misses:%lld LLC-Misses:%lld\n"
            "ContextSwitch:%lld TaskClock:%lld CPU clock:%lld\n"
	        "pagefaults:%lld\n",
            cgrp.c_str(), timeout, insts/cycles, CacheMisses, branchMisses,
            dTLBloads, dTLBMisses, L3Misses,
            cs, tc, cc, faults);

    return 0;
}
