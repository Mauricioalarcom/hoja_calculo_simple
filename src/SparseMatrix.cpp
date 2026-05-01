#include "SparseMatrix.h"
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <limits>

// ─── Destructor ───────────────────────────────────────────────
SparseMatrix::~SparseMatrix() {
    // Recorremos todas las filas y liberamos cada nodo
    for (auto& [row, head] : rowHeads) {
        Node* cur = head;
        while (cur) {
            Node* next = cur->nextInRow;
            delete cur;
            cur = next;
        }
    }
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
    if (rowHeads.find(row) == rowHeads.end() || rowHeads[row]->col > col) {
        // Va al inicio de la fila
        newNode->nextInRow = rowHeads.count(row) ? rowHeads[row] : nullptr;
        rowHeads[row] = newNode;
    } else {
        Node* prev = rowHeads[row];
        while (prev->nextInRow && prev->nextInRow->col < col)
            prev = prev->nextInRow;
        newNode->nextInRow = prev->nextInRow;
        prev->nextInRow = newNode;
    }

    // 3. Enlazar en la columna (orden por row)
    if (colHeads.find(col) == colHeads.end() || colHeads[col]->row > row) {
        // Va al inicio de la columna
        newNode->nextInCol = colHeads.count(col) ? colHeads[col] : nullptr;
        colHeads[col] = newNode;
    } else {
        Node* prev = colHeads[col];
        while (prev->nextInCol && prev->nextInCol->row < row)
            prev = prev->nextInCol;
        newNode->nextInCol = prev->nextInCol;
        prev->nextInCol = newNode;
    }
}

// ─── FIND (auxiliar privado) ──────────────────────────────────
// Agrega esto al header en la sección private:
//   Node* findNode(int row, int col) const;
Node* SparseMatrix::findNode(int row, int col) const {
    auto it = rowHeads.find(row);
    if (it == rowHeads.end()) return nullptr;
    Node* cur = it->second;
    while (cur && cur->col <= col) {
        if (cur->col == col) return cur;
        cur = cur->nextInRow;
    }
    return nullptr;
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
    auto rowIt = rowHeads.find(row);
    if (rowIt == rowHeads.end()) return;  // fila vacía, nada que hacer

    Node* target = nullptr;

    if (rowIt->second->col == col) {
        // Es la cabeza de la fila
        target = rowIt->second;
        rowIt->second = target->nextInRow;
        if (!rowIt->second) rowHeads.erase(rowIt); // fila quedó vacía
    } else {
        Node* prev = rowIt->second;
        while (prev->nextInRow && prev->nextInRow->col != col)
            prev = prev->nextInRow;
        if (!prev->nextInRow) return; // no existe
        target = prev->nextInRow;
        prev->nextInRow = target->nextInRow;
    }

    // 2. Desenlazar de la columna
    auto colIt = colHeads.find(col);
    if (colIt->second->row == row) {
        colIt->second = target->nextInCol;
        if (!colIt->second) colHeads.erase(colIt); // columna quedó vacía
    } else {
        Node* prev = colIt->second;
        while (prev->nextInCol && prev->nextInCol->row != row)
            prev = prev->nextInCol;
        if (prev->nextInCol) prev->nextInCol = target->nextInCol;
    }

    delete target;
}

// ─── VISUALIZACIÓN ────────────────────────────────────────────
std::vector<Node*> SparseMatrix::getAllNodes() const {
    std::vector<Node*> result;
    for (auto& [row, head] : rowHeads) {
        Node* cur = head;
        while (cur) {
            result.push_back(cur);
            cur = cur->nextInRow;
        }
    }
    return result;
}

std::vector<Node*> SparseMatrix::getRow(int row) const {
    std::vector<Node*> result;
    auto it = rowHeads.find(row);
    if (it == rowHeads.end()) return result;
    Node* cur = it->second;
    while (cur) {
        result.push_back(cur);
        cur = cur->nextInRow;
    }
    return result;
}

std::vector<Node*> SparseMatrix::getCol(int col) const {
    std::vector<Node*> result;
    auto it = colHeads.find(col);
    if (it == colHeads.end()) return result;
    Node* cur = it->second;
    while (cur) {
        result.push_back(cur);
        cur = cur->nextInCol;
    }
    return result;
}


