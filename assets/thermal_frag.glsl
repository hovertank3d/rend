#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 fragTexPos;

vec3 thermal(vec3 c)
{
    vec3 result;
    float luminance = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
    float THRESHOLD = 0.3;
    result = (luminance < THRESHOLD) ? mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), luminance * 2.0 ) : mix(vec3(0.0, 1.0, 1.0), vec3(1.0, 0.0, 0.0), (luminance - 0.5) * 2.0);
    result *= 0.1 + 0.25 + 0.75 * pow( 16.0 * fragTexPos.x * fragTexPos.y * (1.0 - fragTexPos.x) * (1.0 - fragTexPos.y), 0.15 );
    return vec3(pow(0.299 * result.r + 0.587 * result.g + 0.114 * result.b, 2.2));
}

void main()
{
    vec3 lightPos = vec3(0.0, 5.0, 0.0); 
    vec3 viewPos = vec3(0.0, 5.0, 0.0); 
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.1);
    vec3 diffuse = diff * lightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular) * vec3(0.7);
    FragColor = vec4(thermal(result), 1.0);
} 