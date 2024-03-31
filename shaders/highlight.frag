#version 410 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 brightColor;

uniform mat4 u_View; // view from the MVP
uniform float u_BloomThreshold = 1.0;

void main() {
	color = vec4(1.0, 0.0, 0.0, 1.0);


	// extract bright colors into the separate color attachment
	float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722)); // common approximation of luminance based on human perception of color (or so I'm told)
    if(brightness > u_BloomThreshold) {
        brightColor = vec4(color.rgb, 1.0);
	} else {
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
