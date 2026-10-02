#pragma once
#include "Matrix.hpp"
#include <filesystem>
#include <string>

enum class Task { Identity, MatrixVector, MatrixMatrix, Solve };
enum class Outcome { Success, PivotFailure, InvalidInput };

struct Dataset {
    std::string name = "Cas manual";
    Task task = Task::Solve;
    Matrix A, B;
    Vector input, expected;
    bool hasExpected = false;
    Outcome withoutPivot = Outcome::Success, withPivot = Outcome::Success;
    double pivotTolerance = 1e-12, accuracy = 1e-9;

    double timeMinFactor = 0.05, timeMaxFactor = 40;
};

Dataset loadDataset(const std::filesystem::path& path);
Dataset manualDataset(Task task, int rows, int inner, int columns);
