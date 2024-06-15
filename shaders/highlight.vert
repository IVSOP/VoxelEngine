#version 460 core

// this is in all instances
layout(location = 0) in vec3 aBasePos; // position as a float of the default plane configuration

// this is for every istance
layout(location = 1) in vec3 aPos;
layout(location = 2) in int aNormal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
// uniform mat3 u_NormalMatrix;

void main()
{
	vec3 position = aBasePos;
	int normal = aNormal;

	const float offset = 0.1; // offset so no z fighting happens

	switch(normal) {
		case 0:
			position.yz = position.zy;
			position.y -= offset;
			break;
		case 1:
			position.z = 1.0 - position.y;
			position.y = 1.0 + offset;
			break;
		case 2:
			position.x = 1.0 - position.x;
			position.z -= offset;
			break;
		case 3:
			position.z = 1.0 + offset;
			break;
		case 4:
			position.xyz = position.zyx;
			position.x -= offset;
			break;
		case 5:
			position.z = 1.0 - position.x;
			position.x = 1.0 + offset;
			break;
	}

	position += aPos;
	gl_Position = u_Projection * u_View * u_Model * vec4(position, 1.0);
}
