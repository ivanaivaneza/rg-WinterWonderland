#version 330 core
out vec4 FragColor;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;

};

#define NUM_POINT_LIGHTS 6

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform DirLight dirLight;
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform Material material;

uniform vec3 lightColor;


uniform vec3 viewPosition;
uniform bool blinn;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0f;
    if(blinn){
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    }
    else{
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords).rgb);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords).rgb);
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).rgb);
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0f;
    if(blinn){
        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    }
    else{
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    }
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords).rgb) ;
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords).rgb) ;
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).rgb) ;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 result = CalcDirLight(dirLight, normal, viewDir);
    for(int i = 0;i < NUM_POINT_LIGHTS;i++){
        result += CalcPointLight(pointLights[i],normal,FragPos,viewDir);
    }

    FragColor = vec4(result, 1.0);
}