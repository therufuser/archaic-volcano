#version 310 es
precision mediump float;
layout(location = 0) in vec4 vColor;
layout(location = 1) in vec3 worldNormal;
layout(location = 0) out vec4 FragColor;

void main()
{
   FragColor = vColor * dot(worldNormal, vec3(1, 1, 1));
}
