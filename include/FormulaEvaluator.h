#pragma once
#include "SparseMatrix.h"
#include <string>

/// Evaluates stored cell raw strings (=formula or literals). Thread-safe if each call uses own visitor set.
class FormulaEvaluator {
public:
    /// Display text for a cell (computed formula result or literal).
    static std::string cellDisplay(const SparseMatrix& m, int row, int col);

    /// Numeric value for aggregations (NaN if non-numeric / error).
    static double cellNumeric(const SparseMatrix& m, int row, int col);

    static double aggregateSum(const SparseMatrix& m, int r1, int c1, int r2, int c2);
    static double aggregateAvg(const SparseMatrix& m, int r1, int c1, int r2, int c2);
    static double aggregateMax(const SparseMatrix& m, int r1, int c1, int r2, int c2);
    static double aggregateMin(const SparseMatrix& m, int r1, int c1, int r2, int c2);
    static double aggregateCount(const SparseMatrix& m, int r1, int c1, int r2, int c2);

    static std::string formatNumber(double v);
};
