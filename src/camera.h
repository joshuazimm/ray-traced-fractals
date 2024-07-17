#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();

    // Setters
    void setPosition(const glm::vec3& position);
    void setRotation(float yaw, float pitch, float roll);

    // Getters
    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const;

    // Camera transformations
    void translate(const glm::vec3& offset);
    void rotate(float yaw, float pitch, float roll);

    // Movement functions
    void moveForward(float deltaTime);
    void moveBackward(float deltaTime);
    void moveLeft(float deltaTime);
    void moveRight(float deltaTime);

private:
    glm::vec3 position;
    float yaw;
    float pitch;
    float roll;
    glm::mat4 viewMatrix;

    void updateViewMatrix();
    glm::vec3 getFront() const;
};

#endif // CAMERA_H