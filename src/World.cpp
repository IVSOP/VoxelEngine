#include "World.hpp"

#include <zlib.h>

// also this can probably be optimized but for now I will leave it to compiler magic
// yes very ugly will clean this up later
// make player position align to the chunk somehow???????/ doing == on floats is kind of bad
void World::buildData(const glm::vec3 &playerPosition) {
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

SelectedBlockInfo World::getBlockInfo(const glm::ivec3 &position) {
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
	ret._isEmpty = chunk->isEmptyAt(pos);

	return ret;
}

SelectedBlockInfo World::getSelectedBlock(const glm::vec3 &position, const glm::vec3 &lookPosition, GLfloat radius) {
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
		return SelectedBlockInfo(-1, 0, 0, true, {});
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
	return SelectedBlockInfo(-1, 0, 0, true, {});
}

void World::saveTo(std::ofstream &file) {
	uLong total = 0; // acumulator for determining compression ratio
	uLong bound = compressBound(WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
	uLong len;
	// in order to not be ridiculous, will compress data in chunks
	uint16_t size;
	Bytef *buff = new Bytef[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE]; // kinda big, will be better on the heap
	for (GLuint x = 0; x < WORLD_SIZE_X; x++) {
		for (GLuint y = 0; y < WORLD_SIZE_Y; y++) {
			for (GLuint z = 0; z < WORLD_SIZE_Z; z++) {
				len = bound;

											// a voxel is already a byte, no need to do anything
				if (compress(buff, &len, reinterpret_cast<Bytef *>(&chunks[x][y][z].voxels[0][0][0]), CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE) != Z_OK) {
					print_error("error compressing");
					exit(1);
				}
				size = len;

				// this is probably really very bad right? idk the best way to write a single int. ill assume the buffering will amortize the slowness of this
				file.write(reinterpret_cast<char *>(&size), sizeof(size));
				file.write(reinterpret_cast<char *>(buff), sizeof(Bytef) * len);

				total += len;
			}
		}
	}

	printf("World compression ratio: %lf (from %ld to %ld)\n", 
		static_cast<double>(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z) / static_cast<double>(total),
		CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z, total);

	delete[] buff;
}

World::World(std::ifstream &file)
: info(1 << 10), indirect(1 << 10), quads(1 << 10)
{
	uint16_t size;
	uLong len;
	Bytef *buff = new Bytef[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE]; // kinda big, will be better on the heap
	for (GLuint x = 0; x < WORLD_SIZE_X; x++) {
		for (GLuint y = 0; y < WORLD_SIZE_Y; y++) {
			for (GLuint z = 0; z < WORLD_SIZE_Z; z++) {

				file.read(reinterpret_cast<char *>(&size), sizeof(size));
				file.read(reinterpret_cast<char *>(buff), size);

				len = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

											// a voxel is already a byte, no need to do anything
				if (uncompress(reinterpret_cast<Bytef *>(&chunks[x][y][z].voxels[0][0][0]), &len, buff, size) != Z_OK) {
					print_error("error decompressing");
					exit(1);
				}
			}
		}
	}

	delete[] buff;
}
