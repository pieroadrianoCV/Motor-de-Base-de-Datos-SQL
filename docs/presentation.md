# Guion de presentación — 12 minutos

## Arquitectura — 4 a 5 minutos

1. Mostrar el flujo `Tuple → página → RowID → B-Tree` de
   `docs/architecture.md`.
2. Explicar el formato binario y los tags de `int64`, `double` y `string`.
3. Explicar el RowID compuesto por PageID y SlotID.
4. Explicar el grado `t`: mínimo `t-1`, máximo `2t-1` y split por mediana.
5. Aclarar que el benchmark usa `t=64`, pero el menú permite cambiarlo.

## Demostración — 7 a 8 minutos

Compilar y validar antes de presentar:

```bash
make clean
make
make test
```

Ejecutar:

```bash
./db_engine
```

Secuencia sugerida:

1. Opción 1: cargar 100 000 registros con `t=64`.
2. Señalar tuplas insertadas, páginas, altura, splits y tiempo.
3. Opción 2: mostrar claves promovidas y niveles del árbol.
4. Opción 3: ejecutar 5 000 consultas.
5. Comparar tiempo y páginas lógicas del índice contra el recorrido completo.
6. Explicar que el checksum impide comparar resultados diferentes.

Como ejecución no interactiva adicional:

```bash
make benchmark ARGS="100000 5000 64"
```

## Preguntas técnicas esperables

- **¿Por qué la búsqueda es logarítmica?** Solo desciende por un hijo por nivel
  y el nodo localiza la posición mediante búsqueda binaria.
- **¿Cómo se conserva el balance?** Un hijo lleno se divide antes de descender;
  todas las hojas permanecen en la misma profundidad.
- **¿Qué ocurre con duplicados?** Se rechazan antes de crear el registro.
- **¿Qué representa I/O?** Lecturas de páginas lógicas: nodos del índice o
  páginas de datos. No pretende medir llamadas físicas del sistema operativo.
- **¿Cómo se detecta corrupción?** El serializador valida límites y el B-Tree
  comprueba orden, capacidad, enlaces, profundidad y conteo total.
