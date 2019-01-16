#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;
in vec3 tangent_cameraspace;
in vec3 bitangent_cameraspace;

// Output data
out vec3 color;

// Values that stay constant for the entire mesh
uniform sampler2D diffuse;
uniform sampler2D tangentnm;
uniform sampler2D specular;
uniform mat4 MV;
uniform vec3 LightPosition_worldspace;

void main() {
    float LightPower = 1.5f;                   // Light emission properties
    vec3 n = normalize( Normal_cameraspace );  // Normal of the computed fragment, in camera space

    mat3 B = mat3(normalize(tangent_cameraspace), normalize(bitangent_cameraspace), n);
    n = normalize((B*normalize(texture(tangentnm, UV).rgb * 2 - 1))); // tangent space normal mapping
    
    vec3 l = normalize( LightDirection_cameraspace );  // Direction of the light (from the fragment to the light)
    float cosTheta = clamp( dot( n,l ), 0,1 );         // Cosine of the angle between the normal and the light direction, 
    
    vec3 E = normalize(EyeDirection_cameraspace);  // Eye vector (towards the camera)
    vec3 R = -reflect(l,n);                        // Direction in which the triangle reflects the light
    float cosAlpha = clamp( dot( E,R ), 0,1 );     // Cosine of the angle between the Eye vector and the Reflect vector,

    color =  texture(diffuse, UV).xyz*(0.1 +
                                       LightPower*cosTheta  +
                                       LightPower*pow(cosAlpha, texture(specular, UV).r*250+1));
}

