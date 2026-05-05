#pragma once
#include <string>

namespace cellref {

bool colLettersToIndex(const std::string& letters, int& colOut);
std::string colIndexToLetters(int col);

/// Parses A1, $A$1, A$1, $A1 (supports AA1). Returns false if invalid.
bool parseCellRef(const std::string& ref, int& rowOut, int& colOut);

struct CellAddress {
    int row = 0;
    int col = 0;
    bool colAbs = false;
    bool rowAbs = false;
};

bool parseCellAddress(const std::string& ref, CellAddress& out);
std::string formatCellAddress(const CellAddress& a);

/// Shift relative parts by (dRow, dCol); absolute refs unchanged.
CellAddress shiftAddress(const CellAddress& a, int dRow, int dCol);

/// Replaces cell references in a formula string (preserves $ locks).
std::string shiftFormulaRefs(const std::string& formulaBody, int dRow, int dCol);

/// Parses "A1:C4" or single cell as degenerate range.
bool parseRangeRef(const std::string& ref, int& r1, int& c1, int& r2, int& c2);

} // namespace cellref
