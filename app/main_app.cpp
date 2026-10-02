#include "Validation.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

static bool matrixTable(const char* label, Matrix& matrix, bool editable) {
    bool changed = false;
    ImGui::PushID(label);
    ImGui::SeparatorText(label);
    const int rows = static_cast<int>(std::min<std::size_t>(matrix.rows, 6));
    const int cols = static_cast<int>(std::min<std::size_t>(matrix.cols, 6));
    ImGui::Text("%zu x %zu", matrix.rows, matrix.cols);
    if (cols && ImGui::BeginTable(label, cols, ImGuiTableFlags_Borders)) {
        for (int i = 0; i < rows; ++i) {
            ImGui::TableNextRow();
            for (int j = 0; j < cols; ++j) {
                ImGui::TableSetColumnIndex(j);
                double& value = matrix.values[i * matrix.cols + j];
                if (editable) {
                    ImGui::PushID(i * cols + j);
                    ImGui::SetNextItemWidth(-1);
                    changed |= ImGui::InputDouble("##valor", &value, 0, 0, "%.6g");
                    ImGui::PopID();
                } else ImGui::Text("%.6g", value);
            }
        }
        ImGui::EndTable();
    }
    if (matrix.rows > 6 || matrix.cols > 6) ImGui::TextDisabled("Vista parcial; les dades completes s'utilitzen en executar.");
    ImGui::PopID();
    return changed;
}

static bool vectorTable(const char* label, Vector& vector, bool editable) {
    bool changed = false;
    ImGui::SeparatorText(label);
    for (std::size_t i = 0; i < std::min<std::size_t>(vector.size(), 8); ++i) {
        ImGui::PushID(static_cast<int>(i));
        if (editable) {
            ImGui::SetNextItemWidth(140);
            changed |= ImGui::InputDouble(label, &vector[i], 0, 0, "%.6g");
        } else ImGui::Text("[%zu] %.10g", i, vector[i]);
        ImGui::PopID();
    }
    if (vector.size() > 8) ImGui::TextDisabled("... (%zu components)", vector.size());
    return changed;
}

static void feedback(const char* label, bool ok) {
    ImGui::TextColored(ok ? ImVec4(0.4f, 0.9f, 0.5f, 1) : ImVec4(1, 0.45f, 0.35f, 1),
                       "%s: %s", label, ok ? "CORRECTE" : "REVISAR");
}

static void operationCounts(const char* label, const Operations& op) {
    ImGui::Text("%s: + %llu | - %llu | * %llu | / %llu", label,
        static_cast<unsigned long long>(op.add), static_cast<unsigned long long>(op.subtract),
        static_cast<unsigned long long>(op.multiply), static_cast<unsigned long long>(op.divide));
}

