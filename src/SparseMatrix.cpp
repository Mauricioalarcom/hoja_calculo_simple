#include "SparseMatrix.h"
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <limits>

// ─── ESTRUCTURA DE CABECERAS ──────────────────────────────────
RowHeader* SparseMatrix::getRowHeader(int row) const {
    RowHeader* cur = firstRow;
    while (cur && cur->row <= row) {
        if (cur->row == row) return cur;
        cur = cur->next;
    }
    return nullptr;
}

ColHeader* SparseMatrix::getColHeader(int col) const {
    ColHeader* cur = firstCol;
    while (cur && cur->col <= col) {
        if (cur->col == col) return cur;
        cur = cur->next;
    }
    return nullptr;
}

RowHeader* SparseMatrix::getOrCreateRowHeader(int row) {
    if (!firstRow || firstRow->row > row) {
        RowHeader* rh = new RowHeader(row);
        rh->next = firstRow;
        firstRow = rh;
        return rh;
    }
    RowHeader* cur = firstRow;
    while (cur->next && cur->next->row <= row) {
        cur = cur->next;
    }
    if (cur->row == row) return cur;
    RowHeader* rh = new RowHeader(row);
    rh->next = cur->next;
    cur->next = rh;
    return rh;
}

ColHeader* SparseMatrix::getOrCreateColHeader(int col) {
    if (!firstCol || firstCol->col > col) {
        ColHeader* ch = new ColHeader(col);
        ch->next = firstCol;
        firstCol = ch;
        return ch;
    }
    ColHeader* cur = firstCol;
    while (cur->next && cur->next->col <= col) {
        cur = cur->next;
    }
    if (cur->col == col) return cur;
    ColHeader* ch = new ColHeader(col);
    ch->next = cur->next;
    cur->next = ch;
    return ch;
}


// ─── Destructor ───────────────────────────────────────────────
SparseMatrix::~SparseMatrix() {
    clearAll();

    // Eliminar headers
    RowHeader* rcur = firstRow;
    while (rcur) {
        RowHeader* next = rcur->next;
        delete rcur;
        rcur = next;
    }
    firstRow = nullptr;

    ColHeader* ccur = firstCol;
    while (ccur) {
        ColHeader* next = ccur->next;
        delete ccur;
        ccur = next;
    }
    firstCol = nullptr;
}

// ─── FIND (auxiliar privado) ──────────────────────────────────
Node* SparseMatrix::findNode(int row, int col) const {
    RowHeader* rh = getRowHeader(row);
    if (!rh) return nullptr;
    Node* cur = rh->firstCell;
    while (cur && cur->col <= col) {
        if (cur->col == col) return cur;
        cur = cur->nextInRow;
    }
    return nullptr;
}


// ─── INSERT ───────────────────────────────────────────────────
void SparseMatrix::insert(int row, int col, const std::string& value) {
    // 1. Si ya existe el nodo, solo modificamos
    Node* existing = findNode(row, col);
    if (existing) {
        existing->value = value;
        return;
    }

    Node* newNode = new Node(row, col, value);

    // 2. Enlazar en la fila (orden por col)
    RowHeader* rHead = getOrCreateRowHeader(row);
    if (!rHead->firstCell || rHead->firstCell->col > col) {
        newNode->nextInRow = rHead->firstCell;
        rHead->firstCell = newNode;
    } else {
        Node* prev = rHead->firstCell;
        while (prev->nextInRow && prev->nextInRow->col < col)
            prev = prev->nextInRow;
        newNode->nextInRow = prev->nextInRow;
        prev->nextInRow = newNode;
    }

    // 3. Enlazar en la columna (orden por row)
    ColHeader* cHead = getOrCreateColHeader(col);
    if (!cHead->firstCell || cHead->firstCell->row > row) {
        newNode->nextInCol = cHead->firstCell;
        cHead->firstCell = newNode;
    } else {
        Node* prev = cHead->firstCell;
        while (prev->nextInCol && prev->nextInCol->row < row)
            prev = prev->nextInCol;
        newNode->nextInCol = prev->nextInCol;
        prev->nextInCol = newNode;
    }
}

// ─── QUERY ────────────────────────────────────────────────────
std::string SparseMatrix::query(int row, int col) const {
    Node* node = findNode(row, col);
    return node ? node->value : "";   // vacío si no existe
}

