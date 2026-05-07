#include "GUI.h"
#include "CellRef.h"
#include "FormulaEvaluator.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace {

std::string trimStr(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) b--;
    return s.substr(a, b - a);
}

} // namespace

GUI::GUI(SparseMatrix& matrix)
    : sheet(matrix),
      window(sf::VideoMode(1280, 720),
             "Hoja de calculo"),
      colWidths(COLS, DEFAULT_COL_W) {
    window.setFramerateLimit(60);
    if (!font.loadFromFile("arial.ttf"))
        throw std::runtime_error("No se pudo cargar arial.ttf");
}

int GUI::totalGridWidth() const {
    int t = 0;
    for (int w : colWidths) t += w;
    return t;
}

int GUI::colOffsetPixel(int col) const {
    int acc = 0;
    for (int c = 0; c < col && c < COLS; c++) acc += colWidths[c];
    return acc;
}

int GUI::pickColumnFromX(float gx, float& edgeDist) const {
    int acc = 0;
    for (int c = 0; c < COLS; c++) {
        int w = colWidths[c];
        if (gx < acc + w) {
            edgeDist = static_cast<float>(acc + w - gx);
            return c;
        }
        acc += w;
    }
    edgeDist = 0.f;
    return COLS - 1;
}

int GUI::pickRowFromY(float gy) const {
    if (gy < 0) return 0;
    int r = static_cast<int>(gy / CELL_H);
    if (r < 0) r = 0;
    if (r >= ROWS) r = ROWS - 1;
    return r;
}

void GUI::clampScroll() {
    float maxScrollX = std::max(0.f, static_cast<float>(totalGridWidth()) -
                                         (window.getSize().x - ROW_HEADER_W - 20));
    float maxScrollY =
        std::max(0.f, static_cast<float>(ROWS * CELL_H) - (window.getSize().y - gridTop() - STATUS_H));
    if (scrollX < 0) scrollX = 0;
    if (scrollX > maxScrollX) scrollX = maxScrollX;
    if (scrollY < 0) scrollY = 0;
    if (scrollY > maxScrollY) scrollY = maxScrollY;
}

void GUI::ensureCellVisible(int r, int c) {
    float viewW = static_cast<float>(window.getSize().x - ROW_HEADER_W);
    float viewH = static_cast<float>(window.getSize().y - gridTop() - STATUS_H);

    float cx1 = static_cast<float>(colOffsetPixel(c));
    float cx2 = cx1 + static_cast<float>(colWidths[c]);

    if (cx1 < scrollX) {
        scrollX = cx1;
    } else if (cx2 > scrollX + viewW) {
        scrollX = cx2 - viewW;
    }

    float cy1 = static_cast<float>(r * CELL_H);
    float cy2 = cy1 + static_cast<float>(CELL_H);

    if (cy1 < scrollY) {
        scrollY = cy1;
    } else if (cy2 > scrollY + viewH) {
        scrollY = cy2 - viewH;
    }

    clampScroll();
}

void GUI::pushUndo() {
    undoStack.push_back(sheet.snapshotCells());
    if (undoStack.size() > MAX_UNDO) undoStack.pop_front();
    redoStack.clear();
}

void GUI::undo() {
    if (undoStack.empty()) return;
    redoStack.push_back(sheet.snapshotCells());
    sheet.restoreSnapshot(undoStack.back());
    undoStack.pop_back();
    formulaDirty = false;
    loadFormulaBarFromActiveCell();
    statusMsg = "Deshacer";
}

void GUI::redo() {
    if (redoStack.empty()) return;
    undoStack.push_back(sheet.snapshotCells());
    sheet.restoreSnapshot(redoStack.back());
    redoStack.pop_back();
    formulaDirty = false;
    loadFormulaBarFromActiveCell();
    statusMsg = "Rehacer";
}

void GUI::syncNameBoxFromSelection() {
    if (selR1 < 0 || selC1 < 0) {
        nameBoxText = "";
        return;
    }
    int loR = std::min(selR1, selR2);
    int hiR = std::max(selR1, selR2);
    int loC = std::min(selC1, selC2);
    int hiC = std::max(selC1, selC2);
    std::string a = cellref::colIndexToLetters(loC) + std::to_string(loR + 1);
    if (loR == hiR && loC == hiC) {
        nameBoxText = a;
        return;
    }
    std::string b = cellref::colIndexToLetters(hiC) + std::to_string(hiR + 1);
    nameBoxText = a + ":" + b;
}

void GUI::loadFormulaBarFromActiveCell() {
    formulaBar = sheet.query(activeRow, activeCol);
    formulaDirty = false;
}

