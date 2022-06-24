#include <string>
#include <vector>
#include <map>
#include <thread>
#include <evhtp.h>

#pragma once

namespace Ecg
{

#define METRIC_URL_HEADER       "/metrics/cgroup"
#define METRIC_RESPONSE_OK      200
#define METRIC_RESPONSE_FAIL    401
#define METRIC_NOT_IMPL         501
#define METRIC_DEFAULT_IP       "127.0.0.1"
#define BACK_LOG_SIZE           1024

typedef enum {
    COUNTER     = 0,
    GAUGE,
    HISTOGRAM,
    SUMMARY,

    METRIC_TYPE_BUTT,
} Metrics_Type;

inline std::string get_metric_name(Metrics_Type e)
{
    const char *metric_type_name[] = {
        "counter",
        "gauge",
        "histogram",
        "summary",
    };

    return metric_type_name[e];
}

class Ecg_Metrics {
public:
    virtual ~Ecg_Metrics() {}

    void Init(std::string name,
            std::string help, Metrics_Type type)
    {
        m_metricName = std::move(name);
        m_helpInfo = std::move(help);
        m_metricType = type;
    }
    std::string GetMetricName() { return m_metricName; }
    std::string GetHelp() { return m_helpInfo; }
    Metrics_Type GetMetricType() { return m_metricType; }

    virtual std::vector<std::string> GetMetricsData() = 0;

    std::string m_metricName;
    std::string m_helpInfo;
    Metrics_Type m_metricType;
};

class Ecg_Server {
public:
    Ecg_Server(int port) : m_port(port) {}
    ~Ecg_Server() {}

    int Start();
    int ShutDown();

private:
    // void GetCgrpMetricsCbk(evhtp_request_t *req, void *arg);
    int MainLoop();

    int m_port;
    evbase_t *m_evBase;
    evhtp_t *m_evHttp;
    std::thread *m_thrd;
};

class Ecg_NodeExporter {
public:
    int RegisterMetric(Ecg_Metrics *metric);
    int UnregisterMetric(Ecg_Metrics *metric);
    std::string NeGetMetricData(std::string metricName);

    Ecg_NodeExporter() {}
    ~Ecg_NodeExporter() {}

    static Ecg_NodeExporter *GetInstance()
    {
        static Ecg_NodeExporter *self = NULL;
        if (self == NULL) {
            self = new Ecg_NodeExporter();
        }
        return self;
    }

private:
    std::vector<Ecg_Metrics *> m_Metrics;
    unsigned m_port;
};

}; // namespace Ecg