#version 440 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

out gl_PerVertex
{
	vec4 gl_Position;
};
out vec4 normalInterpolator;
out vec4 viewVecInterpolator;
out vec4 lightVec0Interpolator;

layout(std140, row_major) uniform cb0
{
	mat4x4 viewMatrix;
	mat4x4 projMatrix;
	vec4 lightPos;
};

layout(std140, row_major) uniform cb1
{
	mat4x4 worldMatrix;
};

void main()
{
	mat4x4 worldView = worldMatrix * viewMatrix;
	vec4 viewPos = vec4( vPosition, 1.0f );
	viewPos = viewPos * worldView;

	gl_Position = viewPos * projMatrix;
	//gl_Position = vec4( vPosition, 1.0f );

	normalInterpolator = (vec4( vNormal, 0.0f ) * worldMatrix );
	normalInterpolator = normalize( normalInterpolator );
	
	viewVecInterpolator = vec4(normalize( -viewPos.xyz ), 1);
	lightVec0Interpolator = vec4( normalize(( (lightPos * viewMatrix) - viewPos ).xyz ), 1);
}