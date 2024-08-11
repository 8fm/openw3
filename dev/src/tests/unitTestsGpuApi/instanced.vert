#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec4 vWorldMatrix0;
layout(location = 2) in vec4 vWorldMatrix1;
layout(location = 3) in vec4 vWorldMatrix2;
layout(location = 4) in vec4 vWorldMatrix3;
layout(location = 5) in vec4 vColor;

//position implied
out vec4 surfaceColor;

layout(std140, row_major) uniform cb0
{
	mat4x4 viewMatrix;
	mat4x4 projMatrix;
};

layout(std140, row_major) uniform cb1
{
	vec4 lightPos;
};

void main()
{
	mat4x4 worldMatrix = mat4x4( vWorldMatrix0.x, vWorldMatrix1.x, vWorldMatrix2.x, vWorldMatrix3.x,
								 vWorldMatrix0.y, vWorldMatrix1.y, vWorldMatrix2.y, vWorldMatrix3.y,
								 vWorldMatrix0.z, vWorldMatrix1.z, vWorldMatrix2.z, vWorldMatrix3.z,
								 vWorldMatrix0.w, vWorldMatrix1.w, vWorldMatrix2.w, vWorldMatrix3.w );

	mat4x4 worldView = worldMatrix * viewMatrix;
	vec4 viewPos = vec4( vPosition.xyz, 1.0f );
	viewPos = viewPos * worldView;
	gl_Position = viewPos * projMatrix;

	surfaceColor = vColor;
}