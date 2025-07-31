# shell

### Búsqueda en $PATH

#### Syscall `execve()`

La syscall `execve()` ejecuta el programa especificado en el *pathname*, lo que causa que el proceso actual sea reemplazado por un nuevo programa. Esto implica que el programa que estaba corriendo en ese proceso es descartado y el nuevo programa es cargado en la memoria.

Es importante destacar que **no se crea un nuevo proceso**. Específicamente:

- Toda la memoria asociada al programa anterior (instrucciones, datos, pila, etc.) es liberada, y el nuevo programa ocupa esa posición.
- La virtualización de los procesos asegura que el nuevo programa se ejecute de manera aislada y controlada dentro del mismo proceso que antes ejecutaba el programa original.


#### Familia de Wrappers `exec()`

Todas las funciones de `exec()` proporcionadas por la librería estándar de C están construidas sobre la syscall `execve()`, pero se diferencian en cómo se especifican el nombre del programa, la lista de argumentos y el entorno del nuevo programa.

1. **Nombre del programa**:
   - La mayoría de las funciones `exec()` esperan un *pathname* (ruta) para especificar el nuevo programa. Sin embargo, `execlp()` y `execvp()` permiten especificar el programa solo usando su nombre, que luego se busca en las rutas definidas por la variable de entorno `PATH`.

2. **Argumentos**:
   - Las funciones `execle()`, `execlp()` y `execl()` permiten pasar los argumentos como una lista dentro de la llamada. La lista de argumentos debe terminar con un puntero `NULL` para marcar el final de la lista.


3. **Entorno del programa**:
   - Las funciones `execve()` y `execle()` permiten especificar explícitamente el entorno para el nuevo programa a través de un array `envp[]`.
   - Las otras funciones utilizan el entorno actual del proceso padre.

Por lo tanto, la diferencia que podemos encontrar es que, mientras la syscall `execve()` reemplaza el programa en ejecución de un proceso por uno nuevo, la familia de funciones `exec()` utiliza `execve()` para implementarse y facilitar su uso, variando en los requerimientos de los argumentos que reciben.

#### Posibles errores

Sí, la llamada a `exec()` puede fallar por varias razones, las cuales se pueden hallar en la página 2 del manual de `execve()`. Los errores más comunes son:

- `EACCES`: El archivo o un directorio en su ruta no permite acceso, o no tiene permisos de ejecución.
- `E2BIG`: Los argumentos de la lista o el entorno son demasiado largos.
- `ENOENT`: El archivo especificado no existe.
- `ENOMEM`: No hay suficiente memoria para ejecutar el programa.

Cuando falla `exec()`, no se reemplaza el proceso actual y retorna un valor de error (-1), y `errno` es configurado para identificar ese error; luego el proceso que llamó a `exec()` continúa su ejecución.


### Procesos en segundo plano


El uso de señales es esencial porque permite que la shell, como proceso padre en este caso, reciba notificaciones inmediatas y asíncronas sobre eventos como la terminación de procesos hijos. 
Cuando un proceso en segundo plano finaliza, el kernel envía una señal SIGCHLD, lo que permite a la shell actuar de inmediato sin bloquearse. Esto es clave para evitar la acumulación de procesos en estado "zombie", ya que el handler de la señal utiliza la syscall **waitpid()** con el flag **WNOHANG** para liberar los recursos del proceso terminado sin interrumpir la ejecución normal de la shell. 
Cuando un comando se ejecuta en segundo plano, la shell crea un proceso hijo y devuelve el prompt de inmediato, permitiendo al usuario seguir ejecutando otros comandos. 
Al terminar el proceso en segundo plano, el handler de **SIGCHLD** toma el estado del proceso con **waitpid()** y libera los recursos sin interferir en la interacción del usuario con la shell,lo que garantiza que la shell siga operando sin bloquearse.


---

### Flujo estándar
 

#### 2>&1

El operador `2>&1` se utiliza para redirigir la salida de error estándar (stderr, descriptor 2) hacia donde está redirigida la salida estándar (stdout, descriptor 1). En este caso, el `&` indica que se está refiriendo a un file descriptor y no a un archivo con nombre "1".

##### Partes de 2>&1

- **`2>`**: Redirige la salida de stderr (descriptor de archivo 2) a un archivo o descriptor especificado. Puede ser consola o archivo, dependiendo de lo definido para stdout.
- **`&1`**: Aquí, `&` indica que lo que sigue es un descriptor de archivo y no un nombre de archivo. Se redirige stderr hacia stdout (descriptor de archivo 1).

### Ejemplos

Es muy importante aclarar que en bash,las redirecciones son procesadas en el orden del que aparecen ,de izquierda a derecha.

Por lo tanto es importante demostrar estos dos ejemplos que ilustran esta informacion.
#### Ejemplo 1

````bash
$ ls -C /home /noexiste >out.txt 2>&1


- En out.txt se almacena el resultado de ls -c/home

- Con 2>&1, los errores se redirigen al mismo lugar que stdout, que en este caso también es out.txt. Los errores referentes al intento de listar el directorio /noexiste también se guardan en out.txt

````
#### Ejemplo 2


