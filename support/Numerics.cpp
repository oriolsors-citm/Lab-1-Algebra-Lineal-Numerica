#include "Numerics.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

Matrix::Matrix(std::size_t r, std::size_t c, double value) : rows(r), cols(c) {
    if (c && r > std::numeric_limits<std::size_t>::max() / c)
        throw std::length_error("Matriu massa gran");
    values.assign(r * c, value);
}

void Matrix::swapRows(std::size_t first, std::size_t second) {
    if (first >= rows || second >= rows) throw std::out_of_range("Fila inexistent");
    for (std::size_t j = 0; j < cols; ++j)
        std::swap(values[first * cols + j], values[second * cols + j]);
}

double norm(const Vector& vector) {

    double result = 0;
    for (double value : vector) result = std::hypot(result, value);
    return result;
}

