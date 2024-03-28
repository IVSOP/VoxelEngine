#ifndef ENGINE_H
#define ENGINE_H

#include "Vertex.hpp"
#include "common.hpp"

#if _WIN32
#include <windows_unistd.h>
#else
#include <unistd.h>
#endif

#include "Renderer.hpp"
#include "Camera.hpp"
#include "InputHandler.hpp"
#include <thread>
#include <mutex>

#define WORLD_SIZE_X 32
#define WORLD_SIZE_Y 32
#define WORLD_SIZE_Z 32

// made into a struct to be easier to store on the heap
struct World {
	Chunk chunks[WORLD_SIZE_X][WORLD_SIZE_Y][WORLD_SIZE_Z]; // this order can be changed, need to test it for performance
	ChunkInfo info[WORLD_SIZE_X][WORLD_SIZE_Y][WORLD_SIZE_Z];

	Chunk &get(const glm::uvec3 &position) {
		return chunks[position.x][position.y][position.z];
	}

	std::vector<ChunkInfo> getInfo() {
		// extremely cursed but this supposedly preserves the underlying pointer and does not allocate memory for it
		// can change it in the future it this is not true
		return std::vector<ChunkInfo>(&info[0][0][0], &info[WORLD_SIZE_X - 1][WORLD_SIZE_Y - 1][WORLD_SIZE_Z - 1]);
	}

	// maybe cache this here instead of in each chunk?
	std::vector<Quad> getQuads() {
		std::vector<Quad> vec;
		for (GLuint x = 0; x < WORLD_SIZE_X; x++) {
			for (GLuint y = 0; y < WORLD_SIZE_Y; y++) {
				for (GLuint z = 0; z < WORLD_SIZE_Z; z++) {
					chunks[x][y][z].addQuadsTo(vec);
				}
			}
		}

		return vec;
	}

	World() {
		// build the info. I want to make it so that [half][half][half] is roughly around (0,0,0)
		// for now this does not happen
		for (GLuint x = 0; x < WORLD_SIZE_X; x++) {
			for (GLuint y = 0; y < WORLD_SIZE_Y; y++) {
				for (GLuint z = 0; z < WORLD_SIZE_Z; z++) {
					info[x][y][z] = ChunkInfo(32.0f * glm::vec3(static_cast<GLfloat>(x), static_cast<GLfloat>(y), static_cast<GLfloat>(z)));
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

class Engine {
public:

	Engine();
	~Engine();

	void loop();
	void physLoop();
	void renderLoop();

	GLdouble windowFov;
	GLdouble windowZNear;
	GLdouble windowZFar;

	int windowWidth;
	int windowHeight;

	// std::mutex mtx;
	GLFWwindow *window = nullptr;
	std::unique_ptr<Camera> camera = nullptr;
	std::unique_ptr<Renderer> renderer = nullptr;
	std::unique_ptr<InputHandler> inputHandler = nullptr;
	std::unique_ptr<World> world = std::make_unique<World>();

	glm::mat4 projection = glm::mat4(1.0f);

	// isto e extremamente roto, mas nas docs basicamente diz que sou burro se tentar fazer como no glut e meter GLFW_CURSOR_HIDDEN e estar sempre a centra-lo, diz para usar GLFW_CURSOR_DISABLED
	// mas ao usar isso, sempre que dou resize, ele manda um mouse callback que lixa tudo, tive de fazer isto para esse callback ser ignorado
	// ou seja acho que vai ter de ficar assim
	bool resize = false;

	bool kill = false;

};

#endif
