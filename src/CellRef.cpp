#include "CellRef.h"
#include <cctype>
#include <algorithm>

namespace cellref {

bool colLettersToIndex(const std::string& letters, int& colOut) {
    colOut = 0;
    if (letters.empty()) return false;
    for (char ch : letters) {
        if (ch < 'A' || ch > 'Z') return false;
        colOut = colOut * 26 + (ch - 'A' + 1);
    }
    colOut -= 1;
    return true;
}

std::string colIndexToLetters(int col) {
    std::string s;
    col++;
    while (col > 0) {
        col--;
        s = char('A' + (col % 26)) + s;
        col /= 26;
    }
    return s;
}

static bool parseOneCell(const std::string& ref, size_t start, size_t end,
                         int& rowOut, int& colOut) {
    if (start >= end) return false;
    size_t i = start;
    std::string colLetters;
    while (i < end && (ref[i] == '$' || std::isalpha(static_cast<unsigned char>(ref[i])))) {
        if (ref[i] != '$')
            colLetters.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ref[i]))));
        i++;
    }
    if (colLetters.empty() || i >= end) return false;
    std::string rowDigits;
    while (i < end && std::isdigit(static_cast<unsigned char>(ref[i])))
        rowDigits.push_back(ref[i++]);
    if (rowDigits.empty() || i != end) return false;
    int colIdx = 0;
    if (!colLettersToIndex(colLetters, colIdx)) return false;
    try {
        rowOut = std::stoi(rowDigits) - 1;
    } catch (...) {
        return false;
    }
    colOut = colIdx;
    return rowOut >= 0 && colOut >= 0;
}

bool parseCellRef(const std::string& ref, int& rowOut, int& colOut) {
    std::string t = ref;
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.front()))) t.erase(t.begin());
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.back()))) t.pop_back();
    return parseOneCell(t, 0, t.size(), rowOut, colOut);
}

bool parseCellAddress(const std::string& ref, CellAddress& out) {
    std::string t = ref;
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.front()))) t.erase(t.begin());
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.back()))) t.pop_back();
    if (t.empty()) return false;

    size_t i = 0;
    bool colAbs = false;
    if (t[i] == '$') {
        colAbs = true;
        i++;
    }
    std::string letters;
    while (i < t.size() && std::isalpha(static_cast<unsigned char>(t[i])))
        letters.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(t[i++]))));
    if (letters.empty()) return false;

    bool rowAbs = false;
    if (i < t.size() && t[i] == '$') {
        rowAbs = true;
        i++;
    }
    std::string digits;
    while (i < t.size() && std::isdigit(static_cast<unsigned char>(t[i])))
        digits.push_back(t[i++]);
    if (digits.empty() || i != t.size()) return false;

    int colIdx = 0;
    if (!colLettersToIndex(letters, colIdx)) return false;
    try {
        out.row = std::stoi(digits) - 1;
    } catch (...) {
        return false;
    }
    out.col = colIdx;
    out.colAbs = colAbs;
    out.rowAbs = rowAbs;
    return out.row >= 0 && out.col >= 0;
}

std::string formatCellAddress(const CellAddress& a) {
    std::string s;
    if (a.colAbs) s += '$';
    s += colIndexToLetters(a.col);
    if (a.rowAbs) s += '$';
    s += std::to_string(a.row + 1);
    return s;
}

CellAddress shiftAddress(const CellAddress& a, int dRow, int dCol) {
    CellAddress b = a;
    if (!b.colAbs) b.col += dCol;
    if (!b.rowAbs) b.row += dRow;
    return b;
}

std::string shiftFormulaRefs(const std::string& formulaBody, int dRow, int dCol) {
    std::string out;
    size_t i = 0;
    const std::string& s = formulaBody;

    auto scanOne = [&](size_t start, size_t& endOut, CellAddress& a) -> bool {
        endOut = start;
        if (start >= s.size()) return false;
        size_t j = start;
        if (s[j] == '$') j++;
        if (j >= s.size() || !std::isalpha(static_cast<unsigned char>(s[j]))) return false;
        while (j < s.size() && std::isalpha(static_cast<unsigned char>(s[j]))) j++;
        if (j < s.size() && s[j] == '$') j++;
        if (j >= s.size() || !std::isdigit(static_cast<unsigned char>(s[j]))) return false;
        while (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) j++;
        std::string tok = s.substr(start, j - start);
        endOut = j;
        return parseCellAddress(tok, a);
    };

    while (i < s.size()) {
        CellAddress a1{};
        size_t end1 = i;
        if (scanOne(i, end1, a1)) {
            if (end1 < s.size() && s[end1] == ':') {
                CellAddress a2{};
                size_t end2 = end1 + 1;
                size_t end2f = end2;
                if (scanOne(end2, end2f, a2)) {
                    out += formatCellAddress(shiftAddress(a1, dRow, dCol));
                    out += ':';
                    out += formatCellAddress(shiftAddress(a2, dRow, dCol));
                    i = end2f;
                    continue;
                }
            }
            out += formatCellAddress(shiftAddress(a1, dRow, dCol));
            i = end1;
            continue;
        }
        out.push_back(s[i]);
        ++i;
    }
    return out;
}

bool parseRangeRef(const std::string& ref, int& r1, int& c1, int& r2, int& c2) {
    auto sep = ref.find(':');
    if (sep == std::string::npos)
        return parseCellRef(ref, r1, c1) && (r2 = r1, c2 = c1, true);
    return parseCellRef(ref.substr(0, sep), r1, c1) &&
           parseCellRef(ref.substr(sep + 1), r2, c2);
}

} // namespace cellref
