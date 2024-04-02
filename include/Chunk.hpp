#ifndef CHUNK_H
#define CHUNK_H

#define CHUNK_SIZE 32
#define CHUNK_SIZE_FLOAT static_cast<GLfloat>(CHUNK_SIZE)

#include "common.hpp"
#include "Quad.hpp"
#include <vector>
#include "QuadContainer.hpp"

// normal {
// 	0 - y (bottom)
// 	1 + y (top)
// 	2 - z (far)
// 	3 + z (near)
// 	4 - x (left)
// 	5 + x (right)
// }

struct Voxel {
	GLint material_id;

	Voxel() : material_id(-1) {}
	Voxel(GLint material_id) : material_id(material_id) {}

	// this way I save memory, plus ints are aligned nicely
	constexpr bool isEmpty() const {
		if (material_id < 0) return true;
		return false;
	}
};

// struct sent to gpu in the chunk TBO
struct ChunkInfo {
	glm::vec3 position;

	GLfloat padding_1;
	
	ChunkInfo() : position(0.0f) {}
	ChunkInfo(const glm::vec3 &position) : position(position) {};
};
static_assert(sizeof(ChunkInfo) == 1 * sizeof(glm::vec4), "Error: ChunkInfo has unexpected size");

struct Chunk {
	GLuint ID = 0; // might get removed from here later

	// 3D array, [y][z][x] (height, depth, width). this can easily be moved around to test what gets better cache performance
	Voxel voxels[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	bool quadsHaveChanged = false;
	// [i] corresponds to normal == i
	std::vector<Quad> quads[6]; // I suspect that most chunks will have empty space so I use a vector. idk how bad this is, memory will be extremely sparse. maybe using a fixed size array here will be better, need to test

	Voxel &getVoxelAt(const glm::u8vec3 &pos) {
		return voxels[pos.y][pos.z][pos.x];
	}

	constexpr void insertVoxelAt(const glm::u8vec3 &pos, const Voxel &voxel) {
		voxels[pos.y][pos.z][pos.x] = voxel;
		quadsHaveChanged = true;
	}

	constexpr bool isEmptyAt(const glm::u8vec3 &pos) {
		return voxels[pos.y][pos.z][pos.x].isEmpty();
	}

	std::vector<Quad> getQuads(GLuint normal) {
		if (quadsHaveChanged) {
			rebuildQuads();
		}
		return this->quads[normal];
	}

	constexpr void addQuadsTo(QuadContainer<Quad> &_quads, GLuint normal) {
		if (quadsHaveChanged) {
			rebuildQuads();
		}
		_quads.add(quads[normal]);
	}

	constexpr bool voxelAt(GLuint y, GLuint z, GLuint x) {
		return ! voxels[y][z][x].isEmpty();
	}

	void rebuildQuads() {
		quadsHaveChanged = false;
		for (GLuint normal = 0; normal < 6; normal ++) {
			quads[normal].clear(); // pray that this does not change the capacity
			for (GLuint y = 0; y < CHUNK_SIZE; y++) {
				for (GLuint z = 0; z < CHUNK_SIZE; z++) {
					for (GLuint x = 0; x < CHUNK_SIZE; x++) {
						const Voxel &voxel = voxels[y][z][x];
						if (! voxel.isEmpty()) {
							switch(normal) { // the compiler will probably unroll the entire loop, but later I might do it manually anyway. this can also be optimized to only check uneven voxel positions and build everything from them
								case 0:
									if (y > 0 && voxelAt(y - 1, z, x)) {
										continue;
									}
									break;
								case 1:
									if (y < CHUNK_SIZE - 1 && voxelAt(y + 1, z, x)) {
										continue;
									}
									// printf("I am at %u %u %u, and did not detect voxel above: voxelAt(%u, %u, %u) was %u \n", x, y, z, y + 1, z, x, voxelAt(y + 1, z, x));
									break;
								case 2:
									if (z > 0 && voxelAt(y, z - 1, x)) {
										continue;
									}
									break;
								case 3:
									if (z < CHUNK_SIZE - 1 && voxelAt(y, z + 1, x)) {
										continue;
									}
									break;
								case 4:
									if (x > 0 && voxelAt(y, z, x - 1)) {
										continue;
									}
									break;
								case 5:
									if (x < CHUNK_SIZE - 1 && voxelAt(y, z, x + 1)) {
										continue;
									}
									break;	
							}
							quads[normal].emplace_back(glm::u8vec3(x, y, z), normal, voxel.material_id, ID);
						}
					}
				}
			}
		}
	}
};

#endif
