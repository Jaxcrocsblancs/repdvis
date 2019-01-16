#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;
in vec3 tangent_cameraspace;
in vec3 bitangent_cameraspace;

// Output data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D diffuse;
uniform sampler2D tangentnm;
uniform mat4 MV;
uniform vec3 LightPosition_worldspace;

void main() {
    // Light emission properties
    // You probably want to put them as uniforms
    float LightPower = 1.5f;


    // Distance to the light
    float distance = 1;//length( LightPosition_worldspace - Position_worldspace );

    // Normal of the computed fragment, in camera space
    vec3 n = normalize( Normal_cameraspace );

    mat3 B = mat3(normalize(tangent_cameraspace), normalize(bitangent_cameraspace), n);
    n = normalize((B*normalize(texture(tangentnm, UV).rgb * 2 - 1)));

    // Direction of the light (from the fragment to the light)
    vec3 l = normalize( LightDirection_cameraspace );
    // Cosine of the angle between the normal and the light direction, 
    // clamped above 0
    //  - light is at the vertical of the triangle -> 1
    //  - light is perpendicular to the triangle -> 0
    //  - light is behind the triangle -> 0
    float cosTheta = clamp( dot( n,l ), 0,1 );

    // Eye vector (towards the camera)
    vec3 E = normalize(EyeDirection_cameraspace);
    // Direction in which the triangle reflects the light
    vec3 R = -reflect(l,n);
    if (dot(n,l)<0) R = vec3(0,0,0);
    // Cosine of the angle between the Eye vector and the Reflect vector,
    // clamped to 0
    //  - Looking into the reflection -> 1
    //  - Looking elsewhere -> < 1
    float cosAlpha = clamp( dot( E,R ), 0,1 );


    color =  texture(diffuse, UV).rgb * (
                                            0.1 +   // Ambient : simulates indirect lighting
                                            LightPower * cosTheta / (distance*distance) +          // Diffuse : "color" of the object
                                            2.0*LightPower * pow(cosAlpha,15) / (distance*distance) // Specular : reflective highlight, like a mirror
          );
}

