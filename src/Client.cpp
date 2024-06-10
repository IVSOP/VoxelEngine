#include "Client.hpp"

Client::Client()
: windowManager(std::make_unique<WindowManager>(1920, 1080, this)),
  player(std::make_unique<Player>(glm::vec3(64, 64, 264), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0))),
  world(std::make_unique<World>()),
  renderer(std::make_unique<Renderer>(1920, 1080)), // get these from window manager???
  inputHandler(glfw_handleMouseMov_callback, glfw_handleMouseKey_callback) // funcs from window manager
{

	resizeViewport(1920, 1080); // these too


	Chunk chunk;
	Voxel voxel = Voxel(0);
	for (GLuint y = 0; y < CHUNK_SIZE; y++) {
		for (GLuint z = 0; z < CHUNK_SIZE; z++) {
			for (GLuint x = 0; x < CHUNK_SIZE; x++) {
				voxel.material_id = 0;
				if (x == 5) voxel.material_id = 1;
				chunk.insertVoxelAt(glm::uvec3(x, y, z), voxel);
			}
		}
	}

	voxel.material_id = 1;
	for (GLuint z = 0; z < CHUNK_SIZE; z++) {
		for (GLuint x = 0; x < CHUNK_SIZE; x++) {
			chunk.insertVoxelAt(glm::uvec3(x, 15, z), voxel);
		}
	}

	for (GLuint x = 0; x < WORLD_SIZE_X; x++) {
		for (GLuint y = 0; y < WORLD_SIZE_Y; y++) {
			for (GLuint z = 0; z < WORLD_SIZE_Z; z++) {
				// world.get()->copyChunkTo(chunk, glm::uvec3(x, 0, z));
				// world.get()->copyChunkTo(chunk, glm::uvec3(x, 15, z));
				world.get()->copyChunkTo(chunk, glm::uvec3(x, y, z));
			}
		}
	}
}

void Client::resizeViewport(int windowWidth, int windowHeight) {
	if (windowWidth == 0 || windowHeight == 0) {
        fprintf(stderr, "Detected window size 0, ignoring resize operation\n");
    } else {
		windowManager.get()->resizeViewport(windowWidth, windowHeight);
		renderer.get()->resizeViewport(windowWidth, windowHeight);

		this->resize = true;
	}
}

void Client::pressKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
    inputHandler.pressKey(window, key, scancode, action, mods);
}

void Client::moveMouseTo(double xpos, double ypos) {
	inputHandler.moveMouseTo(xpos, ypos);
}

void Client::centerMouseTo(double xpos, double ypos) {
	inputHandler.centerMouseTo(xpos, ypos);
}

void Client::pressMouseKey(GLFWwindow* window, int button, int action, int mods) {
	inputHandler.pressMouseKey(window, button, action, mods);
}

void Client::mainloop() {
    double lastFrameTime, currentFrameTime, deltaTime = PHYS_STEP; // to prevent errors when this is first ran, I initialize it to the physics substep
    while (!glfwWindowShouldClose(windowManager.get()->window)) {
        glfwPollEvents(); // at the start due to imgui (??) test moving it to after the unlock()

		lastFrameTime = glfwGetTime();

        // printf("delta is %f (%f fps)\n", deltaTime, 1.0f / deltaTime);
		Camera *camera = player.get()->getCamera();
		SelectedBlockInfo selectedBlock = world.get()->getSelectedBlock(camera->Position, camera->Front, renderer->break_range);
        inputHandler.applyInputs(
			world.get(),
			selectedBlock,
			renderer.get()->break_radius,
			camera, windowManager.get()->windowWidth,windowManager.get()->windowHeight, static_cast<GLfloat>(deltaTime));

        // std::unique_lock<std::mutex> lock = std::unique_lock<std::mutex>(mtx);
        // renderer.get()->draw(draw_quads, projection, *camera.get(), window, deltaTime);
    	world.get()->buildData(camera->Position);
		renderer.get()->draw(
			world.get()->getQuads(),
			world.get()->getIndirect(),
			world.get()->getInfo(),
			selectedBlock,
			windowManager.get()->projection,
			*camera, // ??????????????????????????????????? why
			windowManager.get()->window, deltaTime);
        // lock.unlock();

        currentFrameTime = glfwGetTime();
        deltaTime = currentFrameTime - lastFrameTime;

        // no need for sleep, vsync takes care of mantaining timings
		// HOWEVER macbooks as per usual do not work properly
		// so I made this bandaid fix
		if (renderer.get()->limitFPS) {
			const double fps_time = 1.0f / renderer.get()->fps;
			if (deltaTime < fps_time) {
				const double sleepTime = (fps_time - deltaTime) * 10E5; // multiply to get from seconds to microseconds, this is prob platform dependent and very bad
				usleep(sleepTime);
				deltaTime = fps_time;
			}
		}
    }
}

void Client::saveWorldTo(const std::string &filepath) const {
	std::ofstream file(filepath, std::ios::binary);

	player.get()->saveTo(file);
	world.get()->saveTo(file);

	file.flush();
	file.close();
}

void Client::loadWorldFrom(const std::string &filepath) {
	std::ifstream file(filepath, std::ios::binary);

	// got lazy, maybe it is faster to iterate and change pre-existing world???
	player = std::make_unique<Player>(file);
	world = std::make_unique<World>(file);
	// inputHandler = InputHandler(glfw_handleMouseMov_callback, glfw_handleMouseKey_callback);
}
