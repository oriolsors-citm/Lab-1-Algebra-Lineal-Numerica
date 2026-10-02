#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

using Vector = std::vector<double>;

// Només comptem aritmètica escalar (+, -, *, /), no índexs ni comparacions.
struct Operations {
    std::uint64_t add = 0, subtract = 0, multiply = 0, divide = 0;
    std::uint64_t total() const { return add + subtract + multiply + divide; }
};

// Emmagatzematge per files: la fila i comença a i * cols.
struct Matrix {
    std::size_t rows = 0, cols = 0;
    Vector values;
    Matrix() = default;
    Matrix(std::size_t rows, std::size_t cols, double value = 0);
    void swapRows(std::size_t first, std::size_t second);

    // Exercici 1: aquests són els únics mètodes de Matrix a completar.
    static Matrix identity(std::size_t size);
    double& at(std::size_t row, std::size_t col);
    double at(std::size_t row, std::size_t col) const;
    Vector multiply(const Vector& vector, Operations& operations) const;
    Matrix multiply(const Matrix& matrix, Operations& operations) const;
};
