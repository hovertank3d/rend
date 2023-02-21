#version 460 core

layout (location = 0) in vec2 aPos;

out vec2 texPos;

void main()
{
   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
   texPos = (aPos.xy + 1.0) / 2.0;
}