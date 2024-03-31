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

#define NORMAL_NEG_Y 0
#define NORMAL_POS_Y 1
#define NORMAL_NEG_Z 2
#define NORMAL_POS_Z 3
#define NORMAL_NEG_X 4
#define NORMAL_POS_X 5

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

	

	// I have no idea if this math is correct
	// basically / chunk size to get a chunk ID
	// then add world size / 2 due to the offset that makes chunks[half][half][half] contain (0, 0, 0)
	const Voxel &getVoxel(const glm::ivec3 &position) {

		// had weird innacuracies when values were not floats, like -7.15 + 8 == 0 but actualy was -7 + 8 == 1

		Chunk & chunk = chunks // this gets ID of the chunk
			[static_cast<GLuint>((static_cast<GLfloat>(position.x) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_X_FLOAT / 2.0f))]
			[static_cast<GLuint>((static_cast<GLfloat>(position.y) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Y_FLOAT / 2.0f))]
			[static_cast<GLuint>((static_cast<GLfloat>(position.z) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Z_FLOAT / 2.0f))];


		// this gets position inside of the chunk
		glm::uvec3 pos;
		if (position.x < 0) {
			pos.x = 32 - (abs(position).x % CHUNK_SIZE);
		} else {
			pos.x = position.x % CHUNK_SIZE;
		}
		
		if (position.y < 0) {
			pos.y = 32 - (abs(position).y % CHUNK_SIZE);
		} else {
			pos.y = position.y % CHUNK_SIZE;
		}
		
		if (position.z < 0) {
			pos.z = 32 - (abs(position).z % CHUNK_SIZE);
		} else {
			pos.z = position.z % CHUNK_SIZE;
		}
		// printf("%d %d %d mapped to voxel position: %u %u %u material: %d chunk[%u][%u][%u] (%lu)\n",
			position.x, position.y, position.z,
			pos.x, pos.y, pos.z,
			chunk.getVoxelAt(pos).material_id,
			static_cast<GLuint>((static_cast<GLfloat>(position.x) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_X_FLOAT / 2.0f)),
			static_cast<GLuint>((static_cast<GLfloat>(position.y) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Y_FLOAT / 2.0f)),
			static_cast<GLuint>((static_cast<GLfloat>(position.z) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Z_FLOAT / 2.0f)),
			&chunks[static_cast<GLuint>((static_cast<GLfloat>(position.x) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_X_FLOAT / 2.0f))][static_cast<GLuint>((static_cast<GLfloat>(position.y) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Y_FLOAT / 2.0f))][static_cast<GLuint>((static_cast<GLfloat>(position.z) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Z_FLOAT / 2.0f))] - &chunks[0][0][0]);

		return chunk.getVoxelAt(pos);
	}

	// same as above, but it returns a SelectedBlockInfo so I don't have to get chunkID and position again
	SelectedBlockInfo getBlockInfo(const glm::ivec3 &position) {
		SelectedBlockInfo ret;

		// had weird innacuracies when values were not floats, like -7.15 + 8 == 0 but actualy was -7 + 8 == 1

		Chunk *chunk = &chunks // this gets ID of the chunk
			[static_cast<GLuint>((static_cast<GLfloat>(position.x) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_X_FLOAT / 2.0f))]
			[static_cast<GLuint>((static_cast<GLfloat>(position.y) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Y_FLOAT / 2.0f))]
			[static_cast<GLuint>((static_cast<GLfloat>(position.z) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Z_FLOAT / 2.0f))];

		ret.chunkID = chunk - &chunks[0][0][0];

		// this gets position inside of the chunk
		glm::u8vec3 pos;
		if (position.x < 0) {
			pos.x = 32 - (abs(position).x % CHUNK_SIZE);
		} else {
			pos.x = position.x % CHUNK_SIZE;
		}
		
		if (position.y < 0) {
			pos.y = 32 - (abs(position).y % CHUNK_SIZE);
		} else {
			pos.y = position.y % CHUNK_SIZE;
		}
		
		if (position.z < 0) {
			pos.z = 32 - (abs(position).z % CHUNK_SIZE);
		} else {
			pos.z = position.z % CHUNK_SIZE;
		}

		ret.position = pos;
		ret.materialID = chunk->getVoxelAt(pos).material_id;

		return ret;
	}

	constexpr float custom_mod (float value, float modulus) {
		return fmod(fmod(value, modulus) + modulus, modulus);
	}

	constexpr float intbound(float s, float ds) {
		// Find the smallest positive t such that s+t*ds is an integer.
		if (ds < 0) {
			return intbound(-s, -ds);
		} else {
			s = custom_mod(s, 1.0f);
			// The problem is now s + t * ds = 1.0
			return (1.0f - s) / ds;
		}
	}

	constexpr int signum(float x) {
		return (x > 0) ? 1 : (x < 0) ? -1 : 0;
	}

	SelectedBlockInfo getSelectedBlock(const glm::vec3 &position, const glm::vec3 &lookPosition) {
		int x = static_cast<int>(std::floor(position.x));
		int y = static_cast<int>(std::floor(position.y));
		int z = static_cast<int>(std::floor(position.z));

		const float dx = lookPosition.x;
		const float dy = lookPosition.y;
		const float dz = lookPosition.z;

		const int stepX = signum(dx);
		const int stepY = signum(dy);
		const int stepZ = signum(dz);

		float tMaxX = intbound(position.x, dx);
		float tMaxY = intbound(position.y, dy);
		float tMaxZ = intbound(position.z, dz);

		const float tDeltaX = stepX / dx;
		const float tDeltaY = stepY / dy;
		const float tDeltaZ = stepZ / dz;

		if (dx == 0 && dy == 0 && dz == 0) {
			// throw std::range_error("Raycast in zero direction!");
			return SelectedBlockInfo(-1, 0, 0, {});
		}

		// will optimize this later
		char face[3] = {0, 0, 0};

		// size of the world, will change in the future
		constexpr int wx = (WORLD_SIZE_X * CHUNK_SIZE) / 2;
		constexpr int wy = (WORLD_SIZE_Y * CHUNK_SIZE) / 2;
		constexpr int wz = (WORLD_SIZE_Z * CHUNK_SIZE) / 2;

		float radius = 10.0f; // max distance
		constexpr int max_iter = 10; // to be removed later
		int i = 0;

		// radius /= sqrt(dx*dx+dy*dy+dz*dz); // ?????????

		// put radius condition here?????????????????????
		while (
				// (stepX > 0 ? x < wx : x >= 0) &&
				// (stepY > 0 ? y < wy : y >= 0) &&
				// (stepZ > 0 ? z < wz : z >= 0) &&
				// (stepX < 0 ? x >= -wx : true) && // Check for negative x bounds
				// (stepY < 0 ? y >= -wy : true) && // Check for negative y bounds
				// (stepZ < 0 ? z >= -wz : true) && // Check for negative z bounds
				(i < max_iter)) {

			if (!(x <= -wx || y <= -wy || z <= -wz || x >= wx || y >= wy || z >= wz)) {
				const glm::ivec3 coords = glm::ivec3(x, y, z);

				SelectedBlockInfo blockInfo = getBlockInfo(coords);

				if (! blockInfo.isEmpty()) {

					// printf("%d %d %d face %d %d %d\n", x, y, z, face[0], face[1], face[2]);
					// printf("detected a block at %d %d %d\n", coords.x, coords.y, coords.z);

					// will optimize this later
					if (face[0] == 1) blockInfo.normal = NORMAL_POS_X;
					else if (face[0] == -1) blockInfo.normal = NORMAL_NEG_X;

					if (face[1] == 1) blockInfo.normal = NORMAL_POS_Y;
					else if (face[1] == -1) blockInfo.normal = NORMAL_NEG_Y;

					if (face[2] == 1) blockInfo.normal = NORMAL_POS_Z;
					else if (face[2] == -1) blockInfo.normal = NORMAL_NEG_Z;
					return blockInfo;
				}
			} else { // is this needed????????
				break;
			}

			if (tMaxX < tMaxY) {
				if (tMaxX < tMaxZ) {
					if (tMaxX > radius) break;
					x += stepX;
					tMaxX += tDeltaX;
					face[0] = -stepX;
					face[1] = 0;
					face[2] = 0;
				} else {
					if (tMaxZ > radius) break;
					z += stepZ;
					tMaxZ += tDeltaZ;
					face[0] = 0;
					face[1] = 0;
					face[2] = -stepZ;
				}
			} else {
				if (tMaxY < tMaxZ) {
					if (tMaxY > radius) break;
					y += stepY;
					tMaxY += tDeltaY;
					face[0] = 0;
					face[1] = -stepY;
					face[2] = 0;
				} else {
					if (tMaxZ > radius) break;
					z += stepZ;
					tMaxZ += tDeltaZ;
					face[0] = 0;
					face[1] = 0;
					face[2] = -stepZ;
				}
			}


			i++;

		}
		// nothing found within range
		return SelectedBlockInfo(-1, 0, 0, {});
	}
};


#endif