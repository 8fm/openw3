// 8x8 grid of triangles, each a uniform color.
// Can draw fewer instances, will fill grid left-to-right, top-to-bottom (reading order)

#version 330 core

layout(std140) uniform cb0
{
	vec4 colors[64];
};


out vec4 surfaceColor;

void main()
{
	const vec2 verts[3] = vec2[3](
		vec2( -0.1f,  0.1f ),
		vec2(  0.0f, -0.1f ),
		vec2(  0.1f,  0.1f )
	);

	vec2 gridPos = vec2( gl_InstanceID % 8, gl_InstanceID / 8 );
	vec2 instancePos = vec2( gridPos.x / 8.0f, 1.0f - gridPos.y / 8.0f ) * 2.0f - 1.0f;
	vec2 p = verts[ gl_VertexID ] + instancePos + vec2( 0.1f, -0.1f );

	gl_Position = vec4( p, 0, 1);
	surfaceColor = colors[ gl_InstanceID ];
}
