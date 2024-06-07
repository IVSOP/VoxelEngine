#include "Player.hpp"

// Player::Player()
// : camera()
// { }

Player::Player(const glm::vec3 &position, const glm::vec3 &lookat, const glm::vec3 &up)
: camera(position, lookat, up)
{ }

Player::Player(std::ifstream &file)
: camera(file)
{ }

void Player::saveTo(std::ofstream &file) const {
	camera.saveTo(file);
}
