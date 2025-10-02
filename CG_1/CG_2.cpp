#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <ctime>
#include <cmath>
#include <iomanip>

#define GLEW_STATIC // Статическая линковка GLEW для простоты сборки.
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846 // Определение константы PI, если она отсутствует.
#endif

// Вершинный шейдер для гладкого (интерполированного) закрашивания.
const char* VERTEX_SHADER_SMOOTH = R"glsl(
    #version 400
    in vec3 vertex_position;
    in vec3 vertex_color;
    out vec3 color;
    void main() {
        color = vertex_color;
        gl_Position = vec4(vertex_position, 1.0);
    }
)glsl";

// Фрагментный шейдер для гладкого закрашивания.
const char* FRAGMENT_SHADER_SMOOTH = R"glsl(
    #version 400
    in vec3 color;
    out vec4 frag_color;
    void main() {
        frag_color = vec4(color, 1.0);
    }
)glsl";

// Вершинный шейдер для плоского (монотонного) закрашивания.
const char* VERTEX_SHADER_FLAT = R"glsl(
    #version 400
    in vec3 vertex_position;
    in vec3 vertex_color;
    flat out vec3 color;
    void main() {
        color = vertex_color;
        gl_Position = vec4(vertex_position, 1.0);
    }
)glsl";

// Фрагментный шейдер для плоского закрашивания.
const char* FRAGMENT_SHADER_FLAT = R"glsl(
    #version 400
    flat in vec3 color;
    out vec4 frag_color;
    void main() {
        frag_color = vec4(color, 1.0);
    }
)glsl";

// Перечисление для режимов отрисовки в 5-м задании.
enum class Task5Mode {Triangles=1, Strip, Fan };
// Перечисление для режимов отображения граней в 8-м задании.
enum class Task8Mode { Vertices = 1, FillFrontLineBack, Wireframe };
// Перечисление для выбора типа тонирования (закрашивания).
enum class ToningMode { Flat = 1, Smooth };

// Класс для управления геометрией объекта (вершины, цвета, индексы).
class Model {
private:
    GLuint vao = 0; // ID объекта вершинного массива (Vertex Array Object).
    size_t verteces_count = 0; // Количество вершин модели.
    size_t indices_count = 0; // Количество индексов модели.
    GLuint shaderProgramID = 0; // ID используемой шейдерной программы.
public:
    Model() { glGenVertexArrays(1, &vao); } // Конструктор: создает VAO для модели.
    ~Model() { glDeleteVertexArrays(1, &vao); } // Деструктор: освобождает память VAO при удалении модели.
    void render(GLuint mode) { // Главная функция отрисовки модели с заданным режимом.
        glUseProgram(shaderProgramID); // Активируем шейдер.
        glBindVertexArray(vao); // Привязываем VAO модели.
        if (indices_count > 0) glDrawElements(mode, (GLsizei)indices_count, GL_UNSIGNED_INT, 0); // Рисуем по индексам, если они есть.
        else glDrawArrays(mode, 0, (GLsizei)verteces_count); // Иначе рисуем по вершинам напрямую.
    }
    void load_coords(const std::vector<glm::vec3>& vertices) { // Загрузка координат вершин в видеопамять (VBO).
        verteces_count = vertices.size();
        GLuint vbo; glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);
    }
    void load_colors(const std::vector<glm::vec3>& colors) { // Загрузка цветов вершин в видеопамять (VBO).
        GLuint vbo; glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(1);
    }
    void load_indices(const std::vector<GLuint>& indices) { // Загрузка индексов вершин для оптимизированной отрисовки (EBO).
        indices_count = indices.size();
        GLuint ebo; glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    }
    void setShaderProgram(GLuint programID) { shaderProgramID = programID; } // Установка шейдерной программы для использования этой моделью.
};

// Структура, хранящая все состояние приложения (вместо глобальных переменных).
struct AppState {
    int winWidth = 800, winHeight = 600;
    int currentTask = 1;
    float pointSmoothSize = 188.0f;
    float lineWidth = 4.0f;
    int lastPrintedPointSize = 0; 
    int lastPrintedLineWidth = 0; 
    float keyHoldTimeUp = 0.0f;     
    float keyHoldTimeDown = 0.0f;
    Task5Mode task5Mode = Task5Mode::Triangles;
    Task8Mode task8Mode = Task8Mode::Vertices;
    ToningMode toningMode = ToningMode::Flat;
    Model* task4And5_triangles = nullptr, * task4And5_strip = nullptr, * task4And5_fan = nullptr; // Указатели на модели для динамической смены шейдеров.
    Model* task7And8_flat = nullptr, * task7And8_smooth = nullptr;
    GLuint smoothShaderProgram = 0, flatShaderProgram = 0; // ID скомпилированных шейдерных программ.    
};

