#version 440 core

in vec4 normalInterpolator;
in vec4 viewVecInterpolator;
in vec4 lightVec0Interpolator;

out vec4 oColor;

layout(std140, row_major) uniform cb0
{
	vec4 surfaceColor;
};

const vec3 ambientColor = vec3(0.2f, 0.2f, 0.2f);
const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
const vec3 kd = vec3(0.5f, 0.5f, 0.5f);
const vec3 ks = vec3(0.5f, 0.5f, 0.5f); 
const vec3 m = vec3(100.0f, 100.0f, 100.0f);

void main()
{
	vec3 viewVec = normalize(viewVecInterpolator.xyz);
	vec3 normal = normalize(normalInterpolator.xyz);
	vec3 lightVec = normalize(lightVec0Interpolator.xyz);
	vec3 halfVec = normalize(viewVec + lightVec);
	vec3 color = surfaceColor.xyz * ambientColor;

	float NdHclamped = clamp( dot(normal, halfVec), 0.f, 1.f );

	color += lightColor * surfaceColor.xyz * kd * clamp( dot(normal, lightVec), 0.f, 1.f ) +
			 lightColor * ks * pow( vec3(NdHclamped, NdHclamped, NdHclamped), m);

	oColor = vec4( clamp(2.0f*color, 0.f, 1.f), surfaceColor.a);
	
	//float NdotL = dot(normal, lightVec);
	//oColor = vec4( NdotL, NdotL, NdotL, 1.0f );
	//oColor = vec4( 1.f, 0.f, 0.f, 1.0f );
}