// ─── MODIFY ───────────────────────────────────────────────────
void SparseMatrix::modify(int row, int col, const std::string& value) {
    Node* node = findNode(row, col);
    if (node) node->value = value;
    // Si no existe, no creamos nodo nuevo (diferencia con insert)
}

// ─── DELETE CELL ──────────────────────────────────────────────
void SparseMatrix::deleteCell(int row, int col) {
    // 1. Desenlazar de la fila
    RowHeader* rh = getRowHeader(row);
    if (!rh || !rh->firstCell) return;

    Node* target = nullptr;

    if (rh->firstCell->col == col) {
        target = rh->firstCell;
        rh->firstCell = target->nextInRow;
    } else {
        Node* prev = rh->firstCell;
        while (prev->nextInRow && prev->nextInRow->col != col)
            prev = prev->nextInRow;
        if (!prev->nextInRow) return; // no existe
        target = prev->nextInRow;
        prev->nextInRow = target->nextInRow;
    }

    // 2. Desenlazar de la columna
    ColHeader* ch = getColHeader(col);
    if (ch && ch->firstCell) {
        if (ch->firstCell->row == row) {
            ch->firstCell = target->nextInCol;
        } else {
            Node* prev = ch->firstCell;
            while (prev->nextInCol && prev->nextInCol->row != row)
                prev = prev->nextInCol;
            if (prev->nextInCol) prev->nextInCol = target->nextInCol;
        }
    }

    delete target;
}

// ─── VISUALIZACIÓN ────────────────────────────────────────────
std::vector<Node*> SparseMatrix::getAllNodes() const {
    std::vector<Node*> result;
    RowHeader* curRow = firstRow;
    while (curRow) {
        Node* cur = curRow->firstCell;
        while (cur) {
            result.push_back(cur);
            cur = cur->nextInRow;
        }
        curRow = curRow->next;
    }
    return result;
}

std::vector<Node*> SparseMatrix::getRow(int row) const {
    std::vector<Node*> result;
    RowHeader* rh = getRowHeader(row);
    if (!rh) return result;
    Node* cur = rh->firstCell;
    while (cur) {
        result.push_back(cur);
        cur = cur->nextInRow;
    }
    return result;
}

std::vector<Node*> SparseMatrix::getCol(int col) const {
    std::vector<Node*> result;
    ColHeader* ch = getColHeader(col);
    if (!ch) return result;
    Node* cur = ch->firstCell;
    while (cur) {
        result.push_back(cur);
        cur = cur->nextInCol;
    }
    return result;
}


// ─── DELETE ROW ───────────────────────────────────────────────
void SparseMatrix::deleteRow(int row) {
    RowHeader* rh = getRowHeader(row);
    if (!rh) return; // fila vacía, nada que hacer

    // 1. Recolectar todas las columnas ocupadas en esta fila
    std::vector<int> cols;
    Node* cur = rh->firstCell;
    while (cur) {
        cols.push_back(cur->col);
        cur = cur->nextInRow;
    }

    // 2. Eliminar cada celda
    for (int col : cols)
        deleteCell(row, col);
}

// ─── DELETE COLUMN ────────────────────────────────────────────
void SparseMatrix::deleteCol(int col) {
    ColHeader* ch = getColHeader(col);
    if (!ch) return;

    // 1. Recolectar todas las filas ocupadas en esta columna
    std::vector<int> rows;
    Node* cur = ch->firstCell;
    while (cur) {
        rows.push_back(cur->row);
        cur = cur->nextInCol;
    }

    // 2. Eliminar cada celda
    for (int row : rows)
        deleteCell(row, col);
}

// ─── DELETE RANGE ─────────────────────────────────────────────
void SparseMatrix::deleteRange(int r1, int c1, int r2, int c2) {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    std::vector<std::pair<int,int>> targets;
    RowHeader* rcur = firstRow;
    while (rcur) {
        if (rcur->row >= r1 && rcur->row <= r2) {
            Node* cur = rcur->firstCell;
            while (cur && cur->col <= c2) {
                if (cur->col >= c1) targets.push_back({cur->row, cur->col});
                cur = cur->nextInRow;
            }
        }
        rcur = rcur->next;
    }

    for (auto& [row, col] : targets)
        deleteCell(row, col);
}

