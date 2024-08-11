#version 440 core

layout(location = 0) in vec3 vPosition;

out gl_PerVertex
{
	vec4 gl_Position;
};
out vec4 diffuseInterpolator;

layout(std140, row_major) uniform cb0
{
	mat4x4 worldViewProj;
	float  time;
};

layout(std140, row_major) uniform cb1
{
	vec4 objectPos;
};

void main()
{
	float fSin, fCos;
	float x = length( vPosition ) * sin( time ) * 15.0f;
	//sincos( x, fSin, fCos );
	fSin = sin(x);
	fCos = cos(x);

	vec4 OutPos = (vec4( vPosition.x, fSin * 0.1f, vPosition.y, 0.0f ) + vec4(objectPos.xyz,1)) * worldViewProj;

	gl_Position = OutPos;
	diffuseInterpolator = vec4(0.5f - 0.5f * vec4(fCos, fCos, fCos, fCos));
}