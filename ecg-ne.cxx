#include "ecg-ne.h"
#include "ecg-list.h"

#include <fcntl.h>
#include <iostream>

namespace Ecg
{

static std::string Ecg_GetCpuUsageMetric(std::string metricName)
{
    return "hello-world";
}

int Ecg_NodeExporter::Init()
{
    return 0;
}

int Ecg_NodeExporter::Loop()
{
    for (auto item : m_Metrics) {
        std::cout << item->GetMetricData() << std::endl << item->GetMetricName() << std::endl;
    }
    return 0;
}

int Ecg_NodeExporter::RegisterMetric(Ecg_Metrics *metric)
{
    if (metric == NULL)
    {
        return -1;
    }

    m_Metrics.push_back(metric);
    return 0;
}

int Ecg_NodeExporter::UnregisterMetric(Ecg_Metrics *metric)
{
    std::vector<Ecg_Metrics *>::iterator it;
    for (it = m_Metrics.begin(); it != m_Metrics.end(); it++) {
        if (!(metric->GetMetricName().compare(metric->GetMetricName())) ) {
            m_Metrics.erase(it);
            break;
        }
    }

    return 0;
}

}; // namespace Ecg

int main(int argc, char **argv)
{
    Ecg::Ecg_NodeExporter ecgNe(9999);

    ecgNe.Init();
    Ecg::Ecg_Metrics *cpuMetrics = new Ecg::Ecg_Metrics("cpu", "cpu_usage", "current cpu_usage",
                    Ecg::SUMMARY, Ecg::Ecg_GetCpuUsageMetric);
    ecgNe.RegisterMetric(cpuMetrics);
    ecgNe.Loop();

    return 0;
}