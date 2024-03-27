#version 410 core

// this is in all instances
layout(location = 0) in vec3 aPos; // position as a float of the default plane configuration
layout(location = 1) in vec2 aTexCoords;

// this is for every istance
layout(location = 2) in int aPosAndNormal; // 3 first bytes are the actual data, xyz, the 1 remaining is the normal (0 - 6)
layout(location = 3) in int aMaterialID; // only the first byte actually has the data


uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

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
	// idk if this is how it should be done, I just apply the & at the end to make sure the rest of the number is zeroed out. I wanted to use uint instead of int but got warnings for some reason
	int position_x = (aPosAndNormal >> 24) & 0x000000FF;
	int position_y = (aPosAndNormal >> 16) & 0x000000FF;
	int position_z = (aPosAndNormal >> 8)  & 0x000000FF;
	int normal = 	  aPosAndNormal        & 0x000000FF;

	int materialID = (aMaterialID >> 24) & 0x000000FF;

	// position inside chunk, added to default position
	vec3 position = aPos;

	// rotate it depending on normal
	// there are probably better ways to do this
	switch(normal) {
		case 0:
			position.yz = position.zy;
			break;
		case 1:
			position.z = 1.0 - position.y;
			position.y = 1.0;
			break;
		case 2:
			// the default vertices are already in the right place, but rotated completely incorrectly
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

	position += vec3(float(position_x), float(position_y), float(position_z));



	// add the position of the chunk itself
	float chunk_position_x = 0.0;
	float chunk_position_y = 0.0;
	float chunk_position_z = 0.0;
	position += vec3(chunk_position_x, chunk_position_y, chunk_position_z);

	gl_Position = u_Projection * u_View * u_Model * vec4(position, 1.0);
}
