#ifndef QUAD_H
#define QUAD_H

#define NUM_MATERIALS (1 << 7)

// this is a quad that is processed and ready to go to the gpu in whatever weird format it expects
// you can think of this as Vertex, but I found it confusing since I am using instances (basically I rather think about drawing instances rather than drawing vertices)
struct Quad {
	GLint data = 0;

	constexpr Quad(const glm::u8vec3 &pos, GLubyte _material_id, GLubyte len_x, GLubyte len_y) {
		data = ((pos.x << 27) & 0xF8000000) | ((pos.y << 22) & 0x07C00000) | ((pos.z << 17) & 0x003E0000) | ((_material_id << 10) & 0x0001FC00) | ((len_x << 5) & 0x000003E0) | (len_y & 0x0000001F);

	}

	glm::uvec3 getPosition() {
		return glm::uvec3(
			(data >> 27) & 0x0000001F,
			(data >> 22) & 0x0000001F,
			(data >> 17) & 0x0000001F
		);
	}

	GLint getMaterialID() const {
		return ((data >> 10) & 0x0000007F);
	}

	glm::uvec2 getLen() const {
		return glm::uvec2(
			(data >> 5) & 0x0000001F,
			 data       & 0x0000001F
		);
	}
};

/* starting from the left (from FF in 0xFF000000), that is, the most significant bits
32 bits {
	5 - pos x
	5 - pos y
	5 - pos z
	7 - materialID
	5 - len_x
	5 - len_y
}
*/


	// printf("%u 0x%08x 0x%08x 0x%08x 0x%08x\n", (firstQuad.position_and_normal & 0xFF000000) >> 24, firstQuad.position_and_normal & 0x00FF0000, firstQuad.position_and_normal & 0x0000FF00, firstQuad.position_and_normal & 0x000000FF, firstQuad.material_id & 0xFF000000);

#endif
