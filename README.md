# Proyecto I Bimestre - Sistemas Operativos


# Inicialización y Creación de Hilos en el Sistema Bancario de Alta Concurrencia

Aqui se explica cómo se inicializan los recursos del sistema y cómo se crean y sincronizan los hilos usando POSIX Threads (`pthread`) en el programa bancario. El objetivo es entender claramente el proceso de preparación, ejecución y finalización de los hilos que simulan cajeros operando sobre una base de datos compartida.

---

## Inicialización de Recursos

Antes de crear los hilos que realizarán las transacciones, el programa debe preparar varios elementos clave.

### **1. Inicialización de la Base de Datos**

La función `inicializar_bd()` construye un arreglo con 100 cuentas bancarias. A cada una se le asigna:

- Un número de identificación (`id`)
- Un saldo inicial de **1000 dólares**

```c
inicializar_bd();
```

Esto permite que todos los hilos trabajen sobre un estado inicial uniforme y coherente.

---
## **2. Inicialización del Semáforo**

```c
sem_init(&sem_transaccion, 0, 1);
```

Este semáforo POSIX (`sem_t`) se utiliza para proteger la **zona crítica**, es decir, las partes del código donde se modifica el saldo de las cuentas.  
Con un valor inicial de `1`, el semáforo actúa como un **candado de exclusión mutua (mutex)**:

- Solo un hilo puede entrar a la zona crítica a la vez.
- Se garantiza que las transacciones no se mezclen ni generen corrupción de datos.

---

## Creación de los Hilos (`pthread_create`)

Luego de la inicialización, el programa crea múltiples hilos, cada uno representando un cajero que realiza operaciones sobre las cuentas.

```c
for (int i = 0; i < NUM_HILOS; i++) {
    ids[i] = i;
    pthread_create(&hilos[i], NULL, cajero_thread, &ids[i]);
}
```

### **Proceso**

1. **Bucle `for`**
   - Se repite tantas veces como hilos se desean crear (`NUM_HILOS = 20`).
   - Cada vuelta crea un nuevo hilo.

2. **Asignación del ID del Hilo**
   ```c
   ids[i] = i;
   ```
   Cada hilo recibe un ID único para identificarlo en los registros.

3. **Uso del `pthread_create()`**
   ```c
   pthread_create(&hilos[i], NULL, cajero_thread, &ids[i]);
   ```
   Parámetros:
   - `&hilos[i]`: lugar donde se almacenará el identificador del hilo creado.
   - `NULL`: atributos por defecto.
   - `cajero_thread`: función que ejecutará el hilo.
   - `&ids[i]`: puntero al ID del hilo (su parámetro de entrada).

Cada hilo comienza a ejecutarse en paralelo, generando operaciones bancarias mientras compite por entrar a la zona crítica protegida por el semáforo.

---

## Finalización (`pthread_join`)

Una vez creados los hilos, el programa principal **no puede finalizar** hasta que todos los hilos hayan terminado su trabajo. Para ello se usa:

```c
for (int i = 0; i < NUM_HILOS; i++) {
    pthread_join(hilos[i], NULL);
}
```

### **Funcion del  `pthread_join`?**

- Bloquea la ejecución del hilo principal.
- Espera a que el hilo especificado termine.
- Garantiza que:
  - Ningún hilo quede ejecutándose después del cierre del programa.
  - Todas las transacciones se completen.
  - Los datos finales sean correctos.

### **Importancia del Orden**

El `join` se hace en un bucle secuencial porque:

- Cada llamada mantiene al programa principal detenido hasta que el hilo correspondiente finalice.
- El programa no pasa al reporte final hasta que **todos** los hilos hayan concluido.
---

# Sincronización y Zona Crítica
Para su correcta ejecución concurrente entre hilos, el programa utiliza semáforos POSIX de la biblioteca estándar de linux. 


```c
#include <semaphore.h>

```
Provee:

sem_init()

sem_wait()

sem_post()

sem_destroy()

La zona crítica protege a los datos que  corresponden a las operaciones sobre los recursos compartidos y no ser modificados por dos hilos al mismo tiempo, por lo que requieren una exclusión mutua.

- Base de datos de cuentas (base_datos[])

- Contadores globales (total_ops_ok, total_ops_error)

## Implementacion zona Crítica

Declarar semáforo
```c
sem_t sem_transaccion;

```
Para inicializar en el mai():

```c
sem_init(&sem_transaccion, 0, 1);

```
Crea un semáforo binario, que permite que solo un hilo entre en la zona critica.

## Protección de la zona crítica (Sewait)
Antes de modificar la base de datos, cada hilo ejecuta:
```c
sem_wait(&sem_transaccion);   // Bloquea el acceso (entra a la zona crítica)

```
Funcionamiento:
```c
sem_wait(&sem_transaccion);

/* --- ZONA CRÍTICA ---
   Acceso y modificación de saldos de las cuentas
   Actualización de contadores globales
*/
... operaciones ...

sem_post(&sem_transaccion);   // Libera el semáforo, otro hilo puede entrar

```
sem_wait() → El hilo espera hasta que el semáforo esté disponible y luego entra en la zona crítica.

sem_post() → El hilo sale de la zona crítica y despierta al siguiente hilo que esté esperando.

Si no se implementara el semáforo:
- Dos hilos podrían modificar el mismo saldo simultáneamente.

- Se generarían inconsistencias en la base de datos.

- Los contadores globales producirían condiciones de carrera.

La sincronización asegura que las transacciones sean atómicas, consistentes y libres de errores.
