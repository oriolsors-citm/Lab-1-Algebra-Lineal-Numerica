#pragma once
#include "Dataset.hpp"
#include <limits>
#include <string>

double independentResidual(const Matrix& A, const Vector& x, const Vector& b);
bool checkResidualFunction(std::string& detail);

struct Report {
    Vector result;
    std::size_t resultRows = 0, resultCols = 0;
    Operations product, elimination, substitution;
    double milliseconds = 0, minMilliseconds = 0, maxMilliseconds = 0;
    double residual = std::numeric_limits<double>::quiet_NaN();
    double studentResidual = std::numeric_limits<double>::quiet_NaN();
    double error = std::numeric_limits<double>::quiet_NaN();
    bool correct = false, countsCorrect = false, timeExpected = false;
    bool residualCorrect = false, calibrationCorrect = false;
    bool completed = false, countsApplicable = false, timeApplicable = false;
    std::string detail, calibrationDetail;
};

Report runTask(const Dataset& data, bool pivoting);
