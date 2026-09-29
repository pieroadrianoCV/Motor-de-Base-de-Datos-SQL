#pragma once

#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace db::benchmark {

struct BulkLoadOptions {
    // Cero valida solamente al finalizar. Un valor N valida cada N inserciones.
    std::size_t validationInterval{0};
};

struct BulkLoadResult {
    std::size_t attempted{0};
    std::size_t inserted{0};
    std::size_t rejected{0};
    std::size_t validations{0};
    std::chrono::nanoseconds elapsed{0};
};

class BalanceError : public std::logic_error {
public:
    explicit BalanceError(const std::string& message) : std::logic_error(message) {}
};

// Inserter debe devolver true si el registro fue insertado y false si fue
// rechazado (por ejemplo, por clave duplicada). Validator comprueba todas las
// invariantes del indice y devuelve true si el arbol permanece balanceado.
template <typename Range, typename Inserter, typename Validator>
BulkLoadResult bulkLoad(const Range& records, Inserter insert,
                        Validator validateBalance,
                        BulkLoadOptions options = {}) {
    BulkLoadResult result;
    const auto start = std::chrono::steady_clock::now();

    const auto validate = [&]() {
        ++result.validations;
        if (!validateBalance()) {
            throw BalanceError("el indice violo sus invariantes durante la carga masiva");
        }
    };

    for (const auto& record : records) {
        ++result.attempted;
        if (insert(record)) {
            ++result.inserted;
        } else {
            ++result.rejected;
        }

        if (options.validationInterval != 0 &&
            result.attempted % options.validationInterval == 0) {
            validate();
        }
    }

    // También cubre la carga vacía y el bloque final incompleto.
    if (options.validationInterval == 0 || result.attempted == 0 ||
        result.attempted % options.validationInterval != 0) {
        validate();
    }

    result.elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - start);
    return result;
}

}  // namespace db::benchmark

