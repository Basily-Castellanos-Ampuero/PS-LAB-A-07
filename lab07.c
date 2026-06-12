/*
 * lab07.c - Laboratorio 07: Control de Flujo Excepcional
 * Asignatura : Programación de Sistemas - UNSA
 * Descripción: 10 ejercicios sobre excepciones, procesos y señales en C
 * Compilar   : gcc -o lab07 lab07.c
 * Ejecutar   : ./lab07
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>

/* ─────────────────────────────────────────────
   Prototipos
   ───────────────────────────────────────────── */
void ej1_errno(void);
void ej2_syscalls(void);
void ej3_fork_pid(void);
void ej4_arbol_procesos(void);
void ej5_wait(void);
void ej6_exec(void);
void ej7_zombi(void);
void ej8_sigint(void);
void ej9_sigusr1(void);
void ej10_alarm(void);
void separador(const char *titulo);

/* ─────────────────────────────────────────────
   Manejadores de señales (globales)
   ───────────────────────────────────────────── */

/* Ejercicio 8 – SIGINT */
void manejador_sigint(int sig) {
    printf("\n[EJ8] Señal SIGINT (%d) capturada. Continuando ejecución...\n", sig);
}

/* Ejercicio 9 – SIGUSR1 */
void manejador_sigusr1(int sig) {
    printf("[EJ9] Proceso hijo recibió SIGUSR1 (%d) del padre.\n", sig);
}

