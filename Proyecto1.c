#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// --- CONFIGURACIÓN PARA HTOP ---
#define NUM_CUENTAS 100        // Base de datos
#define NUM_HILOS 20           // Generacion de 20 Hilos 
#define OPS_POR_HILO 50        // Cada hilo hace 50 operaciones
#define DELAY_MICROSEGUNDOS 500000 // Tiempo de retardo para ver las transacciones realizadas

// --- ESTRUCTURAS ---
typedef struct {
    int id;
    double saldo;
} Cuenta;

// --- RECURSOS COMPARTIDOS ---
Cuenta base_datos[NUM_CUENTAS];
sem_t sem_transaccion;         
int total_ops_ok = 0;
int total_ops_error = 0;

// --- INICIALIZACIÓN ---
void inicializar_bd() {
    for (int i = 0; i < NUM_CUENTAS; i++) {
        base_datos[i].id = i;
        base_datos[i].saldo = 1000.0; 
    }
    printf(">> [INIT] Base de datos cargada con %d cuentas.\n", NUM_CUENTAS);
}

// --- LÓGICA DEL HILO ---
void *cajero_thread(void *arg) {
    int id_hilo = *(int *)arg;
    
    for (int i = 0; i < OPS_POR_HILO; i++) {
        
        // === EL DELAY PARA HTOP ===
        // Pausa de medio segundo. El hilo se queda "vivo" pero durmiendo.
        // Esto permite cambiar de ventana y verlo en la lista de procesos.
        usleep(DELAY_MICROSEGUNDOS);

        // Generar datos aleatorios
        int tipo = rand() % 3; 
        int cuenta_A = rand() % NUM_CUENTAS;
        int cuenta_B = rand() % NUM_CUENTAS;
        double monto = 10 + (rand() % 200);

        // --- ZONA CRÍTICA ---
        sem_wait(&sem_transaccion);

        if (tipo == 0) { // Depósito
            base_datos[cuenta_A].saldo += monto;
            printf("Hilo %02d: [+] Depósito $%3.0f a Cta %d. Nuevo: %.2f\n", 
                   id_hilo, monto, cuenta_A, base_datos[cuenta_A].saldo);
            total_ops_ok++;
        } 
        else if (tipo == 1) { // Retiro
            if (base_datos[cuenta_A].saldo >= monto) {
                base_datos[cuenta_A].saldo -= monto;
                printf("Hilo %02d: [-] Retiro   $%3.0f de Cta %d. Nuevo: %.2f\n", 
                       id_hilo, monto, cuenta_A, base_datos[cuenta_A].saldo);
                total_ops_ok++;
            } else {
                printf("Hilo %02d: [!] FONDOS INSUFICIENTES en Cta %d.\n", id_hilo, cuenta_A);
                total_ops_error++;
            }
        } 
        else { // Transferencia
            if (base_datos[cuenta_A].saldo >= monto && cuenta_A != cuenta_B) {
                base_datos[cuenta_A].saldo -= monto;
                base_datos[cuenta_B].saldo += monto;
                printf("Hilo %02d: [>] Transf.  $%3.0f de Cta %d a %d.\n", 
                       id_hilo, monto, cuenta_A, cuenta_B);
                total_ops_ok++;
            } else {
                // Fallos de transferencia no se imprimen para no ensuciar tanto el log
                total_ops_error++;
            }
        }

        sem_post(&sem_transaccion);
        // --- FIN ZONA CRÍTICA ---
    }

    pthread_exit(NULL);
}

// --- MAIN ---
int main() {
    pthread_t hilos[NUM_HILOS];
    int ids[NUM_HILOS];
    struct timespec inicio, fin;

    srand(time(NULL));
    inicializar_bd();
    
    // Inicializar Semáforo
    sem_init(&sem_transaccion, 0, 1);

    printf("\n--- INICIANDO SISTEMA BANCARIO DE ALTA CONCURRENCIA ---\n");
    printf("Configuración: %d Hilos | %d Ops/Hilo | Delay: 0.5s\n", NUM_HILOS, OPS_POR_HILO);
    printf("El sistema se ejecutará lentamente para monitoreo...\n");
    sleep(2); // Breve pausa para leer

    // 1. INICIO RELOJ
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    // 2. CREAR HILOS
    for (int i = 0; i < NUM_HILOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, cajero_thread, &ids[i]);
    }

    // 3. ESPERAR HILOS
    for (int i = 0; i < NUM_HILOS; i++) {
        pthread_join(hilos[i], NULL);
    }

    // 4. FIN RELOJ
    clock_gettime(CLOCK_MONOTONIC, &fin);

    // Resultados
    double tiempo_total = (fin.tv_sec - inicio.tv_sec) + (fin.tv_nsec - inicio.tv_nsec) / 1e9;
    int total_total = total_ops_ok + total_ops_error;

    printf("\n============================================\n");
    printf("          REPORTE FINAL DE EJECUCIÓN        \n");
    printf("============================================\n");
    printf("Estado:              FINALIZADO\n");
    printf("Tiempo Total:        %.4f segundos\n", tiempo_total);
    printf("Transacciones:       %d Totales\n", total_total);
    printf("   - Exitosas:       %d\n", total_ops_ok);
    printf("   - Fallidas:       %d\n", total_ops_error);
    printf("Sincronización:      Semáforo POSIX (sem_t)\n");
    printf("============================================\n");

    sem_destroy(&sem_transaccion);
    return 0;
}
