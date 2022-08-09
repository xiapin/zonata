#include "ecg-base.h"
#include "ecg-ne.h"
#include "ecg-list.h"

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <fstream>

#define M_HELP_PREFIX     "# help "
#define M_TYPE_PREFIX     "# type "

namespace Ecg
{

class CPU_Metrics : public Ecg_Metrics {
public:
    using Ecg_Metrics::Ecg_Metrics;
    ~CPU_Metrics() {}

std::vector<std::string> GetMetricsData()
{
    if (Common_Utils::IsCgroupV2()) {
        return GetMetricsData_v2();
    }
    return GetMetricsData_v1();
}

private:
std::vector<std::string> GetMetricsData_v1()
{
    // TODO. sample
    Ecg::Ecg_list ecgList;
    std::string cpuSubsys;
    std::vector<std::string> v;

    auto cgrpRoots = ecgList.GetCgroupRoots();
    for (auto item : cgrpRoots) {
        if (strstr(item.c_str(), "cpuacct")) {
            cpuSubsys = std::move(item);
            break;
        }
    }

    if (cpuSubsys.empty()) {
        std::cout << "CPU subsys not mounted, Ecg_GetCpuUsageMetric not support.\n";
        return {};
    }

    auto allConts = ecgList.GetAllContainers();
    std::map<std::string, std::vector<std::string>>::iterator it;

    v.emplace_back(M_HELP_PREFIX + m_helpInfo);
    v.emplace_back(M_TYPE_PREFIX + get_metric_name(m_metricType));

    auto tmp = m_metricName + "{name=\"" + cpuSubsys + "\"} " +
                Ecg::Fs_Utils::readFile(cpuSubsys + "/cpuacct.usage");
    v.emplace_back(tmp);
    for (it = allConts.begin(); it != allConts.end(); it++) {
        auto tmp = m_metricName + "{name=\"" + it->first + "\"} " +
                Ecg::Fs_Utils::readFile(cpuSubsys + "/" + it->first + "/cpuacct.usage");
        v.emplace_back(tmp);

        for (auto iter : it->second) {
            tmp = m_metricName + "{name=\"" + it->first + "/" + iter.substr(0, 4) + "\"} " +
                Ecg::Fs_Utils::readFile(cpuSubsys + "/" + it->first + "/" + iter + "/cpuacct.usage");
            v.emplace_back(tmp);
        }
    }

    return v;
}

std::vector<std::string> GetMetricsData_v2()
{
    std::vector<std::string> v;
    Ecg_list ecgList;

    v.emplace_back(M_HELP_PREFIX + m_helpInfo);
    v.emplace_back(M_TYPE_PREFIX + get_metric_name(m_metricType));

    auto contList = ecgList.GetAllContainers();
    std::map<std::string, std::vector<std::string>>::iterator it;
    for (it = contList.begin(); it != contList.end(); it++) {
        v.emplace_back(it->first);
        for (auto item : it->second) {
            v.emplace_back(item);
        }
    }

    return v;
}

};

std::string
Ecg_NodeExporter::NeGetMetricData
(std::string metricName)
{
    std::vector<std::string> dataVec;
    std::string dataStr = {};

    for (auto item : m_Metrics) {
        if (!metricName.compare(item->GetMetricName())) {
            dataVec = item->GetMetricsData();
            break;
        }
    }

    if (dataVec.empty()) {
        return {};
    }

    for (auto item : dataVec) {
        dataStr.append(item);
    }

    return dataStr;
}

int Ecg_NodeExporter::RegisterMetric
(Ecg_Metrics *metric)
{
    if (metric == NULL)
    {
        return -1;
    }

    m_Metrics.push_back(metric);
    return 0;
}

int Ecg_NodeExporter::UnregisterMetric
(Ecg_Metrics *metric)
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


int Ecg_Server::Start()
{
    m_thrd = new std::thread(&Ecg_Server::MainLoop, this);

    return 0;
}

int Ecg_Server::ShutDown()
{
    if (m_evHttp != NULL) {
        return -1;
    }

    evhtp_unbind_socket(m_evHttp);
    evhtp_free(m_evHttp);
    m_evHttp = NULL;

    event_base_loopbreak(m_evBase);
    event_base_free(m_evBase);

    return 0;
}

void GetCgrpMetricsCbk(evhtp_request_t *req, void *arg)
{
    int ret = METRIC_RESPONSE_OK;
    const char *reqType = NULL;

    reqType = req->uri->path->full + strlen(req->uri->path->path); /* full path include request url */
    auto metricData = Ecg::Ecg_NodeExporter::GetInstance()->NeGetMetricData(reqType);
    if (metricData.empty()) {
        ret = METRIC_NOT_IMPL;
        goto out;
    }

    evhtp_headers_add_header(req->headers_out,
                             evhtp_header_new("Content-Type", "text/plain; verion:0.0.4; charset=utf-8", 0, 0));
    evbuffer_add(req->buffer_out, metricData.c_str(), metricData.length());

out:
    evhtp_send_reply(req, ret);
}

int Ecg_Server::MainLoop()
{
    m_evBase = event_base_new();
    if (m_evBase == NULL) {
        std::cout << "Error event_base_new\n";
        return -1;
    }

    m_evHttp = evhtp_new(m_evBase, NULL);
    if (m_evHttp == NULL) {
        std::cout << "Error evhtp_new\n";
        return -1;
    }

    evhtp_set_cb(m_evHttp, METRIC_URL_HEADER, GetCgrpMetricsCbk, NULL);
    evhtp_use_dynamic_threads(m_evHttp, NULL, NULL, 0, 0, 0, NULL);
    evhtp_bind_socket(m_evHttp, METRIC_DEFAULT_IP, m_port, BACK_LOG_SIZE);

    std::cout << "Run ecg node exporter on localhost:" << m_port << std::endl;
    event_base_loop(m_evBase, 0);

    return 0;
}

}; // namespace Ecg

int main(int argc, char **argv)
{
    Ecg::Ecg_Server ecgServer(9999);
    Ecg::Ecg_NodeExporter *ecgNe = Ecg::Ecg_NodeExporter::GetInstance();

    Ecg::CPU_Metrics *cpuMetrics = 
        new Ecg::CPU_Metrics("cpu_usage", "current cpu_usage", Ecg::COUNTER);
    ecgNe->RegisterMetric(cpuMetrics);

    ecgServer.Start();

    while (1) {
        getchar();
    }

    return 0;
}