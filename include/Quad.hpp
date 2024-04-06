#ifndef QUAD_H
#define QUAD_H

// these not currently used
#define MAX_MATERIAL 0x000000FF
#define MAX_CHUNK_ID 0x00FFFFFF

// this is a quad that is processed and ready to go to the gpu in whatever weird format it expects
// you can think of this as Vertex, but I found it confusing since I am using instances (basically I rather think about drawing instances rather than drawing vertices)
struct Quad {
	GLint data = 0;

	constexpr Quad(const glm::u8vec3 &pos, GLubyte normal, GLubyte _material_id, GLint _chunk_id) {
		data = ((pos.x << 27) & 0xF8000000) | ((pos.y << 22) & 0x07C00000) | ((pos.z << 17) & 0x003E0000) | ((_material_id << 10) & 0x0001FC00) |  ((normal << 7) & 0x00000380);

	}

	glm::uvec3 getPosition() {
		return glm::uvec3(
			(data >> 27) & 0x0000001F,
			(data >> 22) & 0x0000001F,
			(data >> 17)  & 0x0000001F
		);
	}

	GLint getMaterialID() const {
		return ((data >> 10) & 0x0000007F);
	}

	GLubyte getNormal() const {
		return ((data >> 7) & 0x00000007);
	}
};

/* starting from the left (from FF in 0xFF000000), that is, the most significant bits
32 bits {
	5 - pos x
	5 - pos y
	5 - pos z
	7 - materialID
	3 - normal
}

32 bits {
	8 - material id
	24 - chunk id
}
*/


	// printf("%u 0x%08x 0x%08x 0x%08x 0x%08x\n", (firstQuad.position_and_normal & 0xFF000000) >> 24, firstQuad.position_and_normal & 0x00FF0000, firstQuad.position_and_normal & 0x0000FF00, firstQuad.position_and_normal & 0x000000FF, firstQuad.material_id & 0xFF000000);

#endif
