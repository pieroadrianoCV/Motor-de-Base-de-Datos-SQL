#include <iostream>

using namespace std;

void mostrarMenu() {
    cout << "\n=== MOTOR DE BASE DE DATOS ===" << endl;
    cout << "1. Carga e insercion masiva de datos (Bulk Load)" << endl;
    cout << "2. Mostrar visualizacion y log del Node Splitting" << endl;
    cout << "3. Ejecutar comparativa: Index Scan vs Full Table Scan" << endl;
    cout << "4. Salir" << endl;
    cout << "=======================================" << endl;
    cout << "Seleccione una opcion: ";
}

int main() {
    int opcion;
    bool salir = false;

    while (!salir) {
        mostrarMenu();
        cin >> opcion;

        switch (opcion) {
            case 1:
                cout << "\n[Log] Iniciando carga masiva de datos..." << endl;
                break;
            case 2:
                cout << "\n[Log] Visualizando crecimiento del arbol B..." << endl;
                break;
            case 3:
                cout << "\n[Log] Comparando metricas de tiempo e I/O de paginas..." << endl;
                break;
            case 4:
                salir = true;
                break;
            default:
                cout << "\nOpcion no valida." << endl;
                break;
        }
    }

    return 0;
}