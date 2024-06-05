#include "Player.hpp"

Player::Player()
: camera()
{ }

Player::Player(const glm::vec3 &position, const glm::vec3 &lookat, const glm::vec3 &up)
: camera(position, lookat, up)
{ }
