#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <ctime>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

// Константы
const float PI = 3.14159265358979f;

int currentTask = 1;  // Текущее отображаемое задание.
float pointSize = 10.0f;  // Текущий размер точек для Задания 1.
float lineWidth = 3.0f;  // Текущая толщина линий для Задания 2.
int task5_mode = 0;  // Режим примитива для Задания 5 (0:TRIANGLES, 1:STRIP, 2:FAN).
bool flatShading = false;  // Флаг для переключения типа тонирования (плоское/гладкое).
int task8_mode = 0;  // Режим отображения для Задания 8 (0:POINTS, 1:LINE, 2:FILL).
int windowWidth = 900, windowHeight = 900;  // Размеры окна.

// ID шейдерных программ и буферов OpenGL.
GLuint shaderProgramSmooth = 0;
GLuint shaderProgramFlat = 0;
GLuint shaderProgramPoints = 0;
GLuint vaoTask1, vaoTask2, vaoTask3, vaoTask4;
GLuint vaoTask5A, vaoTask5B, vaoTask5C;
GLuint vaoTask6, vaoTask7_outer, vaoTask7_inner;
GLuint vboTask1_coords, vboTask1_colors;
GLuint vboTask2_coords, vboTask2_colors;
GLuint vboTask3_coords, vboTask3_colors;
GLuint vboTask4_coords, vboTask4_colors;
GLuint vboTask5A_coords, vboTask5A_colors;
GLuint vboTask5B_coords, vboTask5B_colors;
GLuint vboTask5C_coords, vboTask5C_colors;
GLuint vboTask6_coords, vboTask6_colors;
GLuint vboTask7_outer_coords, vboTask7_outer_colors;
GLuint vboTask7_inner_coords, vboTask7_inner_colors;

const glm::vec3 clearColor = glm::vec3(0.1f, 0.1f, 0.1f);  // Цвет фона.

// Вершинный шейдер для гладкого тонирования.
const char* vs_smooth = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 ourColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
}
)";

// Фрагментный шейдер для гладкого тонирования.
const char* fs_smooth = R"(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(ourColor, 1.0);
}
)";

// Вершинный шейдер для плоского тонирования (с ключевым словом 'flat').
const char* vs_flat = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
flat out vec3 ourColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
}
)";

// Фрагментный шейдер для плоского тонирования.
const char* fs_flat = R"(
#version 330 core
flat in vec3 ourColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(ourColor, 1.0);
}
)";

// Вершинный шейдер для создания круглых точек.
const char* vs_points = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 ourColor;
uniform float u_pointSize;
void main() {
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
    gl_PointSize = u_pointSize;
}
)";

// Фрагментный шейдер, отсекающий пиксели за пределами круга.
const char* fs_points = R"(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;
void main() {
    float dist = distance(gl_PointCoord, vec2(0.5, 0.5));
    if (dist > 0.5) {
        discard;
    }
    FragColor = vec4(ourColor, 1.0);
}
)";

// Компилирует шейдер и проверяет на наличие ошибок.
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
    }
    return s;
}

// Создает шейдерную программу из вершинного и фрагментного шейдеров.
GLuint createProgram(const char* vs, const char* fs) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vs);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, 1024, nullptr, log);
        std::cerr << "Program link error: " << log << std::endl;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// Универсальная функция для создания VAO и VBO с координатами и цветами.
void setupVAO(GLuint& vao, GLuint& vbo_coords, GLuint& vbo_colors,
    const std::vector<glm::vec3>& coords,
    const std::vector<glm::vec3>& colors) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo_coords);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_coords);
    glBufferData(GL_ARRAY_BUFFER, coords.size() * sizeof(glm::vec3),
        coords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &vbo_colors);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3),
        colors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// Подготавливает данные для правильного семиугольника (Задания 1 и 2).
void setupTask1_2() {
    int n = 7;
    float r = 0.8f;
    std::vector<glm::vec3> verts, cols;
    for (int i = 0; i < n; ++i) {
        float a = 2 * PI * i / n;
        verts.push_back({ r * cos(a), r * sin(a), 0 });
        cols.push_back({ 1, 1, 1 });
    }
    setupVAO(vaoTask1, vboTask1_coords, vboTask1_colors, verts, cols);
    setupVAO(vaoTask2, vboTask2_coords, vboTask2_colors, verts, cols);
}

// Подготавливает данные для первой фигуры (Задание 3).
void setupTask3() {
    std::vector<glm::vec3> verts = {
        {-1, 0.5, 0},   {-0.8, -0.3, 0}, {-0.6, 0.2, 0}, {-0.1, 0.2, 0},
        {-0.3, 0.8, 0}, {1, 0.8, 0},     {0.2, -0.3, 0} };
    std::vector<glm::vec3> cols(verts.size(), { 1, 1, 1 });
    setupVAO(vaoTask3, vboTask3_coords, vboTask3_colors, verts, cols);
}

