#pragma once

#include <exception>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace test {

struct Case {
    std::string name;
    std::function<void()> run;
};

inline std::vector<Case>& cases()
{
    static std::vector<Case> registered;
    return registered;
}

class Registration {
public:
    Registration(std::string name, std::function<void()> run)
    {
        cases().push_back(Case{std::move(name), std::move(run)});
    }
};

[[noreturn]] inline void fail(std::string_view expression, std::string_view file, int line)
{
    std::ostringstream message;
    message << file << ':' << line << ": expectation failed: " << expression;
    throw std::runtime_error(message.str());
}

template <typename Actual, typename Expected>
void expectEqual(const Actual& actual, const Expected& expected, std::string_view expression,
                 std::string_view file, int line)
{
    if (!(actual == expected)) {
        fail(expression, file, line);
    }
}

inline void expectContains(std::string_view actual, std::string_view expected,
                           std::string_view expression, std::string_view file, int line)
{
    if (actual.find(expected) == std::string_view::npos) {
        fail(expression, file, line);
    }
}

}

#define TEST(NAME)                                                                               \
    static void NAME();                                                                          \
    static const test::Registration NAME##_registration{#NAME, NAME};                            \
    static void NAME()

#define EXPECT_TRUE(EXPRESSION)                                                                  \
    do {                                                                                         \
        if (!(EXPRESSION)) {                                                                     \
            test::fail(#EXPRESSION, __FILE__, __LINE__);                                         \
        }                                                                                        \
    } while (false)

#define EXPECT_EQ(ACTUAL, EXPECTED)                                                              \
    test::expectEqual((ACTUAL), (EXPECTED), #ACTUAL " == " #EXPECTED, __FILE__, __LINE__)

#define EXPECT_CONTAINS(ACTUAL, EXPECTED)                                                        \
    test::expectContains((ACTUAL), (EXPECTED), #ACTUAL " contains " #EXPECTED, __FILE__,       \
                         __LINE__)