void GUI::cancelEdit() {
    loadFormulaBarFromActiveCell();
    statusMsg = "Edicion cancelada";
}

void GUI::commitActiveCell() {
    pushUndo();
    sheet.insert(activeRow, activeCol, formulaBar);
    formulaDirty = false;
    statusMsg = "Guardado " + cellref::colIndexToLetters(activeCol) + std::to_string(activeRow + 1);
}

void GUI::navigateNameBoxToCell() {
    int r = 0, c = 0;
    std::string t = trimStr(nameBoxText);
    if (t.find(':') != std::string::npos) {
        int r1, c1, r2, c2;
        if (!parseRangeInput(t, r1, c1, r2, c2)) {
            statusMsg = "Rango invalido en cuadro de nombre";
            return;
        }
        selR1 = r1;
        selC1 = c1;
        selR2 = r2;
        selC2 = c2;
        activeRow = anchorRow = r1;
        activeCol = anchorCol = c1;
    } else {
        if (!parseCellInput(t, r, c)) {
            statusMsg = "Celda invalida en cuadro de nombre";
            return;
        }
        selR1 = selR2 = activeRow = anchorRow = r;
        selC1 = selC2 = activeCol = anchorCol = c;
    }
    syncNameBoxFromSelection();
    loadFormulaBarFromActiveCell();
    focusFormula = true;
    focusNameBox = false;
    statusMsg = "Seleccion actualizada";
}

bool GUI::parseCellInput(const std::string& ref, int& row, int& col) {
    return cellref::parseCellRef(trimStr(ref), row, col);
}

bool GUI::parseRangeInput(const std::string& ref, int& r1, int& c1, int& r2, int& c2) {
    return cellref::parseRangeRef(trimStr(ref), r1, c1, r2, c2);
}

std::string GUI::formatCellDisplay(int row, int col) const {
    std::string d = FormulaEvaluator::cellDisplay(sheet, row, col);
    if (d.empty()) return "";
    if (!d.empty() && d[0] == '#') return d;
    return applyThousands(d);
}

std::string GUI::applyThousands(const std::string& numericDisplay) {
    if (numericDisplay.empty() || numericDisplay[0] == '#') return numericDisplay;
    bool neg = false;
    size_t start = 0;
    if (numericDisplay[0] == '-') {
        neg = true;
        start = 1;
    }
    size_t dotPos = numericDisplay.find('.', start);
    std::string intPart =
        dotPos == std::string::npos ? numericDisplay.substr(start) : numericDisplay.substr(start, dotPos - start);
    std::string frac = dotPos == std::string::npos ? "" : numericDisplay.substr(dotPos);
    if (intPart.empty()) return numericDisplay;
    for (char ch : intPart)
        if (!std::isdigit(static_cast<unsigned char>(ch))) return numericDisplay;

    std::string out;
    int count = 0;
    for (int k = static_cast<int>(intPart.size()) - 1; k >= 0; k--) {
        if (count && count % 3 == 0) out.push_back(',');
        out.push_back(intPart[static_cast<size_t>(k)]);
        count++;
    }
    std::reverse(out.begin(), out.end());
    if (neg) out = "-" + out;
    return out + frac;
}

void GUI::executeDeleteSelection() {
    if (selR1 < 0) return;
    pushUndo();
    int loR = std::min(selR1, selR2);
    int hiR = std::max(selR1, selR2);
    int loC = std::min(selC1, selC2);
    int hiC = std::max(selC1, selC2);
    sheet.deleteRange(loR, loC, hiR, hiC);
    loadFormulaBarFromActiveCell();
    statusMsg = "Celdas eliminadas";
}

void GUI::executeAggregation(const std::string& op) {
    if (selR1 < 0) return;
    int loR = std::min(selR1, selR2);
    int hiR = std::max(selR1, selR2);
    int loC = std::min(selC1, selC2);
    int hiC = std::max(selC1, selC2);
    double result = 0;
    if (op == "SUMA")
        result = FormulaEvaluator::aggregateSum(sheet, loR, loC, hiR, hiC);
    else if (op == "PROMEDIO")
        result = FormulaEvaluator::aggregateAvg(sheet, loR, loC, hiR, hiC);
    else if (op == "MAX")
        result = FormulaEvaluator::aggregateMax(sheet, loR, loC, hiR, hiC);
    else if (op == "MIN")
        result = FormulaEvaluator::aggregateMin(sheet, loR, loC, hiR, hiC);
    else if (op == "CONTAR")
        result = FormulaEvaluator::aggregateCount(sheet, loR, loC, hiR, hiC);
    else
        return;
    statusMsg = op + " = " + FormulaEvaluator::formatNumber(result);
}

