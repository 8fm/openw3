/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderParticleEmitter.h"

template<> void CRenderParticleEmitter::GenerateVertexData< SimpleParticle, ParticleSimpleVertex >( const SimpleParticle* RESTRICT particle ) const
{
	// Constants
	Uint32 color;

	Uint8 red = (Uint8) ::Clamp< Float >( particle->m_color.X * 255.0f, 0.0f, 255.0f );
	Uint8 green = (Uint8) ::Clamp< Float >( particle->m_color.Y * 255.0f, 0.0f, 255.0f );
	Uint8 blue = (Uint8) ::Clamp< Float >( particle->m_color.Z * 255.0f, 0.0f, 255.0f );
	Uint8 alpha = (Uint8) ::Clamp< Float >( particle->m_alpha * 255.0f, 0.0f, 255.0f );
	color = (alpha<<24)|(red<<16)|(green<<8)|blue;

	const Float rotationSin = sinf( particle->m_rotation );
	const Float rotationCos = cosf( particle->m_rotation );
	const Float sizeX = particle->m_size.X;
	const Float sizeY = particle->m_size.Y;
	const Float sizeXneg = -sizeX;
	const Float sizeYneg = -sizeY;
	const Float zero = 0.0f;
	const Float one = 1.0f;

	// 0 -- 1
	// | \  |  
	// |  \ |
	// 3 -- 2

	// be cache friendly and write members in order they are defined (so write-combine can kick in)

	ParticleSimpleVertex* RESTRICT vertex = NULL;
	// Vertex 0
	CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleSimpleVertex >( vertex );
	COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.X );
	COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.Y );
	COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.Z );
	COPY_FIELD_AS_INT32( vertex->m_color, color );
	COPY_FIELD_AS_INT32( vertex->m_rotation[0], rotationCos );
	COPY_FIELD_AS_INT32( vertex->m_rotation[1], rotationSin );
	COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
	COPY_FIELD_AS_INT32( vertex->m_size[0], sizeXneg );
	COPY_FIELD_AS_INT32( vertex->m_size[1], sizeYneg );
	COPY_FIELD_AS_INT32( vertex->m_uv[0], zero );
	COPY_FIELD_AS_INT32( vertex->m_uv[1], zero );

	// Vertex 1
	CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleSimpleVertex >( vertex );
	//Red::System::MemoryCopy( (void*)vertex, (void*)prevVertex, sizeof( ParticleSimpleVertex ) );
	COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.X );
	COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.Y );
	COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.Z );
	COPY_FIELD_AS_INT32( vertex->m_color, color );
	COPY_FIELD_AS_INT32( vertex->m_rotation[0], rotationCos );
	COPY_FIELD_AS_INT32( vertex->m_rotation[1], rotationSin );
	COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
	COPY_FIELD_AS_INT32( vertex->m_size[0], sizeX );
	COPY_FIELD_AS_INT32( vertex->m_size[1], sizeYneg );
	COPY_FIELD_AS_INT32( vertex->m_uv[0], one );
	COPY_FIELD_AS_INT32( vertex->m_uv[1], zero );

	// Vertex 2
	CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleSimpleVertex >( vertex );
	//Red::System::MemoryCopy( (void*)vertex, (void*)prevVertex, sizeof( ParticleSimpleVertex ) );
	COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.X );
	COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.Y );
	COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.Z );
	COPY_FIELD_AS_INT32( vertex->m_color, color );
	COPY_FIELD_AS_INT32( vertex->m_rotation[0], rotationCos );
	COPY_FIELD_AS_INT32( vertex->m_rotation[1], rotationSin );
	COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
	COPY_FIELD_AS_INT32( vertex->m_size[0], sizeX );
	COPY_FIELD_AS_INT32( vertex->m_size[1], sizeY );
	COPY_FIELD_AS_INT32( vertex->m_uv[0], one );
	COPY_FIELD_AS_INT32( vertex->m_uv[1], one );

	// Vertex 3
	CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleSimpleVertex >( vertex );
	//Red::System::MemoryCopy( (void*)vertex, (void*)prevVertex, sizeof( ParticleSimpleVertex ) );
	COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.X );
	COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.Y );
	COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.Z );
	COPY_FIELD_AS_INT32( vertex->m_color, color );
	COPY_FIELD_AS_INT32( vertex->m_rotation[0], rotationCos );
	COPY_FIELD_AS_INT32( vertex->m_rotation[1], rotationSin );
	COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
	COPY_FIELD_AS_INT32( vertex->m_size[0], sizeXneg );
	COPY_FIELD_AS_INT32( vertex->m_size[1], sizeY );
	COPY_FIELD_AS_INT32( vertex->m_uv[0], zero );
	COPY_FIELD_AS_INT32( vertex->m_uv[1], one );
}

