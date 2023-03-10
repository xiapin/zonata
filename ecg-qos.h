#include <string>
#include <linux/perf_event.h>    /* Definition of PERF_* constants */
#include <linux/hw_breakpoint.h> /* Definition of HW_* constants */
#include <sys/syscall.h>         /* Definition of SYS_* constants */
#include <map>

#pragma once

namespace Ecg {

class Qos {
public:
    /* Hardware */
    long long Qos_GetInstrumentons(long long timeoutUs);
    long long Qos_GetCpuCycles(long long timeoutUs);
    long long Qos_GetBranchInstructions(long long timeoutUs);
    long long Qos_GetBranchMisses(long long timeoutUs);
    long long Qos_GetCacheRefs(long long timeoutUs);
    long long Qos_GetCacheMisses(long long timeoutUs);
    long long Qos_GetDTLBLoads(long long timeoutUs);
    long long Qos_GetDTLBMisses(long long timeoutUs);
    // current not support
    long long Qos_GetL3Loads(long long timeoutUs);
    long long Qos_GetL3Misses(long long timeoutUs);
    /* software */
    long long Qos_GetAlignmentFaults(long long timeoutUs);
    long long Qos_GetContextSwitches(long long timeoutUs);
    long long Qos_GetPageFaults(long long timeoutUs);
    long long Qos_GetTaskClock(long long timeoutUs);
    long long Qos_GetCPUClock(long long timeoutUs);

    Qos(std::string &cgrp) :
    m_cgrp(cgrp), m_perfEventFd(-1) {}
    ~Qos() { Qos_DestroyPerfEventGrp(); }

private:
    long long Qos_GroupEvents(perf_event_attr *eventAttr, long long timeoutUs);
    long long Qos_GroupEvents(perf_type_id perfType, long long timeoutUs, int config);
    int Qos_PreparePerfEventGrp();
    void Qos_DestroyPerfEventGrp();

    std::string m_cgrp;
    std::string m_perfEventGrp;
    int m_perfEventFd;
    std::map<int, long long> m_evResult;
};

};