```bash

$ ls -C /home /noexiste  2>&1 >out.txt 

- En este caso el comportamiento cambia,y los errores de ls /noexiste pasan a imprimirse por consola
- Esto ocurre porque el file descriptor 1 tiene asociado la consola en ese momento. Los errores de búsqueda del directorio no existente se imprimen en la consola, mientras que los resultados de ls /home se guardan en out.txt.


````
---

### Tuberías múltiples


referencia pipelines de man bash 1


Dentro de Bash, se utiliza `|` (pipe) para comunicar procesos. El exit code de la operación del pipeline tiene un comportamiento particular:

* El código de salida del pipe es el código de salida del último comando de la cadena. Si algo falla pero el último comando se ejecuta bien, el código de salida de todo el pipe será el del último comando.Esto sucede dado que los procesos se ejecutan en paralelo y no va a haber problema para interrumpise entre si.

#### Ejemplo 1: Todo Funciona Bien

```bash
echo "Sistemas operativos" > out.txt
cat out.txt | grep "Sistemas" | head -n 1
echo "Exit code: $?"

En este caso todo funciona de manera correcta dentro del pipe y tenemos la siguiente salida.
##### Salida
Sistemas operativos
Exit code: 0
````
#### Ejemplo 2: Falla el primer comando

```bash
cat non_existent_file.txt | grep "algo" | head -n 1
echo "Exit code: $?"


cat no halla el archivo ,pero como 'head' funciona ,el exit code sera 0
#####Salida
cat: non_existent_file.txt: No existe el archivo o el directorio
Exit code: 0
````
#### Bonus Track: uso de set -o pipefail

El comando `set -o pipefail` en Bash modifica el comportamiento del código de salida de los pipelines. 
Por defecto, el valor de retorno de un pipeline es el código de salida del último comando en la cadena. Sin embargo, al habilitar `pipefail`, el valor de retorno se convierte en el último exit code distinto de `0` de los comandos en el pipeline, o `0` si todos los comandos se ejecutaron correctamente.

Por lo tanto con el ejemplo 2
````
set -o pipefail
cat non_existent_file.txt | grep "algo" | head -n 1
echo "Exit code: $?"

##### Salida

cat: non_existent_file.txt: No existe el archivo o el directorio
Exit code: 1


````
---

### Variables de entorno temporarias


Es necesario setear las variables de entorno luego de la llamada a `fork` porque:

- Al hacer `fork()`, se crea un nuevo proceso hijo que es una copia del proceso padre. 
- Si seteamos las variables de entorno antes del `fork`, tanto el proceso padre como el hijo compartirán las mismas variables de entorno.
- Por lo tanto, para que el proceso hijo tenga sus propias variables de entorno,primero se realiza el fork() y luego dentro del proceso hijo se configuran las variables de entorno respectivas .
- La configuracion de las variables de entorno del proceso padre no se ven modificadas con el cambio dentro del entorno dentro del proceso hijo.


#### Comportamiento de las Variables de Entorno en Wrappers de `exec()`

En algunos wrappers de `exec()`, se puede pasar un tercer argumento con nuevas variables de entorno para la ejecución de un proceso. 
Es importante tener en cuenta que:

- Las variables en este arreglo reemplazarán las variables de entorno existentes del nuevo proceso.
- Si una variable no está definida en el arreglo, mantendrá su valor actual y no se borrará.

#### Posible Implementación

Para lograr el comportamiento de `setenv()`, pensamos en la siguiente implementación:

1. **Realizar un `fork()`** para crear un nuevo proceso.
2. **En el proceso hijo**, crear un arreglo con las nuevas variables de entorno o tomarlo si es provisto de la funcion mediante parametro.
3. **Pasar este arreglo** en la llamada al wrapper de `exec()` que necesite este tercer argumento.

Esta implementación asegura que el nuevo proceso tenga las variables de entorno adecuadas, permitiendo la modificación sin afectar al proceso padre.


---

### Pseudo-variables

#### $0

- Contiene el nombre del script o el comando que se está ejecutando.
- Ejemplo:

````
	bash

$ echo "El nombre del script es: $0"

SALIDA

- El nombre del script es: /usr/bin/bash
````
#### $$

- Se utiliza para identificar y realizar un seguimiento del proceo de la shell
- Ejemplo
````
echo "El PID  de la shell es : $$"

SALIDA
- El PID  de la shell es : 19069
````

#### $USER

- Contiene el nombre del usuario actualmente conectado a la shell.
-  Ejemplo

````
echo "El usuario actual es: $USER"

SALIDA

- El usuario actual es: sebastian2703
````

---

### Comandos built-in

**`pwd`** podría implementarse como un comando externo sin necesidad de ser un built-in, ya que solo accede al sistema de archivos para determinar en qué directorio se está trabajando y no modifica el estado de la shell ni su entorno. 
Esto se diferencia con `cd`, que necesita modificar el estado de la shell para cambiar entre los distintos directorios.

Aunque `pwd` se podría hacer como un comando externo, se utiliza como built-in por varias razones:

-  No requiere la creación de un nuevo proceso, lo que lo hace más eficiente.
-  Como built-in, puede acceder directamente al estado de la shell, lo que es necesario para su funcionalidad.

---

### Historial

---
