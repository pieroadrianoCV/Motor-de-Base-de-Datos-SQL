# Motor de Base de Datos - Fase 1: Índices B-Tree

Implementación en C++17 del almacenamiento e indexación de registros mediante
un Árbol B. La fase busca comparar una búsqueda indexada `O(log n)` con un
recorrido completo `O(n)`.

## Requisitos

- GCC o Clang con soporte para C++17.
- GNU Make.
- Linux, macOS o Windows mediante MinGW/WSL.

No se usan bibliotecas externas para compilar las pruebas ni el benchmark.

## Compilación y ejecución

```bash
make
./db_engine
```

## Pruebas automatizadas

```bash
make test
```

La suite incluye casos de carga desordenada de 10 000 registros, claves
duplicadas, carga vacía, validación de invariantes de balance y equivalencia
entre Index Scan y Full Table Scan.

## Benchmark

La ejecución predeterminada carga 100 000 registros y realiza 5 000 consultas:

```bash
make benchmark
```

El tamaño se puede ajustar con `ARGS="registros consultas"`:

```bash
make benchmark ARGS="20000 1000"
```

El reporte muestra tiempo de carga, tiempo de búsqueda y cantidad de accesos.
También compara el número de resultados y un checksum de los RowIDs; la
ejecución falla si los métodos discrepan. El adaptador ordenado de
`src/benchmark_main.cpp` permite ejecutar la demostración antes de integrar el
B-Tree definitivo y debe reemplazarse por `BTree::search` al integrar.

## Contrato para integrar el B-Tree

El código en `src/benchmark/` usa callbacks para no imponer nombres ni tipos al
módulo `src/index/`:

- El callback de inserción recibe un registro y devuelve `true` si se insertó.
- El validador devuelve `true` cuando se cumplen las invariantes del árbol.
- La búsqueda devuelve `LookupResult<RowId>` con el resultado y los accesos a
  páginas o nodos.

El cargador ejecuta el validador al final y, opcionalmente, cada N intentos. La
integración debe comprobar que todas las hojas estén a igual profundidad, que
cada nodo (salvo la raíz) tenga entre `t-1` y `2t-1` claves y que los hijos y
claves estén ordenados. El grado mínimo pertenece a la configuración de
`BTree`; debe ser `t >= 2` y documentarse el valor elegido por el equipo.

## Limpieza

```bash
make clean
```
