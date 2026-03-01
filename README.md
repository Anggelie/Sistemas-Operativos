# CC3064 – Laboratorio 02

Curso: Sistemas Operativos
Autor: Anggelie Velásquez
Sistema utilizado: Ubuntu Linux (16 CPUs lógicos)

---

# Entorno de ejecución

Se verificó que el sistema cuenta con múltiples procesadores ejecutando:

```bash
lscpu | egrep 'CPU\(s\)|Core|Thread|Model name'
nproc
```

Salida relevante:

```
CPU(s): 16
Core(s) per socket: 8
Thread(s) per core: 2
```

La máquina virtual presentó Kernel Panic recurrente, por lo que los experimentos se realizaron en Ubuntu nativo manteniendo evidencia de multiprocesamiento.

---

# Ejercicio 1 – fork() y crecimiento exponencial

## Programas desarrollados

1. Programa con cuatro llamadas consecutivas a `fork()`.
2. Programa con `fork()` dentro de un ciclo `for` de cuatro iteraciones.

Compilación y ejecución:

```bash
gcc e1_fork4.c -o e1_fork4
gcc e1_forfork.c -o e1_forfork
./e1_fork4
./e1_forfork
```

## Resultados

Ambos programas generan:

* 2⁴ = 16 procesos en total
* 15 procesos nuevos creados

## Explicación

Aunque uno de los programas utiliza cuatro llamadas directas y el otro un ciclo `for`, en ambos casos se ejecutan cuatro llamadas a `fork()`. Cada llamada a `fork()` duplica el proceso actual, produciendo crecimiento exponencial:

* Después de 1 fork → 2 procesos
* Después de 2 forks → 4 procesos
* Después de 3 forks → 8 procesos
* Después de 4 forks → 16 procesos

---

# Ejercicio 2 – Medición de tiempo con clock()

## Programa secuencial

Tres ciclos `for` consecutivos de un millón de iteraciones cada uno.

Compilación:

```bash
gcc e2_seq.c -o e2_seq
./e2_seq
```

Resultados obtenidos (ticks):

| Ejecución | Ticks |
| --------- | ----- |
| 1         | 2261  |
| 2         | 2223  |
| 3         | 2566  |
| 4         | 2316  |
| 5         | 2594  |

---

## Programa concurrente (forks anidados)

Compilación:

```bash
gcc e2_conc.c -o e2_conc
./e2_conc
```

Resultados obtenidos (ticks):

| Ejecución | Ticks |
| --------- | ----- |
| 1         | 177   |
| 2         | 229   |
| 3         | 166   |
| 4         | 220   |
| 5         | 354   |

---

## Comparación

* El programa con `fork()` introduce overhead adicional por creación de procesos y llamadas a `wait()`.
* `clock()` mide tiempo de CPU del proceso que invoca la función, por lo que no siempre refleja el trabajo total realizado por todos los procesos.
* La concurrencia no es equivalente a paralelismo.
* La planificación del sistema operativo influye en los tiempos medidos.

---

# Ejercicio 3 – Cambios de contexto

Instalación del paquete requerido:

```bash
sudo apt-get install sysstat
```

Ejecución para monitoreo:

```bash
pidstat -w 1
```

## Definiciones

* Cambio de contexto voluntario: ocurre cuando el proceso cede voluntariamente la CPU (por ejemplo, por I/O o `wait()`).
* Cambio de contexto involuntario: ocurre cuando el scheduler interrumpe al proceso (por quantum o competencia de CPU).

## Observaciones

* Al interactuar con la interfaz gráfica o el teclado, aumentan los cambios voluntarios debido a operaciones de entrada/salida.
* En programas con ciclos intensivos de CPU, predominan los cambios involuntarios debido a la preempción del scheduler.
* En programas con `fork()` aparecen múltiples procesos, cada uno con su propio patrón de cambios de contexto.

---

# Ejercicio 4 – Procesos Zombie y Huérfanos

## Proceso Zombie

Compilación y ejecución:

```bash
gcc e4_zombie.c -o e4_zombie
./e4_zombie
```

