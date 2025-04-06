#include "backward.hpp"
using namespace backward;
#include "app.h"

#if !defined(_WIN32)
#define BACKWARD_HAS_BFD 1
#define BACKWARD_HAS_LIBUNWIND 1
#endif

#define print(expr) std::cout << expr << std::endl

int main(int argc, char** argv) {
    SignalHandling* sh = nullptr;
    bool crash = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]).find("crash") != std::string::npos) {
            crash = true;
        }
        if (std::string(argv[i]).find("help") != std::string::npos) {
            print("Usage: ");
            print("\t" << argv[0] << " [--crash|-crash|crash] [--help|-help|help]");
            print("\t--crash: disable crash handling");
            print("\t--help: print usage info");
            return 0;
        }
    }
    if (crash) print("crash mode enabled!");
    if (!crash) sh = new SignalHandling();
    Raster::App::Start();
    if (!crash) delete sh;
    return 0;
}