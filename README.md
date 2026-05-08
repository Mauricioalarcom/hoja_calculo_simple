# 🧾 Hoja de cálculo simple (C++ + SFML)

Aplicación de hoja de cálculo desarrollada en **C++**, con interfaz gráfica en **SFML** y un backend basado en **Matriz Dispersa**.

---

## ✨ Características

- 📈 **Grilla dinámica**: la cuadrícula se expande automáticamente en filas/columnas al navegar más allá de los límites actuales.
- 🧭 **Auto-scroll**: al moverte con flechas, `Tab` o `Enter`, la celda activa se mantiene dentro de la vista.
- 🧮 **Selección de rangos en vivo**: al escribir fórmulas como `=SUMA(` puedes seleccionar celdas con el mouse para armar rangos rápidamente.
- 🚫 **Manejo de errores**: reporta errores típicos como `#¡VALOR!`, `#¡DIV/0!` o `#¡REF!` ante sintaxis inválida o tipos incompatibles.
- 🟩 **Relleno automático**: arrastra desde el cuadradito verde en la esquina inferior derecha de la celda activa para autocompletar y adaptar referencias.

---

## 🖱️ Selección

- **Mouse**: clic para seleccionar; clic sostenido + arrastre para marcar rangos.
- **Teclado**: `Shift` + flechas para seleccionar bloques de múltiples celdas dinámicamente.

---

## ⌨️ Atajos de teclado

> Modificador principal: `Ctrl` en Windows/Linux, `Cmd` en macOS.

| Atajo | Acción |
|------|--------|
| `Cmd/Ctrl + C` | Copia datos y fórmulas del rango seleccionado al portapapeles |
| `Cmd/Ctrl + V` | Pega valores desde el portapapeles |
| `Cmd/Ctrl + Z` | Deshacer (Undo) |
| `Cmd/Ctrl + Y` o `Shift + Cmd/Ctrl + Z` | Rehacer (Redo) |
| `Cmd/Ctrl + P` | Multi-uso: si hay rango, muestra la suma temporal; si es una celda, muestra por consola interna la vista |
| `Enter` | Guarda lo editado y baja una fila (crea dinámicamente si estás en el límite) |
| `Tab` | Guarda lo editado y avanza una columna |
| `F2` | Enfoca la barra de fórmulas para editar |
| `Esc` | Cancela la edición y restaura el contenido original |
| `Supr` / `Backspace` | Elimina los valores del rango seleccionado |

---

## 🧩 Operaciones y funciones soportadas

### Fórmulas por rangos

Ejemplos:

- `=SUMA(A1:A5)`
- `=PROMEDIO(A1:B4)`
- `=MAX(Rango)`
- `=MIN(Rango)`
- `=CONTAR(Rango)`

Soporta **fila completa**, **columna completa** o **selección rectangular**.

### Visualización y eliminación

- Puedes seleccionar y eliminar cualquier rango rectangular con `Supr`.
- Puedes eliminar el contenido de columnas seleccionando desde las letras (ej: toda la columna `C`) o filas desde los números, y presionar `Supr`.

---

## 🧠 Estructura de datos: Matriz Dispersa

La lógica de celdas se implementa con una **Matriz Dispersa** basada en una cuadrícula ortogonal (listas enlazadas cruzadas). En una hoja de cálculo real, la mayoría de celdas suelen estar vacías; una matriz 2D tradicional reservaría memoria para todos esos espacios. Con esta estructura se almacenan únicamente nodos (celdas) con datos activos, optimizando la huella de memoria.

---

## ⏱️ Complejidad (resumen)

- **Insertar/Editar celda**: $O(N + M)$ en el peor caso, con $N$ y $M$ como nodos ya inicializados en la fila y columna correspondientes.
- **Consultar (lectura)**: $O(N)$, recorriendo celdas activas de la fila.
- **Operaciones por rango $n \times m$**: fuerza bruta $O(n \cdot m \cdot L)$, donde $L$ es el costo de búsqueda de celdas.
- **Memoria**: $O(k)$, con $k$ celdas efectivamente pobladas.

---

## 🛠️ Compilación y ejecución (CMake)

### Requisitos

- **CMake** (recomendado 3.10+)
- **SFML 2.x**

Instalación rápida:

- **macOS**: 
   ```bash
   brew install sfml cmake
   ```
- **Ubuntu/Debian**:
   ```bash
   sudo apt-get install cmake libsfml-dev
   ```
- **Windows**: instala SFML (vcpkg u otro) y configura correctamente include/lib/bin según tu entorno.

### Build

Desde la carpeta donde está el `CMakeLists.txt`:

```bash
cmake -S . -B build
cmake --build build
```

### Ejecutar

- **Linux/macOS**:
   ```bash
   ./build/project_aed
   ```
- **Windows**: `build\Debug\project_aed.exe` (o el binario equivalente según el generador).

> Nota: asegúrate de que la fuente `arial.ttf` esté en la misma ruta desde la que ejecutas el binario.

---

## 🧯 Troubleshooting rápido

- **No abre / crashea al iniciar**: revisa que `arial.ttf` esté junto al ejecutable (o en el directorio de ejecución actual).
- **Errores de link con SFML**: verifica que SFML esté instalada y que CMake esté encontrando los paquetes correctos.
