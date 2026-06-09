#include "Clock.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace core {

std::string now() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%A, %b %d, %Y  |  %I:%M:%S %p");
    return ss.str();
}

} // namespace core
