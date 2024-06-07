#ifndef PLAYER_H
#define PLAYER_H

#include "Camera.hpp"

class Player {
public:
	Camera camera;

	Player() = delete;
	Player(const glm::vec3 &position, const glm::vec3 &lookat, const glm::vec3 &up);
	~Player() = default;

	Camera *getCamera() { return &camera; };

	Player(std::ifstream &file);
	void saveTo(std::ofstream &file) const;
};

#endif
