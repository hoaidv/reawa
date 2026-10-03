#include "rasterize_probe.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace epaper {
namespace rasterprobe {
namespace {

int g_enabled = -1; // -1 env, 0 off, 1 on
std::string g_lastLog;

const char *logPath()
{
    if (const char *p = std::getenv("EPAPER_RASTER_LOG")) {
        if (p[0])
            return p;
    }
    return "/tmp/epaper-raster.log";
}

void appendFile(const char *line)
{
    const int fd = ::open(logPath(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
        return;
    const std::size_t n = std::strlen(line);
    ::write(fd, line, n);
    ::close(fd);
}

} // namespace

bool enabled()
{
    if (g_enabled >= 0)
        return g_enabled == 1;
    if (const char *off = std::getenv("EPAPER_RASTER")) {
        if (off[0] == '0' || off[0] == 'n' || off[0] == 'N' || off[0] == 'f' || off[0] == 'F') {
            g_enabled = 0;
            return false;
        }
    }
    g_enabled = 1;
    return true;
}

void logRaster(const Record &r)
{
    if (!enabled())
        return;
    const std::string line = formatRasterLine(r);
    g_lastLog = line;
    appendFile(line.c_str());
    std::fprintf(stderr, "%s", line.c_str());
}

void resetForTest()
{
    g_enabled = -1;
    g_lastLog.clear();
}

void setEnabledForTest(bool on)
{
    g_enabled = on ? 1 : 0;
}

std::string takeLastLogForTest()
{
    std::string out = g_lastLog;
    g_lastLog.clear();
    return out;
}

} // namespace rasterprobe
} // namespace epaper
