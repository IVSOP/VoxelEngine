#version 460 core

// this is in all instances. is this even beter than just hardcoding it depending on normal???
layout(location = 0) in vec3 aPos; // position as a float of the default plane configuration

// this is for every istance
// could be uint, int for compatibility reasons or something, but the underlying data actually does not matter as long as it has 32 bits
layout(location = 1) in int aData;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_NormalMatrix; // since it is constant every single vertex

uniform samplerBuffer u_ChunkInfoTBO;
#define VEC4_IN_CHUNKINFO 1

out VS_OUT {
	vec2 v_TexCoord;
	flat int v_MaterialID; // flat since it is always the same between all vertices. int because reasons, should be an uint
	vec3 v_Normal; // in view space. will make the normal vector here, but in the future might just pass the normal from 0 - 6
	vec3 v_FragPos; // in view space
} vs_out;

void main()
{
/* starting from the left (from FF in 0xFF000000), that is, the most significant bits
32 bits {
	5 - pos x
	5 - pos y
	5 - pos z
	7 - materialID
	5 - len_x
	5 - len_y
}

normal {
	0 - y (bottom)
	1 + y (top)
	2 - z (far)
	3 + z (near)
	4 - x (left)
	5 + x (right)
}
*/
	// decode data from the attributes
	// idk if this is how it should be done, I just apply the & at the end to make sure the rest of the number is zeroed out
	const uint position_x = (aData >> 27) & 0x0000001F;
	const uint position_y = (aData >> 22) & 0x0000001F;
	const uint position_z = (aData >> 17) & 0x0000001F;
	const int materialID  = (aData >> 10) & 0x0000007F;
	const float len_x     = float(uint((aData >>  5) & 0x0000001F) + 1); // values are offset by 1 to save 1 bit, since len is never 0
	const float len_y     = float(uint( aData        & 0x0000001F) + 1); // values are offset by 1 to save 1 bit, since len is never 0

	const vec2 len = vec2(len_x, len_y);
	vec3 rel_pos = vec3(position_x, position_y, position_z);

	int chunkID = gl_DrawID;
	vs_out.v_MaterialID = materialID;

	vec3 position = aPos;

	// rotate it depending on normal
	// there are probably better ways to do this
	// to get texture coordinates, since each voxel represents a pixel on a 32x32x32 chunk, and the texture represents the side of the entire chunk, just divide pos by 31. however, this depends on orientation so is done here

	vec3 chunk_position = texelFetch(u_ChunkInfoTBO, chunkID * VEC4_IN_CHUNKINFO).xyz;
	int normal = int(texelFetch(u_ChunkInfoTBO, chunkID * VEC4_IN_CHUNKINFO).w);
	// 0 - 5 can be precisely represented as floats

	switch(normal) {
		case 0:
			vs_out.v_Normal = u_NormalMatrix * vec3(0.0, -1.0, 0.0);
			position.yz = position.zy;
			vs_out.v_TexCoord = vec2(rel_pos.x / 32.0, rel_pos.z / 32.0);

			break;
		case 1:
			vs_out.v_Normal = u_NormalMatrix * vec3(0.0, 1.0, 0.0);
			position.z = 1.0 - position.y;
			position.y = 1.0;
			vs_out.v_TexCoord = vec2(rel_pos.x / 32.0, 1.0 - (rel_pos.z / 32.0));

			break;
		case 2:
			// normal is just hardcoded
			vs_out.v_Normal = u_NormalMatrix * vec3(0.0, 0.0, 1.0);

			position.x = 1.0 - position.x;
			position.xy *= len;

			position += rel_pos;
			vs_out.v_TexCoord = vec2(position.x/ 32.0, position.y / 32.0);
			break;
		case 3:
			// just bring it foward
			vs_out.v_Normal = u_NormalMatrix * vec3(0.0, 0.0, -1.0);
			position.z = 1.0;
			position.xy *= len;

			rel_pos.xy += position.xy;
			vs_out.v_TexCoord = vec2(1.0 - (rel_pos.x / 32.0), rel_pos.y / 32.0);
			break;
		case 4:
			vs_out.v_Normal = u_NormalMatrix * vec3(-1.0, 0.0, 0.0);
			position.xyz = position.zyx;
			vs_out.v_TexCoord = vec2(rel_pos.z / 32.0, rel_pos.y / 32.0);

			break;
		case 5:
			vs_out.v_Normal = u_NormalMatrix * vec3(1.0, 0.0, 0.0);
			position.z = 1.0 - position.x;
			position.x = 1.0;
			vs_out.v_TexCoord = vec2(1.0 - (rel_pos.z / 32.0), rel_pos.y / 32.0);

			break;
	}

	position += vec3(chunk_position.x, chunk_position.y, chunk_position.z);

	vec4 viewspace_pos = u_View * u_Model * vec4(position, 1.0);
	vs_out.v_FragPos = vec3(viewspace_pos);
	gl_Position = u_Projection * viewspace_pos;
}
