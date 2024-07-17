#include "camera.h"

Camera::Camera()
    : position(glm::vec3(0.0f, 0.0f, 3.0f)), yaw(-90.0f), pitch(0.0f), roll(0.0f) {
    updateViewMatrix();
}

void Camera::setPosition(const glm::vec3& position) {
    this->position = position;
    updateViewMatrix();
}

void Camera::setRotation(float yaw, float pitch, float roll) {
    this->yaw = yaw;
    this->pitch = pitch;
    this->roll = roll;
    updateViewMatrix();
}

glm::mat4 Camera::getViewMatrix() const {
    return viewMatrix;
}

glm::vec3 Camera::getPosition() const {
    return position;
}

void Camera::translate(const glm::vec3& offset) {
    position += offset;
    updateViewMatrix();
}

void Camera::rotate(float yaw, float pitch, float roll) {
    this->yaw += yaw;
    this->pitch += pitch;
    this->roll += roll;
    updateViewMatrix();
}

void Camera::moveForward(float deltaTime) {
    position += getFront() * deltaTime;
    updateViewMatrix();
}

void Camera::moveBackward(float deltaTime) {
    position -= getFront() * deltaTime;
    updateViewMatrix();
}

void Camera::moveLeft(float deltaTime) {
    position -= glm::normalize(glm::cross(getFront(), glm::vec3(0.0f, 1.0f, 0.0f))) * deltaTime;
    updateViewMatrix();
}

void Camera::moveRight(float deltaTime) {
    position += glm::normalize(glm::cross(getFront(), glm::vec3(0.0f, 1.0f, 0.0f))) * deltaTime;
    updateViewMatrix();
}

void Camera::updateViewMatrix() {
    glm::vec3 front = getFront();
    // Assuming up vector is (0, 1, 0)
    viewMatrix = glm::lookAt(position, position + front, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::getFront() const {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::normalize(front);
}