// Прототипы функций для предварительного объявления.
void printHelp();
void processInput(GLFWwindow* window, float deltaTime);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void window_size_callback(GLFWwindow* window, int width, int height);
GLuint createShaderProgram(const char* vertex_shader_src, const char* fragment_shader_src);
std::vector<glm::vec3> getRegularPolygonVerticesCoordinates(int n, double r = 0.8);
GLFWwindow* InitAll(int w, int h, void* user_data);


// Главная функция, точка входа в программу.
int main() {
    srand((unsigned int)time(NULL)); // Инициализация генератора случайных чисел.

    AppState state; // Создание экземпляра структуры состояния.
    GLFWwindow* window = InitAll(state.winWidth, state.winHeight, &state); // Инициализация библиотек и создание окна приложения.
    if (window == nullptr) return -1; // Проверка на случай ошибки при создании окна.

    printHelp(); // Вывод справки по управлению в консоль.

    // --- ИНИЦИАЛИЗАЦИЯ РЕСУРСОВ ---
    state.smoothShaderProgram = createShaderProgram(VERTEX_SHADER_SMOOTH, FRAGMENT_SHADER_SMOOTH); // Компиляция шейдеров при запуске.
    state.flatShaderProgram = createShaderProgram(VERTEX_SHADER_FLAT, FRAGMENT_SHADER_FLAT);

    // Создание и настройка всех моделей для каждого задания.
    const int N = 6;
    Model task1And2Model;
    task1And2Model.load_coords(getRegularPolygonVerticesCoordinates(N));
    task1And2Model.load_colors(std::vector<glm::vec3>(N, glm::vec3(0.8f, 0.1f, 0.1f)));
    task1And2Model.setShaderProgram(state.smoothShaderProgram);

    Model task3Model;
    task3Model.load_coords({
        {-0.8f, 0.8f, 0.0f}, {-0.8f, 0.0f, 0.0f}, {-0.4f, 0.0f, 0.0f},
        {-0.55f, 0.2f, 0.0f}, {-0.2f, 0.8f, 0.0f}, {0.0f, 0.35f, 0.0f},
        {-0.2f, 0.0f, 0.0f}, {0.8f, 0.0f, 0.0f}
        });
    task3Model.load_colors(std::vector<glm::vec3>(8, glm::vec3(0.1, 0.8, 0.1)));
    task3Model.setShaderProgram(state.smoothShaderProgram);

    std::vector<glm::vec3> fig2Vertices = {
        {0.0f, 0.0f, 0.0f}, {0.2f, -0.8f, 0.0f}, {-0.4f, -0.6f, 0.0f}, {-0.5f, 0.1f, 0.0f},
        {-0.25f, 0.6f, 0.0f}, {0.2f, 0.4f, 0.0f}, {0.6f, 0.6f, 0.0f}, {0.6f, 0.0f, 0.0f},
    };
    std::vector<glm::vec3> fig2Colors;
    for (size_t i = 0; i < fig2Vertices.size(); ++i) {
        fig2Colors.push_back(glm::vec3((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f));
    }

    Model task4Model;
    task4Model.load_coords(fig2Vertices);
    task4Model.load_colors(fig2Colors);
    task4Model.setShaderProgram(state.smoothShaderProgram);

    Model task5Triangles, task5Strip, task5Fan;
    task5Triangles.load_coords(fig2Vertices); task5Triangles.load_colors(fig2Colors);
    task5Triangles.load_indices({ 1, 2, 3, 3, 0, 1, 3, 5, 0, 3, 5, 4, 0, 7, 5, 5, 6, 7 });
    task5Strip.load_coords(fig2Vertices); task5Strip.load_colors(fig2Colors);
    task5Strip.load_indices({ 6, 7, 5, 0, 4, 1, 3, 2 });
    task5Fan.load_coords(fig2Vertices); task5Fan.load_colors(fig2Colors);
    task5Fan.load_indices({ 0, 7, 6, 5, 4, 3, 2, 1 });
    state.task4And5_triangles = &task5Triangles; state.task4And5_strip = &task5Strip; state.task4And5_fan = &task5Fan;

    Model task6Model;
    task6Model.load_coords(getRegularPolygonVerticesCoordinates(N));
    std::vector<glm::vec3> task6Colors;
    for (int i = 0; i < N; ++i) {
        task6Colors.push_back(glm::vec3((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f));
    }
    task6Model.load_colors(task6Colors);
    task6Model.load_indices({ 0, 1, 2, 3, 4, 5 });
    task6Model.setShaderProgram(state.flatShaderProgram);

    Model task7And8Model_flat, task7And8Model_smooth;
    std::vector<glm::vec3> fig3Vertices = {
        {0.0f, 0.0f, 0.0f},{-0.5f, 0.0f, 0.0f},{-0.5f, 0.6f, 0.0f},{-0.1f, 0.5f, 0.0f},
        {0.7f, 0.8f, 0.0f},{0.8f, 0.0f, 0.0f},{0.2f, 0.0f, 0.0f},{0.2f, 0.4f, 0.0f},
        {0.0f, 0.4f, 0.0f},{-0.2f, 0.2f, 0.0f},
    };
    std::vector<glm::vec3> fig3Colors;
    for (int i = 0; i < 10; ++i) {
        fig3Colors.push_back(glm::vec3((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f));
    }
    std::vector<GLuint> fig3Indices = { 0, 1, 9, 1, 9, 2, 2, 9, 3, 3, 8, 8, 3, 8, 7, 3, 7, 4, 7, 4, 6, 4, 6, 5, 3, 9, 8 };
    task7And8Model_flat.load_coords(fig3Vertices); task7And8Model_flat.load_colors(fig3Colors);
    task7And8Model_flat.load_indices(fig3Indices);
    task7And8Model_flat.setShaderProgram(state.flatShaderProgram);
    task7And8Model_smooth.load_coords(fig3Vertices); task7And8Model_smooth.load_colors(fig3Colors);
    task7And8Model_smooth.load_indices(fig3Indices);
    task7And8Model_smooth.setShaderProgram(state.smoothShaderProgram);
    state.task7And8_flat = &task7And8Model_flat; state.task7And8_smooth = &task7And8Model_smooth;

    float lastFrame = 0.0f; // Переменная для расчета времени кадра (deltaTime).

    // Главный цикл рендеринга, работает до закрытия окна.
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime(); // Расчет времени, прошедшего с предыдущего кадра.
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime); // Обработка непрерывных нажатий клавиш (удержание).

        glViewport(0, 0, state.winWidth, state.winHeight); // Установка области отрисовки в соответствии с размером окна.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Очистка буферов цвета и глубины.
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Сброс режима отрисовки полигонов на стандартный (заливка).
        glDisable(GL_POINT_SMOOTH); // По умолчанию отключаем сглаживание, чтобы включать только где нужно.

        // Выбор логики отрисовки в зависимости от текущего задания.
        switch (state.currentTask) {
        case 1: // Задание 1: отрисовка сглаженных точек.
            glPointSize(state.pointSmoothSize);
            glEnable(GL_POINT_SMOOTH);
            task1And2Model.render(GL_POINTS);
            break;
        case 2: // Задание 2: отрисовка контура линиями.
            glLineWidth(state.lineWidth);
            task1And2Model.render(GL_LINE_LOOP);
            break;
        case 3: // Задание 3: отрисовка ломаной линии.
            glLineWidth(3.0f);
            task3Model.render(GL_LINE_STRIP);
            break;
        case 4: // Задание 4: отрисовка замкнутой ломаной линии.
            glLineWidth(3.0f);
            task4Model.render(GL_LINE_LOOP);
            break;
        case 5: // Задание 5: отрисовка фигуры разными методами.
        {
            Model* chosenModel = nullptr;
            GLuint renderMode = GL_TRIANGLES;
            if (state.task5Mode == Task5Mode::Triangles) { chosenModel = state.task4And5_triangles; renderMode = GL_TRIANGLES; }
            else if (state.task5Mode == Task5Mode::Strip) { chosenModel = state.task4And5_strip; renderMode = GL_TRIANGLE_STRIP; }
            else if (state.task5Mode == Task5Mode::Fan) { chosenModel = state.task4And5_fan; renderMode = GL_TRIANGLE_FAN; }

            if (chosenModel) {
                GLuint shader = (state.toningMode == ToningMode::Flat) ? state.flatShaderProgram : state.smoothShaderProgram;
                chosenModel->setShaderProgram(shader);
                chosenModel->render(renderMode);
            }
        }
        break;
        case 6: // Задание 6: отрисовка многоугольника веером треугольников.
            task6Model.render(GL_TRIANGLE_FAN);
            break;
        case 7: // Задание 7: отрисовка фигуры с выбором тонирования.
            if (state.toningMode == ToningMode::Flat) { state.task7And8_flat->render(GL_TRIANGLES); }
            else { state.task7And8_smooth->render(GL_TRIANGLES); }
            break;
        case 8: // Задание 8: разные режимы отображения граней.
            glPointSize(4.0f);
            if (state.task8Mode == Task8Mode::Vertices) { glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); }
            else if (state.task8Mode == Task8Mode::FillFrontLineBack) { glPolygonMode(GL_FRONT, GL_FILL); glPolygonMode(GL_BACK, GL_LINE); }
            else if (state.task8Mode == Task8Mode::Wireframe) { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); }
            state.task7And8_flat->render(GL_TRIANGLES);
            break;
        }

        glfwPollEvents(); // Опрос событий ввода (клавиатура, мышь).
        glfwSwapBuffers(window); // Обмен переднего и заднего буферов для вывода изображения на экран.
    }

    glfwTerminate(); // Освобождение ресурсов GLFW перед выходом.
    return 0;
}

