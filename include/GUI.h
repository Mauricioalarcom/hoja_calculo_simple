#pragma once
#include <SFML/Graphics.hpp>
#include "SparseMatrix.h"
#include <string>

class GUI {
public:
    GUI(SparseMatrix& matrix);
    void run();

private:
    // ── Core ──────────────────────────────────────────────────
    SparseMatrix& sheet;
    sf::RenderWindow window;
    sf::Font font;

    // ── Grilla ────────────────────────────────────────────────
    static const int COLS       = 10;
    static const int ROWS       = 20;
    static const int CELL_W     = 90;
    static const int CELL_H     = 30;
    static const int OFFSET_X   = 40;  // espacio para headers de fila
    static const int OFFSET_Y   = 120; // espacio para panel superior

    // ── Estado del input ──────────────────────────────────────
    std::string inputCell;   // ej. "B3"
    std::string inputValue;  // ej. "42"
    std::string inputRange;  // ej. "A1:C4"
    std::string statusMsg;   // mensaje de resultado

    bool focusCell  = true;  // true = foco en inputCell
    bool focusValue = false;
    bool focusRange = false;

    // ── Helpers ───────────────────────────────────────────────
    void handleEvents();
    void render();

    void drawGrid();
    void drawTopPanel();
    void drawBottomPanel();

    void executeInsert();
    void executeDelete();
    void executeQuery();
    void executeAggregation(const std::string& op);

    // Convierte "B3" → (row=2, col=1)
    bool parseCell(const std::string& ref, int& row, int& col);
    // Convierte "A1:C4" → (r1,c1,r2,c2)
    bool parseRange(const std::string& ref, int& r1, int& c1, int& r2, int& c2);

    sf::Text makeText(const std::string& str, float x, float y,
                      unsigned size = 14, sf::Color color = sf::Color::Black);
    sf::RectangleShape makeRect(float x, float y, float w, float h,
                                sf::Color fill, sf::Color outline = sf::Color::Black);
};