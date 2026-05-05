#include "FormulaEvaluator.h"
#include "CellRef.h"
#include <cctype>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <string>

namespace {

const char* kDiv = "#¡DIV/0!";
const char* kRef = "#¡REF!";
const char* kVal = "#¡VALOR!";

std::string trim(const std::string& v) {
    size_t a = 0, b = v.size();
    while (a < b && std::isspace(static_cast<unsigned char>(v[a]))) a++;
    while (b > a && std::isspace(static_cast<unsigned char>(v[b - 1]))) b--;
    return v.substr(a, b - a);
}

bool isErrorToken(const std::string& s) {
    return !s.empty() && s[0] == '#';
}

bool scanCellToken(const std::string& s, size_t start, size_t& endOut, cellref::CellAddress& a) {
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
    return cellref::parseCellAddress(tok, a);
}

std::vector<std::string> splitArgs(const std::string& inner) {
    std::vector<std::string> out;
    int depth = 0;
    size_t start = 0;
    for (size_t i = 0; i < inner.size(); i++) {
        char ch = inner[i];
        if (ch == '(') depth++;
        else if (ch == ')') depth--;
        else if (ch == ',' && depth == 0) {
            out.push_back(trim(inner.substr(start, i - start)));
            start = i + 1;
        }
    }
    out.push_back(trim(inner.substr(start)));
    return out;
}

std::string cellDisplayRec(const SparseMatrix& m, int row, int col, std::set<std::pair<int, int>>& vis);

double cellNumericRec(const SparseMatrix& m, int row, int col, std::set<std::pair<int, int>>& vis) {
    std::string d = cellDisplayRec(m, row, col, vis);
    if (d.empty()) return 0.0;
    if (isErrorToken(d)) return std::numeric_limits<double>::quiet_NaN();
    try {
        size_t idx = 0;
        double v = std::stod(d, &idx);
        if (idx != d.size()) return std::numeric_limits<double>::quiet_NaN();
        return v;
    } catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

struct Parser {
    const SparseMatrix& m;
    std::set<std::pair<int, int>>& vis;
    std::string s;
    size_t pos = 0;

    Parser(const SparseMatrix& matrix, std::set<std::pair<int, int>>& v, std::string expr)
        : m(matrix), vis(v), s(std::move(expr)) {}

    void skipSpaces() {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) pos++;
    }

    double parseExpression();
    double parseComparison();

private:
    double parseAddSub();
    double parseMulDiv();
    double parseUnary();
    double parsePrimary();
    double parseNumber();
    double parseCellRefAt(size_t startPos, size_t endPos);
    double parseFunction(const std::string& name, const std::string& inner);
};

double Parser::parseNumber() {
    skipSpaces();
    size_t start = pos;
    if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) pos++;
    bool any = false;
    while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
        any = true;
        pos++;
    }
    if (pos < s.size() && s[pos] == '.') {
        pos++;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
            any = true;
            pos++;
        }
    }
    if (!any) throw std::runtime_error(std::string(kVal));
    std::string tok = s.substr(start, pos - start);
    try {
        return std::stod(tok);
    } catch (...) {
        throw std::runtime_error(std::string(kVal));
    }
}

double Parser::parseCellRefAt(size_t startPos, size_t endPos) {
    std::string tok = s.substr(startPos, endPos - startPos);
    int r = 0, c = 0;
    if (tok.find(':') != std::string::npos) {
        int r1, c1, r2, c2;
        if (!cellref::parseRangeRef(tok, r1, c1, r2, c2)) throw std::runtime_error(std::string(kVal));
        if (r1 > r2) std::swap(r1, r2);
        if (c1 > c2) std::swap(c1, c2);
        double sum = 0;
        for (int rr = r1; rr <= r2; rr++)
            for (int cc = c1; cc <= c2; cc++)
                sum += cellNumericRec(m, rr, cc, vis);
        return sum;
    }
    if (!cellref::parseCellRef(tok, r, c)) throw std::runtime_error(std::string(kVal));
    return cellNumericRec(m, r, c, vis);
}