// Подготавливает данные для второй фигуры (Задания 4 и 5).
void setupTask4_5() {
    std::vector<glm::vec3> vertices = {
        {0.2f, 0.0f, 0},  {0.6f, 0.0f, 0},  {0.6f, -0.3f, 0}, {-0.5f, -0.3f, 0},
        {-0.1f, 0.2f, 0}, {-0.8f, 0.8f, 0}, {0.8f, 0.8f, 0},  {0.2f, 0.5f, 0} };
    std::vector<glm::vec3> cols(vertices.size(), { 1, 1, 1 });
    setupVAO(vaoTask4, vboTask4_coords, vboTask4_colors, vertices, cols);

    glm::vec3 p0 = vertices[0], p1 = vertices[1], p2 = vertices[2],
        p3 = vertices[3];
    glm::vec3 p4 = vertices[4], p5 = vertices[5], p6 = vertices[6],
        p7 = vertices[7];
    glm::vec3 c0(1, 0, 0), c1(0, 1, 0), c2(0, 0, 1), c3(1, 1, 0), c4(1, 0, 1),
        c5(0, 1, 1), c6(0.5, 0.5, 0), c7(0.5, 0, 0.5);

    std::vector<glm::vec3> coordsA = { p5, p6, p7, p5, p7, p4, p4, p7, p0,
                                      p4, p0, p3, p0, p1, p2, p0, p2, p3 };
    std::vector<glm::vec3> colorsA = { c5, c6, c7, c5, c7, c4, c4, c7, c0,
                                      c4, c0, c3, c0, c1, c2, c0, c2, c3 };
    setupVAO(vaoTask5A, vboTask5A_coords, vboTask5A_colors, coordsA, colorsA);

    std::vector<glm::vec3> coordsB = { p6, p5, p7, p4, p0, p3, p2, p1 };
    std::vector<glm::vec3> colorsB = { c6, c5, c7, c4, c0, c3, c2, c1 };
    setupVAO(vaoTask5B, vboTask5B_coords, vboTask5B_colors, coordsB, colorsB);

    std::vector<glm::vec3> coordsC = { p0, p1, p2, p3, p4, p5, p6, p7 };
    std::vector<glm::vec3> colorsC = { c0, c1, c2, c3, c4, c5, c6, c7 };
    setupVAO(vaoTask5C, vboTask5C_coords, vboTask5C_colors, coordsC, colorsC);
}

// Подготавливает данные для n-угольника и третьей фигуры (Задания 6, 7, 8).
void setupTask6_7() {
    int n = 7;
    float r = 0.9f;
    std::vector<glm::vec3> coords6, cols6;
    coords6.push_back({ 0, 0, 0 });
    cols6.push_back({ 1, 1, 1 });
    for (int i = 0; i <= n; i++) {
        float a = 2 * PI * i / n;
        coords6.push_back({ r * cos(a), r * sin(a), 0 });
        cols6.push_back({ (cos(a) + 1) / 2, (sin(a) + 1) / 2, (float)i / n });
    }
    setupVAO(vaoTask6, vboTask6_coords, vboTask6_colors, coords6, cols6);

    glm::vec3 p[6] = { {-0.3, 0.9, 0},  {0.5, 0.7, 0},   {0.7, 0.2, 0},
                      {-0.1, -0.1, 0}, {-0.4, -0.3, 0}, {-0.8, 0.1, 0} };
    glm::vec3 q[3] = { {0.2, -0.2, 0}, {0.0, 0.6, 0}, {-0.5, 0.0, 0} };

    std::vector<glm::vec3> coordsOuter, colorsOuter;
    coordsOuter.push_back({ 0, 0, 0 });
    colorsOuter.push_back({ 0.7, 0.6, 0.3 });
    for (int i = 0; i <= 6; i++) {
        coordsOuter.push_back(p[i % 6]);
        colorsOuter.push_back(
            { 0.2f + 0.1f * (i % 6), 0.4f + 0.08f * (i % 6), 0.7f });
    }
    setupVAO(vaoTask7_outer, vboTask7_outer_coords, vboTask7_outer_colors,
        coordsOuter, colorsOuter);

    std::vector<glm::vec3> coordsInner = { q[0], q[1], q[2] };
    std::vector<glm::vec3> colorsInner(3, clearColor);
    setupVAO(vaoTask7_inner, vboTask7_inner_coords, vboTask7_inner_colors,
        coordsInner, colorsInner);
}