void GUI::copySelectionToClipboard() {
    if (selR1 < 0) return;
    int loR = std::min(selR1, selR2);
    int hiR = std::max(selR1, selR2);
    int loC = std::min(selC1, selC2);
    int hiC = std::max(selC1, selC2);
    std::ostringstream oss;
    for (int r = loR; r <= hiR; r++) {
        for (int c = loC; c <= hiC; c++) {
            if (c > loC) oss << '\t';
            oss << sheet.query(r, c);
        }
        if (r < hiR) oss << '\n';
    }
    sf::Clipboard::setString(oss.str());
    statusMsg = "Copiado al portapapeles";
}

void GUI::pasteFromClipboard() {
    std::string clip = sf::Clipboard::getString();
    if (clip.empty() || selR1 < 0) return;
    pushUndo();
    int startR = activeRow;
    int startC = activeCol;
    std::istringstream iss(clip);
    std::string line;
    int dr = 0;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::stringstream ls(line);
        std::string cell;
        int dc = 0;
        while (std::getline(ls, cell, '\t')) {
            int r = startR + dr;
            int c = startC + dc;
            if (r < ROWS && c < COLS) sheet.insert(r, c, cell);
            dc++;
        }
        dr++;
    }
    statusMsg = "Pegado";
}

void GUI::applyFill(int endRow, int endCol) {
    if (selR1 < 0) return;
    int selLoR = std::min(selR1, selR2);
    int selHiR = std::max(selR1, selR2);
    int selLoC = std::min(selC1, selC2);
    int selHiC = std::max(selC1, selC2);
    int patH = selHiR - selLoR + 1;
    int patW = selHiC - selLoC + 1;

    int destR1 = std::min({selLoR, selHiR, endRow});
    int destR2 = std::max({selLoR, selHiR, endRow});
    int destC1 = std::min({selLoC, selHiC, endCol});
    int destC2 = std::max({selLoC, selHiC, endCol});

    pushUndo();
    for (int r = destR1; r <= destR2; r++) {
        for (int c = destC1; c <= destC2; c++) {
            int sr = selLoR + ((r - selLoR) % patH + patH) % patH;
            int sc = selLoC + ((c - selLoC) % patW + patW) % patW;
            std::string raw = sheet.query(sr, sc);
            if (raw.empty()) continue;
            std::string out;
            if (raw[0] == '=')
                out = "=" + cellref::shiftFormulaRefs(raw.substr(1), r - sr, c - sc);
            else
                out = raw;
            sheet.insert(r, c, out);
        }
    }
    statusMsg = "Relleno aplicado";
}

void GUI::maybeAppendToReferenceInFormula(int r1, int c1, int r2, int c2, bool finish) {
    if (!focusFormula || formulaBar.empty() || formulaBar[0] != '=') return;

    if (formulaRefStartIdx == -1) {
        char lastChar = formulaBar.back();
        if (lastChar == '(' || lastChar == '+' || lastChar == '-' || lastChar == '*' ||
            lastChar == '/' || lastChar == ',' || lastChar == '=') {
            formulaRefStartIdx = formulaBar.size();
        } else {
            return;
        }
    }

    std::string ref;
    if (r1 == r2 && c1 == c1) {
        ref = cellref::colIndexToLetters(c1) + std::to_string(r1 + 1);
    } else {
        int minR = std::min(r1, r2);
        int maxR = std::max(r1, r2);
        int minC = std::min(c1, c2);
        int maxC = std::max(c1, c2);
        ref = cellref::colIndexToLetters(minC) + std::to_string(minR + 1) + ":" +
              cellref::colIndexToLetters(maxC) + std::to_string(maxR + 1);
    }

    formulaBar.erase(formulaRefStartIdx);
    formulaBar += ref;
}

void GUI::run() {
    while (window.isOpen()) {
        handleEvents();
        window.clear(sf::Color(245, 245, 245));
        drawGrid();
        drawTopPanel();
        drawFillHandle();
        drawStatusBar();
        window.display();
    }
}