double aggregateSumRect(const SparseMatrix& m, int r1, int c1, int r2, int c2,
                        std::set<std::pair<int, int>>& vis) {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);
    double sum = 0;
    for (int rr = r1; rr <= r2; rr++)
        for (int cc = c1; cc <= c2; cc++) {
            double v = cellNumericRec(m, rr, cc, vis);
            if (!std::isnan(v)) sum += v;
        }
    return sum;
}

double aggregateAvgRect(const SparseMatrix& m, int r1, int c1, int r2, int c2,
                        std::set<std::pair<int, int>>& vis) {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);
    double sum = 0;
    int n = 0;
    for (int rr = r1; rr <= r2; rr++)
        for (int cc = c1; cc <= c2; cc++) {
            double v = cellNumericRec(m, rr, cc, vis);
            if (!std::isnan(v)) {
                sum += v;
                n++;
            }
        }
    return n > 0 ? sum / n : 0.0;
}

double aggregateMaxRect(const SparseMatrix& m, int r1, int c1, int r2, int c2,
                        std::set<std::pair<int, int>>& vis) {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);
    double mx = std::numeric_limits<double>::lowest();
    bool any = false;
    for (int rr = r1; rr <= r2; rr++)
        for (int cc = c1; cc <= c2; cc++) {
            double v = cellNumericRec(m, rr, cc, vis);
            if (!std::isnan(v)) {
                mx = std::max(mx, v);
                any = true;
            }
        }
    return any ? mx : 0.0;
}

double aggregateMinRect(const SparseMatrix& m, int r1, int c1, int r2, int c2,
                        std::set<std::pair<int, int>>& vis) {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);
    double mn = std::numeric_limits<double>::max();
    bool any = false;
    for (int rr = r1; rr <= r2; rr++)
        for (int cc = c1; cc <= c2; cc++) {
            double v = cellNumericRec(m, rr, cc, vis);
            if (!std::isnan(v)) {
                mn = std::min(mn, v);
                any = true;
            }
        }
    return any ? mn : 0.0;
}

double aggregateCountRect(const SparseMatrix& m, int r1, int c1, int r2, int c2,
                          std::set<std::pair<int, int>>& vis) {
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);
    int n = 0;
    for (int rr = r1; rr <= r2; rr++)
        for (int cc = c1; cc <= c2; cc++) {
            double v = cellNumericRec(m, rr, cc, vis);
            if (!std::isnan(v)) n++;
        }
    return static_cast<double>(n);
}

double evalRangeOrExpr(const SparseMatrix& m, const std::string& arg,
                       std::set<std::pair<int, int>>& vis) {
    std::string t = trim(arg);
    int r1, c1, r2, c2;
    if (t.find(':') != std::string::npos && cellref::parseRangeRef(t, r1, c1, r2, c2))
        return aggregateSumRect(m, r1, c1, r2, c2, vis);
    Parser p(m, vis, t);
    return p.parseExpression();
}

double Parser::parseFunction(const std::string& name, const std::string& inner) {
    std::string up = name;
    for (char& ch : up) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

    auto args = splitArgs(inner);

    if (up == "SUMA" || up == "SUM") {
        double sum = 0;
        for (const auto& a : args) {
            if (a.empty()) continue;
            sum += evalRangeOrExpr(m, a, vis);
        }
        return sum;
    }
    if (up == "PROMEDIO" || up == "PROM" || up == "AVERAGE") {
        if (args.size() != 1) throw std::runtime_error(std::string(kVal));
        std::string t = trim(args[0]);
        int r1, c1, r2, c2;
        if (t.find(':') != std::string::npos && cellref::parseRangeRef(t, r1, c1, r2, c2))
            return aggregateAvgRect(m, r1, c1, r2, c2, vis);
        throw std::runtime_error(std::string(kVal));
    }
    if (up == "MAX") {
        if (args.size() != 1) throw std::runtime_error(std::string(kVal));
        std::string t = trim(args[0]);
        int r1, c1, r2, c2;
        if (t.find(':') != std::string::npos && cellref::parseRangeRef(t, r1, c1, r2, c2))
            return aggregateMaxRect(m, r1, c1, r2, c2, vis);
        throw std::runtime_error(std::string(kVal));
    }
    if (up == "MIN") {
        if (args.size() != 1) throw std::runtime_error(std::string(kVal));
        std::string t = trim(args[0]);
        int r1, c1, r2, c2;
        if (t.find(':') != std::string::npos && cellref::parseRangeRef(t, r1, c1, r2, c2))
            return aggregateMinRect(m, r1, c1, r2, c2, vis);
        throw std::runtime_error(std::string(kVal));
    }
    if (up == "CONTAR" || up == "COUNT") {
        if (args.size() != 1) throw std::runtime_error(std::string(kVal));
        std::string t = trim(args[0]);
        int r1, c1, r2, c2;
        if (t.find(':') != std::string::npos && cellref::parseRangeRef(t, r1, c1, r2, c2))
            return aggregateCountRect(m, r1, c1, r2, c2, vis);
        throw std::runtime_error(std::string(kVal));
    }
    if (up == "SI" || up == "IF") {
        if (args.size() != 3) throw std::runtime_error(std::string(kVal));
        Parser pc(m, vis, trim(args[0]));
        double cond = pc.parseExpression();
        Parser pt(m, vis, trim(args[1]));
        Parser pf(m, vis, trim(args[2]));
        bool truth = (cond != 0.0 && !std::isnan(cond));
        return truth ? pt.parseExpression() : pf.parseExpression();
    }

    throw std::runtime_error(std::string(kVal));
}

