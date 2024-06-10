#ifndef CHUNK_H
#define CHUNK_H

#include <chrono>

#define CHUNK_SIZE 32
#define CHUNK_SIZE_FLOAT static_cast<GLfloat>(CHUNK_SIZE)

static_assert(CHUNK_SIZE == 32, "Chunk size needs to be 32 due to hardcoded values in greedy meshing");

#include "common.hpp"
#include "Quad.hpp"
#include <vector>
#include "QuadContainer.hpp"
#include "Bitmap.hpp"

// normal {
// 	0 - y (bottom)
// 	1 + y (top)
// 	2 - z (far)
// 	3 + z (near)
// 	4 - x (left)
// 	5 + x (forward)
// }

struct Voxel {
	GLbyte material_id;

	constexpr Voxel() : material_id(-1) {}
	constexpr Voxel(GLbyte material_id) : material_id(material_id) {}

	// constexpr bool isEmpty() const {
	// 	if (material_id < 0) return true;
	// 	return false;
	// }
};

// struct sent to gpu in the chunk TBO
struct ChunkInfo {
	glm::vec3 position;
	GLfloat normal;
	
	ChunkInfo() : position(0.0f), normal(6.0f) {}
	ChunkInfo(const glm::vec3 &position, GLfloat normal) : position(position), normal(normal) {};
};
static_assert(sizeof(ChunkInfo) == 1 * sizeof(glm::vec4), "Error: ChunkInfo has unexpected size");

