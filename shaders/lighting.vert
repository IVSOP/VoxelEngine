#version 410 core

// this is in all instances. is this even beter than just hardcoding it depending on normal???
layout(location = 0) in vec3 aPos; // position as a float of the default plane configuration

// this is for every istance
// could be uint, int for compatibility reasons or something, but the underlying data actually does not matter as long as it has 32 bits
layout(location = 1) in int aPosAndNormal; // 3 first bytes are the actual data, xyz, the 1 remaining is the normal (0 - 6)
layout(location = 2) in int aMaterialID; // only the first byte actually has the data

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_NormalMatrix; // since it is constant every single vertex

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
	8 - pos x
	8 - pos y
	8 - pos z
	8 - normal
}

32 bits {
	8 - material id
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
	uint position_x = (aPosAndNormal >> 24) & 0x000000FF;
	uint position_y = (aPosAndNormal >> 16) & 0x000000FF;
	uint position_z = (aPosAndNormal >> 8)  & 0x000000FF;
	uint normal = 	  aPosAndNormal        & 0x000000FF;

	int materialID = (aMaterialID >> 24) & 0x000000FF;
	vs_out.v_MaterialID = materialID;

	// position inside chunk, added to default position
	vec3 position = aPos;

	// rotate it depending on normal
	// there are probably better ways to do this
	// to get texture coordinates, since each voxel represents a pixel on a 32x32x32 chunk, and the texture represents the side of the entire chunk, just divide pos by 31. however, this depends on orientation so is done here

	switch(normal) {
		case 0:
			position.yz = position.zy;
			vs_out.v_Normal = u_NormalMatrix * vec3(0.0, -1.0, 0.0);
			vs_out.v_TexCoord = vec2(position_x / 31.0, position_z / 31.0);
			break;
		case 1:
			position.z = 1.0 - position.y;
			position.y = 1.0;
			vs_out.v_Normal = u_NormalMatrix * vec3(0.0, 1.0, 0.0);
			vs_out.v_TexCoord = vec2(position_x / 31.0, 1.0 - (position_z / 31.0));
			break;
		case 2:
			// the default vertices are already in the right place, but rotated completely incorrectly
			position.x = 1.0 - position.x;
			vs_out.v_Normal = u_NormalMatrix * vec3(0.0, 0.0, 1.0);
			vs_out.v_TexCoord = vec2(position_x / 31.0, position_y / 31.0);
			break;
		case 3:
			// just bring it foward
			position.z = 1.0;
			vs_out.v_Normal = u_NormalMatrix * vec3(0.0, 0.0, -1.0);
			vs_out.v_TexCoord = vec2(1.0 - (position_x / 31.0), position_y / 31.0);
			break;
		case 4:
			position.xyz = position.zyx;
			vs_out.v_Normal = u_NormalMatrix * vec3(-1.0, 0.0, 0.0);
			vs_out.v_TexCoord = vec2(position_z / 31.0, position_y / 31.0);
			break;
		case 5:
			position.z = 1.0 - position.x;
			position.x = 1.0;
			vs_out.v_Normal = u_NormalMatrix * vec3(1.0, 0.0, 0.0);
			vs_out.v_TexCoord = vec2(1.0 - (position_z / 31.0), position_y / 31.0);
			break;
	}

	position += vec3(float(position_x), float(position_y), float(position_z));
	float chunk_position_x = 0.0;
	float chunk_position_y = 0.0;
	float chunk_position_z = 0.0;
	position += vec3(chunk_position_x, chunk_position_y, chunk_position_z);

	vec4 viewspace_pos = u_View * u_Model * vec4(position, 1.0);
	vs_out.v_FragPos = vec3(viewspace_pos);
	gl_Position = u_Projection * viewspace_pos;
}
