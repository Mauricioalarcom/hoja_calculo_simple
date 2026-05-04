#include "GUI.h"
#include <sstream>
#include <cctype>
#include <algorithm>

// ─────────────────────────────────────────────────────────────
GUI::GUI(SparseMatrix& matrix)
    : sheet(matrix),
      window(sf::VideoMode(OFFSET_X + COLS * CELL_W + 20, OFFSET_Y + ROWS * CELL_H + 120),
             "Hoja de Calculo - Sparse Matrix")
{
    window.setFramerateLimit(60);
    if (!font.loadFromFile("arial.ttf"))
        throw std::runtime_error("No se pudo cargar arial.ttf");
}

// ─── RUN LOOP ─────────────────────────────────────────────────
void GUI::run() {
    while (window.isOpen()) {
        handleEvents();
        window.clear(sf::Color(245, 245, 245));
        drawGrid();
        drawTopPanel();
        drawBottomPanel();
        window.display();
    }
}

// ─── EVENTOS ──────────────────────────────────────────────────
void GUI::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {

        if (event.type == sf::Event::Closed)
            window.close();

        if (event.type == sf::Event::KeyPressed) {
            // Tab para cambiar foco entre campos
            if (event.key.code == sf::Keyboard::Tab) {
                if      (focusCell)  { focusCell=false; focusValue=true;  focusRange=false; }
                else if (focusValue) { focusCell=false; focusValue=false; focusRange=true;  }
                else                 { focusCell=true;  focusValue=false; focusRange=false; }
            }
            // Backspace y Delete
            if (event.key.code == sf::Keyboard::BackSpace) {
                if (focusCell  && !inputCell.empty())  inputCell.pop_back();
                else if (focusValue && !inputValue.empty()) inputValue.pop_back();
                else if (focusRange && !inputRange.empty()) inputRange.pop_back();
                else if (focusValue && inputValue.empty()) {
                    executeDelete(); // Si presiona borrar y el campo ya está vacío, borra la celda/rango de la grilla
                }
            }

            if (event.key.code == sf::Keyboard::Delete) {
                executeDelete();
                inputValue = "";
            }

            // Moverse con las flechas del teclado
            if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::Down ||
                event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Right) {

                bool shiftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

                if (selectedStartRow == -1 || selectedStartCol == -1) {
                    selectedStartRow = 0; selectedStartCol = 0;
                    selectedEndRow = 0; selectedEndCol = 0;
                } else {
                    int& tr = shiftPressed ? selectedEndRow : selectedStartRow;
                    int& tc = shiftPressed ? selectedEndCol : selectedStartCol;

                    if (event.key.code == sf::Keyboard::Up && tr > 0) tr--;
                    if (event.key.code == sf::Keyboard::Down && tr < ROWS - 1) tr++;
                    if (event.key.code == sf::Keyboard::Left && tc > 0) tc--;
                    if (event.key.code == sf::Keyboard::Right && tc < COLS - 1) tc++;

                    if (!shiftPressed) {
                        selectedEndRow = selectedStartRow;
                        selectedEndCol = selectedStartCol;
                    }
                }

                std::string startCell = std::string(1, 'A' + std::min(selectedStartCol, selectedEndCol)) + std::to_string(std::min(selectedStartRow, selectedEndRow) + 1);
                std::string endCell = std::string(1, 'A' + std::max(selectedStartCol, selectedEndCol)) + std::to_string(std::max(selectedStartRow, selectedEndRow) + 1);
                inputRange = startCell + ":" + endCell;

                // inputCell se mantiene en su ancla o en la celda normal
                inputCell = std::string(1, 'A' + selectedStartCol) + std::to_string(selectedStartRow + 1);

                focusCell = false;
                focusValue = true;
                focusRange = false;
                inputValue = "";
            }

            // Tecla Enter para insertar el valor en la celda
            if (event.key.code == sf::Keyboard::Enter) {
                if (inputValue.empty()) {
                    executeDelete(); // Si da Enter con el campo vacío, se borra el contenido
                } else {
                    executeInsert();
                }

                // Mover a la celda de abajo
                if (selectedStartRow != -1 && selectedStartCol != -1) {
                    if (selectedStartRow < ROWS - 1) {
                        selectedStartRow++;
                    }
                    selectedEndRow = selectedStartRow;
                    selectedEndCol = selectedStartCol;

                    inputCell = std::string(1, 'A' + selectedStartCol) + std::to_string(selectedStartRow + 1);
                    inputRange = inputCell + ":" + inputCell;

                    focusCell = false;
                    focusValue = true;
                    focusRange = false;
                    inputValue = ""; // Limpiar valor para la nueva celda
                }
            }

            // Comandos con Ctrl + P (o Cmd + P en Mac)
            if (event.key.code == sf::Keyboard::P &&
               (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::LSystem) || sf::Keyboard::isKeyPressed(sf::Keyboard::RSystem))) {
                // Remove buttons functionality and trigger command palette or action
                // For demonstration, let's say it triggers "Consultar" if a cell is selected.
                if (selectedStartRow != -1 && selectedStartCol != -1 && selectedStartRow == selectedEndRow && selectedStartCol == selectedEndCol) {
                    inputCell = std::string(1, 'A' + selectedStartCol) + std::to_string(selectedStartRow + 1);
                    executeQuery();
                } else if (selectedStartRow != -1 && selectedStartCol != -1) {
                    inputRange = std::string(1, 'A' + std::min(selectedStartCol, selectedEndCol)) + std::to_string(std::min(selectedStartRow, selectedEndRow) + 1) + ":" +
                                 std::string(1, 'A' + std::max(selectedStartCol, selectedEndCol)) + std::to_string(std::max(selectedStartRow, selectedEndRow) + 1);
                    executeAggregation("SUMA"); // Default action for range
                }
            }
        }

        if (event.type == sf::Event::TextEntered) {
            char c = static_cast<char>(event.text.unicode);
            if (c >= 32 && c < 127) { // caracteres imprimibles
                if (focusCell)  inputCell  += c;
                if (focusValue) inputValue += c;
                if (focusRange) inputRange += c;
            }
        }

        // Clic en botones
        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            int x = pos.x, y = pos.y;

            // Foco en campos de texto (panel superior)
            if (y > 10 && y < 40) {
                if (x > 70  && x < 230) { focusCell=true;  focusValue=false; focusRange=false; }
                if (x > 290 && x < 450) { focusCell=false; focusValue=true;  focusRange=false; }
                if (x > 510 && x < 670) { focusCell=false; focusValue=false; focusRange=true;  }
            }

            // Clic en la grilla para seleccionar celdas
            if (y > OFFSET_Y && y < window.getSize().y - 40 && x > OFFSET_X && x < window.getSize().x) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    isDragging = true;
                    // Agregar scroll a las coordenadas del mouse
                    selectedStartCol = (x - OFFSET_X + scrollX) / CELL_W;
                    selectedStartRow = (y - OFFSET_Y + scrollY) / CELL_H;
                    selectedEndCol = selectedStartCol;
                    selectedEndRow = selectedStartRow;

                    if (selectedStartCol >= COLS) selectedStartCol = COLS - 1;
                    if (selectedStartRow >= ROWS) selectedStartRow = ROWS - 1;
                    if (selectedStartCol < 0) selectedStartCol = 0;
                    if (selectedStartRow < 0) selectedStartRow = 0;

                    // Update inputCell or inputRange
                    std::string cLabel = "";
                    int tempC = selectedStartCol;
                    while (tempC >= 0) { cLabel = char('A' + (tempC % 26)) + cLabel; tempC = tempC / 26 - 1; }

                    inputCell = cLabel + std::to_string(selectedStartRow + 1);
                    inputRange = inputCell + ":" + inputCell;

                    // Cambiar el foco al campo de valor para poder escribir directamente
                    focusCell = false;
                    focusValue = true;
                    focusRange = false;
                    inputValue = ""; // Opcional: limpiar el valor anterior al elegir nueva celda
                }
            }
        }

        if (event.type == sf::Event::MouseMoved) {
            if (isDragging) {
                int x = event.mouseMove.x;
                int y = event.mouseMove.y;
                if (y > OFFSET_Y && y < window.getSize().y - 40 && x > OFFSET_X && x < window.getSize().x) {
                    int endCol = (x - OFFSET_X + scrollX) / CELL_W;
                    int endRow = (y - OFFSET_Y + scrollY) / CELL_H;

                    if (endCol >= COLS) endCol = COLS - 1;
                    if (endRow >= ROWS) endRow = ROWS - 1;
                    if (endCol < 0) endCol = 0;
                    if (endRow < 0) endRow = 0;

                    selectedEndCol = endCol;
                    selectedEndRow = endRow;

                    std::string startCell = "";
                    int tempC1 = std::min(selectedStartCol, selectedEndCol);
                    while (tempC1 >= 0) { startCell = char('A' + (tempC1 % 26)) + startCell; tempC1 = tempC1 / 26 - 1; }
                    startCell += std::to_string(std::min(selectedStartRow, selectedEndRow) + 1);

                    std::string endCellStr = "";
                    int tempC2 = std::max(selectedStartCol, selectedEndCol);
                    while (tempC2 >= 0) { endCellStr = char('A' + (tempC2 % 26)) + endCellStr; tempC2 = tempC2 / 26 - 1; }
                    endCellStr += std::to_string(std::max(selectedStartRow, selectedEndRow) + 1);

                    inputRange = startCell + ":" + endCellStr;
                }
            }
        }

        if (event.type == sf::Event::MouseButtonReleased) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                isDragging = false;
            }
        }

        if (event.type == sf::Event::MouseWheelScrolled) {
            if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                // Determine if Shift is pressed for horizontal scroll
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                    scrollX -= event.mouseWheelScroll.delta * 20;
                } else {
                    scrollY -= event.mouseWheelScroll.delta * 20;
                }
            } else if (event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel) {
                scrollX -= event.mouseWheelScroll.delta * 20;
            }

            // Clamp scrolling
            float maxScrollX = std::max(0.0f, (float)(COLS * CELL_W) - (window.getSize().x - OFFSET_X));
            float maxScrollY = std::max(0.0f, (float)(ROWS * CELL_H) - (window.getSize().y - OFFSET_Y - 40));

            if (scrollX < 0) scrollX = 0;
            if (scrollX > maxScrollX) scrollX = maxScrollX;
            if (scrollY < 0) scrollY = 0;
            if (scrollY > maxScrollY) scrollY = maxScrollY;
        }
    }
}

