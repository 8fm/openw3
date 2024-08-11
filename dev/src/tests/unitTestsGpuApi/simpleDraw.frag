#version 330 core

in vec4 vertexColor;


layout( location = 0 ) out vec4 fragColor0;
layout( location = 1 ) out vec4 fragColor1;
layout( location = 2 ) out vec4 fragColor2;
layout( location = 3 ) out vec4 fragColor3;

void main()
{
	fragColor0 = vertexColor;
	fragColor1 = vertexColor;
	fragColor2 = vertexColor;
	fragColor3 = vertexColor;
}
