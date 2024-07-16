#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "lodepng.h"

const int IMG_HEIGHT = 6000;
const int IMG_WIDTH = 6000;

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

void initOpenGLContext(GLFWwindow*& window) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(-1);
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(IMG_WIDTH, IMG_HEIGHT, "Off-screen Rendering", NULL, NULL);
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
}

int main() {
    GLFWwindow* window;
    initOpenGLContext(window);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IMG_WIDTH, IMG_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    GLuint shaderProgram = createComputeShaderProgram("../resources/compute.glsl");
    glUseProgram(shaderProgram);

    // Camera properties
    glm::vec3 camera_position(0.0f, 1.5f, 1.5f);
    glm::vec3 camera_direction(0.0f, 1.0f, -1.0f);
    float aspect_ratio = (float) IMG_WIDTH / (float) IMG_HEIGHT;
    
    // Sphere properties
    glm::vec3 sphere_center(0.0f, 0.0f, 0.0f);
    float sphere_radius = 1.0f;

    auto start_raytrace = std::chrono::high_resolution_clock::now();
    
    glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);
    glm::mat4 view_matrix = glm::lookAt(camera_position, camera_position + camera_direction, glm::vec3(0.0f, 1.0f, 0.0f));
    glUniform3fv(glGetUniformLocation(shaderProgram, "camera_position"), 1, &camera_position[0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection_matrix"), 1, GL_FALSE, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view_matrix"), 1, GL_FALSE, glm::value_ptr(view_matrix));
    glUniform3fv(glGetUniformLocation(shaderProgram, "sphere_center"), 1, &sphere_center[0]);
    glUniform1f(glGetUniformLocation(shaderProgram, "sphere_radius"), sphere_radius);

    glDispatchCompute(IMG_WIDTH / 16, IMG_HEIGHT / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    auto end_raytrace = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> raytrace_time = end_raytrace - start_raytrace;
    std::cout << "Ray tracing time: " << raytrace_time.count() << " seconds" << std::endl;

    Image image(IMG_WIDTH, IMG_HEIGHT, 4);

    auto start_image_write = std::chrono::high_resolution_clock::now();

    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());

    image.writeToFile("output.png");

    auto end_image_write = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> image_write_time = end_image_write - start_image_write;
    std::cout << "Image write time: " << image_write_time.count() << " seconds" << std::endl;

    glDeleteTextures(1, &texture);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}