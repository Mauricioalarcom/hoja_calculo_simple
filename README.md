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

*Nota:* Puedes escribir valores en las celdas usando la barra de función superior y modificar las referencias dinámicamente.