double Parser::parsePrimary() {
    skipSpaces();
    if (pos >= s.size()) throw std::runtime_error(std::string(kVal));

    if (s[pos] == '(') {
        pos++;
        double v = parseExpression();
        skipSpaces();
        if (pos >= s.size() || s[pos] != ')') throw std::runtime_error(std::string(kVal));
        pos++;
        return v;
    }

    if (std::isdigit(static_cast<unsigned char>(s[pos])) ||
        (s[pos] == '.' && pos + 1 < s.size() &&
         std::isdigit(static_cast<unsigned char>(s[pos + 1]))))
        return parseNumber();

    if (s[pos] == '$' || std::isalpha(static_cast<unsigned char>(s[pos]))) {
        size_t save = pos;
        cellref::CellAddress addr{};
        size_t endTok = pos;
        if (scanCellToken(s, pos, endTok, addr)) {
            if (endTok < s.size() && s[endTok] == ':') {
                cellref::CellAddress a2{};
                size_t end2 = endTok + 1;
                size_t end2f = end2;
                if (scanCellToken(s, end2, end2f, a2)) {
                    double v = parseCellRefAt(save, end2f);
                    pos = end2f;
                    return v;
                }
            }
            double v = parseCellRefAt(save, endTok);
            pos = endTok;
            return v;
        }

        pos = save;
        std::string ident;
        while (pos < s.size() && std::isalpha(static_cast<unsigned char>(s[pos])))
            ident.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(s[pos++]))));
        skipSpaces();
        if (pos < s.size() && s[pos] == '(') {
            pos++;
            int depth = 1;
            size_t innerStart = pos;
            while (pos < s.size() && depth > 0) {
                if (s[pos] == '(') depth++;
                else if (s[pos] == ')') depth--;
                pos++;
            }
            if (depth != 0) throw std::runtime_error(std::string(kVal));
            size_t innerEnd = pos - 1;
            std::string inner = s.substr(innerStart, innerEnd - innerStart);
            return parseFunction(ident, inner);
        }

        pos = save;
        cellref::CellAddress a{};
        size_t startTok = pos;
        size_t endp = pos;
        if (!scanCellToken(s, startTok, endp, a)) throw std::runtime_error(std::string(kVal));
        double v = parseCellRefAt(startTok, endp);
        pos = endp;
        return v;
    }

    throw std::runtime_error(std::string(kVal));
}

double Parser::parseUnary() {
    skipSpaces();
    if (pos < s.size() && s[pos] == '-') {
        pos++;
        return -parseUnary();
    }
    if (pos < s.size() && s[pos] == '+') {
        pos++;
        return parseUnary();
    }
    return parsePrimary();
}

double Parser::parseMulDiv() {
    double left = parseUnary();
    for (;;) {
        skipSpaces();
        if (pos >= s.size()) break;
        char op = s[pos];
        if (op != '*' && op != '/') break;
        pos++;
        double right = parseUnary();
        if (op == '*')
            left *= right;
        else {
            if (right == 0.0) throw std::runtime_error(std::string(kDiv));
            left /= right;
        }
    }
    return left;
}