// ─── RENDER ───────────────────────────────────────────────────
void GUI::render() { /* llamado desde run() */ }

void GUI::drawGrid() {
    auto nodes = sheet.getAllNodes();

    // Construir mapa rápido de celdas ocupadas
    std::map<std::pair<int,int>, std::string> occupied;
    for (Node* n : nodes)
        occupied[{n->row, n->col}] = n->value;

    // Header de columnas (A, B, C, ...)
    for (int c = 0; c < COLS; c++) {
        std::string label = "";
        int tempC = c;
        while (tempC >= 0) {
            label = char('A' + (tempC % 26)) + label;
            tempC = tempC / 26 - 1;
        }

        float x = OFFSET_X + c * CELL_W - scrollX;

        // Solo dibujar si está al menos parcialmente visible en la pantalla
        if (x + CELL_W > OFFSET_X && x < window.getSize().x) {
            auto rect = makeRect(x, OFFSET_Y - CELL_H, CELL_W, CELL_H,
                                 sf::Color(200, 200, 220));
            window.draw(rect);
            window.draw(makeText(label, x + CELL_W/2 - 5, OFFSET_Y - CELL_H + 7));
        }
    }

    // Header de filas (1, 2, 3, ...)
    for (int r = 0; r < ROWS; r++) {
        float y = OFFSET_Y + r * CELL_H - scrollY;
        if (y + CELL_H > OFFSET_Y && y < window.getSize().y - 40) { // 40 es el tamaño aproximado del panel inferior
            auto rect = makeRect(0, y, OFFSET_X, CELL_H, sf::Color(200, 200, 220));
            window.draw(rect);
            window.draw(makeText(std::to_string(r + 1), 5, y + 7));
        }
    }

    // Celdas
    for (int r = 0; r < ROWS; r++) {
        float y = OFFSET_Y + r * CELL_H - scrollY;
        if (y + CELL_H <= OFFSET_Y || y >= window.getSize().y - 40) continue; // Culling vertical

        for (int c = 0; c < COLS; c++) {
            float x = OFFSET_X + c * CELL_W - scrollX;
            if (x + CELL_W <= OFFSET_X || x >= window.getSize().x) continue; // Culling horizontal

            auto key = std::make_pair(r, c);

            bool hasValue = occupied.count(key);
            sf::Color fill = hasValue ? sf::Color(220, 240, 220) : sf::Color::White;

            // Highlight selected cells
            if (selectedStartRow != -1 && selectedStartCol != -1) {
                int minCol = std::min(selectedStartCol, selectedEndCol);
                int maxCol = std::max(selectedStartCol, selectedEndCol);
                int minRow = std::min(selectedStartRow, selectedEndRow);
                int maxRow = std::max(selectedStartRow, selectedEndRow);

                if (r >= minRow && r <= maxRow && c >= minCol && c <= maxCol) {
                    fill = sf::Color(180, 200, 255); // Selection color
                }
            }

            auto rect = makeRect(x, y, CELL_W, CELL_H, fill);
            window.draw(rect);

            if (hasValue) {
                std::string val = occupied[key];
                if (val.size() > 10) val = val.substr(0, 9) + "~";
                window.draw(makeText(val, x + 4, y + 7, 12));
            }
        }
    }

    // Tapamos el área de los headers para que no se superpongan las celdas y filas al scrollear
    auto topCover = makeRect(0, 0, OFFSET_X, OFFSET_Y, sf::Color(230, 230, 240), sf::Color::Transparent);
    window.draw(topCover);
}

