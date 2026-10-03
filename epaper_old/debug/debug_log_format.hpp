/**
 * @implements [SRS-EP-15] env gate, port, ink/enclose log lines, inbound type filter
 */

#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

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

/** Ordinary pen ingest — never an [enclose] line. */
inline std::string formatInkLog(const std::string &inkId)
{
    return "[ink] id=" + inkId;
}

/**
 * One [enclose] line after ink_box enclose ingest returns.
 * Only call when the latched tool was ink_box.
 */
inline std::string formatEncloseLog(const std::string &kind, const std::string &reason,
                                   const std::string &smartGroupId,
                                   const std::vector<std::string> &childIds = {})
{
    std::string outcome;
    std::string guard = "none";
    if (kind == "Created") {
        outcome = "created";
    } else {
        outcome = "stayed_ink";
    }
    if (reason == "too_small")
        guard = "size";
    else if (reason == "no_content")
        guard = "content";
    else if (reason == "already_grouped")
        guard = "already_grouped";

    std::string line = "[enclose] armed=ink_box outcome=" + outcome;
    if (outcome == "created") {
        if (!smartGroupId.empty())
            line += " id=" + smartGroupId;
        line += " children=[";
        for (size_t i = 0; i < childIds.size(); ++i) {
            if (i)
                line += ',';
            line += childIds[i];
        }
        line += ']';
        const int captured = childIds.size() > 1 ? int(childIds.size()) - 1 : 0;
        line += " captured=" + std::to_string(captured);
    } else {
        line += " guard=" + guard + " captured=0";
    }
    return line;
}

/** @implements [SRS-EP-10] one [recog] line per pen-up (ADR-0022) */
inline std::string formatRecogLog(const std::string &outcome, const std::string &guard,
                                 const std::string &encloseWhy = {}, const std::string &extra = {})
{
    std::string g = guard.empty() ? "none" : guard;
    std::string line = "[recog] outcome=" + outcome + " guard=" + g;
    if (!extra.empty())
        line += " " + extra;
    if (!encloseWhy.empty())
        line += " " + encloseWhy;
    return line;
}

inline std::string formatConnLog(const std::string &diag, const std::string &reason)
{
    std::string line = "[conn] " + diag;
    if (!reason.empty())
        line += " fail=" + reason;
    return line;
}

} // namespace debuglog
} // namespace epaper