void GUI::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window.close();

        bool cmd =
            sf::Keyboard::isKeyPressed(sf::Keyboard::LSystem) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::RSystem) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

        if (event.type == sf::Event::KeyPressed) {
            if (cmd && event.key.code == sf::Keyboard::Z) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
                    redo();
                else
                    undo();
                continue;
            }
            if (cmd && event.key.code == sf::Keyboard::Y) {
                redo();
                continue;
            }
            if (cmd && event.key.code == sf::Keyboard::C) {
                copySelectionToClipboard();
                continue;
            }
            if (cmd && event.key.code == sf::Keyboard::V) {
                pasteFromClipboard();
                continue;
            }
            if (cmd && event.key.code == sf::Keyboard::X) {
                copySelectionToClipboard();
                executeDeleteSelection();
                continue;
            }

            if (event.key.code == sf::Keyboard::Escape) {
                cancelEdit();
                continue;
            }

            if (event.key.code == sf::Keyboard::Tab) {
                if (focusNameBox) {
                    focusNameBox = false;
                    focusFormula = true;
                } else if (focusFormula) {
                    commitActiveCell();
                    if (activeCol < COLS - 1) {
                        activeCol++;
                    } else {
                        COLS++;
                        colWidths.push_back(DEFAULT_COL_W);
                        activeCol++;
                    }
                    selR1 = selR2 = activeRow = anchorRow;
                    selC1 = selC2 = activeCol = anchorCol;
                    syncNameBoxFromSelection();
                    loadFormulaBarFromActiveCell();
                }
                continue;
            }

            if (event.key.code == sf::Keyboard::Enter) {
                if (focusNameBox) {
                    navigateNameBoxToCell();
                    ensureCellVisible(activeRow, activeCol);
                } else {
                    commitActiveCell();
                    if (activeRow < ROWS - 1) {
                        activeRow++;
                    } else {
                        ROWS++;
                        activeRow++;
                    }
                    anchorRow = activeRow;
                    anchorCol = activeCol;
                    selR1 = selR2 = activeRow;
                    selC1 = selC2 = activeCol;
                    syncNameBoxFromSelection();
                    loadFormulaBarFromActiveCell();
                    ensureCellVisible(activeRow, activeCol);
                    formulaRefStartIdx = -1;
                }
                continue;
            }

            if (event.key.code == sf::Keyboard::F2) {
                focusFormula = true;
                focusNameBox = false;
                continue;
            }

            if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::Down ||
                event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Right) {
                bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

                bool isEditingFormula = focusFormula && !formulaBar.empty() && formulaBar[0] == '=';
                bool formulaExpectsRef = false;
                if (isEditingFormula) {
                    if (formulaRefStartIdx != -1) {
                        formulaExpectsRef = true;
                    } else {
                        char lastChar = formulaBar.back();
                        if (lastChar == '(' || lastChar == '+' || lastChar == '-' || lastChar == '*' ||
                            lastChar == '/' || lastChar == ',' || lastChar == '=') {
                            formulaExpectsRef = true;
                        }
                    }
                }

                if (selR1 < 0) {
                    selR1 = selR2 = activeRow = anchorRow = 0;
                    selC1 = selC2 = activeCol = anchorCol = 0;
                }
                int dr = 0, dc = 0;
                if (event.key.code == sf::Keyboard::Up) dr = -1;
                if (event.key.code == sf::Keyboard::Down) dr = 1;
                if (event.key.code == sf::Keyboard::Left) dc = -1;
                if (event.key.code == sf::Keyboard::Right) dc = 1;

                if (isEditingFormula && formulaExpectsRef) {
                    if (!shift) {
                        int nr = selR2 + dr;
                        int nc = selC2 + dc;
                        if (nr >= ROWS) ROWS = nr + 1;
                        if (nc >= COLS) {
                            while (COLS <= nc) {
                                COLS++;
                                colWidths.push_back(DEFAULT_COL_W);
                            }
                        }
                        nr = std::max(0, std::min(ROWS - 1, nr));
                        nc = std::max(0, std::min(COLS - 1, nc));
                        selR1 = anchorRow = nr;
                        selC1 = anchorCol = nc;
                        selR2 = nr;
                        selC2 = nc;
                        maybeAppendToReferenceInFormula(selR1, selC1, selR2, selC2, false);
                    } else {
                        selR1 = anchorRow;
                        selC1 = anchorCol;
                        int nr = selR2 + dr;
                        int nc = selC2 + dc;
                        if (nr >= ROWS) ROWS = nr + 1;
                        if (nc >= COLS) {
                            while (COLS <= nc) {
                                COLS++;
                                colWidths.push_back(DEFAULT_COL_W);
                            }
                        }
                        nr = std::max(0, std::min(ROWS - 1, nr));
                        nc = std::max(0, std::min(COLS - 1, nc));
                        selR2 = nr;
                        selC2 = nc;
                        maybeAppendToReferenceInFormula(selR1, selC1, selR2, selC2, false);
                    }
                    ensureCellVisible(selR2, selC2);
                    continue;
                }

                if (isEditingFormula && !formulaExpectsRef) {
                    commitActiveCell();
                    formulaRefStartIdx = -1;
                }

                if (!shift) {
                    int nr = activeRow + dr;
                    int nc = activeCol + dc;
                    if (nr >= ROWS) ROWS = nr + 1;
                    if (nc >= COLS) {
                        while (COLS <= nc) {
                            COLS++;
                            colWidths.push_back(DEFAULT_COL_W);
                        }
                    }
                    nr = std::max(0, std::min(ROWS - 1, nr));
                    nc = std::max(0, std::min(COLS - 1, nc));
                    activeRow = anchorRow = selR1 = selR2 = nr;
                    activeCol = anchorCol = selC1 = selC2 = nc;
                } else {
                    selR1 = anchorRow;
                    selC1 = anchorCol;
                    int nr = selR2 + dr;
                    int nc = selC2 + dc;
                    nr = std::max(0, std::min(ROWS - 1, nr));
                    nc = std::max(0, std::min(COLS - 1, nc));
                    selR2 = nr;
                    selC2 = nc;
                    activeRow = anchorRow;
                    activeCol = anchorCol;
                }
                syncNameBoxFromSelection();
                loadFormulaBarFromActiveCell();
                ensureCellVisible(selR2, selC2);
                continue;
            }

            if (event.key.code == sf::Keyboard::Delete || event.key.code == sf::Keyboard::BackSpace) {
                if (focusFormula && event.key.code == sf::Keyboard::BackSpace &&
                    !formulaBar.empty()) {
                    formulaRefStartIdx = -1;
                    formulaBar.pop_back();
                    formulaDirty = true;
                } else if (focusFormula && event.key.code == sf::Keyboard::BackSpace &&
                           formulaBar.empty()) {
                    executeDeleteSelection();
                } else if (event.key.code == sf::Keyboard::Delete) {
                    executeDeleteSelection();
                }
                continue;
            }

            if (event.key.code == sf::Keyboard::P && cmd) {
                if (selR1 >= 0 && selR1 == selR2 && selC1 == selC2)
                    statusMsg = formatCellDisplay(selR1, selC1);
                else if (selR1 >= 0)
                    executeAggregation("SUMA");
                continue;
            }
        }

        if (event.type == sf::Event::TextEntered) {
            char c = static_cast<char>(event.text.unicode);
            if (c >= 32 && c < 127) {
                if (focusNameBox)
                    nameBoxText.push_back(c);
                else {
                    focusFormula = true;
                    // Reset selection reference tracking on manual text input
                    formulaRefStartIdx = -1;
                    formulaBar.push_back(c);
                    formulaDirty = true;
                }
            }
        }

        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2i mp = sf::Mouse::getPosition(window);
            float mx = static_cast<float>(mp.x);
            float my = static_cast<float>(mp.y);

            sf::FloatRect nameRect(58.f, 18.f, 140.f, 28.f);
            sf::FloatRect fxRect(210.f, 18.f, static_cast<float>(window.getSize().x) - 230.f, 28.f);

            if (my < TOP_PANEL_H) {
                if (nameRect.contains(mx, my)) {
                    focusNameBox = true;
                    focusFormula = false;
                } else if (fxRect.contains(mx, my)) {
                    focusNameBox = false;
                    focusFormula = true;
                }
                continue;
            }

            // Column resize on header row
            if (my >= TOP_PANEL_H && my < gridTop() && mx >= ROW_HEADER_W) {
                float gx = mx - ROW_HEADER_W + scrollX;
                float edge = 0.f;
                int col = pickColumnFromX(gx, edge);
                if (edge >= 0 && edge <= 6.f && col >= 0) {
                    resizeColIndex = col;
                    resizeStartMouseX = mx;
                    resizeStartWidth = colWidths[col];
                    continue;
                }
            }

            // Row header -> select row
            if (mx < ROW_HEADER_W && my >= gridTop()) {
                float gy = my - gridTop() + scrollY;
                int r = pickRowFromY(gy);
                clickHadShift =
                    sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
                if (!clickHadShift) {
                    selR1 = selR2 = activeRow = anchorRow = r;
                    selC1 = 0;
                    selC2 = COLS - 1;
                    activeCol = anchorCol = 0;
                } else {
                    selR2 = r;
                    activeRow = anchorRow;
                    activeCol = anchorCol;
                }
                syncNameBoxFromSelection();
                loadFormulaBarFromActiveCell();
                focusFormula = true;
                continue;
            }

            // Column letter header -> select column
            if (my >= TOP_PANEL_H && my < gridTop() && mx >= ROW_HEADER_W) {
                float gx = mx - ROW_HEADER_W + scrollX;
                float edge = 0.f;
                int c = pickColumnFromX(gx, edge);
                clickHadShift =
                    sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
                if (!clickHadShift) {
                    selC1 = selC2 = activeCol = anchorCol = c;
                    selR1 = 0;
                    selR2 = ROWS - 1;
                    activeRow = anchorRow = 0;
                } else {
                    selC2 = c;
                    activeCol = anchorCol;
                    activeRow = anchorRow;
                }
                syncNameBoxFromSelection();
                loadFormulaBarFromActiveCell();
                focusFormula = true;
                continue;
            }

            // Grid body
            if (mx >= ROW_HEADER_W && my >= gridTop() &&
                my < window.getSize().y - STATUS_H) {
                float gx = mx - ROW_HEADER_W + scrollX;
                float gy = my - gridTop() + scrollY;
                float edge = 0.f;
                int c = pickColumnFromX(gx, edge);
                int r = pickRowFromY(gy);

                // Fill handle hit
                if (selR1 >= 0) {
                    int loR = std::min(selR1, selR2);
                    int hiR = std::max(selR1, selR2);
                    int loC = std::min(selC1, selC2);
                    int hiC = std::max(selC1, selC2);
                    float x2 = static_cast<float>(ROW_HEADER_W + colOffsetPixel(hiC) +
                                                    colWidths[hiC] - scrollX);
                    float y2 = static_cast<float>(gridTop() + (hiR + 1) * CELL_H - scrollY);
                    sf::FloatRect fh(x2 - FILL_HANDLE, y2 - FILL_HANDLE,
                                     static_cast<float>(FILL_HANDLE + 2),
                                     static_cast<float>(FILL_HANDLE + 2));
                    if (fh.contains(mx, my)) {
                        isFillDragging = true;
                        fillEndRow = hiR;
                        fillEndCol = hiC;
                        continue;
                    }
                }

                bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

                if (dblClickClock.getElapsedTime().asMilliseconds() < 400 &&
                    lastClickRow == r && lastClickCol == c) {
                    focusFormula = true;
                    focusNameBox = false;
                    dblClickClock.restart();
                    lastClickRow = -1;
                    continue;
                }
                dblClickClock.restart();
                lastClickRow = r;
                lastClickCol = c;

                isDragging = true;

                bool isEditingFormula = focusFormula && !formulaBar.empty() && formulaBar[0] == '=';
                bool formulaExpectsRef = false;
                if (isEditingFormula) {
                    if (formulaRefStartIdx != -1) {
                        formulaExpectsRef = true;
                    } else {
                        char lastChar = formulaBar.back();
                        if (lastChar == '(' || lastChar == '+' || lastChar == '-' || lastChar == '*' ||
                            lastChar == '/' || lastChar == ',' || lastChar == '=') {
                            formulaExpectsRef = true;
                        }
                    }
                }

                if (isEditingFormula && formulaExpectsRef) {
                    if (!shift) {
                        selR1 = anchorRow = r;
                        selC1 = anchorCol = c;
                        selR2 = r;
                        selC2 = c;
                    } else {
                        selR2 = r;
                        selC2 = c;
                    }
                    maybeAppendToReferenceInFormula(selR1, selC1, selR2, selC2, false);
                } else {
                    if (focusFormula && formulaDirty) {
                        commitActiveCell();
                    }
                    if (!shift) {
                        selR1 = selR2 = activeRow = anchorRow = r;
                        selC1 = selC2 = activeCol = anchorCol = c;
                        syncNameBoxFromSelection();
                        loadFormulaBarFromActiveCell();
                        focusFormula = true;
                        focusNameBox = false;
                    } else {
                        selR1 = anchorRow;
                        selC1 = anchorCol;
                        selR2 = r;
                        selC2 = c;
                        activeRow = anchorRow;
                        activeCol = anchorCol;
                        syncNameBoxFromSelection();
                        loadFormulaBarFromActiveCell();
                        focusFormula = true;
                        focusNameBox = false;
                    }
                    formulaRefStartIdx = -1;
                }
            }
        }

        if (event.type == sf::Event::MouseMoved) {
            if (resizeColIndex >= 0) {
                float dx = static_cast<float>(event.mouseMove.x) - resizeStartMouseX;
                int nw = resizeStartWidth + static_cast<int>(dx);
                if (nw < 40) nw = 40;
                if (nw > 600) nw = 600;
                colWidths[resizeColIndex] = nw;
            }
            if (isDragging) {
                float mx = static_cast<float>(event.mouseMove.x);
                float my = static_cast<float>(event.mouseMove.y);
                if (mx >= ROW_HEADER_W && my >= gridTop()) {
                    float gx = mx - ROW_HEADER_W + scrollX;
                    float gy = my - gridTop() + scrollY;
                    float edge = 0.f;
                    selR2 = pickRowFromY(gy);
                    selC2 = pickColumnFromX(gx, edge);

                    bool isEditingFormula = focusFormula && !formulaBar.empty() && formulaBar[0] == '=';
                    if (isEditingFormula && formulaRefStartIdx != -1) {
                        maybeAppendToReferenceInFormula(selR1, selC1, selR2, selC2, false);
                    } else {
                        syncNameBoxFromSelection();
                    }
                }
            }
            if (isFillDragging) {
                float mx = static_cast<float>(event.mouseMove.x);
                float my = static_cast<float>(event.mouseMove.y);
                if (mx >= ROW_HEADER_W && my >= gridTop()) {
                    float gx = mx - ROW_HEADER_W + scrollX;
                    float gy = my - gridTop() + scrollY;
                    float edge = 0.f;
                    fillEndRow = pickRowFromY(gy);
                    fillEndCol = pickColumnFromX(gx, edge);
                }
            }
        }

        if (event.type == sf::Event::MouseButtonReleased) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                if (resizeColIndex >= 0) {
                    resizeColIndex = -1;
                    clampScroll();
                }
                if (isFillDragging) {
                    applyFill(fillEndRow, fillEndCol);
                    isFillDragging = false;
                }
                isDragging = false;
            }
        }

        if (event.type == sf::Event::MouseWheelScrolled) {
            if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
                    scrollX -= event.mouseWheelScroll.delta * 24.f;
                else
                    scrollY -= event.mouseWheelScroll.delta * 24.f;
            } else if (event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel) {
                scrollX -= event.mouseWheelScroll.delta * 24.f;
            }
            clampScroll();
        }

        if (event.type == sf::Event::Resized) {
            sf::FloatRect visible(0.f, 0.f, static_cast<float>(event.size.width),
                                  static_cast<float>(event.size.height));
            window.setView(sf::View(visible));
            clampScroll();
        }
    }
}

