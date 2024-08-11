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
out vec4 colorInterpolator;

layout( std140, row_major ) uniform cb3
{
	mat4x4 viewMatrix;
	mat4x4 projMatrix;
	vec4 lightPos;
	mat4x4 worldMatrix;
	vec4 surfaceColor;
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

	colorInterpolator = (vec4( vNormal, 0.0f ) * worldMatrix ) * 0.5f + 0.5f;
	//colorInterpolator = vec4( 1.f, 0.f, 0.f, 1.0f );
}