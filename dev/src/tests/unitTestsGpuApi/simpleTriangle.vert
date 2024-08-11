#version 330 core

out vec4 vertexColor;

layout(std140) uniform cb0
{
	vec4 colors0;
	vec4 colors1;
	vec4 colors2;
};

void main()
{
	const vec4 positions[3] = vec4[3](	vec4( -0.5f, -0.5f, 0.5f, 1.f ),	
										vec4(  0.0f,  0.5f, 0.5f, 1.f ),
										vec4(  0.5f, -0.5f, 0.5f, 1.f ) );

	vec4 colors[3] = vec4[3]( colors0, colors1, colors2 );
	gl_Position = positions[gl_VertexID];
	vertexColor = colors[gl_VertexID];
}