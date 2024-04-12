#ifndef CHUNK_H
#define CHUNK_H

#define CHUNK_SIZE 32
#define CHUNK_SIZE_FLOAT static_cast<GLfloat>(CHUNK_SIZE)

#include "common.hpp"
#include "Quad.hpp"
#include <vector>
#include "QuadContainer.hpp"
#include "Bitmap.h"

// normal {
// 	0 - y (bottom)
// 	1 + y (top)
// 	2 - z (far)
// 	3 + z (near)
// 	4 - x (left)
// 	5 + x (right)
// }

struct Voxel {
	GLbyte material_id;

	constexpr Voxel() : material_id(-1) {}
	constexpr Voxel(GLbyte material_id) : material_id(material_id) {}

	// this way I save memory, plus ints are aligned nicely
	constexpr bool isEmpty() const {
		if (material_id < 0) return true;
		return false;
	}
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
	bool quadsHaveChanged = false;
	// [i] corresponds to normal == i
	std::vector<Quad> quads[6]; // I suspect that most chunks will have empty space so I use a vector. idk how bad this is, memory will be extremely sparse. maybe using a fixed size array here will be better, need to test
	Bitmap<CHUNK_SIZE> visited[CHUNK_SIZE]; //  CAN BE USED AS [y][x] // visited[y] has all info on that row (values vary along x). basically a bitmap for every single row (for easier math)

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

	constexpr void breakVoxelAt(GLubyte x, GLubyte y, GLubyte z) {
		voxels[y][z][x] = Voxel(-1);
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
		return ! voxels[y][z][x].isEmpty();
	}

