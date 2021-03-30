#version 310 es
layout(location = 0) in vec4 Position;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec3 Normal;
layout(location = 0) out vec4 vColor;
layout(location = 1) out vec3 worldNormal;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
};

void main()
{
   gl_Position = MVP * Position;
   vColor = Color;
   vec4 calcNormal = MVP * vec4(Normal, 1);
   worldNormal = calcNormal.xyz;
}
