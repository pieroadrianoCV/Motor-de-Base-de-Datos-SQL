#include "benchmark/ScanBenchmark.hpp"
#include "demo/DemoEngine.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <variant>

namespace {

void showMenu()
{
  std::cout << "\n=== MOTOR DE BASE DE DATOS — FASE 1 ===\n"
            << "1. Carga e insercion masiva\n"
            << "2. Mostrar splits y estructura del B-Tree\n"
            << "3. Comparar Index Scan vs Full Table Scan\n"
            << "4. Buscar registro por clave\n"
            << "5. Salir\n"
            << "========================================\n"
            << "Seleccione una opcion: ";
}

std::int64_t readKey(const std::string& prompt)
{
  std::cout << prompt;
  std::int64_t value = 0;
  if (!(std::cin >> value)) {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    throw std::invalid_argument("se esperaba una clave entera");
  }
  return value;
}

void printTuple(const storage::Tuple& tuple)
{
  std::cout << '[';
  for (std::size_t position = 0; position < tuple.size(); ++position) {
    if (position != 0) std::cout << ", ";
    std::visit([](const auto& value) { std::cout << value; }, tuple[position]);
  }
  std::cout << ']';
}

std::size_t readPositive(const std::string& prompt)
{
  std::cout << prompt;
  std::size_t value = 0;
  if (!(std::cin >> value) || value == 0) {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    throw std::invalid_argument("se esperaba un entero positivo");
  }
  return value;
}

void printMetric(const char* name, const db::benchmark::ScanMetrics& metric)
{
  std::cout << std::left << std::setw(20) << name << std::right
            << std::setw(14) << db::benchmark::milliseconds(metric.elapsed)
            << std::setw(16) << metric.pageAccesses
            << std::setw(14) << metric.found << '\n';
}

}  // namespace

int main(int argc, char** argv)
{
  if (argc > 2) {
    std::cerr << "Uso: " << argv[0] << " [archivo.bin]\n";
    return 1;
  }

  db::demo::DemoEngine demo;
  const std::filesystem::path dataFile = argc == 2 ? argv[1] : "";
  if (!dataFile.empty()) {
    if (std::filesystem::exists(dataFile)) {
      try {
        const auto summary = demo.loadFile(dataFile, 256);
        std::cout << "Base de datos recuperada de " << dataFile << "\n"
                  << "Registros: " << summary.load.inserted
                  << ", paginas: " << summary.pages
                  << ", altura: " << summary.height << "\n";
      } catch (const std::exception& error) {
        std::cerr << "No se pudo abrir la base de datos: " << error.what()
                  << '\n';
        return 1;
      }
    } else {
      std::cout << "Se creara una nueva base de datos en " << dataFile << "\n";
    }
  }

  bool running = true;
  while (running) {
    showMenu();
    int option = 0;
    if (!(std::cin >> option)) {
      std::cout << "\nEntrada finalizada.\n";
      break;
    }

    try {
      switch (option) {
      case 1: {
        const auto count = readPositive("Cantidad de registros: ");
        const auto degree = readPositive("Grado minimo t (>= 2): ");
        const auto tuples = db::demo::DemoEngine::makeWorkload(count);
        const auto summary = demo.load(tuples, degree, 256);
        if (!dataFile.empty()) {
          demo.saveFile(dataFile);
        }
        std::cout << "\n[Carga completada]\n"
                  << "Tuplas insertadas: " << summary.load.inserted << '\n'
                  << "Tuplas rechazadas: " << summary.load.rejected << '\n'
                  << "Paginas de datos: " << summary.pages << '\n'
                  << "Altura del B-Tree: " << summary.height << '\n'
                  << "Splits: " << summary.splits << '\n'
                  << "Tiempo: " << std::fixed << std::setprecision(3)
                  << db::benchmark::milliseconds(summary.load.elapsed)
                  << " ms\n";
        if (!dataFile.empty()) {
          std::cout << "Guardado en: " << dataFile << '\n';
        }
        break;
      }
      case 2:
        std::cout << '\n' << demo.splitReport() << demo.treeReport();
        break;
      case 3: {
        const auto queries = readPositive("Cantidad de consultas: ");
        const auto result = demo.benchmarkScans(queries);
        std::cout << '\n' << std::left << std::setw(20) << "Metodo" << std::right
                  << std::setw(14) << "Tiempo (ms)" << std::setw(16)
                  << "Paginas I/O" << std::setw(14) << "Encontrados" << '\n';
        printMetric("Index Scan", result.indexScan);
        printMetric("Full Table Scan", result.fullTableScan);
        break;
      }
      case 4: {
        const auto key = readKey("Clave a buscar: ");
        const auto result = demo.find(key);
        if (!result.found) {
          std::cout << "Clave " << key << " no encontrada. Nodos visitados: "
                    << result.indexPageAccesses << '\n';
          break;
        }
        std::cout << "Registro encontrado\n"
                  << "RowID: " << result.rowId << '\n'
                  << "PageID: " << result.pageId << '\n'
                  << "SlotID: " << result.slotId << '\n'
                  << "Nodos del indice visitados: "
                  << result.indexPageAccesses << "\nTupla: ";
        printTuple(*result.tuple);
        std::cout << '\n';
        break;
      }
      case 5:
        running = false;
        break;
      default:
        std::cout << "Opcion no valida.\n";
      }
    } catch (const std::exception& error) {
      std::cout << "Error: " << error.what() << '\n';
    }
  }
  return 0;
}
