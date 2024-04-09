#ifndef WORLD_H
#define WORLD_H

#include "common.hpp"
#include "Chunk.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp> // distance2

#define WORLD_SIZE_X 16
#define WORLD_SIZE_Y 16
#define WORLD_SIZE_Z 16

#define WORLD_SIZE_X_FLOAT static_cast<GLfloat>(WORLD_SIZE_X)
#define WORLD_SIZE_Y_FLOAT static_cast<GLfloat>(WORLD_SIZE_Y)
#define WORLD_SIZE_Z_FLOAT static_cast<GLfloat>(WORLD_SIZE_Z)

#define MAX_X ((CHUNK_SIZE * (WORLD_SIZE_X / 2)) - 1)
#define MAX_Y ((CHUNK_SIZE * (WORLD_SIZE_X / 2)) - 1)
#define MAX_Z ((CHUNK_SIZE * (WORLD_SIZE_X / 2)) - 1)

#define MIN_X ((-MAX_X) - 1)
#define MIN_Y ((-MAX_Y) - 1)
#define MIN_Z ((-MAX_Z) - 1)

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
	std::vector<ChunkInfo> info; // [WORLD_SIZE_X][WORLD_SIZE_Y][WORLD_SIZE_Z];
	std::vector<IndirectData> indirect; // avoid the first alocations, try different numbers
	
	QuadContainer<Quad> quads; // so I dont have to constantly alloc and free

	constexpr Chunk &get(const glm::uvec3 &position) {
		return chunks[position.x][position.y][position.z];
	}

	std::vector<ChunkInfo> &getInfo() {
		return info;
	}

	QuadContainer<Quad> &getQuads() {
		return quads;
	}

	std::vector<IndirectData> &getIndirect() {
		return indirect;
	}

	// also this can probably be optimized but for now I will leave it to compiler magic
	// yes very ugly will clean this up later
	// make player position align to the chunk somehow???????/ doing == on floats is kind of bad
	void buildData(const glm::vec3 &playerPosition) {
		quads.clear();
		indirect.clear();
		info.clear();

		GLuint start_index = 0, end_index = 0;

		for (GLuint x = 0; x < WORLD_SIZE_X; x++) {
			for (GLuint y = 0; y < WORLD_SIZE_Y; y++) {
				for (GLuint z = 0; z < WORLD_SIZE_Z; z++) {

					const glm::vec3 coords = getChunkCoordsFloat(x, y, z);

					if (playerPosition.y > coords.y + CHUNK_SIZE_FLOAT) {
						end_index += chunks[x][y][z].addQuadsTo(quads, 1); // copy quads into  my array

						indirect.emplace_back(start_index, end_index - start_index); // add indirect data
						info.emplace_back(coords, 1.0f); // add to the info buffer, for this normal
					} else if (playerPosition.y < coords.y) {
						end_index += chunks[x][y][z].addQuadsTo(quads, 0);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 0.0f);
					} else { // at same height
						end_index += chunks[x][y][z].addQuadsTo(quads, 0);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 0.0f);
						start_index = end_index;

						end_index += chunks[x][y][z].addQuadsTo(quads, 1);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 1.0f);
					}
					start_index = end_index;

					if (playerPosition.x > coords.x + CHUNK_SIZE_FLOAT) {
						end_index += chunks[x][y][z].addQuadsTo(quads, 5);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 5.0f);

					} else if (playerPosition.x < coords.x) {
						end_index += chunks[x][y][z].addQuadsTo(quads, 4);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 4.0f);

					} else {
						end_index += chunks[x][y][z].addQuadsTo(quads, 4);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 4.0f);
						start_index = end_index;

						end_index += chunks[x][y][z].addQuadsTo(quads, 5);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 5.0f);

					}
					start_index = end_index;

					if (playerPosition.z > coords.z + CHUNK_SIZE_FLOAT) {
						end_index += chunks[x][y][z].addQuadsTo(quads, 3);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 3.0f);

					} else if (playerPosition.z < coords.z) {
						end_index += chunks[x][y][z].addQuadsTo(quads, 2);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 2.0f);

					} else {
						end_index += chunks[x][y][z].addQuadsTo(quads, 2);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 2.0f);
						start_index = end_index;

						end_index += chunks[x][y][z].addQuadsTo(quads, 3);

						indirect.emplace_back(start_index, end_index - start_index);
						info.emplace_back(coords, 3.0f);
					}
					start_index = end_index;
				}
			}
		}
	}

	// from the chunk's position in array, return its world position
	constexpr glm::vec3 getChunkCoordsFloat(GLuint x, GLuint y, GLuint z) const {
		return
			glm::vec3( // make it so that [half][half][half] is roughly around (0,0,0)
				(static_cast<GLfloat>(x) - WORLD_SIZE_X_FLOAT / 2.0f) * static_cast<GLfloat>(CHUNK_SIZE),
				(static_cast<GLfloat>(y) - WORLD_SIZE_Y_FLOAT / 2.0f) * static_cast<GLfloat>(CHUNK_SIZE),
				(static_cast<GLfloat>(z) - WORLD_SIZE_Z_FLOAT / 2.0f) * static_cast<GLfloat>(CHUNK_SIZE)
			);
	}

	constexpr glm::ivec3 getChunkCoords(GLuint x, GLuint y, GLuint z) const {
		return
			glm::ivec3( // make it so that [half][half][half] is roughly around (0,0,0)
				((static_cast<GLint>(x) - WORLD_SIZE_X / 2) * CHUNK_SIZE),
				((static_cast<GLint>(y) - WORLD_SIZE_Y / 2) * CHUNK_SIZE),
				((static_cast<GLint>(z) - WORLD_SIZE_Z / 2) * CHUNK_SIZE)
			);
	}

	// same as other ones but uses chunk ID
	constexpr glm::ivec3 getChunkCoordsByID(GLuint ID) { // const
		// what the fuck?
		GLuint x = ID / (WORLD_SIZE_X * WORLD_SIZE_Y);
		GLuint idk = ID -  x * WORLD_SIZE_X * WORLD_SIZE_Y;
		GLuint y = idk / WORLD_SIZE_X;
		GLuint z = idk % WORLD_SIZE_X;

		// xyz are now [x][y][z] where chunk is located. this means this is not very optimized but since everytthing is constexpr I trust the compiler will manage this
		return getChunkCoords(x, y, z);
	}

	constexpr GLuint getChunkID(GLuint x, GLuint y, GLuint z) {
		return (&chunks[x][y][z] - &chunks[0][0][0]);
	}

	// from position within chunk, retrieve position in the world
	constexpr glm::ivec3 getWorldCoords(GLuint chunkID, glm::u8vec3 position) { // const
		const glm::ivec3 chunk_offset = getChunkCoordsByID(chunkID);
		return chunk_offset + glm::ivec3(position);
	}

	World()
	: quads(1 << 10), info(1 << 10), indirect(1 << 10) // why td is 2e10 20000000000?????????????
	{
	}

	void copyChunkTo(const Chunk &chunk, const glm::uvec3 position) {
		chunks[position.x][position.y][position.z] = chunk;
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
			pos.x = (32 - (abs(position.x) % CHUNK_SIZE)) % CHUNK_SIZE; // weird math needed since 1) negative valeus 2) starts at -1 not 0. the last % feels bad, is only for when == 32 to prevent resulting in 32. have to redo this math
		} else {
			pos.x = position.x % CHUNK_SIZE;
		}
		
		if (position.y < 0) {
			pos.y = (32 - (abs(position.y) % CHUNK_SIZE)) % CHUNK_SIZE; // weird math needed since 1) negative valeus 2) starts at -1 not 0. the last % feels bad, is only for when == 32 to prevent resulting in 32. have to redo this math
		} else {
			pos.y = position.y % CHUNK_SIZE;
		}
		
		if (position.z < 0) {
			pos.z = (32 - (abs(position.z) % CHUNK_SIZE)) % CHUNK_SIZE; // weird math needed since 1) negative valeus 2) starts at -1 not 0. the last % feels bad, is only for when == 32 to prevent resulting in 32. have to redo this math
		} else {
			pos.z = position.z % CHUNK_SIZE;
		}
		// printf("%d %d %d mapped to voxel position: %u %u %u material: %d chunk[%u][%u][%u] (%lu)\n",
		// 	position.x, position.y, position.z,
		// 	pos.x, pos.y, pos.z,
		// 	chunk.getVoxelAt(pos).material_id,
		// 	static_cast<GLuint>((static_cast<GLfloat>(position.x) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_X_FLOAT / 2.0f)),
		// 	static_cast<GLuint>((static_cast<GLfloat>(position.y) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Y_FLOAT / 2.0f)),
		// 	static_cast<GLuint>((static_cast<GLfloat>(position.z) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Z_FLOAT / 2.0f)),
		// 	&chunks[static_cast<GLuint>((static_cast<GLfloat>(position.x) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_X_FLOAT / 2.0f))][static_cast<GLuint>((static_cast<GLfloat>(position.y) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Y_FLOAT / 2.0f))][static_cast<GLuint>((static_cast<GLfloat>(position.z) / CHUNK_SIZE_FLOAT) + (WORLD_SIZE_Z_FLOAT / 2.0f))] - &chunks[0][0][0]);

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
			pos.x = (32 - (abs(position.x) % CHUNK_SIZE)) % CHUNK_SIZE; // weird math needed since 1) negative valeus 2) starts at -1 not 0. the last % feels bad, is only for when == 32 to prevent resulting in 32. have to redo this math
		} else {
			pos.x = position.x % CHUNK_SIZE;
		}
		
		if (position.y < 0) {
			pos.y = (32 - (abs(position.y) % CHUNK_SIZE)) % CHUNK_SIZE; // weird math needed since 1) negative valeus 2) starts at -1 not 0. the last % feels bad, is only for when == 32 to prevent resulting in 32. have to redo this math
		} else {
			pos.y = position.y % CHUNK_SIZE;
		}
		
		if (position.z < 0) {
			pos.z = (32 - (abs(position.z) % CHUNK_SIZE)) % CHUNK_SIZE; // weird math needed since 1) negative valeus 2) starts at -1 not 0. the last % feels bad, is only for when == 32 to prevent resulting in 32. have to redo this math
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

	SelectedBlockInfo getSelectedBlock(const glm::vec3 &position, const glm::vec3 &lookPosition, GLfloat radius) {
		radius *= 2.0f; // to act as range
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

		// constexpr int max_iter = 10; // to be removed later
		// int i = 0;

		// radius /= sqrt(dx*dx+dy*dy+dz*dz); // ?????????

		// put radius condition here?????????????????????
		while (
				// (stepX > 0 ? x < wx : x >= 0) &&
				// (stepY > 0 ? y < wy : y >= 0) &&
				// (stepZ > 0 ? z < wz : z >= 0) &&
				// (stepX < 0 ? x >= -wx : true) && // Check for negative x bounds
				// (stepY < 0 ? y >= -wy : true) && // Check for negative y bounds
				// (stepZ < 0 ? z >= -wz : true) && // Check for negative z bounds
				//(i < max_iter)) {
				(true)) {

			if (!(x < MIN_X || y < MIN_Y || z < MIN_Z || x > MAX_X || y > MAX_Y || z > MAX_Z)) {
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
			}
			// else {
			// 	break;
			// }

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


			// i++;

		}
		// nothing found within range
		return SelectedBlockInfo(-1, 0, 0, {});
	}

	// this being a reference is VERY cursed!!!!!!!
	Chunk &getChunkByID(GLuint chunkID) {
		Chunk * cursed_ptr = reinterpret_cast<Chunk *>(chunks);
		return cursed_ptr[chunkID];
	}

	// SelectedBlockInfo is what the caller will have, and it contains all the information needed to do this
	void breakVoxel(const SelectedBlockInfo &selectedInfo) {
		Chunk &chunk = getChunkByID(selectedInfo.chunkID);
		chunk.insertVoxelAt(selectedInfo.position, Voxel(-1));
	}

	void breakVoxel(const glm::ivec3 position) {
		// got lazy, this will automatically get chunk and position inside it
		const SelectedBlockInfo blockInfo = getBlockInfo(position);

		Chunk &chunk = getChunkByID(blockInfo.chunkID);
		chunk.insertVoxelAt(blockInfo.position, Voxel(-1));
	}

	void breakVoxelSphere(const SelectedBlockInfo &selectedInfo, GLfloat radius) {
		const glm::vec3 real_center_float = glm::vec3(getWorldCoords(selectedInfo.chunkID, selectedInfo.position));
		const GLfloat radius_squared = radius * radius;

		// box that sphere is contained in
		GLint min_x = glm::clamp(static_cast<GLint>(real_center_float.x - radius), MIN_X, MAX_X),
		max_x = glm::clamp(static_cast<GLint>(real_center_float.x + radius), MIN_X, MAX_X),
		min_y = glm::clamp(static_cast<GLint>(real_center_float.y - radius), MIN_Y, MAX_Y),
		max_y = glm::clamp(static_cast<GLint>(real_center_float.y + radius), MIN_Y, MAX_Y),
		min_z = glm::clamp(static_cast<GLint>(real_center_float.z - radius), MIN_Z, MAX_Z),
		max_z = glm::clamp(static_cast<GLint>(real_center_float.z + radius), MIN_Z, MAX_Z);

		GLfloat dist_squared;
		for (GLint x = min_x; x <= max_x; x++) {
			for (GLint y = min_y; y <= max_y; y++) {
				for (GLint z = min_z; z <= max_z; z++) {
					const glm::vec3 vec = glm::vec3(x, y, z);
					dist_squared = glm::distance2(real_center_float, vec);

					if (dist_squared <= radius_squared) {
						breakVoxel(glm::ivec3(vec));
					}
				}
			}
		}
	}
};


#endif
