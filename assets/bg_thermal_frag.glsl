#version 460 core

out vec4 FragColor;

in vec4 gl_FragCoord;
in vec2 texPos;

uniform sampler2D tex;

void main()
{
   vec3 col = texture(tex, texPos).rgb;
   float lum = dot(col, vec3(0.15));
   FragColor = vec4(vec3(lum), 1.0);
}