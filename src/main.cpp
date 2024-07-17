#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "lodepng.h"
#include "camera.h"

// Initialize camera
Camera camera;

// Window dimensions
const int IMG_HEIGHT = 1080;
const int IMG_WIDTH = 1920;

// Movement settings
bool keys[1024] = { 0 };
const float deltaTime = 0.001f; // wasd velocity
const float sensitivity = 0.1; // mouse velocity

class Image {
public:
    int width, height, comp;
    std::vector<unsigned char> pixels;

    Image(int w, int h, int c) : width(w), height(h), comp(c), pixels(w * h * c) {}

    void writeToFile(const std::string& filename) const;
};

void Image::writeToFile(const std::string& filename) const {
    std::vector<unsigned char> png;
    unsigned error = lodepng::encode(png, pixels, width, height);

    if (!error) {
        lodepng::save_file(png, filename);
        std::cout << "Wrote to " << filename << std::endl;
    } else {
        std::cout << "Error " << error << ": " << lodepng_error_text(error) << std::endl;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (action == GLFW_PRESS) {
        keys[key] = true;
    }
    else if (action == GLFW_RELEASE) {
        keys[key] = false;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static double lastX, lastY;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.rotate(xoffset, yoffset, 0.0f);
}

std::string readShaderSource(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        exit(-1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint createComputeShaderProgram(const std::string& shaderPath) {
    std::string computeShaderSource = readShaderSource(shaderPath);
    GLuint computeShader = compileShader(GL_COMPUTE_SHADER, computeShaderSource.c_str());

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, computeShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(computeShader);

    return shaderProgram;
}

GLuint createScreenShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = readShaderSource(vertexPath);
    std::string fragmentCode = readShaderSource(fragmentPath);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void initOpenGLContext(GLFWwindow*& window, bool invisible) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(-1);
    }

    camera.setPosition(glm::vec3(0.0f, 0.0f, 1.0f));

    if (invisible) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    window = glfwCreateWindow(IMG_WIDTH, IMG_HEIGHT, "Raytraced Fractal", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }

    if (!invisible) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSwapInterval(1); // Enable vsync
    }
}

void renderToImage(GLuint shaderProgram, const std::string& outputFilename) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IMG_WIDTH, IMG_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    float aspect_ratio = (float) IMG_WIDTH / (float) IMG_HEIGHT;
    glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    glm::mat4 view_matrix = camera.getViewMatrix();
    glm::vec3 camera_position = camera.getPosition();
    glUniform3fv(glGetUniformLocation(shaderProgram, "camera_position"), 1, &camera_position[0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection_matrix"), 1, GL_FALSE, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view_matrix"), 1, GL_FALSE, glm::value_ptr(view_matrix));

    glDispatchCompute(IMG_WIDTH / 16, IMG_HEIGHT / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    Image image(IMG_WIDTH, IMG_HEIGHT, 4);

    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());

    image.writeToFile(outputFilename);

    glDeleteTextures(1, &texture);
}

// Key Callbacks
void processInput() {
    if (keys[GLFW_KEY_W])
        camera.moveForward(deltaTime);
    if (keys[GLFW_KEY_S])
        camera.moveBackward(deltaTime);
    if (keys[GLFW_KEY_A])
        camera.moveLeft(deltaTime);
    if (keys[GLFW_KEY_D])
        camera.moveRight(deltaTime);
}

void renderToWindow(GLFWwindow* window, GLuint computeShaderProgram) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IMG_WIDTH, IMG_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    float aspect_ratio = (float) IMG_WIDTH / (float) IMG_HEIGHT;
    glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);

    GLuint screenShaderProgram = createScreenShaderProgram("../resources/vertex.glsl", "../resources/fragment.glsl");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processInput();

        glm::mat4 view_matrix = camera.getViewMatrix();
        glm::vec3 camera_position = camera.getPosition();
        glUseProgram(computeShaderProgram);
        glUniform3fv(glGetUniformLocation(computeShaderProgram, "camera_position"), 1, &camera_position[0]);
        glUniformMatrix4fv(glGetUniformLocation(computeShaderProgram, "projection_matrix"), 1, GL_FALSE, glm::value_ptr(projection_matrix));
        glUniformMatrix4fv(glGetUniformLocation(computeShaderProgram, "view_matrix"), 1, GL_FALSE, glm::value_ptr(view_matrix));
        glDispatchCompute(IMG_WIDTH / 16, IMG_HEIGHT / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(screenShaderProgram);
        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    glDeleteTextures(1, &texture);
    glDeleteProgram(screenShaderProgram);
}

int main(int argc, char* argv[]) {
    bool writeToImage = false;
    std::string outputFilename = "output.png";

    if (argc > 1) {
        writeToImage = true;
        if (argc > 2) {
            outputFilename = argv[2];
        }
    }

    GLFWwindow* window;
    initOpenGLContext(window, writeToImage);

    GLuint shaderProgram = createComputeShaderProgram("../resources/compute.glsl");
    glUseProgram(shaderProgram);

    if (writeToImage) {
        renderToImage(shaderProgram, outputFilename);
    } else {
        renderToWindow(window, shaderProgram);
    }

    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}