// Функция выводит в консоль подробную инструкцию по управлению.
void printHelp() {
    std::cout << "---------------------------------------------------\n";
    std::cout << "               Application Controls                \n";
    std::cout << "---------------------------------------------------\n";
    std::cout << "General Controls:\n";
    std::cout << "  [1] - [8]    : Switch Task\n";
    std::cout << "  [V]          : Enable Flat Shading\n";
    std::cout << "  [B]          : Enable Smooth Shading\n";
    std::cout << "  [ESC]        : Close Application\n\n";
    std::cout << "Task-Specific Controls:\n";
    std::cout << "  Task 1 (Points):\n";
    std::cout << "    [UP/DOWN]  : Increase/decrease point size (hold down)\n";
    std::cout << "  Task 2 (Lines):\n";
    std::cout << "    [UP/DOWN]  : Increase/decrease line width (hold down)\n";
    std::cout << "  Task 5 (Filled Shape):\n";
    std::cout << "    [Z]        : 'Triangles' mode\n";
    std::cout << "    [X]        : 'Triangle Strip' mode\n";
    std::cout << "    [C]        : 'Triangle Fan' mode\n";
    std::cout << "  Task 8 (Display Modes):\n";
    std::cout << "    [Z]        : 'Vertices Only' mode\n";
    std::cout << "    [X]        : 'Fill Front, Line Back' mode\n";
    std::cout << "    [C]        : 'Wireframe' mode\n";
    std::cout << "---------------------------------------------------\n";
}

