#version 330 core

in vec2 uvInterpolator;

out vec4 oColor;

uniform sampler2D colorSampler;

void main()
{
	oColor = texture( colorSampler, uvInterpolator );
}