# hoja_calculo_simple

Una aplicación de hoja de cálculo desarrollada en C++ utilizando SFML para la interfaz gráfica y un backend de Matriz Dispersa.

## Características y Navegación

- **Grilla Dinámica**: La cuadrícula se expande automáticamente en filas y columnas si navegas más allá de los límites actuales.
- **Auto-Scroll**: La interfaz se desplaza por sí misma asegurando que la celda a la que te diriges al usar las flechitas, `Tab` o `Enter` esté siempre dentro de la vista.
- **Selección de Rangos Dinámica en Fórmulas**: Al redactar operaciones como `=SUMA(`, puedes seleccionar las celdas en vivo con el cursor del mouse, facilitando ampliamente el armado de fórmulas complejas.
- **Manejo de Errores**: Si evalúas operaciones con errores de sintaxis o celdas de texto donde se esperan números, el motor refleja errores característicos como `#¡VALOR!`, `#¡DIV/0!` o `#¡REF!`.
- **Relleno Automático**: Arrastra desde el pequeño cuadrado verde en la esquina inferior derecha de la celda activa para estirar y autocompletar fórmulas con referencias adaptadas de forma automática.

## Requisitos de Selección
- **Mouse**: Clic para seleccionar o clic sostenido y arrastre para marcar rangos.
- **Teclado**: Usa `Shift` + `Flechas de dirección` para seleccionar bloques de múltiples celdas dinámicamente.

## Comandos y Atajos de Teclado

Utiliza el modificador principal según tu sistema operativo (`Ctrl` en Windows/Linux, `Cmd` en macOS):

- **Cmd/Ctrl + C**: Copia los datos y fórmulas del rango seleccionado en el portapapeles.
- **Cmd/Ctrl + V**: Pega los valores del portapapeles.
- **Cmd/Ctrl + Z**: Deshacer (*Undo*).
- **Cmd/Ctrl + Y** (o **Shift + Cmd/Ctrl + Z**): Rehacer (*Redo*).
- **Cmd/Ctrl + P**: Atajo multi-uso. Si tienes un bloque seleccionado, muestra la suma de los valores temporales. Si es una sola celda, muestra por consola interna la vista.
- **Enter**: Guarda el valor editado y baja el cursor una fila. (Crea dinámicamente si estás en el límite).
- **Tab**: Guarda el valor y mueve el cursor una columna a la derecha.
- **F2**: Activa el foco en la barra de fórmulas para editar.
- **Esc**: Cancela cualquier edición en curso y devuelve el contenido original.
- **Supr / Backspace**: Elimina los valores del rango seleccionado.

## Operaciones y Funciones Soportadas

- **Operaciones por rangos (Fórmulas):** `=SUMA(A1:A5)`, `=PROMEDIO(A1:B4)`, `=MAX(Rango)`, `=MIN(Rango)`, `=CONTAR(Rango)`. Soportan fila completa, columna completa o selección rectangular.
- **Visualización y Eliminación:** 
  - Se puede seleccionar y eliminar cualquier rango rectangular con `Supr` o borrando la celda.
  - Puedes eliminar el contenido de columnas seleccionando a través de las letras (ej. Seleccionar toda C) o filas a través de los números y presionar `Supr`.

## Justificación de la Estructura de Datos (Matriz Dispersa)
Se ha elegido implementar la lógica de las celdas mediante una **Matriz Dispersa** conformada por una cuadrícula ortogonal (listas enlazadas cruzadas), debido a que en una hoja de cálculo real el 99% de las celdas suelen estar vacías. La instanciación de una matriz tradicional (Array 2D de NxM celdas) reservaría memoria para todos esos espacios vacíos ocasionando graves ineficiencias, mientras que este diseño solo almacena objetos de nodos (celdas) donde existen datos activos evaluables, optimizando drásticamente la huella de memoria.

## Análisis de Complejidad Temporal
- **Insertar/Editar Celda**: `O(N + M)` en el peor de los casos, siendo N y M la cantidad de nodos ya inicializados en la fila y la columna de la celda específica, puesto que debe recorrer las listas enlazadas asociadas.
- **Consultar (Lectura)**: `O(N)`. Recorre únicamente las celdas activadas en esa fila para encontrar correspondencia con la columna, o devuelve vacío.
- **Calculo de Operaciones(SUMA, PROMEDIO, etc) en Rango nxm**: Al requerir chequear cada cuadrante, la evaluación en fuerza bruta es `O(n * m * L)` donde `L` es el costo de encontrar las celdas correspondientes.
- **Espacio en Memoria**: `O(k)` donde `k` corresponde únicamente a las celdas con datos.

## Instrucciones de Compilación Paso a Paso
Este proyecto requiere **CMake** (versión recomendada 3.10+) y el gestor de paquetes/biblioteca multimedia **SFML** (2.x) instalada.

1. **Instalación de requisitos:**
   - *macOS:* `brew install sfml cmake`
   - *Ubuntu/Debian:* `sudo apt-get install cmake libsfml-dev`
   - *Windows (vcpkg u otros):* Importar correctamente o definir el dir PATH de SFML en las variables de entorno.
2. **Generar archivos Make/Proyectos:** Sitúate en la ruta raíz del repositorio donde se ubica nuestro `CMakeLists.txt` y ejecuta en la terminal:
   `cmake -S . -B build`
3. **Compilar el Proyecto:**
   `cmake --build build`
4. **Ejecutar el programa:**
   - *En Linux/Mac:* `./build/project_aed`
   - *En Windows:* `build\Debug\project_aed.exe` (o el nombre asociado).
   (Nota: Asegúrate que la fuente `arial.ttf` está presente en la misma ruta donde ejecutas el archivo binario).