	void rebuildQuads() {
		quadsHaveChanged = false;
		for (GLuint normal = 0; normal < 6; normal ++) {
			quads[normal].clear(); // pray that this does not change the capacity

			switch(normal) {
				case 0: // bottom
					// slices used are [z][x]
					{
						GLuint x = 0, x_copy = 0, z, z_copy, max_x;
						GLbyte material;
						bool end;
						for (GLuint y = 0; y < CHUNK_SIZE; y++) {
							for (GLuint i = 0; i < CHUNK_SIZE; i++) {
								visited[i].clear();
							}
							// iterate until lines are all processed
							for (z = 0; z < CHUNK_SIZE; z++) {
								// in every line every x has to be processed
								for (x = visited[z].findNext(); x < CHUNK_SIZE; x = visited[z].findNext()) {
									visited[z].setTrue(x);				// check for occlusion
									if (voxels[y][z][x].isEmpty() || (y > 0 && voxelAt(x, y - 1, z))) {
										// voxel is empty, not eligible for starter of greedy mesh, skip it
										continue;
									}

									material = voxels[y][z][x].material_id;

									// save the starting values
									x_copy = x;
									z_copy = z;


									///////////////////////////////// do the actual greedy meshing

									// on the first line, try do expand horizontally as much as possible
									for (x = x + 1; x < CHUNK_SIZE; x++) {
										if (visited[z][x] == true) break; // seems off, use findNext instead of x++????
										visited[z].setTrue(x);

										const Voxel &voxel = voxels[y][z][x];
										if (voxel.isEmpty() || (y > 0 && voxelAt(x, y - 1, z))) { // we can mark it as visited, it will be useless
											break;
										}

										if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
											// pretend it was never visited
											visited[z].setFalse(x);
											break;
										}
									}

									// since last loop breaks at the invalid x, correct max y is x - 1
									max_x = x - 1;

									// after getting that result, iterate to find max line where we can expand
									end = false; // this flag is so the X loop can tell the Y loop it is finished (in case of error)
									for (z = z + 1; !end && z < CHUNK_SIZE; z++) {
										// every single voxel added needs to have a valid voxel above them, otherwise break
										for (x = x_copy; x <= max_x; x++) {
											if (visited[z][x]) break;


											const Voxel &voxel = voxels[y][z][x];
											if (voxel.isEmpty() || (y > 0 && voxelAt(x, y - 1, z))) { // we can mark it as visited, it will be useless
												visited[z].setTrue(x);
												end = true;
												break;
											}
											// could be else and remove body from above
											if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
												end = true;
												break;
											}
										}
										// this is done before so the line is not filled in in the bitmap
										if (end) {
											break;
										}
										// reached the end of the line and everything is fine, need to mark everything we went through as visited
										for (GLuint temp_x = x_copy; temp_x < x; temp_x++) {
											visited[z].setTrue(temp_x);
										}
									}

									// end results
									// since last loop breaks at the invalid y, correct max y is y - 1
									x = max_x;
									z = z - 1;

									// printf("creating quad from %u %u %u to %u %u\n", x_copy, y_copy, z, x, y);

									quads[normal].emplace_back(glm::u8vec3(x_copy, y, z_copy),
															   material,
															   static_cast<GLfloat>(x - x_copy), static_cast<GLfloat>(z - z_copy));

									// printf("quad position: %u %u %u len: %u %u\n", quads[normal].back().getPosition().x, quads[normal].back().getPosition().y, quads[normal].back().getPosition().z, quads[normal].back().getLen().x, quads[normal].back().getLen().y);
									// x = x_copy;
									z = z_copy;
								}
							}
						}
					}
					break;
				case 1: // top
					// slices used are [z][x]
					{
						GLuint x = 0, x_copy = 0, z, z_copy, max_x;
						GLbyte material;
						bool end;
						for (GLuint y = 0; y < CHUNK_SIZE; y++) {
							for (GLuint i = 0; i < CHUNK_SIZE; i++) {
								visited[i].clear();
							}
							// iterate until lines are all processed
							for (z = 0; z < CHUNK_SIZE; z++) {
								// in every line every x has to be processed
								for (x = visited[z].findNext(); x < CHUNK_SIZE; x = visited[z].findNext()) {
									visited[z].setTrue(x);				// check for occlusion
									if (voxels[y][z][x].isEmpty() || (y < CHUNK_SIZE - 1 && voxelAt(x, y + 1, z))) {
										// voxel is empty, not eligible for starter of greedy mesh, skip it
										continue;
									}

									material = voxels[y][z][x].material_id;

									// save the starting values
									x_copy = x;
									z_copy = z;


									///////////////////////////////// do the actual greedy meshing

									// on the first line, try do expand horizontally as much as possible
									for (x = x + 1; x < CHUNK_SIZE; x++) {
										if (visited[z][x] == true) break; // seems off, use findNext instead of x++????
										visited[z].setTrue(x);

										const Voxel &voxel = voxels[y][z][x];
										if (voxel.isEmpty() || (y < CHUNK_SIZE - 1 && voxelAt(x, y + 1, z))) { // we can mark it as visited, it will be useless
											break;
										}

										if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
											// pretend it was never visited
											visited[z].setFalse(x);
											break;
										}
									}

									// since last loop breaks at the invalid x, correct max y is x - 1
									max_x = x - 1;

									// after getting that result, iterate to find max line where we can expand
									end = false; // this flag is so the X loop can tell the Y loop it is finished (in case of error)
									for (z = z + 1; !end && z < CHUNK_SIZE; z++) {
										// every single voxel added needs to have a valid voxel above them, otherwise break
										for (x = x_copy; x <= max_x; x++) {
											if (visited[z][x]) break;


											const Voxel &voxel = voxels[y][z][x];
											if (voxel.isEmpty() || (y < CHUNK_SIZE - 1 && voxelAt(x, y + 1, z))) { // we can mark it as visited, it will be useless
												visited[z].setTrue(x);
												end = true;
												break;
											}
											// could be else and remove body from above
											if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
												end = true;
												break;
											}
										}
										// this is done before so the line is not filled in in the bitmap
										if (end) {
											break;
										}
										// reached the end of the line and everything is fine, need to mark everything we went through as visited
										for (GLuint temp_x = x_copy; temp_x < x; temp_x++) {
											visited[z].setTrue(temp_x);
										}
									}

									// end results
									// since last loop breaks at the invalid y, correct max y is y - 1
									x = max_x;
									z = z - 1;

									// printf("creating quad from %u %u %u to %u %u\n", x_copy, y_copy, z, x, y);

									quads[normal].emplace_back(glm::u8vec3(x_copy, y, z_copy),
															   material,
															   static_cast<GLfloat>(x - x_copy), static_cast<GLfloat>(z - z_copy));

									// printf("quad position: %u %u %u len: %u %u\n", quads[normal].back().getPosition().x, quads[normal].back().getPosition().y, quads[normal].back().getPosition().z, quads[normal].back().getLen().x, quads[normal].back().getLen().y);
									// x = x_copy;
									z = z_copy;
								}
							}
						}
					}
					break;
				case 2: // far
					{
						GLuint x = 0, x_copy = 0, y, y_copy, max_x;
						GLbyte material;
						bool end;
						for (GLuint z = 0; z < CHUNK_SIZE; z++) {
							for (GLuint i = 0; i < CHUNK_SIZE; i++) {
								visited[i].clear();
							}
							// iterate until lines are all processed
							for (y = 0; y < CHUNK_SIZE; y++) {
								// in every line every x has to be processed
								for (x = visited[y].findNext(); x < CHUNK_SIZE; x = visited[y].findNext()) {
									visited[y].setTrue(x);
									if (voxels[y][z][x].isEmpty() || (z > 0 && voxelAt(x, y, z - 1))) {
										// voxel is empty, not eligible for starter of greedy mesh, skip it
										continue;
									}

									material = voxels[y][z][x].material_id;

									// save the starting values
									x_copy = x;
									y_copy = y;


									///////////////////////////////// do the actual greedy meshing

									// on the first line, try do expand horizontally as much as possible
									for (x = x + 1; x < CHUNK_SIZE; x++) {
										if (visited[y][x] == true) break; // seems off, use findNext instead of x++????
										visited[y].setTrue(x);

										const Voxel &voxel = voxels[y][z][x];
										if (voxel.isEmpty() || (z > 0 && voxelAt(x, y, z - 1))) { // we can mark it as visited, it will be useless
											break;
										}

										if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
											// pretend it was never visited
											visited[y].setFalse(x);
											break;
										}
									}

									// since last loop breaks at the invalid x, correct max y is x - 1
									max_x = x - 1;

									// after getting that result, iterate to find max line where we can expand
									end = false; // this flag is so the X loop can tell the Y loop it is finished (in case of error)
									for (y = y + 1; !end && y < CHUNK_SIZE; y++) {
										// every single voxel added needs to have a valid voxel above them, otherwise break
										for (x = x_copy; x <= max_x; x++) {
											if (visited[y][x]) break;


											const Voxel &voxel = voxels[y][z][x];
											if (voxel.isEmpty() || (z > 0 && voxelAt(x, y, z - 1))) { // we can mark it as visited, it will be useless
												visited[y].setTrue(x);
												end = true;
												break;
											}
											// could be else and remove body from above
											if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
												end = true;
												break;
											}
										}
										// this is done before so the line is not filled in in the bitmap
										if (end) {
											break;
										}
										// reached the end of the line and everything is fine, need to mark everything we went through as visited
										for (GLuint temp_x = x_copy; temp_x < x; temp_x++) {
											visited[y].setTrue(temp_x);
										}
									}

									// end results
									// since last loop breaks at the invalid y, correct max y is y - 1
									x = max_x;
									y = y - 1;

									// printf("creating quad from %u %u %u to %u %u\n", x_copy, y_copy, z, x, y);

									quads[normal].emplace_back(glm::u8vec3(x_copy, y_copy, z),
															   material,
															   static_cast<GLfloat>(x - x_copy), static_cast<GLfloat>(y - y_copy));

									// printf("quad position: %u %u %u len: %u %u\n", quads[normal].back().getPosition().x, quads[normal].back().getPosition().y, quads[normal].back().getPosition().z, quads[normal].back().getLen().x, quads[normal].back().getLen().y);
									y = y_copy;
								}
							}
						}
					}

					break;
				case 3: // near
					// slices used are [y][x]
					{
						GLuint x = 0, x_copy = 0, y, y_copy, max_x;
						GLbyte material;
						bool end;
						for (GLuint z = 0; z < CHUNK_SIZE; z++) {
							for (GLuint i = 0; i < CHUNK_SIZE; i++) {
								visited[i].clear();
							}
							// iterate until lines are all processed
							for (y = 0; y < CHUNK_SIZE; y++) {
								// in every line every x has to be processed
								for (x = visited[y].findNext(); x < CHUNK_SIZE; x = visited[y].findNext()) {
									visited[y].setTrue(x);				// check for occlusion
									if (voxels[y][z][x].isEmpty() || (z < CHUNK_SIZE - 1 && voxelAt(x, y, z + 1))) {
										// voxel is empty, not eligible for starter of greedy mesh, skip it
										continue;
									}

									material = voxels[y][z][x].material_id;

									// save the starting values
									x_copy = x;
									y_copy = y;


									///////////////////////////////// do the actual greedy meshing

									// on the first line, try do expand horizontally as much as possible
									for (x = x + 1; x < CHUNK_SIZE; x++) {
										if (visited[y][x] == true) break; // seems off, use findNext instead of x++????
										visited[y].setTrue(x);

										const Voxel &voxel = voxels[y][z][x];
										if (voxel.isEmpty() || (z < CHUNK_SIZE - 1 && voxelAt(x, y, z + 1))) { // we can mark it as visited, it will be useless
											break;
										}

										if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
											// pretend it was never visited
											visited[y].setFalse(x);
											break;
										}
									}

									// since last loop breaks at the invalid x, correct max y is x - 1
									max_x = x - 1;

									// after getting that result, iterate to find max line where we can expand
									end = false; // this flag is so the X loop can tell the Y loop it is finished (in case of error)
									for (y = y + 1; !end && y < CHUNK_SIZE; y++) {
										// every single voxel added needs to have a valid voxel above them, otherwise break
										for (x = x_copy; x <= max_x; x++) {
											if (visited[y][x]) break;


											const Voxel &voxel = voxels[y][z][x];
											if (voxel.isEmpty() || (z < CHUNK_SIZE - 1 && voxelAt(x, y, z + 1))) { // we can mark it as visited, it will be useless
												visited[y].setTrue(x);
												end = true;
												break;
											}
											// could be else and remove body from above
											if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
												end = true;
												break;
											}
										}
										// this is done before so the line is not filled in in the bitmap
										if (end) {
											break;
										}
										// reached the end of the line and everything is fine, need to mark everything we went through as visited
										for (GLuint temp_x = x_copy; temp_x < x; temp_x++) {
											visited[y].setTrue(temp_x);
										}
									}

									// end results
									// since last loop breaks at the invalid y, correct max y is y - 1
									x = max_x;
									y = y - 1;

									// printf("creating quad from %u %u %u to %u %u\n", x_copy, y_copy, z, x, y);

									quads[normal].emplace_back(glm::u8vec3(x_copy, y_copy, z),
															   material,
															   static_cast<GLfloat>(x - x_copy), static_cast<GLfloat>(y - y_copy));

									// printf("quad position: %u %u %u len: %u %u\n", quads[normal].back().getPosition().x, quads[normal].back().getPosition().y, quads[normal].back().getPosition().z, quads[normal].back().getLen().x, quads[normal].back().getLen().y);
									// x = x_copy;
									y = y_copy;
								}
							}
						}
					}

					break;
				case 4: // left
					// slices used are [y][z]
					{
						GLuint z = 0, z_copy = 0, y, y_copy, max_z;
						GLbyte material;
						bool end;
						for (GLuint x = 0; x < CHUNK_SIZE; x++) {
							for (GLuint i = 0; i < CHUNK_SIZE; i++) {
								visited[i].clear();
							}
							// iterate until lines are all processed
							for (y = 0; y < CHUNK_SIZE; y++) {
								// in every line every x has to be processed
								for (z = visited[y].findNext(); z < CHUNK_SIZE; z = visited[y].findNext()) {
									visited[y].setTrue(z);				// check for occlusion
									if (voxels[y][z][x].isEmpty() || (x > 0 && voxelAt(x - 1, y, z))) {
										// voxel is empty, not eligible for starter of greedy mesh, skip it
										continue;
									}

									material = voxels[y][z][x].material_id;

									// save the starting values
									z_copy = z;
									y_copy = y;


									///////////////////////////////// do the actual greedy meshing

									// on the first line, try do expand horizontally as much as possible
									for (z = z + 1; z < CHUNK_SIZE; z++) {
										if (visited[y][z] == true) break; // seems off, use findNext instead of x++????
										visited[y].setTrue(z);

										const Voxel &voxel = voxels[y][z][x];
										if (voxel.isEmpty() || (x > 0 && voxelAt(x - 1, y, z))) { // we can mark it as visited, it will be useless
											break;
										}

										if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
											// pretend it was never visited
											visited[y].setFalse(z);
											break;
										}
									}

									// since last loop breaks at the invalid x, correct max y is x - 1
									max_z = z - 1;

									// after getting that result, iterate to find max line where we can expand
									end = false; // this flag is so the X loop can tell the Y loop it is finished (in case of error)
									for (y = y + 1; !end && y < CHUNK_SIZE; y++) {
										// every single voxel added needs to have a valid voxel above them, otherwise break
										for (z = z_copy; z <= max_z; z++) {
											if (visited[y][z]) break;


											const Voxel &voxel = voxels[y][z][x];
											if (voxel.isEmpty() || (x > 0 && voxelAt(x - 1, y, z))) { // we can mark it as visited, it will be useless
												visited[y].setTrue(z);
												end = true;
												break;
											}
											// could be else and remove body from above
											if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
												end = true;
												break;
											}
										}
										// this is done before so the line is not filled in in the bitmap
										if (end) {
											break;
										}
										// reached the end of the line and everything is fine, need to mark everything we went through as visited
										for (GLuint temp_z = z_copy; temp_z < z; temp_z++) {
											visited[y].setTrue(temp_z);
										}
									}

									// end results
									// since last loop breaks at the invalid y, correct max y is y - 1
									z = max_z;
									y = y - 1;

									// printf("creating quad from %u %u %u to %u %u\n", x_copy, y_copy, z, x, y);

									quads[normal].emplace_back(glm::u8vec3(x, y_copy, z_copy),
															   material,
															   static_cast<GLfloat>(z - z_copy), static_cast<GLfloat>(y - y_copy));

									// printf("quad position: %u %u %u len: %u %u\n", quads[normal].back().getPosition().x, quads[normal].back().getPosition().y, quads[normal].back().getPosition().z, quads[normal].back().getLen().x, quads[normal].back().getLen().y);
									// x = x_copy;
									y = y_copy;
								}
							}
						}
					}
					break;
				case 5: // right
					// slices used are [y][z]
					{
						GLuint z = 0, z_copy = 0, y, y_copy, max_z;
						GLbyte material;
						bool end;
						for (GLuint x = 0; x < CHUNK_SIZE; x++) {
							for (GLuint i = 0; i < CHUNK_SIZE; i++) {
								visited[i].clear();
							}
							// iterate until lines are all processed
							for (y = 0; y < CHUNK_SIZE; y++) {
								// in every line every x has to be processed
								for (z = visited[y].findNext(); z < CHUNK_SIZE; z = visited[y].findNext()) {
									visited[y].setTrue(z);				// check for occlusion
									if (voxels[y][z][x].isEmpty() || (x < CHUNK_SIZE - 1 && voxelAt(x + 1, y, z))) {
										// voxel is empty, not eligible for starter of greedy mesh, skip it
										continue;
									}

									material = voxels[y][z][x].material_id;

									// save the starting values
									z_copy = z;
									y_copy = y;


									///////////////////////////////// do the actual greedy meshing

									// on the first line, try do expand horizontally as much as possible
									for (z = z + 1; z < CHUNK_SIZE; z++) {
										if (visited[y][z] == true) break; // seems off, use findNext instead of x++????
										visited[y].setTrue(z);

										const Voxel &voxel = voxels[y][z][x];
										if (voxel.isEmpty() || (x < CHUNK_SIZE - 1 && voxelAt(x + 1, y, z))) { // we can mark it as visited, it will be useless
											break;
										}

										if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
											// pretend it was never visited
											visited[y].setFalse(z);
											break;
										}
									}

									// since last loop breaks at the invalid x, correct max y is x - 1
									max_z = z - 1;

									// after getting that result, iterate to find max line where we can expand
									end = false; // this flag is so the X loop can tell the Y loop it is finished (in case of error)
									for (y = y + 1; !end && y < CHUNK_SIZE; y++) {
										// every single voxel added needs to have a valid voxel above them, otherwise break
										for (z = z_copy; z <= max_z; z++) {
											if (visited[y][z]) break;


											const Voxel &voxel = voxels[y][z][x];
											if (voxel.isEmpty() || (x < CHUNK_SIZE - 1 && voxelAt(x + 1, y, z))) { // we can mark it as visited, it will be useless
												visited[y].setTrue(z);
												end = true;
												break;
											}
											// could be else and remove body from above
											if (voxel.material_id != material) { // we cannot mark it as visited now, it might be useful in the future
												end = true;
												break;
											}
										}
										// this is done before so the line is not filled in in the bitmap
										if (end) {
											break;
										}
										// reached the end of the line and everything is fine, need to mark everything we went through as visited
										for (GLuint temp_z = z_copy; temp_z < z; temp_z++) {
											visited[y].setTrue(temp_z);
										}
									}

									// end results
									// since last loop breaks at the invalid y, correct max y is y - 1
									z = max_z;
									y = y - 1;

									// printf("creating quad from %u %u %u to %u %u\n", x_copy, y_copy, z, x, y);

									quads[normal].emplace_back(glm::u8vec3(x, y_copy, z_copy),
															   material,
															   static_cast<GLfloat>(z - z_copy), static_cast<GLfloat>(y - y_copy));

									// printf("quad position: %u %u %u len: %u %u\n", quads[normal].back().getPosition().x, quads[normal].back().getPosition().y, quads[normal].back().getPosition().z, quads[normal].back().getLen().x, quads[normal].back().getLen().y);
									// x = x_copy;
									y = y_copy;
								}
							}
						}
					}
					break;
			}
		}
	}
};

#endif