void drawTask1() {
    glEnable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(shaderProgramPoints);
    GLint loc = glGetUniformLocation(shaderProgramPoints, "u_pointSize");
    glUniform1f(loc, pointSize);
    glBindVertexArray(vaoTask1);
    glDrawArrays(GL_POINTS, 0, 7);
    glBindVertexArray(0);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void drawTask2() {
    glUseProgram(shaderProgramSmooth);
    glBindVertexArray(vaoTask2);
    glLineWidth(lineWidth);
    glDrawArrays(GL_LINE_LOOP, 0, 7);
    glBindVertexArray(0);
}

void drawTask3() {
    glUseProgram(shaderProgramSmooth);
    glBindVertexArray(vaoTask3);
    glLineWidth(3);
    glDrawArrays(GL_LINE_STRIP, 0, 7);
    glBindVertexArray(0);
}

void drawTask4() {
    glUseProgram(shaderProgramSmooth);
    glBindVertexArray(vaoTask4);
    glLineWidth(3);
    glDrawArrays(GL_LINE_LOOP, 0, 8);
    glBindVertexArray(0);
}

void drawTask5() {
    glUseProgram(flatShading ? shaderProgramFlat : shaderProgramSmooth);
    if (task5_mode == 0) {
        glBindVertexArray(vaoTask5A);
        glDrawArrays(GL_TRIANGLES, 0, 18);
    }
    else if (task5_mode == 1) {
        glBindVertexArray(vaoTask5B);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
    }
    else {
        glBindVertexArray(vaoTask5C);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 8);
    }
    glBindVertexArray(0);
}

void drawTask6() {
    glUseProgram(shaderProgramSmooth);
    glBindVertexArray(vaoTask6);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 9);
    glBindVertexArray(0);
}

void drawTask7_8() {
    glUseProgram(flatShading ? shaderProgramFlat : shaderProgramSmooth);
    if (currentTask == 8) {
        if (task8_mode == 0) glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        if (task8_mode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        if (task8_mode == 2) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glBindVertexArray(vaoTask7_outer);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 8);
    glBindVertexArray(vaoTask7_inner);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Обработка ввода с клавиатуры
void key_callback(GLFWwindow* w, int key, int sc, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
        currentTask = key - GLFW_KEY_1 + 1;
        std::cout << "Task " << currentTask << "\n";
    }
    if (currentTask == 1) {
        if (key == GLFW_KEY_UP) pointSize += 1;
        if (key == GLFW_KEY_DOWN) pointSize = std::max(1.0f, pointSize - 1);
        std::cout << "Point Size: " << pointSize << "\n";
    }
    if (currentTask == 2) {
        if (key == GLFW_KEY_UP) lineWidth += 1;
        if (key == GLFW_KEY_DOWN) lineWidth = std::max(1.0f, lineWidth - 1);
        std::cout << "Line Width: " << lineWidth << "\n";
    }
    if (currentTask == 5) {
        if (key == GLFW_KEY_Z) task5_mode = 0;
        if (key == GLFW_KEY_X) task5_mode = 1;
        if (key == GLFW_KEY_C) task5_mode = 2;
        if (key == GLFW_KEY_V) flatShading = false;
        if (key == GLFW_KEY_B) flatShading = true;
    }
    if (currentTask == 7 || currentTask == 8) {
        if (key == GLFW_KEY_V) flatShading = false;
        if (key == GLFW_KEY_B) flatShading = true;
    }
    if (currentTask == 8) {
        if (key == GLFW_KEY_Z) task8_mode = 0;
        if (key == GLFW_KEY_X) task8_mode = 1;
        if (key == GLFW_KEY_C) task8_mode = 2;
    }
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, true);
}

int main() {
    // Инициализация GLFW и создание окна.
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(windowWidth, windowHeight, "CG 2",
        nullptr, nullptr);
    if (!win) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, key_callback);

    // Инициализация GLEW.
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n";
        return -1;
    }

    // Создание всех шейдерных программ.
    shaderProgramSmooth = createProgram(vs_smooth, fs_smooth);
    shaderProgramFlat = createProgram(vs_flat, fs_flat);
    shaderProgramPoints = createProgram(vs_points, fs_points);

    // Подготовка геометрии для всех заданий.
    setupTask1_2();
    setupTask3();
    setupTask4_5();
    setupTask6_7();

    // Вывод инструкций в консоль.
    std::cout << "Controls:\n1-8: switch task\nTask1/2: up/down arrows "
        "size/width\nTask5: Z/X/C mode, V/B smooth/flat\nTask7/8: V/B "
        "smooth/flat\nTask8: Z/X/C points/line/fill\n";

    // Главный цикл рендеринга.
    while (!glfwWindowShouldClose(win)) {
        // Очистка экрана.
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Выбор и отрисовка текущего задания.
        switch (currentTask) {
        case 1:
            drawTask1();
            break;
        case 2:
            drawTask2();
            break;
        case 3:
            drawTask3();
            break;
        case 4:
            drawTask4();
            break;
        case 5:
            drawTask5();
            break;
        case 6:
            drawTask6();
            break;
        case 7:
        case 8:
            drawTask7_8();
            break;
        }

        // Обмен буферов и опрос событий.
        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // Освобождение ресурсов и завершение работы.
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}