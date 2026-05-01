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