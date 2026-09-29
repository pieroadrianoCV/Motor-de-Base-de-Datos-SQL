#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace test {

using TestFunction = std::function<void()>;

struct TestCase {
    std::string name;
    TestFunction function;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

class Registrar {
public:
    Registrar(std::string name, TestFunction function) {
        registry().push_back({std::move(name), std::move(function)});
    }
};

inline void fail(const char* expression, const char* file, int line,
                 const std::string& detail = {}) {
    std::ostringstream message;
    message << file << ':' << line << ": fallo `" << expression << '`';
    if (!detail.empty()) {
        message << " (" << detail << ')';
    }
    throw std::runtime_error(message.str());
}

inline int runAll() {
    std::size_t passed = 0;
    for (const auto& current : registry()) {
        try {
            current.function();
            ++passed;
            std::cout << "[OK] " << current.name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] " << current.name << ": " << error.what() << '\n';
        } catch (...) {
            std::cerr << "[FAIL] " << current.name << ": excepcion desconocida\n";
        }
    }

    std::cout << passed << '/' << registry().size() << " pruebas aprobadas\n";
    return passed == registry().size() ? 0 : 1;
}

}  // namespace test

#define TEST_CASE(name)                                                        \
    static void name();                                                        \
    static const test::Registrar registrar_##name(#name, name);                \
    static void name()

#define EXPECT_TRUE(expression)                                                \
    do {                                                                       \
        if (!(expression)) {                                                   \
            test::fail(#expression, __FILE__, __LINE__);                       \
        }                                                                      \
    } while (false)

#define EXPECT_EQ(actual, expected)                                            \
    do {                                                                       \
        const auto actual_value = (actual);                                    \
        const auto expected_value = (expected);                                \
        if (!(actual_value == expected_value)) {                               \
            std::ostringstream detail;                                         \
            detail << "obtenido=" << actual_value << ", esperado="            \
                   << expected_value;                                          \
            test::fail(#actual " == " #expected, __FILE__, __LINE__,           \
                       detail.str());                                          \
        }                                                                      \
    } while (false)