// Функция обрабатывает удержание клавиш с задержкой.
void processInput(GLFWwindow* window, float deltaTime) {
    AppState* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));

    const float holdDelay = 0.5f; // Задержка в секундах перед началом непрерывного изменения
    const float pointChangeSpeed = 50.0f;
    const float lineChangeSpeed = 5.0f;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        state->keyHoldTimeUp += deltaTime; // Увеличиваем счетчик времени удержания
    }
    else {
        state->keyHoldTimeUp = 0.0f; // Сбрасываем счетчик, если клавиша отпущена
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        state->keyHoldTimeDown += deltaTime;
    }
    else {
        state->keyHoldTimeDown = 0.0f;
    }

    if (state->keyHoldTimeUp > holdDelay) {
        if (state->currentTask == 1) state->pointSmoothSize += pointChangeSpeed * deltaTime;
        if (state->currentTask == 2) state->lineWidth += lineChangeSpeed * deltaTime;
    }
    if (state->keyHoldTimeDown > holdDelay) {
        if (state->currentTask == 1) {
            state->pointSmoothSize -= pointChangeSpeed * deltaTime;
            if (state->pointSmoothSize < 1.0f) state->pointSmoothSize = 1.0f;
        }
        if (state->currentTask == 2) {
            state->lineWidth -= lineChangeSpeed * deltaTime;
            if (state->lineWidth < 1.0f) state->lineWidth = 1.0f;
        }
    }

    if (state->currentTask == 1 && static_cast<int>(state->pointSmoothSize) != state->lastPrintedPointSize) {
        state->lastPrintedPointSize = static_cast<int>(state->pointSmoothSize);
        std::cout << "New point size: " << state->lastPrintedPointSize << std::endl;
    }
    if (state->currentTask == 2 && static_cast<int>(state->lineWidth) != state->lastPrintedLineWidth) {
        state->lastPrintedLineWidth = static_cast<int>(state->lineWidth);
        std::cout << "New line width: " << state->lastPrintedLineWidth << std::endl;
    }
}

