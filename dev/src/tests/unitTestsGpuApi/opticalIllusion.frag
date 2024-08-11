#version 440 core

in vec4 diffuseInterpolator;

out vec4 oColor;

void main()
{
	oColor = diffuseInterpolator;
}