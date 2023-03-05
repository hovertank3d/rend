#version 460 core

layout (location = 0) in vec2 aPos;

out vec2 texPos;

void main()
{
   texPos = (aPos.xy + 1.0) / 2.0;
   gl_Position = vec4(aPos.x, aPos.y, 1.0, 1.0);
}