// Функция обрабатывает одиночные нажатия клавиш и сразу обновляет "запомненные" значения.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return; // Игнорируем события отпускания и повтора клавиш.
    AppState* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window, true); return; }

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
        state->currentTask = key - GLFW_KEY_0;
        std::cout << ">> Switched to Task " << state->currentTask << std::endl;
    }

    if (state->currentTask == 5) {
        if (key == GLFW_KEY_Z) { state->task5Mode = Task5Mode::Triangles; std::cout << "Task 5 Mode: Triangles\n"; }
        if (key == GLFW_KEY_X) { state->task5Mode = Task5Mode::Strip; std::cout << "Task 5 Mode: Triangle Strip\n"; }
        if (key == GLFW_KEY_C) { state->task5Mode = Task5Mode::Fan; std::cout << "Task 5 Mode: Triangle Fan\n"; }
    }
    else if (state->currentTask == 8) {
        if (key == GLFW_KEY_Z) { state->task8Mode = Task8Mode::Vertices; std::cout << "Task 8 Mode: Vertices Only\n"; }
        if (key == GLFW_KEY_X) { state->task8Mode = Task8Mode::FillFrontLineBack; std::cout << "Task 8 Mode: Fill Front / Line Back\n"; }
        if (key == GLFW_KEY_C) { state->task8Mode = Task8Mode::Wireframe; std::cout << "Task 8 Mode: Wireframe\n"; }
    }

    // --- ИСПРАВЛЕННАЯ ЛОГИКА ДЛЯ UP/DOWN ---
    if (state->currentTask == 1) { // Для размера точек
        if (key == GLFW_KEY_UP) state->pointSmoothSize++;
        if (key == GLFW_KEY_DOWN) state->pointSmoothSize--;
        if (state->pointSmoothSize < 1.0f) state->pointSmoothSize = 1.0f;

        // Сразу печатаем и ОБНОВЛЯЕМ запомненное значение, чтобы processInput не напечатал его снова.
        state->lastPrintedPointSize = static_cast<int>(state->pointSmoothSize);
        std::cout << "New point size: " << state->lastPrintedPointSize << std::endl;
    }
    else if (state->currentTask == 2) { // Для толщины линий
        if (key == GLFW_KEY_UP) state->lineWidth++;
        if (key == GLFW_KEY_DOWN) state->lineWidth--;
        if (state->lineWidth < 1.0f) state->lineWidth = 1.0f;

        // Аналогично обновляем запомненное значение для толщины линии.
        state->lastPrintedLineWidth = static_cast<int>(state->lineWidth);
        std::cout << "New line width: " << state->lastPrintedLineWidth << std::endl;
    }
    // --- КОНЕЦ ИСПРАВЛЕНИЙ ---

    if (key == GLFW_KEY_V) { state->toningMode = ToningMode::Flat; std::cout << ">> Shading Mode: Flat\n"; }
    if (key == GLFW_KEY_B) { state->toningMode = ToningMode::Smooth; std::cout << ">> Shading Mode: Smooth\n"; }
}

// Функция вызывается при изменении размеров окна.
void window_size_callback(GLFWwindow* window, int width, int height) {
    AppState* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    state->winWidth = width; state->winHeight = height;
}

// Функция компилирует шейдеры из строк и линкует их в одну программу.
GLuint createShaderProgram(const char* vertex_shader_src, const char* fragment_shader_src) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader_src, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader_src, NULL);
    glCompileShader(fs);
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, fs);
    glAttachShader(shader_program, vs);
    glLinkProgram(shader_program);
    glDeleteShader(vs); glDeleteShader(fs);
    return shader_program;
}

// Вспомогательная функция для расчета вершин правильного многоугольника.
std::vector<glm::vec3> getRegularPolygonVerticesCoordinates(int n, double r) {
    std::vector<glm::vec3> vertices;
    float angleStep = 2.0f * (float)M_PI / n;
    for (int i = 0; i < n; i++) {
        float angle = i * angleStep;
        vertices.emplace_back((float)(r * cos(angle)), (float)(r * sin(angle)), 0.0f);
    }
    return vertices;
}

// Главная функция инициализации: создает окно и настраивает OpenGL.
GLFWwindow* InitAll(int w, int h, void* user_data) {
    if (!glfwInit()) { std::cerr << "ERROR: could not start GLFW3\n"; return nullptr; }

    GLFWwindow* window = glfwCreateWindow(w, h, "CG 2", NULL, NULL);
    if (!window) { glfwTerminate(); std::cerr << "ERROR: could not create window\n"; return nullptr; }

    glfwSetWindowUserPointer(window, user_data);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "ERROR: could not start GLEW\n"; return nullptr; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); // Включение смешивания цветов, необходимо для корректной работы сглаживания.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Установка черного цвета фона, как в оригинале.

    return window;
}