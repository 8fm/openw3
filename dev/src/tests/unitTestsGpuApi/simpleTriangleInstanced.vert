#version 330 core

layout(std140) uniform cb0
{
	vec4 colors0;
	vec4 colors1;
	vec4 colors2;
};

//position implied
out vec4 surfaceColor;

void main()
{
	const vec4 positions[3] = vec4[3](	vec4( -0.1f, -0.1f, 0.f, 1.f ),
										vec4(  0.0f,  0.1f, 0.f, 1.f ),
										vec4(  0.1f, -0.1f, 0.f, 1.f ) );
	vec4 colors[3] = vec4[3]( colors0, colors1, colors2 );

	gl_Position = vec4( -0.5f + positions[gl_VertexID].x + ((gl_InstanceID % 8) / 6.0f), -0.5f + positions[gl_VertexID].y + ((gl_InstanceID / 8) / 6.0f), 0, 1);
	surfaceColor = colors[gl_VertexID];
}