void GUI::drawGrid() {
    const float gridTopF = static_cast<float>(gridTop());

    // Headers columnas
    for (int c = 0; c < COLS; c++) {
        float x = static_cast<float>(ROW_HEADER_W + colOffsetPixel(c)) - scrollX;
        float w = static_cast<float>(colWidths[c]);
        if (x + w < ROW_HEADER_W || x > window.getSize().x) continue;
        auto rect = makeRect(x, static_cast<float>(TOP_PANEL_H), w, static_cast<float>(HEADER_COL_H),
                              sf::Color(208, 208, 228));
        window.draw(rect);
        std::string lab = cellref::colIndexToLetters(c);
        window.draw(makeText(lab, x + w * 0.35f, static_cast<float>(TOP_PANEL_H + 4), 13));
    }

    // Headers filas
    for (int r = 0; r < ROWS; r++) {
        float y = gridTopF + r * CELL_H - scrollY;
        if (y + CELL_H < gridTopF || y > window.getSize().y) continue;
        auto rect =
            makeRect(0.f, y, static_cast<float>(ROW_HEADER_W), static_cast<float>(CELL_H),
                     sf::Color(208, 208, 228));
        window.draw(rect);
        window.draw(makeText(std::to_string(r + 1), 6.f, y + 6.f, 12));
    }

    // Celdas
    for (int r = 0; r < ROWS; r++) {
        float y = gridTopF + r * CELL_H - scrollY;
        if (y + CELL_H < gridTopF || y > window.getSize().y - STATUS_H) continue;

        for (int c = 0; c < COLS; c++) {
            float x = static_cast<float>(ROW_HEADER_W + colOffsetPixel(c)) - scrollX;
            float w = static_cast<float>(colWidths[c]);
            if (x + w < ROW_HEADER_W || x > window.getSize().x) continue;

            sf::Color fill(255, 255, 255);
            if (selR1 >= 0) {
                int loR = std::min(selR1, selR2);
                int hiR = std::max(selR1, selR2);
                int loC = std::min(selC1, selC2);
                int hiC = std::max(selC1, selC2);
                if (r >= loR && r <= hiR && c >= loC && c <= hiC)
                    fill = sf::Color(210, 225, 255);
            }
            if (r == activeRow && c == activeCol && selR1 >= 0)
                fill = sf::Color(190, 210, 255);

            window.draw(makeRect(x, y, w, static_cast<float>(CELL_H), fill));

            std::string show = formatCellDisplay(r, c);
            if (!show.empty()) {
                if (show.size() > 14) show = show.substr(0, 13) + "~";
                bool numLike =
                    !show.empty() && (std::isdigit(static_cast<unsigned char>(show[0])) ||
                                      show[0] == '-' || show[0] == '#');
                float tx = numLike ? x + w - 10.f - show.size() * 6.f : x + 4.f;
                if (tx < x + 4.f) tx = x + 4.f;
                window.draw(makeText(show, tx, y + 6.f, 12));
            }
        }
    }

    // Borde celda activa
    if (selR1 >= 0) {
        float ax = static_cast<float>(ROW_HEADER_W + colOffsetPixel(activeCol)) - scrollX;
        float ay = gridTopF + activeRow * CELL_H - scrollY;
        float aw = static_cast<float>(colWidths[activeCol]);
        sf::RectangleShape border(sf::Vector2f(aw, static_cast<float>(CELL_H)));
        border.setPosition(ax, ay);
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(sf::Color(20, 120, 40));
        border.setOutlineThickness(2.f);
        window.draw(border);
    }

    // Esquina congelada
    window.draw(makeRect(0.f, static_cast<float>(TOP_PANEL_H), static_cast<float>(ROW_HEADER_W),
                         static_cast<float>(HEADER_COL_H), sf::Color(190, 190, 210)));
}

