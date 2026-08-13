#pragma once
/**
 * @implements [SRS-EP-15] env gate, port, enclose log line, inbound type filter
 */

#include <cctype>
#include <cstdlib>
#include <string>

namespace epaper {
namespace debuglog {

inline std::string lowerCopy(const char *v)
{
    std::string s = v ? v : "";
    for (char &c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/** EPAPER_DEBUG_LOG in {1,true,on,yes} (case-insensitive). Default off. */
inline bool debugLogEnvOn(const char *value)
{
    const std::string s = lowerCopy(value);
    return s == "1" || s == "true" || s == "on" || s == "yes";
}

/** EPAPER_DEBUG_PORT else INFINI_DEBUG_PORT else 9878. */
inline int debugLogPort(const char *epaperPort, const char *infiniPort)
{
    if (epaperPort && *epaperPort) {
        const int p = std::atoi(epaperPort);
        if (p > 0 && p < 65536)
            return p;
    }
    if (infiniPort && *infiniPort) {
        const int p = std::atoi(infiniPort);
        if (p > 0 && p < 65536)
            return p;
    }
    return 9878;
}

inline bool isDebugControlType(const std::string &type)
{
    return type == "debug_request" || type == "debug_start" || type == "debug_stop";
}

inline bool isDocumentTypeOnDebugPort(const std::string &type)
{
    return type == "viewport" || type == "doc_change" || type == "doc_load"
        || type == "doc_snapshot" || type == "hello" || type == "stroke_begin"
        || type == "stroke_point" || type == "stroke_end" || type == "drain_ack"
        || type == "queue_empty" || type == "load_ack" || type == "region_refresh"
        || type == "pickables" || type == "tool_intent";
}

/**
 * One [enclose] line after ingestStrokeAtPenUp returns.
 * Does not change recognizer behaviour.
 */
inline std::string formatEncloseLog(const std::string &armed, const std::string &kind,
                                   const std::string &reason, const std::string &smartGroupId)
{
    std::string outcome;
    std::string guard = "none";
    if (kind == "Created") {
        outcome = "created";
    } else if (kind == "Skipped" || reason == "pen_armed" || reason == "too_few_samples") {
        outcome = "not_evaluated";
    } else {
        outcome = "stayed_ink";
    }
    if (reason == "too_small")
        guard = "size";
    else if (reason == "no_content")
        guard = "content";
    else if (reason == "already_grouped")
        guard = "already_grouped";

    std::string line = "[enclose] armed=" + armed + " outcome=" + outcome;
    if (outcome == "created" && !smartGroupId.empty())
        line += " id=" + smartGroupId;
    if (outcome == "stayed_ink")
        line += " guard=" + guard + " captured=0";
    else if (outcome == "not_evaluated")
        line += " captured=0";
    return line;
}

} // namespace debuglog
} // namespace epaper
