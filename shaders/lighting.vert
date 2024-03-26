#version 410 core

// this is in all instances
layout(location = 0) in vec3 aPos; // position as a float of the default plane configuration


// this is for every istance
layout(location = 1) in int aPosAndNormal; // 3 first bytes are the actual data, xyz, the 1 remaining is the normal (0 - 6)
layout(location = 2) in int aMaterialID; // only the first byte actually has the data


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
*/

	// decode data from the attributes
	int position_x = (aPosAndNormal & 0xFF000000) >> 24;
	int position_y = (aPosAndNormal & 0x00FF0000) >> 16;
	int position_z = (aPosAndNormal & 0x0000FF00) >> 8;
	int normal = 	  aPosAndNormal & 0x000000FF;

	int materialID = (aMaterialID & 0xFF000000) >> 24;


	int chunkID = 0;
	vec3 position_in_chunk = vec3(float(position_x), float(position_y), float(position_z));
	// vec4 position = vec4(position_in_chunk + aPos, 1.0);
	vec4 position = vec4(aPos, 1.0);

	gl_Position = u_Projection * u_View * u_Model * position;
}
