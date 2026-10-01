# Arquitectura de la Fase 1

## Flujo de almacenamiento e indexación

```text
Tuple
  │ TupleSerializer::serialize
  ▼
PageManager ── asigna PageID + SlotID ──► RowID
  │                                      │
  │ almacena bytes                       │ BTree::insert(clave, RowID)
  ▼                                      ▼
Página lógica                        B-Tree balanceado
  ▲                                      │
  └──── RecordManager::get(RowID) ◄──────┘ BTree::search(clave)
```

`loadIndexedTuples` coordina este flujo y comprueba `BTree::validate()` durante
la carga. Una clave duplicada se rechaza antes de reservar un slot, evitando
registros huérfanos.

## Páginas, slots y RowIDs

`PageManager` agrupa los registros en páginas lógicas de 128 slots por defecto.
Los slots almacenan bytes, no objetos `Tuple`; la deserialización ocurre cuando
se recupera o escanea un registro.

El RowID es un entero de 64 bits:

```text
63                       32 31                        0
+--------------------------+--------------------------+
|      PageID (32 bits)    |    SlotID + 1 (32 bits) |
+--------------------------+--------------------------+
```

El valor cero se reserva como `INVALID_ROW_ID`. Los RowIDs no se reutilizan
después de borrar un registro, evitando que una referencia antigua apunte a una
tupla diferente.

## Formato binario de tuplas

Cada tupla comienza con un contador de campos `uint32`. Cada valor lleva un tag
de un byte y su payload:

| Tag | Tipo | Payload |
|---|---|---|
| 0 | `int64_t` | 8 bytes |
| 1 | `double` | 8 bytes IEEE-754 |
| 2 | `string` | longitud `uint32` seguida de bytes UTF-8 |

Los enteros se codifican en little-endian. El lector rechaza buffers truncados,
tags desconocidos y bytes sobrantes.

## B-Tree y grado mínimo

Se implementa un B-Tree clásico. Para un grado mínimo `t`:

- cada nodo no raíz contiene entre `t-1` y `2t-1` claves;
- un nodo interno contiene una cantidad de hijos igual a claves más uno;
- todas las hojas permanecen a la misma profundidad;
- al llenarse un hijo, su mediana asciende al padre y el nodo se divide;
- la búsqueda visita una única ruta desde la raíz, con complejidad `O(log n)`.

El benchmark usa `t=64` por defecto: hasta 127 claves y 128 hijos por nodo. El
valor es configurable para demostrar el efecto del grado sobre altura y splits.

## Observabilidad y métricas

Cada split produce un `SplitEvent` con clave promovida, ocupación de ambos
nodos y altura actual. La demostración conserva esos eventos y presenta el
árbol por niveles.

Para las métricas:

- Index Scan cuenta cada nodo del B-Tree visitado como una página lógica;
- Full Table Scan cuenta cada página de datos que debe inspeccionar;
- ambos métodos deben devolver igual cantidad de resultados y checksum de
  RowIDs, o el benchmark termina con error.

## Persistencia

`DatabaseFile` almacena una imagen portable con esta estructura:

```text
magic "EDADB001"
version uint32
grado t uint64
cantidad de tuplas uint64
  └─ tamaño uint64 + bytes de TupleSerializer (repetido)
checksum FNV-1a uint64
```

La escritura se realiza primero sobre `archivo.tmp` y luego se renombra, para
no dejar una base parcialmente escrita si ocurre un error. Al abrir se validan
firma, versión, metadatos, tamaños, tuplas, datos sobrantes y checksum.

Se persisten datos y configuración, no punteros ni memoria del árbol. Al iniciar
una nueva instancia, las tuplas vuelven a páginas y el B-Tree se reconstruye con
el mismo grado. Esto mantiene el formato independiente de direcciones de
memoria y de detalles internos de `std::vector` o `unique_ptr`.

## Componentes

```text
src/storage/     páginas, RowIDs, registros y serialización
src/index/       nodos, búsqueda, inserción, split y validación
src/benchmark/   carga masiva y comparación de scans
src/demo/        flujo de demostración y reportes
tests/           pruebas unitarias, integración, volumen y casos límite
```
