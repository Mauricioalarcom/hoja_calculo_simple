#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window/Clipboard.hpp>
#include "SparseMatrix.h"
#include <deque>
#include <map>
#include <string>
#include <vector>

class GUI {
public:
    GUI(SparseMatrix& matrix);
    void run();

private:
    SparseMatrix& sheet;
    sf::RenderWindow window;
    sf::Font font;

    int COLS = 40;
    int ROWS = 200;
    static constexpr int DEFAULT_COL_W = 88;
    static const int CELL_H = 28;
    static const int ROW_HEADER_W = 52;
    static const int TOP_PANEL_H = 72;
    static const int HEADER_COL_H = 26;
    static const int STATUS_H = 36;
    static const int FILL_HANDLE = 6;

    int gridTop() const { return TOP_PANEL_H + HEADER_COL_H; }

    std::vector<int> colWidths;

    float scrollX = 0.f;
    float scrollY = 0.f;

    int selR1 = -1, selC1 = -1, selR2 = -1, selC2 = -1;
    int activeRow = 0;
    int activeCol = 0;
    int anchorRow = 0;
    int anchorCol = 0;

    std::string nameBoxText;
    std::string formulaBar;
    bool focusNameBox = false;
    bool focusFormula = true;
    bool formulaDirty = false;

    bool isWritingFormula = false;
    int formulaRefStartIdx = -1;

    std::string statusMsg;

    std::deque<std::map<std::pair<int, int>, std::string>> undoStack;
    std::deque<std::map<std::pair<int, int>, std::string>> redoStack;
    static const size_t MAX_UNDO = 80;

    bool isDragging = false;
    bool isFillDragging = false;
    int fillEndRow = -1;
    int fillEndCol = -1;
    int resizeColIndex = -1;
    float resizeStartMouseX = 0.f;
    int resizeStartWidth = 0;
    bool clickHadShift = false;

    sf::Clock dblClickClock;
    int lastClickRow = -1;
    int lastClickCol = -1;

    int totalGridWidth() const;
    int colOffsetPixel(int col) const;
    /// Returns column index; edgeDist = distance to right edge of header (for resize).
    int pickColumnFromX(float gx, float& edgeDist) const;
    int pickRowFromY(float gy) const;

    void pushUndo();
    void undo();
    void redo();

    void syncNameBoxFromSelection();
    void loadFormulaBarFromActiveCell();
    void commitActiveCell();
    void cancelEdit();

    void clampScroll();
    void ensureCellVisible(int r, int c);
    void handleEvents();
    void drawGrid();
    void drawTopPanel();
    void drawStatusBar();
    void drawFillHandle();

    void executeDeleteSelection();
    void executeAggregation(const std::string& op);
    void copySelectionToClipboard();
    void pasteFromClipboard();

    void maybeAppendToReferenceInFormula(int r1, int c1, int r2, int c2, bool finish);

    void applyFill(int endRow, int endCol);

    std::string formatCellDisplay(int row, int col) const;
    static std::string applyThousands(const std::string& numericDisplay);

    bool parseCellInput(const std::string& ref, int& row, int& col);
    bool parseRangeInput(const std::string& ref, int& r1, int& c1, int& r2, int& c2);
    void navigateNameBoxToCell();

    sf::Text makeText(const std::string& str, float x, float y,
                      unsigned size = 14, sf::Color color = sf::Color::Black);
    sf::RectangleShape makeRect(float x, float y, float w, float h,
                                  sf::Color fill, sf::Color outline = sf::Color(160, 160, 160));
};
