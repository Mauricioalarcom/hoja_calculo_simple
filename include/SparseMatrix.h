#pragma once
#include "Node.h"
#include <string>
#include <vector>

class SparseMatrix {
private:
    RowHeader* firstRow = nullptr;
    ColHeader* firstCol = nullptr;

    RowHeader* getRowHeader(int row) const;
    ColHeader* getColHeader(int col) const;
    RowHeader* getOrCreateRowHeader(int row);
    ColHeader* getOrCreateColHeader(int col);

    Node* findNode(int row, int col) const; // auxiliar para buscar nodo
    bool toDouble(const std::string& val, double& out) const;

public:
    SparseMatrix() = default;
    ~SparseMatrix();

    // Fase 2
    void    insert(int row, int col, const std::string& value);
    std::string query(int row, int col) const;
    void    modify(int row, int col, const std::string& value);
    void    deleteCell(int row, int col);

    // Fase 3
    void    deleteRow(int row);
    void    deleteCol(int col);
    void    deleteRange(int r1, int c1, int r2, int c2);

    // Fase 4
    double  sumRow(int row) const;
    double  sumCol(int col) const;
    double  sumRange(int r1, int c1, int r2, int c2) const;
    double  avgRange(int r1, int c1, int r2, int c2) const;
    double  maxRange(int r1, int c1, int r2, int c2) const;
    double  minRange(int r1, int c1, int r2, int c2) const;

    // Visualización
    std::vector<Node*> getAllNodes() const;
    std::vector<Node*> getRow(int row) const;
    std::vector<Node*> getCol(int col) const;

    /// Vacía la hoja (todas las celdas).
    void clearAll();

    /// Copia completa para deshacer / portapapeles interno.
    std::vector<std::pair<std::pair<int, int>, std::string>> snapshotCells() const;
    void restoreSnapshot(const std::vector<std::pair<std::pair<int, int>, std::string>>& snap);
};