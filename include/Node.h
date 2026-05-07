#pragma once
#include <string>

struct Node {
    int row, col;
    std::string value;
    Node* nextInRow;
    Node* nextInCol;

    Node(int r, int c, const std::string& v)
        : row(r), col(c), value(v),
          nextInRow(nullptr), nextInCol(nullptr) {}
};

struct RowHeader {
    int row;
    Node* firstCell;
    RowHeader* next;

    RowHeader(int r) : row(r), firstCell(nullptr), next(nullptr) {}
};

struct ColHeader {
    int col;
    Node* firstCell;
    ColHeader* next;

    ColHeader(int c) : col(c), firstCell(nullptr), next(nullptr) {}
};
