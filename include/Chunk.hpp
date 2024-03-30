#ifndef CHUNK_H
#define CHUNK_H

#define CHUNK_SIZE 32

#include "common.hpp"
#include "Quad.hpp"
#include <vector>

struct Voxel {
	GLint material_id;

	Voxel() : material_id(-1) {}
	Voxel(GLint material_id) : material_id(material_id) {}

	// this way I save memory, plus ints are aligned nicely
	bool isEmpty() const {
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

	Voxel &getVoxelAt(const glm::uvec3 &pos) {
		return voxels[pos.y][pos.z][pos.x];
	}

	void insertVoxelAt(const glm::uvec3 &pos, const Voxel &voxel) {
		voxels[pos.y][pos.z][pos.x] = voxel;
		quadsHaveChanged = true;
	}

	bool isEmptyAt(const glm::uvec3 &pos) {
		return voxels[pos.y][pos.z][pos.x].isEmpty();
	}

	std::vector<Quad> getQuads(GLuint normal) {
		if (quadsHaveChanged) {
			rebuildQuads();
		}
		return this->quads[normal];
	}

	void addQuadsTo(std::vector<Quad> &_quads, GLuint normal) {
		if (quadsHaveChanged) {
			rebuildQuads();
		}
		_quads.insert(_quads.end(), this->quads[normal].begin(), this->quads[normal].end()); // idk
	}

	void rebuildQuads() {
		quadsHaveChanged = false;
		for (GLuint normal = 0; normal < 6; normal ++) {
			quads[normal].clear(); // pray that this does not change the capacity
			// very simple algorithm for now
			for (GLuint y = 0; y < CHUNK_SIZE; y++) {
				for (GLuint z = 0; z < CHUNK_SIZE; z++) {
					for (GLuint x = 0; x < CHUNK_SIZE; x++) {
						const Voxel &voxel = voxels[y][z][x];
						if (! voxel.isEmpty()) {
							quads[normal].emplace_back(glm::uvec3(x, y, z), normal, voxel.material_id, ID);
						}
					}
				}
			}
		}
	}
};

#endif
