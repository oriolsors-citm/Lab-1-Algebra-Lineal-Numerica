#pragma once
#include "Matrix.hpp"

// Exercicis 2 i 3. A i b es modifiquen.
// En tots dos mètodes: sistema quadrat no buit i vector de mida compatible;
// si no es compleix, llanceu std::invalid_argument.
// Retorna false si un pivot no supera la tolerància. Amb pivotatge=false,
// això NO implica que A sigui singular: pot caldre intercanviar files.
bool gaussianElimination(Matrix& A, Vector& b, bool pivoting,
                         double tolerance, Operations& operations);
// U conté els multiplicadors sota la diagonal, que aquí s'han d'ignorar.
// Substitueix c per la solució; retorna false si un pivot és massa petit.
bool backSubstitution(const Matrix& U, Vector& c, double tolerance,
                      Operations& operations);
// Exercici 2: ||Ax-b||_2 / ||b||_2. Si b=0, retorna ||Ax-b||_2.
// La norma ja ve donada al suport.
double relativeResidual(const Matrix& A, const Vector& x, const Vector& b);
