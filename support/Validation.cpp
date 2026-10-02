#include "Validation.hpp"
#include "Solver.hpp"
#include "Numerics.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

using Clock = std::chrono::steady_clock;
static double elapsed(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

static bool finite(const Vector& values) {
    return std::all_of(values.begin(), values.end(), [](double x) { return std::isfinite(x); });
}

static long double magnitude(const Vector& values) {
    long double result = 0;
    for (double value : values) result = std::hypot(result, static_cast<long double>(value));
    return result;
}

double independentResidual(const Matrix& A, const Vector& x, const Vector& b) {
    if (A.cols != x.size() || A.rows != b.size() || !finite(x) || !finite(b))
        return std::numeric_limits<double>::infinity();
    long double numerator = 0;
    for (std::size_t i = 0; i < A.rows; ++i) {
        long double value = -static_cast<long double>(b[i]);
        for (std::size_t j = 0; j < A.cols; ++j)
            value += static_cast<long double>(A.values[i * A.cols + j]) * x[j];
        numerator = std::hypot(numerator, value);
    }
    const long double denominator = magnitude(b);
    return static_cast<double>(denominator == 0 ? numerator : numerator / denominator);
}

static bool agrees(double found, double expected) {
    return std::isfinite(found) && std::isfinite(expected) && found >= 0
        && std::abs(found - expected) <= 1e-12 + 1e-6 * std::abs(expected);
}

bool checkResidualFunction(std::string& detail) {

    Matrix A(3, 3);
    A.values = {4, -2, 1, 1, 3, -1, 2, 1, 5};
    const Vector b = {11, -8, 15}; 
    const Vector probes[] = {{1, -2, 3}, {1.25, -2, 3}, {0, 0, 0}, {4, 2, -3}};
    try {
        for (const Vector& x : probes)
            if (!agrees(relativeResidual(A, x, b), independentResidual(A, x, b))) {
                detail = "El residu falla amb b no nul i una solució coneguda o alterada.";
                return false;
            }
        const Vector zero(3, 0);
        for (const Vector& x : {zero, Vector{1, -2, 3}})
            if (!agrees(relativeResidual(A, x, zero), independentResidual(A, x, zero))) {
                detail = "El residu falla en el cas b=0 (cal retornar el residu absolut).";
                return false;
            }
    } catch (const std::exception& error) {
        detail = error.what();
        return false;
    }
    detail = "Cas exacte, pertorbats, x=0 i b=0 superats.";
    return true;
}

static Vector referenceProduct(const Dataset& data) {

    Vector result;
    if (data.task == Task::Identity) {
        result.assign(data.A.rows * data.A.rows, 0);
        for (std::size_t i = 0; i < data.A.rows; ++i) result[i * data.A.rows + i] = 1;
    } else if (data.task == Task::MatrixVector) {
        if (data.A.cols != data.input.size()) throw std::invalid_argument("Dimensions incompatibles");
        result.resize(data.A.rows);
        for (std::size_t i = 0; i < data.A.rows; ++i) {
            long double sum = 0;
            for (std::size_t j = 0; j < data.A.cols; ++j)
                sum += static_cast<long double>(data.A.values[i * data.A.cols + j]) * data.input[j];
            result[i] = static_cast<double>(sum);
        }
    } else if (data.task == Task::MatrixMatrix) {
        if (data.A.cols != data.B.rows) throw std::invalid_argument("Dimensions incompatibles");
        result.resize(data.A.rows * data.B.cols);
        for (std::size_t i = 0; i < data.A.rows; ++i)
            for (std::size_t j = 0; j < data.B.cols; ++j) {
                long double sum = 0;
                for (std::size_t k = 0; k < data.A.cols; ++k)
                    sum += static_cast<long double>(data.A.values[i * data.A.cols + k])
                         * data.B.values[k * data.B.cols + j];
                result[i * data.B.cols + j] = static_cast<double>(sum);
            }
    }
    return result;
}

static double scaledError(const Vector& found, const Vector& expected) {
    if (found.size() != expected.size() || !finite(found)) return std::numeric_limits<double>::infinity();
    long double difference = 0;
    for (std::size_t i = 0; i < found.size(); ++i)
        difference = std::hypot(difference, static_cast<long double>(found[i]) - expected[i]);
    return static_cast<double>(difference / std::max(1.0L, magnitude(expected)));
}

static bool near(std::uint64_t value, long double expected) {
    return std::abs(static_cast<long double>(value) - expected) <= 0.1L * expected;
}

static bool counts(const Operations& op, long double add, long double sub,
                   long double mul, long double div) {
    return near(op.add, add) && near(op.subtract, sub)
        && near(op.multiply, mul) && near(op.divide, div);
}

static double millisecondsPerOperation() {

    static volatile double sink = 0;
    Vector a(32768), b(32768);
    for (std::size_t i = 0; i < a.size(); ++i) { a[i] = (i % 19) * 0.01; b[i] = (i % 7) * 0.03; }
    double best = std::numeric_limits<double>::infinity();
    for (int trial = 0; trial < 3; ++trial) {
        const auto start = Clock::now();
        double sum = 0;
        for (int repeat = 0; repeat < 64; ++repeat)
            for (std::size_t i = 0; i < a.size(); ++i) sum += a[i] * b[i];
        sink = sum;
        best = std::min(best, elapsed(start));
    }
    return best / (2.0 * 64 * a.size());
}

Report runTask(const Dataset& data, bool pivoting) {
    Report report;
    const Outcome expectedOutcome = pivoting ? data.withPivot : data.withoutPivot;
    Outcome outcome = Outcome::Success;
    bool implemented = true;
    const auto start = Clock::now();
    try {
        if (data.task == Task::Identity) {
            Matrix result = Matrix::identity(data.A.rows);
            report.result = result.values;
            report.resultRows = result.rows; report.resultCols = result.cols;
        } else if (data.task == Task::MatrixVector) {
            report.result = data.A.multiply(data.input, report.product);
            report.resultRows = report.result.size(); report.resultCols = 1;
        } else if (data.task == Task::MatrixMatrix) {
            Matrix result = data.A.multiply(data.B, report.product);
            report.result = result.values;
            report.resultRows = result.rows; report.resultCols = result.cols;
        } else {
            Matrix U = data.A;
            Vector c = data.input;
            if (!gaussianElimination(U, c, pivoting, data.pivotTolerance, report.elimination)
                || !backSubstitution(U, c, data.pivotTolerance, report.substitution)) {
                outcome = Outcome::PivotFailure;
                report.detail = "Pivot massa petit amb la tolerància seleccionada.";
            } else {
                report.result = std::move(c);
                report.resultRows = report.result.size(); report.resultCols = 1;
            }
        }
    } catch (const std::invalid_argument& error) {
        outcome = Outcome::InvalidInput; report.detail = error.what();
    } catch (const std::exception& error) {
        implemented = false; report.detail = error.what();
    }
    report.milliseconds = elapsed(start);
    report.completed = implemented && outcome == Outcome::Success;
    report.correct = implemented && outcome == expectedOutcome;
    if (!report.completed) {
        if (report.correct) report.detail += " Aturada esperada.";
        return report;
    }

    const long double m = static_cast<long double>(data.A.rows);
    const long double k = static_cast<long double>(data.A.cols);
    const long double p = static_cast<long double>(data.B.cols);
    long double expectedOps = 0;
    report.countsApplicable = data.task != Task::Identity;
    if (data.task == Task::Solve) {
        report.residual = independentResidual(data.A, report.result, data.input);
        report.correct = report.correct && std::isfinite(report.residual) && report.residual <= data.accuracy;
        report.calibrationCorrect = checkResidualFunction(report.calibrationDetail);
        try { report.studentResidual = relativeResidual(data.A, report.result, data.input); }
        catch (const std::exception& error) { report.detail = error.what(); }
        report.residualCorrect = agrees(report.studentResidual, report.residual);
        if (data.hasExpected && expectedOutcome == Outcome::Success) {
            report.error = scaledError(report.result, data.expected);
            report.correct = report.correct && report.error <= data.accuracy;
        }
        const long double pairs = m * (m - 1) / 2;
        const long double updates = m * (m - 1) * (2 * m - 1) / 6 + pairs;
        report.countsCorrect = counts(report.elimination, 0, updates, updates, pairs)
            && counts(report.substitution, 0, pairs, pairs, m);
        expectedOps = 2 * updates + pairs + 2 * pairs + m;
    } else {
        Vector expected = data.hasExpected ? data.expected : referenceProduct(data);
        report.error = scaledError(report.result, expected);
        const std::size_t rows = data.A.rows;
        const std::size_t cols = data.task == Task::Identity ? rows
            : data.task == Task::MatrixMatrix ? data.B.cols : 1;
        report.correct = report.correct && report.resultRows == rows && report.resultCols == cols
            && std::isfinite(report.error) && report.error <= data.accuracy;
        const long double terms = m * k * (data.task == Task::MatrixMatrix ? p : 1);

        const long double minAdd = m * (k - 1) * (data.task == Task::MatrixMatrix ? p : 1);
        report.countsCorrect = near(report.product.multiply, terms)
            && report.product.add >= 0.9L * minAdd && report.product.add <= 1.1L * terms
            && report.product.subtract == 0 && report.product.divide == 0;
        expectedOps = 2 * terms;
    }
    report.timeApplicable = report.countsApplicable && expectedOps >= 1000000;
    if (report.timeApplicable) {
        const double reference = millisecondsPerOperation() * static_cast<double>(expectedOps);
        report.minMilliseconds = reference * data.timeMinFactor;
        report.maxMilliseconds = reference * data.timeMaxFactor + 2;
        report.timeExpected = report.milliseconds >= report.minMilliseconds
            && report.milliseconds <= report.maxMilliseconds;
    }
    if (report.detail.empty() && !report.correct)
        report.detail = "El resultat no compleix el límit d'error.";
    return report;
}
