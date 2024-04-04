#ifndef QUAD_H
#define QUAD_H

// these not currently used
#define MAX_MATERIAL 0x000000FF
#define MAX_CHUNK_ID 0x00FFFFFF

// this is a quad that is processed and ready to go to the gpu in whatever weird format it expects
// you can think of this as Vertex, but I found it confusing since I am using instances (basically I rather think about drawing instances rather than drawing vertices)
struct Quad {
	GLint position_and_normal = 0;
	GLint material_and_chunkid = 0;

	constexpr Quad(const glm::u8vec3 &pos, GLubyte normal, GLubyte _material_id, GLint _chunk_id) {
		position_and_normal = ((pos.x << 26) & 0xFC000000) | ((pos.y << 20) & 0x03F00000) | ((pos.z << 14) & 0x000FC000) | (normal & 0x000000FF);

		material_and_chunkid = ((static_cast<GLuint>(_material_id) << 24) & 0xFF000000) | (_chunk_id & 0x00FFFFFF);
	}

	glm::uvec3 getPosition() {
		return glm::uvec3(
			(position_and_normal >> 26) & 0x0000003F,
			(position_and_normal >> 20) & 0x0000003F,
			(position_and_normal >> 14)  & 0x0000003F
		);
	}

	GLint getMaterialID() const {
		return ((material_and_chunkid >> 24) & 0x000000FF);
	}

	GLuint getChunkID() const {
		return (material_and_chunkid & 0x00FFFFFF);
	}

	GLubyte getNormal() const {
		return (position_and_normal & 0x000000FF);
	}
};

/* starting from the left (from FF in 0xFF000000), that is, the most significant bits
32 bits {
	6 - pos x
	6 - pos y
	6 - pos z
	last 8 - normal
}

32 bits {
	8 - material id
	24 - chunk id
}
*/


	// printf("%u 0x%08x 0x%08x 0x%08x 0x%08x\n", (firstQuad.position_and_normal & 0xFF000000) >> 24, firstQuad.position_and_normal & 0x00FF0000, firstQuad.position_and_normal & 0x0000FF00, firstQuad.position_and_normal & 0x000000FF, firstQuad.material_id & 0xFF000000);

#endif
