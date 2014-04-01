#version 330 core
// Based on the shader from tutorial #8 from opengl-tutorial.org

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

// Output data ; will be interpolated for each fragment.
out vec2 UV;
out vec2 UVSphere;
out vec3 Position_worldspace;
out vec3 Normal_cameraspace;
out vec3 EyeDirection_cameraspace;
out vec3 LightDirection_cameraspace;
out vec3 Light2Direction_cameraspace;
out vec3 Normal;

// Values that stay constant for the whole mesh.
uniform mat4 modelViewProj;
uniform mat4 viewMat;
uniform mat4 modelMat;
uniform mat3 normalMatrix;
uniform vec3 lightPosWorld;
uniform vec3 light2PosWorld;


void main(){

	// Output position of the vertex, in clip space : modelViewProj * position
	gl_Position =  modelViewProj * vec4(vertexPosition_modelspace,1);
	
	// Position of the vertex, in worldspace : modelMat * position
	Position_worldspace = (modelMat * vec4(vertexPosition_modelspace,1)).xyz;
	
	// Vector that goes from the vertex to the camera, in camera space.
	// In camera space, the camera is at the origin (0,0,0).
	vec3 vertexPosition_cameraspace = ( viewMat * modelMat * vec4(vertexPosition_modelspace,1)).xyz;
	EyeDirection_cameraspace = vec3(0,0,0) - vertexPosition_cameraspace;

	// Vector that goes from the vertex to the light, in camera space. modelMat is ommited because it's identity.
	vec3 Light2Position_cameraspace = ( viewMat * vec4(light2PosWorld,1)).xyz;
	Light2Direction_cameraspace = Light2Position_cameraspace + EyeDirection_cameraspace;
	
	// Same for light #1
	vec3 LightPosition_cameraspace = ( viewMat * vec4(lightPosWorld,1)).xyz;
	LightDirection_cameraspace = LightPosition_cameraspace + EyeDirection_cameraspace;
	
	// Normal of the the vertex, in camera space
	Normal_cameraspace = ( viewMat * modelMat * vec4(vertexNormal_modelspace,0)).xyz; // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
	
	// Project to find the spherical texture coordinates for the sphere map
	vec3 u = normalize( vertexPosition_cameraspace.xyz );
	vec3 n2 = Normal_cameraspace;
	vec3 r = reflect( u, n2 );
	float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
	UVSphere = vec2(r.x/m + 0.5, r.y/m + 0.5);
	
	// UV of the vertex. No special space for this one.
	UV = vertexUV;
}


