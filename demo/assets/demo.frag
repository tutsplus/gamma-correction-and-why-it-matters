#version 330 core
// Based on the shader from tutorial #8 from opengl-tutorial.org

// It's slow and unoptimized. It can be done *way* better, but the
// shader is not the point of the demo so it has to suffice

// Interpolated values from the vertex shaders
in vec2 UV;
in vec2 UVSphere;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;
in vec3 Light2Direction_cameraspace;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D diffuse;
uniform sampler2D sphereMap;
uniform mat4 modelView;
uniform vec3 lightPosWorld;
uniform vec3 light2PosWorld;

// This function will compute light contributions
void blinnPhong(vec3 lightPos, vec3 lightDir, vec3 diffuse, vec3 specular, vec3 lightColor, float lightPower, out vec3 oDiffuse, out vec3 oSpecular)
{
	float distance = length( lightPos - Position_worldspace );
	
	// Normal of the computed fragment, in camera space
	vec3 n = normalize( Normal_cameraspace );
	// Direction of the light (from the fragment to the light)
	vec3 l = normalize( lightDir );
	// Cosine of the angle between the normal and the light direction, 
	// clamped above 0
	//  - light is at the vertical of the triangle -> 1
	//  - light is perpendicular to the triangle -> 0
	//  - light is behind the triangle -> 0
	float cosTheta = clamp( dot( n,l ), 0,1 );
	
	// Eye vector (towards the camera)
	vec3 E = normalize(EyeDirection_cameraspace);
	// Direction in which the triangle reflects the light
	vec3 R = reflect(-l,n);
	// Cosine of the angle between the Eye vector and the Reflect vector,
	// clamped to 0
	//  - Looking into the reflection -> 1
	//  - Looking elsewhere -> < 1
	float cosAlpha = clamp( dot( E,R ), 0,1 );
	
	oDiffuse =
		// Diffuse : "color" of the object
		diffuse * lightColor * lightPower * cosTheta / (distance * distance);
		
	oSpecular = 
		// Specular : reflective highlight, like a mirror
		specular * lightColor * lightPower * pow(cosAlpha, 30) / (distance * distance); 
		// NB the shininess value is 30. This can be lowered for a more plastic
		// look (less steep falloff)
}

void main()
{

	// Light emission properties
	// We ought to put them as uniforms when we're doing something serious,
	// but for a small demo this will do
	vec3 LightColor1 = vec3(1,0.7,0.7);
	vec3 LightColor2 = vec3(0.7, 0.7, 1);
	float LightPower = 3.0f;
	
	// Material properties
	vec3 MaterialDiffuseColor = texture2D( diffuse, UV * 3 ).rgb;
	vec3 MaterialAmbientColor = vec3(0.1,0.1,0.1);
	vec3 MaterialSpecularColor = vec3(6, 6, 6);

	vec3 l1d, l1s, l2d, l2s;

	// Call blinnPhong() to compute the light contributions
	blinnPhong(lightPosWorld, LightDirection_cameraspace, MaterialDiffuseColor, MaterialSpecularColor, LightColor1, LightPower, l1d, l1s);
	blinnPhong(light2PosWorld, Light2Direction_cameraspace, MaterialDiffuseColor, MaterialSpecularColor, LightColor2, LightPower, l2d, l2s);
	
	// Sample the spheremap using the second set of texcoords (passed from the vertex
	// shader) and add light contributions to it
	color = MaterialDiffuseColor * texture2D(sphereMap, UVSphere).xyz * 0.2 + l2d + l2s + l1d + l1s;
}