void GUI::drawTopPanel() {
    // Fondo panel
    auto bg = makeRect(0, 0, window.getSize().x, OFFSET_Y - CELL_H,
                       sf::Color(230, 230, 240), sf::Color::Transparent);
    window.draw(bg);

    // Labels
    window.draw(makeText("Celda:", 5, 15));
    window.draw(makeText("Valor:", 265, 15));
    window.draw(makeText("Rango:", 480, 15));

    // Campos de texto
    auto cellBox = makeRect(65, 10, 160, 28,
                            focusCell ? sf::Color(255,255,200) : sf::Color::White);
    window.draw(cellBox);
    window.draw(makeText(inputCell + (focusCell ? "|" : ""), 70, 15));

    auto valBox = makeRect(325, 10, 160, 28,
                           focusValue ? sf::Color(255,255,200) : sf::Color::White);
    window.draw(valBox);
    window.draw(makeText(inputValue + (focusValue ? "|" : ""), 330, 15));

    auto rangeBox = makeRect(530, 10, 160, 28,
                             focusRange ? sf::Color(255,255,200) : sf::Color::White);
    window.draw(rangeBox);
    window.draw(makeText(inputRange + (focusRange ? "|" : ""), 535, 15));

    // Remove old buttons code here since requested to convert to commands
}

