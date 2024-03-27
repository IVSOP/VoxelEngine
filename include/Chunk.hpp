#ifndef CHUNK_H
#define CHUNK_H

#define CHUNK_SIZE 32

#include "common.hpp"
#include <array>

// very similar to a Quad, but it is more flexible to use a diferent struct. also does not contain length
// I will still use the weid bitshifts since I plan on having many voxels and saving space is always good
struct Voxel {
	GLint position_and_normal = 0;
	GLint material_id = 0;

	Voxel(const glm::uvec3 &pos, GLint normal, GLubyte _material_id) {
		position_and_normal = (pos.x << 24) & 0xFF000000 | (pos.y << 16) & 0x00FF0000 | (pos.z << 8) & 0x0000FF00 | normal & 0x000000FF;

		material_id |= (_material_id << 24);
	}

};

struct Chunk {
	// 3D array, [y][z][x] (height, depth, width). this can easily be moved around to test what gets better cache performance
	Voxel voxels[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

	Voxel &getVoxelAt(const glm::uvec3 &pos) {
		return voxels[pos.y][pos.z][pos.x];
	}

	void insertVoxelAt(const glm::uvec3 &pos, const Voxel &voxel) {
		voxels[pos.y][pos.z][pos.x] = voxel;
	}
};

#endif
