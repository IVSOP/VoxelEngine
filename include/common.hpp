#ifndef COMMON_H
#define COMMON_H

#include <filesystem>
#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "GLErrors.hpp"

#define PHYS_FPS 60.0f
#define PHYS_STEP 1.0f / PHYS_FPS

const GLchar *readFromFile(const char *filepath);

// wrapper around a simple pointer. did not want to include the size into the template for now
template<typename T> struct custom_array {
	const T* _data;
	const GLsizei _size;

	constexpr const GLsizei size() const { return _size; }
	constexpr const T* data() const { return _data; }

	custom_array(T* data, GLsizei size)
	: _data(data), _size(size) {}
	~custom_array() = default;
};

#endif
