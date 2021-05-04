#version 430 core

layout (location = 0) in vec3 in_Position;
layout (location = 3) in ivec3 in_Index;

layout (std430, binding = 0) buffer points {
  vec2 p[];
};

uniform float zoom = 1.5f;

out vec3 ex_Color;

void main() {

  vec3 tpos = vec3(0);
  if(in_Position.x > 0.0f) tpos = vec3(p[in_Index[0]], dot(p[in_Index[0]],p[in_Index[0]])/10.0f);//length()/10.0f);
  if(in_Position.y > 0.0f) tpos = vec3(p[in_Index[1]], dot(p[in_Index[1]],p[in_Index[1]])/10.0f);//length(p[in_Index[1]])/10.0f);
  if(in_Position.z > 0.0f) tpos = vec3(p[in_Index[2]], dot(p[in_Index[2]],p[in_Index[2]])/10.0f);//length(p[in_Index[2]])/10.0f);
  gl_Position = vec4(tpos/zoom, 1.0f);

  ex_Color = vec3(0);

}
