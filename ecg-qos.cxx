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

    return Fs_Utils::readFile("/sys/devices/system/cpu/cpu"
                    + std::to_string(cpu) + "online").compare("0");
}

long long Qos::Qos_GroupEvents(perf_event_attr *eventAttr, long long timeoutUs)
{
    long long Total = 0;
    long long count = 0;
    struct timespec tv = {
	    .tv_sec = timeoutUs / 1000000,
	    .tv_nsec = (timeoutUs % 1000000) * 1000};
    std::vector<int> perfFds;
    size_t i = 0;

    if (eventAttr == NULL) {
        return -1;
    }

    if (Qos_PreparePerfEventGrp()) {
        std::cout << "Qos_PreparePerfEventGrp failed!\n";
        return -1;
    }

    /* common config */
    eventAttr->size = sizeof(perf_event_attr);
    eventAttr->disabled = 1;
    eventAttr->inherit = 1;
    eventAttr->precise_ip = 0;

    for (i = 0; i < (size_t)get_nprocs_conf(); i++) {
        if (!CPUOnline(i)) {
            continue;
        }

        int fd = perf_event_open(eventAttr, m_perfEventFd, i, -1, PERF_FLAG_PID_CGROUP|PERF_FLAG_FD_CLOEXEC);
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
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetCpuCycles(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_CPU_CYCLES;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetBranchInstructions(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetBranchMisses(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_BRANCH_MISSES;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetCacheRefs(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_CACHE_REFERENCES;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetCacheMisses(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_CACHE_MISSES;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetDTLBLoads(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HW_CACHE;
    pe.config = PERF_COUNT_HW_CACHE_DTLB |
                PERF_COUNT_HW_CACHE_OP_READ << 8 |
                PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetDTLBMisses(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HW_CACHE;
    pe.config = PERF_COUNT_HW_CACHE_DTLB |
                PERF_COUNT_HW_CACHE_OP_READ << 8 |
                PERF_COUNT_HW_CACHE_RESULT_MISS << 16;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetL3Loads(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HW_CACHE;
    pe.config = PERF_COUNT_HW_CACHE_LL |
                PERF_COUNT_HW_CACHE_OP_READ << 8 |
                PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetL3Misses(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_HW_CACHE;
    pe.config = PERF_COUNT_HW_CACHE_LL |
                PERF_COUNT_HW_CACHE_OP_READ << 8 |
                PERF_COUNT_HW_CACHE_RESULT_MISS << 16;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetAlignmentFaults(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_SOFTWARE;
    pe.config = PERF_COUNT_SW_ALIGNMENT_FAULTS;
    pe.sample_period = 0;
    pe.sample_type = PERF_SAMPLE_IDENTIFIER;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetContextSwitches(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_SOFTWARE;
    pe.config = PERF_COUNT_SW_CONTEXT_SWITCHES;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetPageFaults(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_SOFTWARE;
    pe.config = PERF_COUNT_SW_PAGE_FAULTS;
    pe.sample_period = 0;
    pe.sample_type = PERF_SAMPLE_IDENTIFIER;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetTaskClock(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_SOFTWARE;
    pe.config = PERF_COUNT_SW_TASK_CLOCK;
    pe.sample_period = 0;
    pe.sample_type = PERF_SAMPLE_IDENTIFIER;

    return Qos_GroupEvents(&pe, timeoutUs);
}

long long Qos::Qos_GetCPUClock(long long timeoutUs)
{
    struct perf_event_attr pe = {0};

    pe.type = PERF_TYPE_SOFTWARE;
    pe.config = PERF_COUNT_SW_CPU_CLOCK;
    pe.sample_period = 0;
    pe.sample_type = PERF_SAMPLE_IDENTIFIER;

    return Qos_GroupEvents(&pe, timeoutUs);
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

    printf("cgrp:%s in %lldus:\n", cgrp.c_str(), timeout);
    while (1) {
        long long insts = EcgQos.Qos_GetInstrumentons(timeout);
        long long cycles = EcgQos.Qos_GetCpuCycles(timeout);
        long long cacheRefs = EcgQos.Qos_GetCacheRefs(timeout);
        long long CacheMisses = EcgQos.Qos_GetCacheMisses(timeout);
        //long long branchRefs = EcgQos.Qos_GetBranchInstructions(timeout);
        long long branchMisses = EcgQos.Qos_GetBranchMisses(timeout);
        long long dTLBloads = EcgQos.Qos_GetDTLBLoads(timeout);
        long long dTLBMisses = EcgQos.Qos_GetDTLBMisses(timeout);
        // long long L3Misses = EcgQos.Qos_GetL3Misses(timeout);

        long long cs = EcgQos.Qos_GetContextSwitches(timeout);
        long long faults = EcgQos.Qos_GetPageFaults(timeout);
        long long tc = EcgQos.Qos_GetTaskClock(timeout);
        long long cc = EcgQos.Qos_GetCPUClock(timeout);

        printf("\n"
            "IPC:%0.2f\n"
            "CacheRefs:%lld Cachemiss:%lld\n"
            "BranchMiss:%lld\n"
            "dTLB-loads:%lld dTLB-Misses:%lld\n"
            "ContextSwitch:%lld TaskClock:%lld CPU clock:%lld\n"
            "pagefaults:%lld\n",
            (double)insts/(double)cycles,
            cacheRefs, CacheMisses,
            branchMisses,
            dTLBloads, dTLBMisses,
            cs, tc, cc, faults);
        sleep(1);
    }

    return 0;
}