// ─── DELETE ROW ───────────────────────────────────────────────
void SparseMatrix::deleteRow(int row) {
    auto rowIt = rowHeads.find(row);
    if (rowIt == rowHeads.end()) return; // fila vacía, nada que hacer

    // 1. Recolectar todas las columnas ocupadas en esta fila
    std::vector<int> cols;
    Node* cur = rowIt->second;
    while (cur) {
        cols.push_back(cur->col);
        cur = cur->nextInRow;
    }

    // 2. Eliminar cada celda (desenlaza de columna y libera memoria)
    for (int col : cols)
        deleteCell(row, col);
    // rowHeads[row] ya fue borrado por el último deleteCell
}

// ─── DELETE COLUMN ────────────────────────────────────────────
void SparseMatrix::deleteCol(int col) {
    auto colIt = colHeads.find(col);
    if (colIt == colHeads.end()) return; // columna vacía, nada que hacer

    // 1. Recolectar todas las filas ocupadas en esta columna
    std::vector<int> rows;
    Node* cur = colIt->second;
    while (cur) {
        rows.push_back(cur->row);
        cur = cur->nextInCol;
    }

    // 2. Eliminar cada celda
    for (int row : rows)
        deleteCell(row, col);
    // colHeads[col] ya fue borrado por el último deleteCell
}

// ─── DELETE RANGE ─────────────────────────────────────────────
void SparseMatrix::deleteRange(int r1, int c1, int r2, int c2) {
    // Normalizar por si vienen invertidos (ej. r2 < r1)
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    // 1. Recolectar todas las celdas dentro del rango
    std::vector<std::pair<int,int>> targets;

    for (auto& [row, head] : rowHeads) {
        if (row < r1 || row > r2) continue;
        Node* cur = head;
        while (cur) {
            if (cur->col >= c1 && cur->col <= c2)
                targets.push_back({cur->row, cur->col});
            cur = cur->nextInRow;
        }
    }

    // 2. Eliminar todas las celdas recolectadas
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
    for (auto& [row, head] : rowHeads) {
        if (row < r1 || row > r2) continue;
        Node* cur = head;
        while (cur) {
            if (cur->col >= c1 && cur->col <= c2) {
                double v;
                if (toDouble(cur->value, v)) total += v;
            }
            cur = cur->nextInRow;
        }
    }
    return total;
}

// ─── PROMEDIO RANGO ───────────────────────────────────────────
double SparseMatrix::avgRange(int r1, int c1, int r2, int c2) const {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    double total = 0.0;
    int count = 0;
    for (auto& [row, head] : rowHeads) {
        if (row < r1 || row > r2) continue;
        Node* cur = head;
        while (cur) {
            if (cur->col >= c1 && cur->col <= c2) {
                double v;
                if (toDouble(cur->value, v)) {
                    total += v;
                    count++;
                }
            }
            cur = cur->nextInRow;
        }
    }
    // Caso borde: ninguna celda numérica en el rango
    return count > 0 ? total / count : 0.0;
}

// ─── MÁXIMO RANGO ─────────────────────────────────────────────
double SparseMatrix::maxRange(int r1, int c1, int r2, int c2) const {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    double result = std::numeric_limits<double>::lowest();
    bool found = false;
    for (auto& [row, head] : rowHeads) {
        if (row < r1 || row > r2) continue;
        Node* cur = head;
        while (cur) {
            if (cur->col >= c1 && cur->col <= c2) {
                double v;
                if (toDouble(cur->value, v)) {
                    result = std::max(result, v);
                    found = true;
                }
            }
            cur = cur->nextInRow;
        }
    }
    return found ? result : 0.0;
}

// ─── MÍNIMO RANGO ─────────────────────────────────────────────
double SparseMatrix::minRange(int r1, int c1, int r2, int c2) const {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);

    double result = std::numeric_limits<double>::max();
    bool found = false;
    for (auto& [row, head] : rowHeads) {
        if (row < r1 || row > r2) continue;
        Node* cur = head;
        while (cur) {
            if (cur->col >= c1 && cur->col <= c2) {
                double v;
                if (toDouble(cur->value, v)) {
                    result = std::min(result, v);
                    found = true;
                }
            }
            cur = cur->nextInRow;
        }
    }
    return found ? result : 0.0;
}