template<> void CRenderParticleEmitter::GenerateVertexData< SimpleParticle, ParticleMotionVertex >( const SimpleParticle* RESTRICT particle ) const
{
	// Constants
	Uint32 color;

	Uint8 red = (Uint8) ::Clamp< Float >( particle->m_color.X * 255.0f, 0.0f, 255.0f );
	Uint8 green = (Uint8) ::Clamp< Float >( particle->m_color.Y * 255.0f, 0.0f, 255.0f );
	Uint8 blue = (Uint8) ::Clamp< Float >( particle->m_color.Z * 255.0f, 0.0f, 255.0f );
	Uint8 alpha = (Uint8) ::Clamp< Float >( particle->m_alpha * 255.0f, 0.0f, 255.0f );
	color = (alpha<<24)|(red<<16)|(green<<8)|blue;

	const Float rotationSin = sinf( particle->m_rotation );
	const Float rotationCos = cosf( particle->m_rotation );
	const Float sizeX = particle->m_size.X;
	const Float sizeY = particle->m_size.Y;
	const Float sizeXneg = -sizeX;
	const Float sizeYneg = -sizeY;
	const Float zero = 0.0f;
	const Float one = 1.0f;
	
	// compute the value of motion blur
	Vector tempVelocity = particle->m_velocity;
	const Float velocityLength = tempVelocity.Normalize3();
	const Float motionBlend = Red::Math::NumericalUtils::Min( 1.0f, velocityLength * m_drawerData.motionBlur.m_oneOverBlendVelocityRange );

	// compute the amount of particle stretch
	const Float stretch = m_drawerData.motionBlur.m_stretchPerVelocity * velocityLength;

	// 0 -- 1
	// | \  |  
	// |  \ |
	// 3 -- 2

	// be cache friendly and write members in order they are defined (so write-combine can kick in)

	ParticleMotionVertex* RESTRICT vertex = NULL;
	// Vertex 0
	CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleMotionVertex >( vertex );
	//Red::System::MemoryCopy( (void*)&vertex->m_position, (void*)&particle->m_position, 3*sizeof( Float ) );
	COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.X );
	COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.Y );
	COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.Z );
	COPY_FIELD_AS_INT32( vertex->m_color, color );
	COPY_FIELD_AS_INT32( vertex->m_rotation[0], rotationCos );
	COPY_FIELD_AS_INT32( vertex->m_rotation[1], rotationSin );
	COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
	COPY_FIELD_AS_INT32( vertex->m_size[0], sizeXneg );
	COPY_FIELD_AS_INT32( vertex->m_size[1], sizeYneg );
	COPY_FIELD_AS_INT32( vertex->m_uv[0], zero );
	COPY_FIELD_AS_INT32( vertex->m_uv[1], zero );
	//Red::System::MemoryCopy( (void*)&vertex->m_direction, (void*)&tempVelocity, 3*sizeof( Float ) );
	COPY_FIELD_AS_INT32( vertex->m_direction[0], tempVelocity.X );
	COPY_FIELD_AS_INT32( vertex->m_direction[1], tempVelocity.Y );
	COPY_FIELD_AS_INT32( vertex->m_direction[2], tempVelocity.Z );
	COPY_FIELD_AS_INT32( vertex->m_stretch, stretch );
	COPY_FIELD_AS_INT32( vertex->m_motionBlend, motionBlend );

	// Vertex 1
	CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleMotionVertex >( vertex );
	//Red::System::MemoryCopy( (void*)vertex, (void*)prevVertex, sizeof( ParticleMotionVertex ) );
	COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.X );
	COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.Y );
	COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.Z );
	COPY_FIELD_AS_INT32( vertex->m_color, color );
	COPY_FIELD_AS_INT32( vertex->m_rotation[0], rotationCos );
	COPY_FIELD_AS_INT32( vertex->m_rotation[1], rotationSin );
	COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
	COPY_FIELD_AS_INT32( vertex->m_size[0], sizeX );
	COPY_FIELD_AS_INT32( vertex->m_size[1], sizeYneg );
	COPY_FIELD_AS_INT32( vertex->m_uv[0], one );
	COPY_FIELD_AS_INT32( vertex->m_uv[1], zero );
	COPY_FIELD_AS_INT32( vertex->m_direction[0], tempVelocity.X );
	COPY_FIELD_AS_INT32( vertex->m_direction[1], tempVelocity.Y );
	COPY_FIELD_AS_INT32( vertex->m_direction[2], tempVelocity.Z );
	COPY_FIELD_AS_INT32( vertex->m_stretch, stretch );
	COPY_FIELD_AS_INT32( vertex->m_motionBlend, motionBlend );

	// Vertex 2
	CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleMotionVertex >( vertex );
	//Red::System::MemoryCopy( (void*)vertex, (void*)prevVertex, sizeof( ParticleMotionVertex ) );
	COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.X );
	COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.Y );
	COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.Z );
	COPY_FIELD_AS_INT32( vertex->m_color, color );
	COPY_FIELD_AS_INT32( vertex->m_rotation[0], rotationCos );
	COPY_FIELD_AS_INT32( vertex->m_rotation[1], rotationSin );
	COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
	COPY_FIELD_AS_INT32( vertex->m_size[0], sizeX );
	COPY_FIELD_AS_INT32( vertex->m_size[1], sizeY );
	COPY_FIELD_AS_INT32( vertex->m_uv[0], one );
	COPY_FIELD_AS_INT32( vertex->m_uv[1], one );
	COPY_FIELD_AS_INT32( vertex->m_direction[0], tempVelocity.X );
	COPY_FIELD_AS_INT32( vertex->m_direction[1], tempVelocity.Y );
	COPY_FIELD_AS_INT32( vertex->m_direction[2], tempVelocity.Z );
	COPY_FIELD_AS_INT32( vertex->m_stretch, stretch );
	COPY_FIELD_AS_INT32( vertex->m_motionBlend, motionBlend );

	// Vertex 3
	CParticleVertexBuffer::GetNextVertexPtrIncrement< ParticleMotionVertex >( vertex );
	//Red::System::MemoryCopy( (void*)vertex, (void*)prevVertex, sizeof( ParticleMotionVertex ) );
	COPY_FIELD_AS_INT32( vertex->m_position[0], particle->m_position.X );
	COPY_FIELD_AS_INT32( vertex->m_position[1], particle->m_position.Y );
	COPY_FIELD_AS_INT32( vertex->m_position[2], particle->m_position.Z );
	COPY_FIELD_AS_INT32( vertex->m_color, color );
	COPY_FIELD_AS_INT32( vertex->m_rotation[0], rotationCos );
	COPY_FIELD_AS_INT32( vertex->m_rotation[1], rotationSin );
	COPY_FIELD_AS_INT32( vertex->m_frame, particle->m_frame );
	COPY_FIELD_AS_INT32( vertex->m_size[0], sizeXneg );
	COPY_FIELD_AS_INT32( vertex->m_size[1], sizeY );
	COPY_FIELD_AS_INT32( vertex->m_uv[0], zero );
	COPY_FIELD_AS_INT32( vertex->m_uv[1], one );
	COPY_FIELD_AS_INT32( vertex->m_direction[0], tempVelocity.X );
	COPY_FIELD_AS_INT32( vertex->m_direction[1], tempVelocity.Y );
	COPY_FIELD_AS_INT32( vertex->m_direction[2], tempVelocity.Z );
	COPY_FIELD_AS_INT32( vertex->m_stretch, stretch );
	COPY_FIELD_AS_INT32( vertex->m_motionBlend, motionBlend );
}

template<> void CRenderParticleEmitter::GenerateVertexData< MeshParticle, void >( const MeshParticle* RESTRICT particle ) const
{
}