#version 420

in vec3 Position_eye;
in vec3 Normal_eye;
in vec2 TexCoord;

struct LightPositional
{
    vec3 position; // in eye coordinates
    vec3 intensity;
};

struct LightSpotlight
{
    vec3 position;  // in eye coordinates
    vec3 direction; // in eye coordinates
    vec3 intensity;
    float cutoff;   // (angle) in radians
};

uniform int num_pos_lights;
uniform LightPositional light_pos[20];
uniform int num_spot_lights;
uniform LightSpotlight light_spot[20];

struct Material
{
	vec3 Kd;            // Diffuse reflectivity
	vec3 Ka;            // Ambient reflectivity
	vec3 Ks;            // Specular reflectivity
	float shininess;    // Specular shininess factor
};

uniform Material material;

subroutine void RenderPassType();
subroutine uniform RenderPassType RenderPass;

// The G-buffer textures
uniform sampler2D PositionTex, NormalTex, ColorTex, ShininessTex;

layout( location = 0 ) out vec4 FragColor;
layout( location = 1 ) out vec3 PositionData;
layout( location = 2 ) out vec3 NormalData;
layout( location = 3 ) out vec3 ColorData;
layout( location = 4 ) out float ShininessData;


vec3 adsPositional(uint index, vec3 position, vec3 norm, vec3 color, float shininess)
{
    vec3 s = normalize( light_pos[index].position - position);
    vec3 v = normalize(-position);
    vec3 r = reflect( -s, norm );
    
    //vec3 ambient = light_pos[index].intensity * material.Ka;
     
    float sDotN = max(dot(s,norm), 0.0);
    vec3 diffuse = light_pos[index].intensity * color * sDotN;
    
    float rDotV = dot(r, v);
    vec3 specular = light_pos[index].intensity * color * pow(max(rDotV, 0.0), shininess);

    //return ambient + diffuse + specular;
    return diffuse;
}



vec3 adsSpotlight(uint index, vec3 position, vec3 norm, vec3 color, float shininess)
{
    vec3 s = normalize( light_spot[index].position - position);
    vec3 v = normalize(-position);
    vec3 r = reflect( -s, norm );
    
    vec3 ads = vec3(0.0f);   // Ambient + Diffuse + Specular
    
    // Ambient
    //ads += light_spot[index].intensity * material.Ka;
     
    // Discard fragments that fall outside of the spotlight cutoff angle
    if (dot(s, light_spot[index].direction) > cos(light_spot[index].cutoff))
    {
        // Diffuse      
        float sDotN = max(dot(s,norm), 0.0);
        ads += light_spot[index].intensity * color * sDotN;
        
        // Specular
        float rDotV = dot(r, v);
        ads += light_spot[index].intensity * color * pow(max(rDotV, 0.0), shininess);
    }
        
    return ads;
}


subroutine (RenderPassType)
void pass1()
{
    // Store position, normal and diffuse color in g-buffer
    PositionData 	= Position_eye;
    NormalData   	= Normal_eye;
    ColorData    	= material.Kd;
    ShininessData	= material.shininess;
    
    //FragColor = vec4(0.8f, 0.0f, 0.0f, 1.0f);
}


subroutine (RenderPassType)
void pass2()
{
    // Get the position, normal and diffuse coeffient from the G-buffer
    vec3 pos  = vec3( texture(PositionTex, TexCoord));
    vec3 norm = vec3( texture(NormalTex, TexCoord));
    vec3 color = vec3( texture(ColorTex, TexCoord));
    float shininess = texture(ShininessTex, TexCoord));
    
    FragColor = vec4(0.0f);
    
    // Positional
    for (int index = 0; index < num_pos_lights; ++index)
        FragColor += vec4(adsPositional(index, pos, norm, color, shininess), 1.0f);
        
    // Spotlight
    for (int index = 0; index < num_spot_lights; ++index)
        FragColor += vec4(adsSpotlight(index, pos, norm, color, shininess), 1.0f);
    
}


void main()
{
    RenderPass();
}
