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

// idk where else to put this
struct SelectedBlockInfo {
	GLint materialID;
	GLuint chunkID;
	GLubyte normal;
	glm::u8vec3 position; // relative to chunk

	SelectedBlockInfo(GLint materialID, GLuint chunkID, GLubyte normal, const glm::u8vec3 &position)
	: materialID(materialID), chunkID(chunkID), normal(normal), position(position) {}

	SelectedBlockInfo() = default;

	constexpr bool isEmpty() const { return materialID < 0; }
};

// same
struct IndirectData {
	GLuint count;
	GLuint instanceCount;
	GLuint first;
	GLuint baseInstance; // does nothing in opengl < 4.2. do I need this??????????????????????

// 												           							                      6                              
// same as glDrawArraysInstancedBaseInstance(mode,     cmd->first,             cmd->count,         cmd->instanceCount,             cmd->baseInstance); many times


	IndirectData()
	: count(6), baseInstance(0) {}

	~IndirectData() = default;
};

// wrapper around a simple pointer. did not want to include the size into the template for now
template<typename T> struct custom_array {
	const T* _data;
	const GLsizei _size;

	constexpr GLsizei size() const { return _size; }
	constexpr const T* data() const { return _data; }

	custom_array(T* data, GLsizei size)
	: _data(data), _size(size) {}
	~custom_array() = default;
};

#endif
