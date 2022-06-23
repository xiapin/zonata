#include <string>
#include <vector>
#include <map>

#pragma once

namespace Ecg
{

typedef enum {
    COUNTER     = 0,
    GAUGE,
    HISTOGRAM,
    SUMMARY,

    METRIC_TYPE_BUTT,
} Metrics_Type;

inline const char *get_metric_name(Metrics_Type e)
{
    const char *metric_type_name[] = {
        "counter",
        "gauge",
        "histogram",
        "summary",
    };

    return metric_type_name[e];
}

typedef std::string (*Ecg_GetMetricData)(std::string metricName);

class Ecg_Metrics {
public:
    Ecg_Metrics(std::string url, std::string name,
                std::string help, Metrics_Type type,
                Ecg_GetMetricData dataGet) :
            m_url(url), m_metricName(name), m_helpInfo(help),
            m_metricType(type), m_GetMetricCbk(dataGet) {};
    ~Ecg_Metrics() {};

    std::string GetUrl() { return m_url; }
    std::string GetMetricName() { return m_metricName; }
    std::string GetHelp() { return m_helpInfo; }
    Metrics_Type GetMetricType() { return m_metricType; }
    std::string GetMetricData()
    {
        if (m_GetMetricCbk != NULL)
        {
            return m_GetMetricCbk(m_metricName);
        }
        return nullptr;
    }

private:
    std::string m_url; // match key word.
    std::string m_metricName;
    std::string m_helpInfo;
    Metrics_Type m_metricType;
    Ecg_GetMetricData m_GetMetricCbk;
};

class Ecg_NodeExporter {
public:
    int Init();
    int RegisterMetric(Ecg_Metrics *metric);
    int UnregisterMetric(Ecg_Metrics *metric);
    int Loop();

    Ecg_NodeExporter(unsigned port) :
        m_port(port) {}
    ~Ecg_NodeExporter() {}

private:
    std::vector<Ecg_Metrics *> m_Metrics;
    unsigned m_port;
};

}; // namespace Ecg