int main(int argc, char** argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) { std::fprintf(stderr, "%s\n", SDL_GetError()); return 1; }
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    const char* glsl = "#version 150";
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    const char* glsl = "#version 330";
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Lab 1 - Matemàtiques II - 2026", 1200, 850,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_GLContext context = window ? SDL_GL_CreateContext(window) : nullptr;
    if (!context || !SDL_GL_MakeCurrent(window, context)) {
        std::fprintf(stderr, "%s\n", SDL_GetError());
        if (context) SDL_GL_DestroyContext(context);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit(); return 1;
    }
    SDL_GL_SetSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetIO().IniFilename = nullptr;

    ImGui::GetIO().Fonts->AddFontDefaultBitmap();
    ImGui::StyleColorsDark();
    const ImGuiStyle baseStyle = ImGui::GetStyle();
    float currentScale = 0;
    if (!ImGui_ImplSDL3_InitForOpenGL(window, context) || !ImGui_ImplOpenGL3_Init(glsl)) {
        std::fprintf(stderr, "No es poden inicialitzar els backends d'ImGui\n");
        ImGui::DestroyContext(); SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }

    std::filesystem::path datasetDirectory = "datasets";
    if (!std::filesystem::exists(datasetDirectory))
        datasetDirectory = std::filesystem::absolute(argv[0]).parent_path() / "datasets";
    std::vector<std::filesystem::path> files;
    std::error_code directoryError;
    for (const auto& entry : std::filesystem::directory_iterator(datasetDirectory, directoryError))
        if (entry.path().extension() == ".lab") files.push_back(entry.path());
    std::sort(files.begin(), files.end());

    Dataset data = manualDataset(Task::Solve, 3, 3, 3);
    Report report;
    bool hasReport = false, pivoting = false, running = true;
    int task = 3, rows = 3, inner = 3, columns = 3, selection = 0;
    char path[2048] = "";
    std::string error;
    if (!files.empty()) std::snprintf(path, sizeof(path), "%s", files[0].string().c_str());

    if (argc > 1) std::snprintf(path, sizeof(path), "%s", argv[1]);
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
                && event.window.windowID == SDL_GetWindowID(window))) running = false;
        }

        float scale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(window));
        if (!std::isfinite(scale) || scale <= 0) scale = 1;

        scale = std::max(1.0f, std::round(scale));
        if (std::abs(scale - currentScale) > 0.001f) {
            ImGui::GetStyle() = baseStyle;
            ImGui::GetStyle().ScaleAllSizes(scale);
            ImGui::GetStyle().FontScaleDpi = scale;
            currentScale = scale;
        }
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Laboratori 1", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Laboratori 1 · Àlgebra lineal numèrica · Tardor 2026");
        if (ImGui::BeginTable("panells", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableNextColumn();
            ImGui::SeparatorText("Dades");
            if (ImGui::BeginCombo("Dataset inclòs", files.empty() ? "Cap fitxer" : files[selection].filename().string().c_str())) {
                for (std::size_t i = 0; i < files.size(); ++i)
                    if (ImGui::Selectable(files[i].filename().string().c_str(), selection == static_cast<int>(i))) {
                        selection = static_cast<int>(i);
                        std::snprintf(path, sizeof(path), "%s", files[i].string().c_str());
                    }
                ImGui::EndCombo();
            }
            ImGui::InputText("Fitxer .lab", path, sizeof(path));
            if (ImGui::Button("Carrega el fitxer")) {
                try {
                    data = loadDataset(std::filesystem::u8path(path));
                    task = static_cast<int>(data.task); error.clear(); hasReport = false;
                } catch (const std::exception& exception) { error = exception.what(); }
            }
            const char* tasks[] = {"Identitat", "Producte Ax", "Producte AB", "Sistema Ax=b"};
            ImGui::Combo("Nou cas manual", &task, tasks, 4);
            ImGui::InputInt("Files m / sistema n", &rows);
            ImGui::BeginDisabled(task == 0 || task == 3);
            ImGui::InputInt("Dimensió interior k", &inner);
            ImGui::EndDisabled();
            ImGui::BeginDisabled(task != 2);
            ImGui::InputInt("Columnes p de B", &columns);
            ImGui::EndDisabled();
            rows = std::clamp(rows, 1, 8); inner = std::clamp(inner, 1, 8); columns = std::clamp(columns, 1, 8);
            if (ImGui::Button("Crea cas manual editable")) {
                data = manualDataset(static_cast<Task>(task), rows, inner, columns);
                hasReport = false; error.clear();
            }
            ImGui::TextWrapped("Cas actiu: %s", data.name.c_str());
            ImGui::Text("Operació activa: %s", tasks[static_cast<int>(data.task)]);
            if (data.task == Task::Solve && ImGui::Checkbox("Pivotatge parcial", &pivoting)) hasReport = false;
            if (data.task == Task::Solve && ImGui::InputDouble("Tolerància del pivot", &data.pivotTolerance, 0, 0, "%.2e")) {
                data.hasExpected = false; data.withoutPivot = data.withPivot = Outcome::Success;
                data.name = "Cas manual (tolerància modificada)"; hasReport = false;
            }
            const bool editable = !data.hasExpected && data.A.rows <= 8 && data.A.cols <= 8;
            bool changed = matrixTable("A", data.A, editable);
            if (data.task == Task::MatrixMatrix) changed |= matrixTable("B", data.B, editable);
            if (data.task == Task::MatrixVector || data.task == Task::Solve)
                changed |= vectorTable(data.task == Task::Solve ? "b" : "x", data.input, editable);
            if (changed) hasReport = false;
            ImGui::TableNextColumn();
            ImGui::SeparatorText("Comprovació");
            if (ImGui::Button("Executa l'operació")) {
                try { report = runTask(data, pivoting); hasReport = true; error.clear(); }
                catch (const std::exception& exception) { error = exception.what(); hasReport = false; }
            }
            if (!error.empty()) ImGui::TextWrapped("Error: %s", error.c_str());
            if (hasReport) {
                feedback("Resultat", report.correct);
                if (!report.detail.empty()) ImGui::TextWrapped("%s", report.detail.c_str());
                ImGui::Text("Temps: %.3f ms", report.milliseconds);
                if (report.countsApplicable) {
                    ImGui::TextColored(report.countsCorrect ? ImVec4(0.4f, 0.9f, 0.5f, 1) : ImVec4(1, 0.75f, 0.3f, 1),
                        "Nombre d'operacions: %s", report.countsCorrect ? "DINS DEL MARGE" : "FORA DEL MARGE");
                    ImGui::TextUnformatted("Marge orientatiu: +/-10% respecte de la teoria.");
                    operationCounts("Producte", report.product);
                    operationCounts("Eliminació", report.elimination);
                    operationCounts("Substitució", report.substitution);
                }
                if (report.timeApplicable) {
                    ImGui::TextColored(report.timeExpected ? ImVec4(0.4f, 0.9f, 0.5f, 1) : ImVec4(1, 0.75f, 0.3f, 1),
                        "Temps esperat: %s", report.timeExpected ? "DINS DEL RANG" : "FORA DEL RANG");
                    ImGui::Text("Rang orientatiu: [%.3f, %.3f] ms", report.minMilliseconds, report.maxMilliseconds);
                }
                if (data.task == Task::Solve && report.completed) {
                    ImGui::Text("Residu calculat: %.6e", report.studentResidual);
                    ImGui::Text("Residu de referència: %.6e", report.residual);
                    feedback("Funció de residu", report.residualCorrect && report.calibrationCorrect);
                    if (!report.residualCorrect || !report.calibrationCorrect)
                        ImGui::TextWrapped("Reviseu relativeResidual: no calcula correctament el residu.");
                }
                if (data.hasExpected && report.completed) ImGui::Text("Error de la resposta: %.6e", report.error);
                if (report.completed) ImGui::Text("Error màxim admès: %.1e", data.accuracy);
                if (report.completed) {
                    if (report.resultCols > 1) {
                        Matrix result(report.resultRows, report.resultCols); result.values = report.result;
                        matrixTable("Resultat", result, false);
                    } else vectorTable("Resultat", report.result, false);
                }
            }
            ImGui::Separator();
            ImGui::EndTable();
        }
        ImGui::End(); ImGui::Render();
        int width, height; SDL_GetWindowSizeInPixels(window, &width, &height);
        glViewport(0, 0, width, height); glClearColor(0.10f, 0.11f, 0.14f, 1); glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); SDL_GL_SwapWindow(window);
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext();
    SDL_GL_DestroyContext(context); SDL_DestroyWindow(window); SDL_Quit();
    return 0;
}
