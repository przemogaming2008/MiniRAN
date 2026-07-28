#include <iostream>
#include <string>

#include "support/test_framework.h"

namespace
{

void printUsage(const char* programName)
{
    std::cerr
        << "Usage:\n"
        << "  " << programName << " [--junit <path>]\n";
}

bool parseArguments(int argc,
                    char** argv,
                    std::string& junitPath)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--junit") {
            if (i + 1 >= argc) {
                std::cerr << "ERROR: --junit requires output path\n";
                return false;
            }

            const std::string value = argv[i + 1];
            if (value.empty() || value.rfind("--", 0) == 0) {
                std::cerr << "ERROR: --junit requires valid output path\n";
                return false;
            }

            junitPath = value;
            ++i;
            continue;
        }

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            std::exit(0);
        }

        std::cerr << "ERROR: unknown argument: " << arg << "\n";
        return false;
    }

    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    std::string junitPath;

    if (!parseArguments(argc, argv, junitPath)) {
        printUsage(argv[0]);
        return 2;
    }

    return miniran::test::runAllTests(junitPath);
}