void GUI::drawTopPanel() {
    window.draw(makeRect(0.f, 0.f, static_cast<float>(window.getSize().x),
                         static_cast<float>(TOP_PANEL_H), sf::Color(235, 235, 240)));

    window.draw(makeText("Nombre", 8.f, 8.f, 12, sf::Color(60, 60, 60)));
    window.draw(makeText("fx", 188.f, 22.f, 14, sf::Color(80, 80, 80)));

    auto nameBox = makeRect(58.f, 18.f, 140.f, 28.f,
                            focusNameBox ? sf::Color(255, 255, 210) : sf::Color::White);
    window.draw(nameBox);
    std::string nb = nameBoxText + (focusNameBox ? "|" : "");
    window.draw(makeText(nb, 64.f, 23.f, 14));

    float fxX = 210.f;
    float fxW = static_cast<float>(window.getSize().x) - fxX - 12.f;
    auto fxBox = makeRect(fxX, 18.f, fxW, 28.f,
                          focusFormula ? sf::Color(255, 255, 210) : sf::Color::White);
    window.draw(fxBox);
    std::string fb = formulaBar + (focusFormula ? "|" : "");
    window.draw(makeText(fb, fxX + 8.f, 23.f, 14));

    window.draw(makeText("Tab=sig celda  Enter=guardar  Esc=cancelar  Cmd/Ctrl+Z deshacer  C/V copiar/pegar",
                         8.f, 52.f, 11, sf::Color(100, 100, 120)));
}

