#include "Matrix.hpp"
#include <stdexcept>

Matrix Matrix::identity(std::size_t /*size*/) {
    // TODO: construir I amb uns a la diagonal i zeros a la resta.
    throw std::logic_error("Pendent: Matrix::identity");
}

double& Matrix::at(std::size_t /*row*/, std::size_t /*col*/) {
    // TODO: comprovar els índexs i retornar una referència al valor.
    throw std::logic_error("Pendent: Matrix::at (escriptura)");
}

double Matrix::at(std::size_t /*row*/, std::size_t /*col*/) const {
    // TODO: comprovar els índexs i retornar el valor.
    throw std::logic_error("Pendent: Matrix::at (lectura)");
}

Vector Matrix::multiply(const Vector& /*vector*/, Operations& /*operations*/) const {
    // TODO: validar dimensions, calcular Ax i comptar sumes i productes.
    throw std::logic_error("Pendent: Matrix::multiply (vector)");
}

Matrix Matrix::multiply(const Matrix& /*matrix*/, Operations& /*operations*/) const {
    // TODO: validar dimensions, calcular AB i comptar sumes i productes.
    throw std::logic_error("Pendent: Matrix::multiply (matriu)");
}
