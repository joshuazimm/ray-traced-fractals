#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

// Compute shader source
const char* computeShaderSource = R"(
    #version 430 core
    layout (local_size_x = 16, local_size_y = 16) in;

    layout (rgba8, binding = 0) uniform image2D img_output;

    void main() {
        ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
        vec4 color = vec4(1.0, 0.0, 0.0, 1.0); // Red color
        imageStore(img_output, pixel_coords, color);
    }
)";

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

GLuint createComputeShaderProgram() {
    GLuint computeShader = compileShader(GL_COMPUTE_SHADER, computeShaderSource);

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

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create an OpenGL context (hidden window)
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Off-screen Rendering", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Set up texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Bind texture to image unit 0
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // Create compute shader program
    GLuint shaderProgram = createComputeShaderProgram();
    glUseProgram(shaderProgram);

    // Dispatch compute shader
    glDispatchCompute(800 / 16, 600 / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Read pixels
    std::vector<unsigned char> pixels(800 * 600 * 4);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Save pixels to an image (not implemented in this example)
    // You can use a library like stb_image_write.h to save the pixels to a file

    // Cleanup
    glDeleteTextures(1, &texture);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}