#include "support/test_harness.h"

#include <exception>
#include <iostream>

int main()
{
    int failures{};
    for (const auto& testCase : test::cases()) {
        try {
            testCase.run();
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL " << testCase.name << ": " << error.what() << '\n';
        } catch (...) {
            ++failures;
            std::cerr << "FAIL " << testCase.name << ": unknown exception\n";
        }
    }

    if (failures == 0) {
        std::cout << "PASS " << test::cases().size() << " tests\n";
        return 0;
    }

    std::cerr << "FAILED " << failures << " of " << test::cases().size() << " tests\n";
    return 1;
}
