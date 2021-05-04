#version 430 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Color;

uniform float zoom = 1.5f;

out vec3 ex_Color;

void main(void) {
	gl_Position = vec4(in_Position/zoom, 1.0f);
	ex_Color = in_Color;
}