double Parser::parseAddSub() {
    double left = parseMulDiv();
    for (;;) {
        skipSpaces();
        if (pos >= s.size()) break;
        char op = s[pos];
        if (op != '+' && op != '-') break;
        pos++;
        double right = parseMulDiv();
        if (op == '+')
            left += right;
        else
            left -= right;
    }
    return left;
}

double Parser::parseComparison() {
    double left = parseAddSub();
    skipSpaces();
    if (pos >= s.size()) return left;

    std::string op;
    if (s.compare(pos, 2, ">=") == 0)
        op = ">=";
    else if (s.compare(pos, 2, "<=") == 0)
        op = "<=";
    else if (s.compare(pos, 2, "<>") == 0)
        op = "<>";
    else if (s[pos] == '>')
        op = ">";
    else if (s[pos] == '<')
        op = "<";
    else if (s[pos] == '=')
        op = "=";
    else
        return left;

    pos += op.size();
    double right = parseAddSub();

    if (op == ">") return left > right ? 1.0 : 0.0;
    if (op == "<") return left < right ? 1.0 : 0.0;
    if (op == ">=") return left >= right ? 1.0 : 0.0;
    if (op == "<=") return left <= right ? 1.0 : 0.0;
    if (op == "=") return left == right ? 1.0 : 0.0;
    return left != right ? 1.0 : 0.0;
}

double Parser::parseExpression() {
    double v = parseComparison();
    skipSpaces();
    if (pos < s.size()) throw std::runtime_error(std::string(kVal));
    return v;
}

std::string cellDisplayRec(const SparseMatrix& m, int row, int col, std::set<std::pair<int, int>>& vis) {
    std::string raw = m.query(row, col);
    if (raw.empty()) return "";

    if (raw[0] != '=') return raw;

    auto key = std::make_pair(row, col);
    auto [it, inserted] = vis.insert(key);
    if (!inserted) return kRef;

    std::string expr = trim(raw.substr(1));
    std::string result;
    try {
        Parser p(m, vis, expr);
        double v = p.parseExpression();
        result = FormulaEvaluator::formatNumber(v);
    } catch (const std::exception& e) {
        result = e.what();
        if (result != kDiv && result != kRef && result != kVal)
            result = kVal;
    }
    vis.erase(it);
    return result;
}

} // namespace

std::string FormulaEvaluator::cellDisplay(const SparseMatrix& m, int row, int col) {
    std::set<std::pair<int, int>> vis;
    return cellDisplayRec(m, row, col, vis);
}

double FormulaEvaluator::cellNumeric(const SparseMatrix& m, int row, int col) {
    std::set<std::pair<int, int>> vis;
    return cellNumericRec(m, row, col, vis);
}

double FormulaEvaluator::aggregateSum(const SparseMatrix& m, int r1, int c1, int r2, int c2) {
    std::set<std::pair<int, int>> vis;
    return aggregateSumRect(m, r1, c1, r2, c2, vis);
}

double FormulaEvaluator::aggregateAvg(const SparseMatrix& m, int r1, int c1, int r2, int c2) {
    std::set<std::pair<int, int>> vis;
    return aggregateAvgRect(m, r1, c1, r2, c2, vis);
}

double FormulaEvaluator::aggregateMax(const SparseMatrix& m, int r1, int c1, int r2, int c2) {
    std::set<std::pair<int, int>> vis;
    return aggregateMaxRect(m, r1, c1, r2, c2, vis);
}

double FormulaEvaluator::aggregateMin(const SparseMatrix& m, int r1, int c1, int r2, int c2) {
    std::set<std::pair<int, int>> vis;
    return aggregateMinRect(m, r1, c1, r2, c2, vis);
}

double FormulaEvaluator::aggregateCount(const SparseMatrix& m, int r1, int c1, int r2, int c2) {
    std::set<std::pair<int, int>> vis;
    return aggregateCountRect(m, r1, c1, r2, c2, vis);
}

std::string FormulaEvaluator::formatNumber(double value) {
    if (std::isnan(value)) return std::string(kVal);
    std::string str = std::to_string(value);
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    if (!str.empty() && str.back() == '.') str.pop_back();
    return str;
}