struct Chunk {
	// 3D array, [y][z][x] (height, depth, width). this can easily be moved around to test what gets better cache performance
	Voxel voxels[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	bool quadsHaveChanged = true;
	// [i] corresponds to normal == i
	std::vector<Quad> quads[6]; // I suspect that most chunks will have empty space so I use a vector. idk how bad this is, memory will be extremely sparse. maybe using a fixed size array here will be better, need to test
	Bitmap<CHUNK_SIZE> visited[CHUNK_SIZE]; //  CAN BE USED AS [y][x] // visited[y] has all info on that row (values vary along x). basically a bitmap for every single row (for easier math)

	// tells what positions are filled by an opaque block or not
	// used as [y][z] to get the bitmask
	// 1 == is filled
	std::array<std::array<Bitmap<CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE> opaqueMask;

	Voxel &getVoxelAt(const glm::u8vec3 &pos) {
		return voxels[pos.y][pos.z][pos.x];
	}

	constexpr void insertVoxelAt(const glm::u8vec3 &pos, const Voxel &voxel) {
		voxels[pos.y][pos.z][pos.x] = voxel;
		opaqueMask[pos.y][pos.z].setBit(pos.x);
		quadsHaveChanged = true;
	}

	constexpr bool isEmptyAt(const glm::u8vec3 &pos) {
		return ! opaqueMask[pos.y][pos.z][pos.x];
	}

	constexpr bool isEmptyAt(GLubyte x, GLubyte y, GLubyte z) {
		// printf("%d %d %d is %d\n", x, y, z, opaqueMask[y][z][x]);
		return ! opaqueMask[y][z][x];
	}

	constexpr void breakVoxelAt(GLubyte x, GLubyte y, GLubyte z) {
		opaqueMask[y][z].clearBit(x);
		quadsHaveChanged = true;
	}

	constexpr void breakVoxelAt(const glm::u8vec3 &pos) {
		opaqueMask[pos.y][pos.z].clearBit(pos.x);
		quadsHaveChanged = true;
	}

	std::vector<Quad> getQuads(GLuint normal) {
		if (quadsHaveChanged) {
			rebuildQuads();
		}
		return this->quads[normal];
	}

	// returns how much was added
	constexpr GLuint addQuadsTo(QuadContainer<Quad> &_quads, GLuint normal) {
		if (quadsHaveChanged) {
			rebuildQuads();
		}
		_quads.add(quads[normal]);
		return quads[normal].size();
	}

	constexpr bool voxelAt(GLuint x, GLuint y, GLuint z) {
		return ! isEmptyAt(x, y, z);
	}

	GLbyte get_material(const int axis, const int a, const int b, const int c) {
		if (axis == 0)
			return voxels[c][a][b].material_id;
		else if (axis == 2)
			return voxels[b][c][a].material_id;
		else
			return voxels[a][b][c].material_id;
	}

	void rebuildQuads() {
		quadsHaveChanged = false;

		auto start = std::chrono::high_resolution_clock::now();

		// [face][y][z][x]
		std::array<std::array<std::array<Bitmap<CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE>, 6 + 2> faceMasks;

		Bitmap<CHUNK_SIZE> currOpaqueMask;

		// this black magic is to take the big occlusion mask and make a smaller one for each face,
		// using bitwise operations to perform the culling
		// TODO rework these ifs!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		for (GLuint y = 0; y < CHUNK_SIZE; y++) {
			for (GLuint z = 0; z < CHUNK_SIZE; z++) {
				currOpaqueMask = opaqueMask[y][z];

				if (y > 0) {
					faceMasks[0][y][z] = currOpaqueMask & (~opaqueMask[y - 1][z]);
				} else {
					faceMasks[0][y][z] = currOpaqueMask;
				}
				if (y < CHUNK_SIZE - 1) {
					faceMasks[1][y][z] = currOpaqueMask & (~opaqueMask[y + 1][z]);
				} else {
					faceMasks[1][y][z] = currOpaqueMask;
				}

				// THE ORDER HERE IS SWITCHED!!!!!!!!!!!!!!!! for better cache locality
				if (z > 0) {
					faceMasks[2][z][y] = currOpaqueMask & (~opaqueMask[y][z - 1]);
				} else {
					faceMasks[2][z][y] = currOpaqueMask;
				}
				if (z < CHUNK_SIZE - 1) {
					faceMasks[3][z][y] = currOpaqueMask & (~opaqueMask[y][z + 1]);
				} else {
					faceMasks[3][z][y] = currOpaqueMask;
				}

				// x and -x uses bitshifts since this is the same direction as the mask itself
				// will be transposed later into [5] and [6] so placed here for now
				faceMasks[6][y][z] = currOpaqueMask & ~(opaqueMask[y][z] << 1);
				faceMasks[7][y][z] = currOpaqueMask & ~(opaqueMask[y][z] >> 1);
			}
		}

		// here, faceMasks[normal] already indicates what can and cannot be drawn, culled for occlusion

		// -/+ y
		{
			GLbyte material;
			GLuint start_x, end_x;
			GLuint len_x, len_z;
			Bitmap<32> mask;
			GLuint x;
			for (uint8_t face = 0; face < 2; face++) {
				quads[face].clear();
				for (GLuint y = 0; y < CHUNK_SIZE; y++) {
					for (GLuint z = 0; z < CHUNK_SIZE;) { // do not do z++ unless all bits are set to 0 (the first if below this)
						mask = faceMasks[face][y][z];
						start_x = mask.trailing_zeroes();

						// all bits are at 0, nothing to do at this z level
						// this is cursed
						if (start_x == CHUNK_SIZE) {
							z++;
							continue;
						}

						material = voxels[y][z][start_x].material_id;

						// printf("initial mask:                  ");
						// mask.print();

						// shift right to make the 1s be at the start. this way, trailing_ones will get you the end position
						mask >>= start_x;

						// printf("after shift right:             ");
						// mask.print();

						end_x = mask.trailing_ones();


						// here we already have start and end, need to check if all materials are the same
						// go in a loop from start to end. when finished or material is different, change end
						for (x = 1; x < end_x; x++) {
							if (voxels[y][z][x + start_x].material_id != material) {
								break;
							}
						}
						// here, [x] contains the last valid index
						// end is always index + 1
						end_x = x;

						// two shifts to cleanup the mask so only bits set to 1 are the ones we are considering
						mask <<= (CHUNK_SIZE - end_x);
						// printf("after shift left:              ");
						// mask.print();
						mask >>= (CHUNK_SIZE - (end_x + start_x));
						// printf("after shift right again:       ");
						// mask.print();

						len_x = start_x - end_x;


						// commit changes to original mask, zero out the bits in use
						faceMasks[face][y][z] &= ~mask;

						// printf("after commiting changes:       ");
						// faceMasks[face][y][z].print();


						// loop to go over and check next Z values
						GLuint end_z = z + 1;
						for (; end_z < CHUNK_SIZE; end_z++) {
							// use bit operations to check if there are voxels in the necessary positions
							// faceMasks[face][y][end_z].print();
							if ((mask & faceMasks[face][y][end_z]) == mask) {
								// the voxels are in the right place, just need to check materials
								for (x = 1; x < end_x; x++) {
									if (voxels[y][end_z][x + start_x].material_id != material) {
										break;
									}
								}
								if (x != end_x) { // previous loop failed, materials are not all the same
									// last valid z was z-1
									// but it is already expected for end_z to be 1 greater, so do nothing
									break;
								}

								// commit changes to the mask
								faceMasks[face][y][end_z] &= ~mask;
							} else {
								// end_z--;
								// printf("z was %u, end is %u\n", z, end_z);
								break;
							}
						}
						// -1 since they are guaranteed to never be 0. this way we can save one bit
						len_z = end_z - z - 1;
						len_x = end_x - 1;

						// printf("x:%u y:%u z:%u len_x:%u len_z:%u start_x:%u end_x:%u\n", start_x, y, z, len_x, len_z, start_x, end_x);
						// printf("x: from %u to %u\nz: from %u to %u\n", start_x, start_x + len_x, z, z + len_z);

						quads[face].emplace_back(glm::u8vec3(start_x, y, z),
										material,
										static_cast<GLfloat>(len_x), static_cast<GLfloat>(len_z));
					}
				}
			}
		}

		// -/+ z
		{
			GLuint start_x, end_x;
			GLuint len_x, len_y;
			Bitmap<32> mask;
			GLuint x;
			GLbyte material;
			for (uint8_t face = 2; face < 4; face++) {
				quads[face].clear();
				for (GLuint z = 0; z < CHUNK_SIZE; z++) {
					for (GLuint y = 0; y < CHUNK_SIZE;) { // do not do y++ unless all bits are set to 0 (the first if below this)
						mask = faceMasks[face][z][y];
						start_x = mask.trailing_zeroes();

						// all bits are at 0, nothing to do at this z level
						// this is cursed
						if (start_x == CHUNK_SIZE) {
							y++;
							continue;
						}

						material = voxels[y][z][start_x].material_id;

						mask >>= start_x;

						end_x = mask.trailing_ones();

						for (x = 1; x < end_x; x++) {
							if (voxels[y][z][x + start_x].material_id != material) {
								break;
							}
						}
						end_x = x;

						mask <<= (CHUNK_SIZE - end_x);
						mask >>= (CHUNK_SIZE - (end_x + start_x));

						len_x = start_x - end_x;


						// here, mask contains 1 for the voxels we need to check
						// check if all their materias match, if not reduce end_x until they do
						// for now will assume yes

						// commit changes to original mask, zero out the bits in use
						faceMasks[face][z][y] &= ~mask;


						// loop to go over and check next Z values
						GLuint end_y = y + 1;
						for (; end_y < CHUNK_SIZE; end_y++) {
							// use bit operations to check if there are voxels in the necessary positions
							if ((mask & faceMasks[face][z][end_y]) == mask) {
								// the voxels are in the right place, just need to check materials
								for (x = 1; x < end_x; x++) {
									if (voxels[end_y][z][x + start_x].material_id != material) {
										break;
									}

								}
								if (x != end_x) {
									break;
								}

								// commit changes to the mask
								faceMasks[face][z][end_y] &= ~mask;
							} else {
								break;
							}
						}
						// -1 since they are guaranteed to never be 0. this way we can save one bit
						len_y = end_y - y - 1;
						len_x = end_x - 1;

						quads[face].emplace_back(glm::u8vec3(start_x, y, z),
										material,
										static_cast<GLfloat>(len_x), static_cast<GLfloat>(len_y));
					}
				}
			}
		}

		// for x axis, meshing will be a mess and I won't be able to use the same algorithm
		// goal: masks[y][x] has the bits on the Z axis
		// TODO optimize this
		{
			std::array<Bitmap<CHUNK_SIZE>, 2> masks;
			Bitmap<CHUNK_SIZE> clear_mask = 0x00000001; // mask to clear all but the last bit
			for (GLuint y = 0; y < CHUNK_SIZE; y++) {
				// x to be filled in final masks
				for (GLuint x = 0; x < CHUNK_SIZE; x++) {
					// reset for safety to not screw up the | below, but rewrite this in the future
					masks[0].clear();
					masks[1].clear();

					// need to consult mask [y][z][x] to get the bit
					for (GLuint z = 0; z < CHUNK_SIZE; z++) {
						// this is the logic of what I need to do here:
						// if (faceMasks[6][y][z][x] == true) {
						// 	masks[0].setBit(z);
						// }
						// if (faceMasks[7][y][z][x] == true) {
						// 	masks[1].setBit(z);
						// }
						// first shift is to end with either 0x0 or 0x1
						// & is to clear all except for that bit
						// next shift moves it to position z on the final mask
						masks[0] |= (((faceMasks[6][y][z] >> x) & clear_mask) << z);
						masks[1] |= (((faceMasks[7][y][z] >> x) & clear_mask) << z);
					}

					faceMasks[4][y][x] = masks[0];
					faceMasks[5][y][x] = masks[1];
				}
			}
		}


		// -/+ x
		{
			GLuint start_z, end_z;
			GLuint len_y, len_z;
			Bitmap<32> mask;
			GLuint z;
			GLbyte material;
			for (uint8_t face = 4; face < 6; face++) {
				quads[face].clear();
				for (GLuint x = 0; x < CHUNK_SIZE; x++) {
					for (GLuint y = 0; y < CHUNK_SIZE;) { // do not do y++ unless all bits are set to 0 (the first if below this)
						mask = faceMasks[face][y][x];
						start_z = mask.trailing_zeroes();



						// all bits are at 0, nothing to do at this z level
						// this is cursed
						if (start_z == CHUNK_SIZE) {
							y++;
							// puts("all empty");
							continue;
						}

						// printf("initial mask:                  ");
						// mask.print();

						material = voxels[y][start_z][x].material_id;

						mask >>= start_z;
						// printf("after shift right:             ");
						// mask.print();

						end_z = mask.trailing_ones();
						// printf("end_z: %u\n", end_z);

						for (z = 1; z < end_z; z++) {
							if (voxels[y][z + start_z][x].material_id != material) {
								break;
							}
						}
						end_z = z;

						// printf("after material check, end_z is %u\n", end_z);

						mask <<= (CHUNK_SIZE - end_z);
						// printf("after shift left:              ");
						// mask.print();
						mask >>= (CHUNK_SIZE - (end_z + start_z));
						// printf("after shift right again:       ");
						// mask.print();

						len_z = start_z - end_z;

						// here, mask contains 1 for the voxels we need to check
						// check if all their materias match, if not reduce end_x until they do
						// for now will assume yes

						// commit changes to original mask, zero out the bits in use
						faceMasks[face][y][x] &= ~mask;


						// loop to go over and check next Z values
						GLuint end_y = y + 1;
						for (; end_y < CHUNK_SIZE; end_y++) {
							// use bit operations to check if there are voxels in the necessary positions
							if ((mask & faceMasks[face][end_y][x]) == mask) {
								// the voxels are in the right place, just need to check materials
								for (z = 1; z < end_z; z++) {
									if (voxels[end_y][z + start_z][x].material_id != material) {
										break;
									}

									if (z != end_z) {
										break;
									}
								}

								// commit changes to the mask
								faceMasks[face][end_y][x] &= ~mask;
							} else {
								break;
							}
						}
						// -1 since they are guaranteed to never be 0. this way we can save one bit
						len_y = end_y - y - 1;
						len_z = end_z - 1;

						// printf("x: %u\nz: from %u to %u\ny: from %u to %u\nstart_z: %u end_z: %u\n\n", x, start_z, start_z + len_z, y, y + len_y, start_z, end_z);

						quads[face].emplace_back(glm::u8vec3(x, y, start_z),
										material,
										static_cast<GLfloat>(len_z), static_cast<GLfloat>(len_y));
					}
				}
			}
		}

		// exit(0);

		// this is a changed version of the greedy mesher that is on github
		// for (uint8_t face = 0; face < 6; face++) {
		// 	int axis = face / 2;

		// 	// reset merged_up
		// 	memset(reinterpret_cast<void *>(&merged_up[0][0]), 0, sizeof(merged_up));

		// 	for (GLuint up = 0; up < CHUNK_SIZE; up++) {
		// 		// reset merged_forward
		// 		memset(reinterpret_cast<void *>(&merged_forward[0]), 0, sizeof(merged_forward));

		// 		Bitmap<CHUNK_SIZE> bits_walking_forward;

		// 		for (GLuint forward = 0; forward < CHUNK_SIZE; forward++) {

		// 			// mask with bits on [y][z]
		// 			Bitmap<CHUNK_SIZE> bits_here = faceMasks[face][up][forward];
		// 			// mask with bits on [y][z + 1]. if z == 31, stays the same as z
		// 			Bitmap<CHUNK_SIZE> bits_forward = forward == CHUNK_SIZE ? bits_here : faceMasks[face][up][forward + 1];
		// 			// mask with bits on [y + 1][z]. if y == 31, stays the same as y
		// 			Bitmap<CHUNK_SIZE> bits_up = up == CHUNK_SIZE ? bits_here : faceMasks[face][up + 1][forward];

		// 			Bitmap<CHUNK_SIZE> bits_merging_up = bits_here & bits_up & ~bits_walking_forward;
		// 			Bitmap<CHUNK_SIZE> bits_merging_forward = bits_here & bits_forward;

		// 			Bitmap<CHUNK_SIZE> copy_front = bits_merging_up;
		// 			GLuint bit_pos;

		// 			// check that all materials are the same
		// 			while(copy_front.value()) {
		// 				bit_pos = copy_front.findNextEmpty();

		// 				copy_front &= ~(1U << bit_pos);

		// 				if (
		// 					get_material(axis, forward, up, bit_pos) == get_material(axis, forward, up + 1, bit_pos))
		// 				{
		// 					merged_up[forward][bit_pos]++;
		// 				}
		// 				else
		// 				{
		// 					bits_merging_up &= ~(1U << bit_pos);
		// 				}
		// 			}

		// 			Bitmap<CHUNK_SIZE> bits_stopped_up = bits_here & ~bits_merging_up;

		// 			while (bits_stopped_up.value()) {
		// 				bit_pos = bits_stopped_up.findNextEmpty();

		// 				bits_stopped_up &= ~(1U << bit_pos);

		// 				GLbyte material = get_material(axis, forward, up, bit_pos);

		// 				if (
		// 					(bits_merging_forward & (1U << bit_pos)) != Bitmap<CHUNK_SIZE>(0) &&
		// 					(merged_up[forward][bit_pos] == merged_up[forward + 1][bit_pos]) &&
		// 					(material == get_material(axis, forward + 1, up, bit_pos)))
		// 				{
		// 					bits_walking_forward |= 1U << bit_pos;
		// 					merged_forward[bit_pos]++;
		// 					merged_up[forward][bit_pos] = Bitmap<CHUNK_SIZE>(0);
		// 					continue;
		// 				}

		// 				bits_walking_forward &= ~(1U << bit_pos);

		// 				uint8_t mesh_left = forward - merged_forward[bit_pos].value();
		// 				uint8_t mesh_forward = forward + 1;
		// 				uint8_t mesh_front = up - merged_up[forward][bit_pos].value();
		// 				uint8_t mesh_back = up + 1;
		// 				uint8_t mesh_up = bit_pos + (face % 2 == 0 ? 1 : 0);

		// 				merged_up[forward][bit_pos] = 0;
		// 				merged_forward[bit_pos] = 0;

		// 				switch(face) {
		// 					case 0:
		// 						// printf("mesh_left: %u\n", mesh_left);
		// 						// printf("mesh_forward: %u\n", mesh_forward);
		// 						// printf("mesh_up: %u\n", mesh_up);
		// 						// printf("mesh_back: %u\n", mesh_back);
		// 					// const glm::u8vec3 &pos, GLubyte _material_id, GLubyte len_x, GLubyte len_y
		// 						quads[0].emplace_back(glm::u8vec3(mesh_left, mesh_up, mesh_back),
		// 													material,
		// 													static_cast<GLfloat>(mesh_forward - mesh_left), static_cast<GLfloat>(mesh_front - mesh_back));
		// 						break;
		// 					case 1:
		// 						quads[1].emplace_back(glm::u8vec3(mesh_left, mesh_up, mesh_back),
		// 													material,
		// 													static_cast<GLfloat>(mesh_forward - mesh_left), static_cast<GLfloat>(mesh_front - mesh_back));
		// 						break;
		// 					case 2:
		// 						break;
		// 					case 3:
		// 						break;
		// 					case 4:
		// 						break;
		// 					case 5:
		// 						break;
		// 				}
		// 			}
		// 		}
		// 	}
		// }

		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    	std::cout << "Function took " << duration.count() << " microseconds to execute." << std::endl;
	}
};

static_assert(sizeof(Chunk::opaqueMask) == sizeof(uint32_t) * CHUNK_SIZE * CHUNK_SIZE, "ERROR: opaqueMask has unexpected size");

#endif