void GUI::drawBottomPanel() {
    float y = OFFSET_Y + ROWS * CELL_H + 5;
    window.draw(makeRect(0, y, window.getSize().x, 40,
                         sf::Color(230,230,240), sf::Color::Transparent));
    window.draw(makeText(">> " + statusMsg, 10, y + 10, 14, sf::Color(50,50,150)));
}

// ─── OPERACIONES ──────────────────────────────────────────────
void GUI::executeInsert() {
    int r, c;
    if (!parseCell(inputCell, r, c)) { statusMsg = "Celda invalida. Usa formato A1"; return; }
    if (inputValue.empty())          { statusMsg = "Ingresa un valor"; return; }

    std::string valueToInsert = inputValue;

    // Evaluador básico de fórmulas (ej: =SUMA(A1:B2) o suma simple)
    if (inputValue.length() > 0 && inputValue[0] == '=') {
        std::string formula = inputValue.substr(1);
        // Convertir a mayúsculas
        std::transform(formula.begin(), formula.end(), formula.begin(), ::toupper);

        if (formula.find("SUMA(") == 0 || formula.find("SUM(") == 0 ||
            formula.find("PROMEDIO(") == 0 || formula.find("PROM(") == 0 ||
            formula.find("MAX(") == 0 || formula.find("MIN(") == 0) {

            size_t start = formula.find('(') + 1;
            size_t end = formula.find(')');
            if (start != std::string::npos && end != std::string::npos) {
                std::string rangeStr = formula.substr(start, end - start);
                int r1, c1, r2, c2;

                double result = 0.0;
                bool isRange = parseRange(rangeStr, r1, c1, r2, c2);
                bool isCell = false;
                if (!isRange) {
                    isCell = parseCell(rangeStr, r1, c1);
                    r2 = r1; c2 = c1;
                }

                if (isRange || isCell) {
                    if (formula.find("SUM") == 0) {
                        result = sheet.sumRange(r1, c1, r2, c2);
                    } else if (formula.find("PROM") == 0) {
                        result = sheet.avgRange(r1, c1, r2, c2);
                    } else if (formula.find("MAX") == 0) {
                        result = sheet.maxRange(r1, c1, r2, c2);
                    } else if (formula.find("MIN") == 0) {
                        result = sheet.minRange(r1, c1, r2, c2);
                    }

                    valueToInsert = formatDouble(result);
                } else {
                    // Evaluar suma de celdas simples separadas por + (ej: =A1+B2)
                    statusMsg = "Error en formato de fórmula"; return;
                }
            }
        } else if (formula.find('+') != std::string::npos) {
            // Soporte básico para =A1+B2+C3
            std::stringstream ss(formula);
            std::string cellRef;
            double totalSum = 0.0;
            bool valid = true;

            while (std::getline(ss, cellRef, '+')) {
                int rr, cc;
                if (parseCell(cellRef, rr, cc)) {
                    std::string valStr = sheet.query(rr, cc);
                    try {
                        totalSum += std::stod(valStr);
                    } catch (...) {
                        // Ignorar si no es número
                    }
                } else {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                valueToInsert = formatDouble(totalSum);
            } else {
                statusMsg = "Error sumando celdas: " + formula; return;
            }
        }
    }

    sheet.insert(r, c, valueToInsert);
    statusMsg = "Insertado " + inputCell + " = " + valueToInsert;
}

void GUI::executeDelete() {
    // Si hay rango, eliminar rango; si hay celda, eliminar celda
    if (!inputRange.empty()) {
        int r1,c1,r2,c2;
        if (!parseRange(inputRange, r1,c1,r2,c2)) { statusMsg = "Rango invalido. Usa A1:C4"; return; }
        sheet.deleteRange(r1,c1,r2,c2);
        statusMsg = "Rango " + inputRange + " eliminado";
    } else {
        int r, c;
        if (!parseCell(inputCell, r, c)) { statusMsg = "Celda invalida. Usa formato A1"; return; }
        sheet.deleteCell(r, c);
        statusMsg = "Celda " + inputCell + " eliminada";
    }
}

void GUI::executeQuery() {
    int r, c;
    if (!parseCell(inputCell, r, c)) { statusMsg = "Celda invalida. Usa formato A1"; return; }
    std::string val = sheet.query(r, c);
    statusMsg = val.empty() ? inputCell + " esta vacia" : inputCell + " = " + val;
}

void GUI::executeAggregation(const std::string& op) {
    double result = 0.0;

    // Si hay rango, operar sobre rango
    if (!inputRange.empty()) {
        int r1,c1,r2,c2;
        if (!parseRange(inputRange, r1,c1,r2,c2)) { statusMsg = "Rango invalido"; return; }
        if      (op == "SUMA")     result = sheet.sumRange(r1,c1,r2,c2);
        else if (op == "PROMEDIO") result = sheet.avgRange(r1,c1,r2,c2);
        else if (op == "MAX")      result = sheet.maxRange(r1,c1,r2,c2);
        else if (op == "MIN")      result = sheet.minRange(r1,c1,r2,c2);
        statusMsg = op + "(" + inputRange + ") = " + formatDouble(result);
    }
    // Si no hay rango, intentar con celda/fila/columna
    else {
        int r, c;
        if (!parseCell(inputCell, r, c)) { statusMsg = "Ingresa un rango (ej. A1:C4)"; return; }
        if      (op == "SUMA")     result = sheet.sumRow(r);
        else if (op == "PROMEDIO") result = sheet.avgRange(r,0,r,COLS-1);
        else if (op == "MAX")      result = sheet.maxRange(r,0,r,COLS-1);
        else if (op == "MIN")      result = sheet.minRange(r,0,r,COLS-1);
        statusMsg = op + "(fila " + std::to_string(r+1) + ") = " + formatDouble(result);
    }
}

// ─── PARSERS ──────────────────────────────────────────────────
// "B3" → col=1, row=2
bool GUI::parseCell(const std::string& ref, int& row, int& col) {
    if (ref.size() < 2) return false;
    char colChar = std::toupper(ref[0]);
    if (colChar < 'A' || colChar > 'Z') return false;
    col = colChar - 'A';
    try {
        row = std::stoi(ref.substr(1)) - 1; // 1-indexed → 0-indexed
    } catch (...) { return false; }
    return row >= 0;
}

// "A1:C4" → r1=0,c1=0,r2=3,c2=2
bool GUI::parseRange(const std::string& ref, int& r1, int& c1, int& r2, int& c2) {
    auto sep = ref.find(':');
    if (sep == std::string::npos) return false;
    return parseCell(ref.substr(0, sep), r1, c1) &&
           parseCell(ref.substr(sep + 1), r2, c2);
}

// ─── HELPERS ──────────────────────────────────────────────────
std::string GUI::formatDouble(double value) {
    std::string str = std::to_string(value);
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    if (!str.empty() && str.back() == '.') {
        str.pop_back();
    }
    return str;
}

sf::Text GUI::makeText(const std::string& str, float x, float y,
                       unsigned size, sf::Color color) {
    sf::Text t;
    t.setFont(font);
    t.setString(str);
    t.setCharacterSize(size);
    t.setFillColor(color);
    t.setPosition(x, y);
    return t;
}

sf::RectangleShape GUI::makeRect(float x, float y, float w, float h,
                                 sf::Color fill, sf::Color outline) {
    sf::RectangleShape r(sf::Vector2f(w, h));
    r.setPosition(x, y);
    r.setFillColor(fill);
    r.setOutlineColor(outline);
    r.setOutlineThickness(1);
    return r;
}