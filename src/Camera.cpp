#include "Camera.hpp"

Camera::Camera(glm::vec3 position)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), Zoom(45.0f), MouseSensitivity(0.002f), MovementSpeed(2.5f),
      Yaw(-90.0f), Pitch(0.0f), WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)) {
    Position = position;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(Camera_Movement dir, float dt) {
    float velocity = MovementSpeed * dt;
    if (dir == FORWARD) Position += Front * velocity;
    if (dir == BACKWARD) Position -= Front * velocity;
    if (dir == LEFT) Position -= Right * velocity;
    if (dir == RIGHT) Position += Right * velocity;
}

void Camera::ProcessMouseMovement(float xoff, float yoff) {
    xoff *= MouseSensitivity;
    yoff *= MouseSensitivity;
    Yaw += xoff;
    Pitch += yoff;
    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;
    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}