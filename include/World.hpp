#ifndef WORLD_H
#define WORLD_H

#include "common.hpp"
#include "Chunk.hpp"
#include <array>

#define WORLD_SIZE_X 16
#define WORLD_SIZE_Y 16
#define WORLD_SIZE_Z 16

#define WORLD_SIZE_X_FLOAT static_cast<GLfloat>(WORLD_SIZE_X)
#define WORLD_SIZE_Y_FLOAT static_cast<GLfloat>(WORLD_SIZE_Y)
#define WORLD_SIZE_Z_FLOAT static_cast<GLfloat>(WORLD_SIZE_Z)

// normal {
// 	0 - y (bottom)
// 	1 + y (top)
// 	2 - z (far)
// 	3 + z (near)
// 	4 - x (left)
// 	5 + x (right)
// }

struct World {
	Chunk chunks[WORLD_SIZE_X][WORLD_SIZE_Y][WORLD_SIZE_Z]; // this order can be changed, need to test it for performance
	ChunkInfo info[WORLD_SIZE_X][WORLD_SIZE_Y][WORLD_SIZE_Z];
	std::vector<Quad> quads; // so I dont have to constantly alloca ad free

	constexpr Chunk &get(const glm::uvec3 &position) {
		return chunks[position.x][position.y][position.z];
	}

	const custom_array<ChunkInfo> getInfo() {
		return custom_array(&info[0][0][0], WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z); // ??????? use [max] and not [max - 1] ??????? idk, it works
	}

	// also this can probably be optimized but for now I will leave it to compiler magic
	std::vector<Quad> getQuads(const glm::vec3 &playerPosition) {
		quads.clear();

		for (GLuint x = 0; x < WORLD_SIZE_X; x++) {
			for (GLuint y = 0; y < WORLD_SIZE_Y; y++) {
				for (GLuint z = 0; z < WORLD_SIZE_Z; z++) {
					const glm::vec3 coords = getRealCoords(x, y, z);
					if (playerPosition.y > coords.y + CHUNK_SIZE_FLOAT) {
						chunks[x][y][z].addQuadsTo(quads, 1);
					} else if (playerPosition.y < coords.y) {
						chunks[x][y][z].addQuadsTo(quads, 0);
					} else { // at same height
						chunks[x][y][z].addQuadsTo(quads, 0);
						chunks[x][y][z].addQuadsTo(quads, 1);
					}

					if (playerPosition.x > coords.x + CHUNK_SIZE_FLOAT) {
						chunks[x][y][z].addQuadsTo(quads, 5);
					} else if (playerPosition.x < coords.x) {
						chunks[x][y][z].addQuadsTo(quads, 4);
					} else {
						chunks[x][y][z].addQuadsTo(quads, 4);
						chunks[x][y][z].addQuadsTo(quads, 5);
					}

					if (playerPosition.z > coords.z + CHUNK_SIZE_FLOAT) {
						chunks[x][y][z].addQuadsTo(quads, 3);
					} else if (playerPosition.z < coords.z) {
						chunks[x][y][z].addQuadsTo(quads, 2);
					} else {
						chunks[x][y][z].addQuadsTo(quads, 2);
						chunks[x][y][z].addQuadsTo(quads, 3);
					}
				}
			}
		}

		return quads;
	}

	constexpr glm::vec3 getRealCoords(GLuint x, GLuint y, GLuint z) const {
		return
			glm::vec3( // make it so that [half][half][half] is roughly around (0,0,0)
				(static_cast<GLfloat>(x) - WORLD_SIZE_X_FLOAT / 2.0f) * static_cast<GLfloat>(CHUNK_SIZE),
				(static_cast<GLfloat>(y) - WORLD_SIZE_Y_FLOAT / 2.0f) * static_cast<GLfloat>(CHUNK_SIZE),
				(static_cast<GLfloat>(z) - WORLD_SIZE_Z_FLOAT / 2.0f) * static_cast<GLfloat>(CHUNK_SIZE)
			);
	}

	World() {
		// build the info
		for (GLuint x = 0; x < WORLD_SIZE_X; x++) {
			for (GLuint y = 0; y < WORLD_SIZE_Y; y++) {
				for (GLuint z = 0; z < WORLD_SIZE_Z; z++) {
					info[x][y][z] = ChunkInfo(
						getRealCoords(x, y, z)
					);
					chunks[x][y][z].ID = &chunks[x][y][z] - &chunks[0][0][0];// idc let the compiler sort this out
					// printf("ID %u is in %f %f %f\n", chunks[x][y][z].ID, info[x][y][z].position.x, info[x][y][z].position.y, info[x][y][z].position.z);
				}
			}
		}
	}

	// void addVoxelAt(const glm::uvec3 &position) {
	// got lazy didnt feel like doing the math
	// }

	void copyChunkTo(const Chunk &chunk, const glm::uvec3 position) {
		// printf("\n\nThe id of the chunk in %u %u %u is %u. Its info position is %f %f %f\n\n",
		// 	position.x, position.y, position.z,
		// 	chunks[position.x][position.y][position.z].ID,
		// 	info[position.x][position.y][position.z].position.x,
		// 	info[position.x][position.y][position.z].position.y,
		// 	info[position.x][position.y][position.z].position.z);
		GLuint ID = chunks[position.x][position.y][position.z].ID;
		chunks[position.x][position.y][position.z] = chunk;
		// for safety, so it applies new ID to all quads (ID was probably not defined, if it was then remove this)
		chunks[position.x][position.y][position.z].ID = ID;
		chunks[position.x][position.y][position.z].quadsHaveChanged = true;
	}
};


#endif
