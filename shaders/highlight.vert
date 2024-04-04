#version 410 core

// this is in all instances. is this even beter than just hardcoding it depending on normal???
layout(location = 0) in vec3 aPos; // position as a float of the default plane configuration

// this is for every istance
// could be uint, int for compatibility reasons or something, but the underlying data actually does not matter as long as it has 32 bits
layout(location = 1) in int aPosAndNormal; // 3 first bytes are the actual data, xyz, the 1 remaining is the normal (0 - 6)
layout(location = 2) in int aMaterialAndChunkID; // only the first byte actually has the data

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_NormalMatrix; // since it is constant every single vertex

uniform samplerBuffer u_ChunkInfoTBO;
#define VEC4_IN_CHUNKINFO 1

void main()
{
	// decode data from the attributes
	// idk if this is how it should be done, I just apply the & at the end to make sure the rest of the number is zeroed out
	uint position_x = (aPosAndNormal >> 27) & 0x0000001F;
	uint position_y = (aPosAndNormal >> 22) & 0x0000001F;
	uint position_z = (aPosAndNormal >> 17) & 0x0000001F;
	uint normal     =  aPosAndNormal >> 14  & 0x00000007;

	int chunkID    =  aMaterialAndChunkID        & 0x00FFFFFF;

	// position inside chunk, added to default position
	vec3 position = aPos;

	// rotate it depending on normal
	// there are probably better ways to do this
	// to get texture coordinates, since each voxel represents a pixel on a 32x32x32 chunk, and the texture represents the side of the entire chunk, just divide pos by 31. however, this depends on orientation so is done here

	switch(normal) {
		case 0:
			position.yz = position.zy;
			break;
		case 1:
			position.z = 1.0 - position.y;
			position.y = 1.0;
			break;
		case 2:
			position.x = 1.0 - position.x;
			break;
		case 3:
			// just bring it foward
			position.z = 1.0;
			break;
		case 4:
			position.xyz = position.zyx;
			break;
		case 5:
			position.z = 1.0 - position.x;
			position.x = 1.0;
			break;
	}

	vec3 chunk_position = texelFetch(u_ChunkInfoTBO, chunkID * VEC4_IN_CHUNKINFO).xyz;
	position += vec3(float(position_x) + chunk_position.x, float(position_y) + chunk_position.y, float(position_z) + chunk_position.z);

	vec4 viewspace_pos = u_View * u_Model * vec4(position, 1.0);
	gl_Position = u_Projection * viewspace_pos;
}
