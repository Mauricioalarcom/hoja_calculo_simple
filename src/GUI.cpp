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
            // Backspace
            if (event.key.code == sf::Keyboard::BackSpace) {
                if (focusCell  && !inputCell.empty())  inputCell.pop_back();
                if (focusValue && !inputValue.empty()) inputValue.pop_back();
                if (focusRange && !inputRange.empty()) inputRange.pop_back();
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

            // Botones de operación (fila 1 de botones y=50)
            if (y > 50 && y < 80) {
                if (x > 10  && x < 110) executeInsert();
                if (x > 120 && x < 220) executeDelete();
                if (x > 230 && x < 330) executeQuery();
            }

            // Botones de agregación (fila 2 y=85)
            if (y > 85 && y < 115) {
                if (x > 10  && x < 110) executeAggregation("SUMA");
                if (x > 120 && x < 220) executeAggregation("PROMEDIO");
                if (x > 230 && x < 330) executeAggregation("MAX");
                if (x > 340 && x < 440) executeAggregation("MIN");
            }
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
        std::string label(1, char('A' + c));
        float x = OFFSET_X + c * CELL_W;
        auto rect = makeRect(x, OFFSET_Y - CELL_H, CELL_W, CELL_H,
                             sf::Color(200, 200, 220));
        window.draw(rect);
        window.draw(makeText(label, x + CELL_W/2 - 5, OFFSET_Y - CELL_H + 7));
    }

    // Header de filas (1, 2, 3, ...)
    for (int r = 0; r < ROWS; r++) {
        float y = OFFSET_Y + r * CELL_H;
        auto rect = makeRect(0, y, OFFSET_X, CELL_H, sf::Color(200, 200, 220));
        window.draw(rect);
        window.draw(makeText(std::to_string(r + 1), 5, y + 7));
    }

    // Celdas
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            float x = OFFSET_X + c * CELL_W;
            float y = OFFSET_Y + r * CELL_H;
            auto key = std::make_pair(r, c);

            bool hasValue = occupied.count(key);
            sf::Color fill = hasValue ? sf::Color(220, 240, 220) : sf::Color::White;

            auto rect = makeRect(x, y, CELL_W, CELL_H, fill);
            window.draw(rect);

            if (hasValue) {
                std::string val = occupied[key];
                if (val.size() > 10) val = val.substr(0, 9) + "~";
                window.draw(makeText(val, x + 4, y + 7, 12));
            }
        }
    }
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

    // Botones operaciones básicas
    struct Btn { std::string label; float x; sf::Color color; };
    std::vector<Btn> btns = {
        {"Insertar",  10,  sf::Color(100,180,100)},
        {"Eliminar", 120,  sf::Color(180,100,100)},
        {"Consultar",230,  sf::Color(100,130,200)},
    };
    for (auto& b : btns) {
        window.draw(makeRect(b.x, 50, 100, 28, b.color));
        window.draw(makeText(b.label, b.x + 8, 55, 13, sf::Color::White));
    }

    // Botones agregación
    std::vector<Btn> aggBtns = {
        {"SUMA",    10,  sf::Color(80,150,200)},
        {"PROM",   120,  sf::Color(80,150,200)},
        {"MAX",    230,  sf::Color(80,150,200)},
        {"MIN",    340,  sf::Color(80,150,200)},
    };
    for (auto& b : aggBtns) {
        window.draw(makeRect(b.x, 85, 100, 28, b.color));
        window.draw(makeText(b.label, b.x + 25, 90, 13, sf::Color::White));
    }
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
    sheet.insert(r, c, inputValue);
    statusMsg = "Insertado " + inputCell + " = " + inputValue;
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
        statusMsg = op + "(" + inputRange + ") = " + std::to_string(result);
    }
    // Si no hay rango, intentar con celda/fila/columna
    else {
        int r, c;
        if (!parseCell(inputCell, r, c)) { statusMsg = "Ingresa un rango (ej. A1:C4)"; return; }
        if      (op == "SUMA")     result = sheet.sumRow(r);
        else if (op == "PROMEDIO") result = sheet.avgRange(r,0,r,COLS-1);
        else if (op == "MAX")      result = sheet.maxRange(r,0,r,COLS-1);
        else if (op == "MIN")      result = sheet.minRange(r,0,r,COLS-1);
        statusMsg = op + "(fila " + std::to_string(r+1) + ") = " + std::to_string(result);
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