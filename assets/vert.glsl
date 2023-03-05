#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 texPos;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec2 fragTexPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    
    fragTexPos = texPos;
    gl_Position = (projection * view * model) * vec4(aPos, 1.0);
}