void GUI::drawFillHandle() {
    if (selR1 < 0) return;
    int loR = std::min(selR1, selR2);
    int hiR = std::max(selR1, selR2);
    int loC = std::min(selC1, selC2);
    int hiC = std::max(selC1, selC2);
    float x2 = static_cast<float>(ROW_HEADER_W + colOffsetPixel(hiC) + colWidths[hiC]) - scrollX;
    float y2 = static_cast<float>(gridTop() + (hiR + 1) * CELL_H - scrollY);
    sf::RectangleShape fh(
        sf::Vector2f(static_cast<float>(FILL_HANDLE), static_cast<float>(FILL_HANDLE)));
    fh.setPosition(x2 - FILL_HANDLE - 1.f, y2 - FILL_HANDLE - 1.f);
    fh.setFillColor(sf::Color(20, 120, 40));
    fh.setOutlineColor(sf::Color::White);
    fh.setOutlineThickness(1.f);
    window.draw(fh);
}

void GUI::drawStatusBar() {
    float y = static_cast<float>(window.getSize().y - STATUS_H);
    window.draw(makeRect(0.f, y, static_cast<float>(window.getSize().x),
                         static_cast<float>(STATUS_H), sf::Color(230, 230, 235)));
    window.draw(makeText(">> " + statusMsg, 10.f, y + 10.f, 14, sf::Color(40, 40, 120)));
}

sf::Text GUI::makeText(const std::string& str, float x, float y, unsigned size,
                       sf::Color color) {
    sf::Text t;
    t.setFont(font);
    t.setString(str);
    t.setCharacterSize(size);
    t.setFillColor(color);
    t.setPosition(x, y);
    return t;
}

sf::RectangleShape GUI::makeRect(float x, float y, float w, float h, sf::Color fill,
                                   sf::Color outline) {
    sf::RectangleShape r(sf::Vector2f(w, h));
    r.setPosition(x, y);
    r.setFillColor(fill);
    r.setOutlineColor(outline);
    r.setOutlineThickness(1.f);
    return r;
}
