#include "Solver.hpp"
#include "Numerics.hpp"
#include <cmath>

bool gaussianElimination(Matrix& /*A*/, Vector& /*b*/, bool /*pivoting*/,
                         double /*tolerance*/, Operations& /*operations*/) {
    // TODO (ex. 2): comprovar dimensions, pivots, multiplicadors i actualitzacions.
    // TODO (ex. 3): si pivoting=true, seleccionar i intercanviar la fila pivot.
    throw std::logic_error("Pendent: gaussianElimination");
}

bool backSubstitution(const Matrix& /*U*/, Vector& /*c*/, double /*tolerance*/,
                      Operations& /*operations*/) {
    // TODO: comprovar dimensions, recórrer de baix a dalt, substituir c per x.
    throw std::logic_error("Pendent: backSubstitution");
}

double relativeResidual(const Matrix& /*A*/, const Vector& /*x*/, const Vector& /*b*/) {
    // TODO: calcular ||Ax-b|| / ||b|| amb norm (donada); tractar b=0.
    // Les operacions del residu no s'inclouen en el comptatge de la resolució.
    throw std::logic_error("Pendent: relativeResidual");
}
