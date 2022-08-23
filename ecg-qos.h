#include <string>

#pragma once

namespace Ecg {

class Qos {
public:
    Qos(std::string &cgrp) :
    m_cgrp(cgrp), m_perfEventFd(-1) {}
    ~Qos() {}

    long long Qos_GetCPUCycles(unsigned timeout);
    long long Qos_GetInstrumentions(unsigned timeout);
    long long Qos_GetCacheMisses(unsigned timeout);

private:
    int Qos_PreparePerfEventGrp();
    void Qos_DestroyPerfEventGrp();

    std::string m_cgrp;
    std::string m_perfEventGrp;
    int m_perfEventFd;
};

};