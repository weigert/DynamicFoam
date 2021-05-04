/*
================================================================================
                  Helper Functions for Triangle Math
================================================================================
*/

namespace tri {
using namespace glm;

vec2 circumcenter(vec2 a, vec2 b, vec2 c){
  const float ad = a.x * a.x + a.y * a.y;
  const float bd = b.x * b.x + b.y * b.y;
  const float cd = c.x * c.x + c.y * c.y;
  const float D = 2.0f * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
  return 1.0f/D*vec2( ad*(b.y-c.y) + bd*(c.y-a.y) + cd*(a.y-b.y), ad*(c.x-b.x) + bd*(a.x-c.x) + cd*(b.x-a.x) );
}

vec2 barycenter(vec2 a, vec2 b, vec2 c){
  return 1.0f/3.0f*(a + b + c);
}

vec2 incenter(vec2 a, vec2 b, vec2 c){
  const float da = length(c-b);
  const float db = length(a-c);
  const float dc = length(b-a);
  return 1.0f/(da+db+dc)*glm::vec2(da*a.x+db*b.x+dc*c.x, da*a.y+db*b.y+dc*c.y);
}

float area(vec2 a, vec2 b, vec2 c){
  const float A = a.x*(b.y-c.y)+b.x*(c.y-a.y)+c.x*(a.y-b.y);
  return 12.0f*abs(A);
}

};
