#include "Dataset.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdexcept>

static void expect(std::istream& file, const char* word) {
    std::string token;
    if (!(file >> token) || token != word)
        throw std::runtime_error(std::string("Camp esperat: ") + word);
}

static Vector readValues(std::istream& file, std::size_t count) {
    if (count > 4 * 1024 * 1024) throw std::runtime_error("Dataset massa gran");
    Vector result(count);
    for (double& value : result)
        if (!(file >> value) || !std::isfinite(value))
            throw std::runtime_error("Dades incompletes o no finites");
    return result;
}

static Matrix readMatrix(std::istream& file, const char* label) {
    expect(file, label);
    std::size_t rows, cols;
    if (!(file >> rows >> cols) || rows > 2048 || cols > 2048)
        throw std::runtime_error("Dimensions del dataset fora del límit 2048");
    Matrix result(rows, cols);
    result.values = readValues(file, rows * cols);
    return result;
}

Dataset loadDataset(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("No es pot obrir el dataset");
    expect(file, "LAB1");
    int version;
    if (!(file >> version) || version != 1) throw std::runtime_error("Versió de dataset desconeguda");
    Dataset data;
    expect(file, "name");
    if (!(file >> std::quoted(data.name))) throw std::runtime_error("Falta el nom del cas");
    expect(file, "task");
    int task, noPivot, pivot;
    if (!(file >> task) || task < 0 || task > 3) throw std::runtime_error("Operació desconeguda");
    data.task = static_cast<Task>(task);
    expect(file, "outcomes");
    if (!(file >> noPivot >> pivot) || noPivot < 0 || noPivot > 2 || pivot < 0 || pivot > 2)
        throw std::runtime_error("Estat esperat desconegut");
    data.withoutPivot = static_cast<Outcome>(noPivot);
    data.withPivot = static_cast<Outcome>(pivot);
    expect(file, "limits");
    if (!(file >> data.pivotTolerance >> data.accuracy >> data.timeMinFactor >> data.timeMaxFactor)
        || !std::isfinite(data.pivotTolerance) || data.pivotTolerance <= 0
        || !std::isfinite(data.accuracy) || data.accuracy <= 0
        || !std::isfinite(data.timeMinFactor) || data.timeMinFactor < 0
        || !std::isfinite(data.timeMaxFactor) || data.timeMaxFactor <= data.timeMinFactor)
        throw std::runtime_error("Llindars invàlids");
    data.A = readMatrix(file, "A");
    data.B = readMatrix(file, "B");
    expect(file, "input");
    std::size_t count;
    if (!(file >> count)) throw std::runtime_error("Falta la mida del vector");
    data.input = readValues(file, count);
    expect(file, "expected");
    if (!(file >> count)) throw std::runtime_error("Falta la mida de la resposta");
    data.expected = readValues(file, count);
    data.hasExpected = true;
    const auto outcome = data.withPivot;
    if (outcome == Outcome::Success) {
        std::size_t size = data.task == Task::Solve ? data.A.cols : data.A.rows;
        if (data.task == Task::Identity) size *= data.A.rows;
        if (data.task == Task::MatrixMatrix) size *= data.B.cols;
        if (data.expected.size() != size) throw std::runtime_error("Mida de resposta incorrecta");
    }
    expect(file, "end");
    std::string extra;
    if (file >> extra) throw std::runtime_error("Contingut sobrant al dataset");
    return data;
}

Dataset manualDataset(Task task, int rows, int inner, int columns) {
    Dataset data;
    data.task = task;
    data.A = Matrix(rows, task == Task::Solve || task == Task::Identity ? rows : inner);
    data.B = task == Task::MatrixMatrix ? Matrix(inner, columns) : Matrix();
    data.input.assign(task == Task::Solve ? rows : inner, 1);

    for (std::size_t i = 0; i < data.A.rows; ++i)
        for (std::size_t j = 0; j < data.A.cols; ++j)
            data.A.values[i * data.A.cols + j] = i == j ? 4 : 1;
    for (std::size_t i = 0; i < data.B.rows; ++i)
        for (std::size_t j = 0; j < data.B.cols; ++j)
            data.B.values[i * data.B.cols + j] = i == j ? 1 : 0;
    return data;
}