En otra terminal:

```bash
ps -ael | grep e4_zombie
```

Se observa un proceso en estado `Z` y marcado como `<defunct>`.

### Explicación

La letra `Z` indica que el proceso está en estado Zombie. El proceso hijo ya terminó, pero el padre no ha llamado a `wait()`, por lo que el sistema mantiene su entrada en la tabla de procesos hasta que el padre recoja su estado o termine.

---

## Proceso Huérfano

Versión modificada del programa donde el hijo realiza un conteo prolongado.

Durante la ejecución:

```bash
ps -ael | grep e4_zombie
kill -9 <PID_padre>
```

### Resultado

* El proceso hijo continúa ejecutándose.
* El PPID cambia a 1.

### Explicación

Cuando el padre termina abruptamente, el proceso hijo queda huérfano y es adoptado por el proceso con PID 1 (systemd o init en Ubuntu).

---

# Ejercicio 5 – Comunicación entre procesos (IPC)

## Compilación

```bash
gcc ipc.c -o ipc
gcc ipcRunner.c -o ipcRunner
./ipcRunner
```

## Descripción del funcionamiento

* Se crea o abre un espacio de memoria compartida usando `shm_open`.
* Se envía el file descriptor a la otra instancia mediante un socket UNIX.
* El file descriptor recibido no se utiliza para `mmap`; cada instancia abre su propia referencia.
* Se realiza un `fork()` para crear el proceso hijo.
* El padre comunica al hijo mediante un pipe.
* El hijo escribe los caracteres recibidos en la memoria compartida.
* El padre espera al hijo y luego imprime el contenido final de la memoria compartida.

---

# Preguntas teóricas

## Memoria compartida vs archivo de texto

* Memoria compartida: comunicación rápida en RAM, sin I/O de disco.
* Archivo de texto: requiere I/O, manejo de locking y puede introducir latencia e inconsistencias.

## ¿Por qué no usar el file descriptor recibido para mmap?

Cada instancia debe abrir su propia referencia al objeto de memoria compartida para controlar permisos y estado. El descriptor recibido solo demuestra la comunicación entre procesos.

## ¿Se puede enviar el output de un programa ejecutado con exec mediante un pipe?

Sí. La shell:

1. Crea un pipe.
2. Ejecuta `fork()`.
3. Redirige `stdout` del primer proceso al pipe con `dup2`.
4. Redirige `stdin` del segundo proceso desde el pipe.
5. Ejecuta ambos programas con `exec`.

Ejemplo:

```bash
ls | less
```

## ¿Cómo detectar si ya existe un objeto de memoria compartida?

```c
shm_open(name, O_CREAT | O_EXCL, ...)
```

Si retorna -1 y `errno == EEXIST`, el objeto ya existía.

## ¿Qué sucede si se ejecuta shm_unlink mientras hay procesos usándolo?

Se elimina el nombre del objeto, pero la memoria continúa disponible mientras existan procesos que lo tengan abierto o mapeado. Se libera definitivamente cuando el último proceso lo cierre.

## ¿Cómo liberar manualmente un objeto de memoria compartida?

```bash
ls /dev/shm
rm /dev/shm/<nombre>
```

O ejecutando un programa que llame a:

```c
shm_unlink(SHM_NAME);
```

## Tiempo aproximado de fork()

La creación de un proceso con `fork()` suele tomar desde microsegundos hasta algunos milisegundos, dependiendo del sistema. El uso de `usleep(50000)` introduce una pausa de 50 ms y ayuda a evitar condiciones de carrera entre instancias concurrentes.

---

# Conclusión

Este laboratorio permitió analizar:

* El crecimiento exponencial generado por `fork()`.
* Diferencias entre concurrencia y paralelismo.
* Medición de tiempo de CPU con `clock()`.
* Cambios de contexto voluntarios e involuntarios.
* Comportamiento de procesos zombie y huérfanos.
* Comunicación entre procesos utilizando memoria compartida, pipes y mecanismos de IPC en Linux.