/* Ejercicio 10 – SIGALRM */
void manejador_alarm(int sig) {
    printf("[EJ10] ¡Alarma activada! Señal SIGALRM (%d) recibida después de 10 segundos.\n", sig);
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 1 – Manejo básico de errores con errno
   ═══════════════════════════════════════════════════════ */
void ej1_errno(void) {
    separador("EJ1 - Manejo básico de errores con errno");

    FILE *fp = fopen("archivo_inexistente_lab07.txt", "r");
    if (fp == NULL) {
        /* perror() imprime el mensaje del sistema prefijado por la cadena dada */
        perror("perror()");
        /* strerror() convierte errno en una cadena legible */
        printf("strerror(): %s\n", strerror(errno));
        printf("Código errno: %d\n", errno);
    } else {
        printf("Archivo abierto (inesperado).\n");
        fclose(fp);
    }
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 2 – Exploración de llamadas al sistema
   ═══════════════════════════════════════════════════════ */
void ej2_syscalls(void) {
    separador("EJ2 - Exploración de llamadas al sistema");

    const char *nombre = "lab07_temp.txt";

    /* Crear el archivo temporal con contenido de prueba */
    FILE *fw = fopen(nombre, "w");
    if (fw == NULL) { perror("fopen escritura"); return; }
    fprintf(fw, "Contenido de prueba para el Ejercicio 2.\n");
    fclose(fw);

    /* Abrir, leer y cerrar */
    FILE *fr = fopen(nombre, "r");
    if (fr == NULL) { perror("fopen lectura"); return; }

    printf("Contenido de '%s':\n", nombre);
    char linea[256];
    while (fgets(linea, sizeof(linea), fr) != NULL) {
        printf("  %s", linea);
    }
    fclose(fr);
    remove(nombre);   /* limpieza */

    printf("\nPara analizar llamadas al sistema, ejecuta:\n");
    printf("  strace ./lab07 2>&1 | grep -E 'open|read|close|write'\n");
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 3 – Creación de procesos con fork()
   ═══════════════════════════════════════════════════════ */
void ej3_fork_pid(void) {
    separador("EJ3 - Creación de procesos con fork()");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        /* Proceso hijo */
        printf("[HIJO ] PID propio : %d\n", getpid());
        printf("[HIJO ] PID padre  : %d\n", getppid());
        exit(EXIT_SUCCESS);
    } else {
        /* Proceso padre */
        printf("[PADRE] PID propio : %d\n", getpid());
        printf("[PADRE] PID del hijo creado: %d\n", pid);
        wait(NULL);   /* espera al hijo */
    }
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 4 – Árbol simple de procesos
   ═══════════════════════════════════════════════════════ */
void ej4_arbol_procesos(void) {
    separador("EJ4 - Árbol simple de procesos");

    pid_t h1, h2;

    h1 = fork();
    if (h1 < 0) { perror("fork hijo1"); return; }

    if (h1 == 0) {
        /* Hijo 1 */
        printf("[HIJO 1] PID=%d  |  PPID=%d\n", getpid(), getppid());
        exit(EXIT_SUCCESS);
    }

    h2 = fork();
    if (h2 < 0) { perror("fork hijo2"); return; }

    if (h2 == 0) {
        /* Hijo 2 */
        printf("[HIJO 2] PID=%d  |  PPID=%d\n", getpid(), getppid());
        exit(EXIT_SUCCESS);
    }

    /* Padre espera a ambos hijos */
    waitpid(h1, NULL, 0);
    waitpid(h2, NULL, 0);
    printf("[PADRE ] PID=%d  |  Hijos: %d y %d finalizados.\n",
           getpid(), h1, h2);
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 5 – Uso de wait()
   ═══════════════════════════════════════════════════════ */
void ej5_wait(void) {
    separador("EJ5 - Uso de wait()");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        printf("[HIJO ] Durmiendo 5 segundos...\n");
        sleep(5);
        printf("[HIJO ] Terminando con código 42.\n");
        exit(42);
    } else {
        int estado;
        printf("[PADRE] Esperando al hijo (PID=%d)...\n", pid);
        pid_t terminado = wait(&estado);

        if (WIFEXITED(estado)) {
            printf("[PADRE] Hijo %d terminó. Código de salida: %d\n",
                   terminado, WEXITSTATUS(estado));
        }
    }
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 6 – Ejecución de programas externos con exec()
   ═══════════════════════════════════════════════════════ */
void ej6_exec(void) {
    separador("EJ6 - Ejecución de programas externos con exec()");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        printf("[HIJO ] Ejecutando 'ls -l' con execvp():\n\n");
        /* execvp busca el ejecutable en PATH */
        char *args[] = {"ls", "-l", NULL};
        execvp("ls", args);
        /* Si execvp retorna, hubo error */
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("\n[PADRE] Hijo terminó la ejecución de ls -l.\n");
    }
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 7 – Simulación de proceso zombi
   ═══════════════════════════════════════════════════════ */
void ej7_zombi(void) {
    separador("EJ7 - Simulación de proceso zombi");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        printf("[HIJO ] PID=%d terminando de inmediato.\n", getpid());
        exit(EXIT_SUCCESS);          /* hijo termina: queda en estado zombi */
    } else {
        printf("[PADRE] PID=%d  |  El hijo (PID=%d) ha terminado.\n", getpid(), pid);
        printf("[PADRE] El padre NO llama a wait() por 8 segundos.\n");
        printf("[PADRE] Puedes verificar el zombi con: ps aux | grep Z\n");
        sleep(8);
        printf("[PADRE] Terminando (el zombi será recogido por init/systemd).\n");
        /* Al terminar el padre sin wait(), el hijo zombi es adoptado
           y limpiado por el proceso init (PID=1). */
    }
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 8 – Manejo de SIGINT
   ═══════════════════════════════════════════════════════ */
void ej8_sigint(void) {
    separador("EJ8 - Manejo de SIGINT (Ctrl+C)");

    signal(SIGINT, manejador_sigint);

    printf("Presiona Ctrl+C para enviar SIGINT. El programa continuará.\n");
    printf("Presiona Enter para salir de este ejercicio.\n");

    /* Bucle simple: espera input del usuario */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    /* Restaurar comportamiento por defecto antes de volver al menú */
    signal(SIGINT, SIG_DFL);
    printf("[EJ8] Señal restaurada a comportamiento por defecto.\n");
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 9 – Comunicación mediante señales (SIGUSR1)
   ═══════════════════════════════════════════════════════ */
void ej9_sigusr1(void) {
    separador("EJ9 - Comunicación mediante señales (SIGUSR1)");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        /* Hijo: registra manejador y espera la señal */
        signal(SIGUSR1, manejador_sigusr1);
        printf("[HIJO ] PID=%d esperando SIGUSR1...\n", getpid());
        pause();   /* suspende hasta recibir cualquier señal */
        printf("[HIJO ] Continuando después de la señal. Terminando.\n");
        exit(EXIT_SUCCESS);
    } else {
        /* Padre: espera un momento y envía la señal */
        sleep(1);
        printf("[PADRE] Enviando SIGUSR1 al hijo (PID=%d)...\n", pid);
        kill(pid, SIGUSR1);
        wait(NULL);
        printf("[PADRE] Hijo finalizó correctamente.\n");
    }
}

/* ═══════════════════════════════════════════════════════
   EJERCICIO 10 – Temporizadores con alarm()
   ═══════════════════════════════════════════════════════ */
void ej10_alarm(void) {
    separador("EJ10 - Temporizadores con alarm()");

    signal(SIGALRM, manejador_alarm);

    printf("Alarma programada para 10 segundos. Esperando...\n");
    alarm(10);
    pause();   /* suspende el proceso hasta que llegue SIGALRM */

    printf("[EJ10] Programa continúa tras la alarma.\n");
    /* Restaurar señal */
    signal(SIGALRM, SIG_DFL);
}

/* ─────────────────────────────────────────────
   Utilidad: separador visual
   ───────────────────────────────────────────── */
void separador(const char *titulo) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("  %s\n", titulo);
    printf("╚══════════════════════════════════════════════════╝\n");
}

/* ─────────────────────────────────────────────
   MENÚ PRINCIPAL
   ───────────────────────────────────────────── */
int main(void) {
    int opcion;

    do {
        printf("\n");
        printf("╔══════════════════════════════════════════════════════╗\n");
        printf("║   LAB 07 – Control de Flujo Excepcional (ECF)       ║\n");
        printf("║   Programación de Sistemas – UNSA 2026              ║\n");
        printf("╠══════════════════════════════════════════════════════╣\n");
        printf("║  1.  Manejo de errores con errno                    ║\n");
        printf("║  2.  Exploración de llamadas al sistema             ║\n");
        printf("║  3.  Creación de procesos con fork()                ║\n");
        printf("║  4.  Árbol simple de procesos                       ║\n");
        printf("║  5.  Uso de wait()                                  ║\n");
        printf("║  6.  Ejecución externa con exec()                   ║\n");
        printf("║  7.  Simulación de proceso zombi                    ║\n");
        printf("║  8.  Manejo de SIGINT (Ctrl+C)                      ║\n");
        printf("║  9.  Comunicación mediante señales (SIGUSR1)        ║\n");
        printf("║  10. Temporizadores con alarm()                     ║\n");
        printf("║  0.  Salir                                          ║\n");
        printf("╚══════════════════════════════════════════════════════╝\n");
        printf("Selecciona un ejercicio: ");

        if (scanf("%d", &opcion) != 1) {
            /* limpiar buffer en caso de entrada inválida */
            int c; while ((c = getchar()) != '\n' && c != EOF);
            opcion = -1;
        }
        /* consumir el '\n' restante */
        int c; while ((c = getchar()) != '\n' && c != EOF);

        switch (opcion) {
            case 1:  ej1_errno();         break;
            case 2:  ej2_syscalls();      break;
            case 3:  ej3_fork_pid();      break;
            case 4:  ej4_arbol_procesos();break;
            case 5:  ej5_wait();          break;
            case 6:  ej6_exec();          break;
            case 7:  ej7_zombi();         break;
            case 8:  ej8_sigint();        break;
            case 9:  ej9_sigusr1();       break;
            case 10: ej10_alarm();        break;
            case 0:  printf("Saliendo...\n"); break;
            default: printf("Opción inválida. Intenta de nuevo.\n"); break;
        }

    } while (opcion != 0);

    return EXIT_SUCCESS;
}
