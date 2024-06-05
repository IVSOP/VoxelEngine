#ifndef PLAYER_H
#define PLAYER_H

#include "Camera.hpp"

class Player {
public:
	Camera camera;

	Player();
	Player(const glm::vec3 &position, const glm::vec3 &lookat, const glm::vec3 &up);
	~Player() = default;

	Camera *getCamera() { return &camera; };
};

#endif
