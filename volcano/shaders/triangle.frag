#version 310 es
precision mediump float;
layout(location = 0) in vec4 vColor;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 vertPos;
layout(location = 0) out vec4 FragColor;

vec3 lightPos = vec3(4.0f, 4.0f, -4.0f);

void main()
{
   vec3 toLight = normalize(lightPos - vertPos);
   float dirPart = dot(toLight, worldNormal);

   float lightDist = distance(lightPos, vertPos);
   
   FragColor = vColor * min(1500.0f * dirPart / (lightDist * lightDist * lightDist) + 0.2, 1.3f);
}