// ─── HELPER: intenta convertir string a double ─────────────────
bool SparseMatrix::toDouble(const std::string& val, double& out) const {
    try {
        size_t pos;
        out = std::stod(val, &pos);
        return pos == val.size(); // asegura que todo el string es numérico
    } catch (...) {
        return false;
    }
}

// ─── SUMA FILA ────────────────────────────────────────────────
double SparseMatrix::sumRow(int row) const {
    double total = 0.0;
    for (Node* n : getRow(row)) {
        double v;
        if (toDouble(n->value, v)) total += v;
    }
    return total;
}

// ─── SUMA COLUMNA ─────────────────────────────────────────────
double SparseMatrix::sumCol(int col) const {
    double total = 0.0;
    for (Node* n : getCol(col)) {
        double v;
        if (toDouble(n->value, v)) total += v;
    }
    return total;
}

// ─── SUMA RANGO ───────────────────────────────────────────────
double SparseMatrix::sumRange(int r1, int c1, int r2, int c2) const {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    double total = 0.0;
    RowHeader* rcur = firstRow;
    while (rcur) {
        if (rcur->row >= r1 && rcur->row <= r2) {
            Node* cur = rcur->firstCell;
            while (cur && cur->col <= c2) {
                if (cur->col >= c1) {
                    double v;
                    if (toDouble(cur->value, v)) total += v;
                }
                cur = cur->nextInRow;
            }
        }
        rcur = rcur->next;
    }
    return total;
}

// ─── PROMEDIO RANGO ───────────────────────────────────────────
double SparseMatrix::avgRange(int r1, int c1, int r2, int c2) const {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    double total = 0.0;
    int count = 0;
    RowHeader* rcur = firstRow;
    while (rcur) {
        if (rcur->row >= r1 && rcur->row <= r2) {
            Node* cur = rcur->firstCell;
            while (cur && cur->col <= c2) {
                if (cur->col >= c1) {
                    double v;
                    if (toDouble(cur->value, v)) {
                        total += v;
                        count++;
                    }
                }
                cur = cur->nextInRow;
            }
        }
        rcur = rcur->next;
    }
    return count > 0 ? total / count : 0.0;
}

// ─── MÁXIMO RANGO ─────────────────────────────────────────────
double SparseMatrix::maxRange(int r1, int c1, int r2, int c2) const {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    double result = std::numeric_limits<double>::lowest();
    bool found = false;
    RowHeader* rcur = firstRow;
    while (rcur) {
        if (rcur->row >= r1 && rcur->row <= r2) {
            Node* cur = rcur->firstCell;
            while (cur && cur->col <= c2) {
                if (cur->col >= c1) {
                    double v;
                    if (toDouble(cur->value, v)) {
                        result = std::max(result, v);
                        found = true;
                    }
                }
                cur = cur->nextInRow;
            }
        }
        rcur = rcur->next;
    }
    return found ? result : 0.0;
}

// ─── MÍNIMO RANGO ─────────────────────────────────────────────
double SparseMatrix::minRange(int r1, int c1, int r2, int c2) const {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    double result = std::numeric_limits<double>::max();
    bool found = false;
    RowHeader* rcur = firstRow;
    while (rcur) {
        if (rcur->row >= r1 && rcur->row <= r2) {
            Node* cur = rcur->firstCell;
            while (cur && cur->col <= c2) {
                if (cur->col >= c1) {
                    double v;
                    if (toDouble(cur->value, v)) {
                        result = std::min(result, v);
                        found = true;
                    }
                }
                cur = cur->nextInRow;
            }
        }
        rcur = rcur->next;
    }
    return found ? result : 0.0;
}

// ─── SNAPSHOT / CLEAR ─────────────────────────────────────────
void SparseMatrix::clearAll() {
    std::vector<std::pair<int, int>> coords;
    for (Node* n : getAllNodes())
        coords.push_back({n->row, n->col});
    for (auto& p : coords)
        deleteCell(p.first, p.second);
}

std::vector<std::pair<std::pair<int, int>, std::string>> SparseMatrix::snapshotCells() const {
    std::vector<std::pair<std::pair<int, int>, std::string>> out;
    for (Node* n : getAllNodes())
        out.push_back({{n->row, n->col}, n->value});
    return out;
}

void SparseMatrix::restoreSnapshot(const std::vector<std::pair<std::pair<int, int>, std::string>>& snap) {
    clearAll();
    for (const auto& kv : snap)
        insert(kv.first.first, kv.first.second, kv.second);
}
