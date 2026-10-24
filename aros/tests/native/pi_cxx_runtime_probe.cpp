/* Link and native-run probe: socket headers must not reduce POSIX visibility. */
#include <sys/socket.h>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <stdexcept>
#include <cstdio>
#if _POSIX_C_SOURCE < 200809L
#error Native libc++ needs the requested POSIX.1-2008 declarations
#endif
int main()
{
    std::vector<int> values{1, 2, 3};
    std::map<std::string, int> expected{{"sum", 6}};
    auto sum = std::make_unique<int>(values[0] + values[1] + values[2]);
    try { throw std::runtime_error("native-cxx"); }
    catch (const std::exception& error) {
        std::printf("%s %d\n", error.what(), *sum);
        return *sum == expected["sum"] ? 0 : 1;
    }
    return 2;
}
