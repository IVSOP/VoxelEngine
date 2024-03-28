#ifndef VERTEX_H
#define VERTEX_H

#include <iostream>
#include <glm/glm.hpp>

#include <GL/glew.h> // GLfloat

// struct Vertex { // struct of vertices as sent to the GPU. position is relative to the chunk, and the normal can only be 0, 1, ..., 5
// 	glm::u8vec3 pos = glm::u8vec3(0);
// 	GLubyte normal = 0;
// 	GLubyte material_id = 0;

//     // friend std::ostream &operator<<(std::ostream &os, const Vertex &p) {
//     //     os << "Vertex: { x: " << p.coords.x << ", y: " << p.coords.y << ", z: " << p.coords.z << ", n_x: " << p.normal.x << ", n_y: " << p.normal.y << ", n_z: " << p.normal.z << ", t_x: " << p.tex_coord.x << ", t_y: " << p.tex_coord.y << " }";
//     //     return os;
//     // }

//     Vertex() = default;
// };

struct InstanceVertex {
	glm::vec3 coords;
	// glm::vec2 tex_coords;

	InstanceVertex(const glm::vec3 &coords)
	: coords(coords) {}

	// InstanceVertex(const glm::vec3 &coords, const glm::vec2 &tex_coords)
	// : coords(coords), tex_coords(tex_coords) {}

	// InstanceVertex(GLfloat x, GLfloat y, GLfloat z, GLfloat u, GLfloat v)
	// : coords(x, y, z), tex_coords(u, v) {}
};

struct AxisVertex { // to draw the axis
	glm::vec4 coords = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	AxisVertex() = default;
	AxisVertex(float x, float y, float z, float R, float G, float B) : coords(x, y, z, 1.0f), color(R, G, B, 1.0f) {}
};

struct ViewportVertex { // if needed, to draw on the entire viewport
	glm::vec4 coords = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec2 tex_coord = glm::vec2(0.0f, 0.0f);

	ViewportVertex(float x, float y, float z, float tex_x, float tex_y) : coords(x, y, z, 1.0f), tex_coord(tex_x, tex_y) {}
};

#endif //CG_VERTEX_H
