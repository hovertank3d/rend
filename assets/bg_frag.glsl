#version 460 core

out vec4 FragColor;

in vec2 texPos;

uniform sampler2D tex;

void main()
{
   vec3 col = texture(tex, texPos).rgb;
   FragColor = vec4(col, 1.0);
}