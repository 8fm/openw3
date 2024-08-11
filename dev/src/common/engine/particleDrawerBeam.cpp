/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"


#if 0

CParticleDrawerBeam::CParticleDrawerBeam()
	: m_texturesPerUnit( 1.0f )
	, m_numSegments( 10 )
{
	m_spread = new CEvaluatorVectorConst( this, Vector::ZEROS );
}

void CParticleDrawerBeam::InitParticles( IParticleData *data, Uint32 firstIndex, Uint32 count ) const
{
	CParticleData< BeamParticle >* beamParticleData = ( CParticleData< BeamParticle >* ) data;
	CParticleBuffer< BeamParticle >& buffer = beamParticleData->GetBuffer();

	Uint32 numBeams = count / m_numSegments;
	for (Uint32 i=0; i<count; i+=m_numSegments )
	{
		BeamParticle* newParticle = buffer.GetParticleAt( firstIndex + i );
		newParticle->m_beamSpread = EvaluateVector( m_spread );
	}
}

//// Update particles
//void CParticleDrawerBeam::UpdateParticles( IParticleBuffer& particleBuffer, const IParticleDrawer::UpdateContext& updateContext  ) const
//{
//	if ( !updateContext.m_targetNode  )
//	{
//		// Need target for a beam
//		return;
//	}
//
//	// Update living particles
//	Uint32 numParticles = particleBuffer.GetNumParticles();
//
//	Int32 beamsCount = numParticles / m_numSegments;
//	Uint32 clampedCount = numParticles - numParticles % m_numSegments;
//
//	const Vector emitterPos = updateContext.m_componentTransform.GetTranslation();
//	
//	Uint32 i = 0;
//	Uint32 maxParticle = m_numSegments;
//	for ( Int32 j=0; j<beamsCount; ++j )
//	{
//		Particle* firstBeamParticle = particleBuffer.GetParticleAt( j * m_numSegments );
//		
//		const Vector targetPos = updateContext.m_targetNode->GetWorldPosition() + Vector( firstBeamParticle->m_beamSpread );
//		// Compute distance to target
//		const Float targetDistance = targetPos.DistanceTo( emitterPos );
//		// Compute segment length
//		const Float segmentLength = targetDistance / m_numSegments;
//
//		// Compute beam direction
//		const Vector beamDirection = ( targetPos - emitterPos ).Normalized3();
//
//		// Compute segment offset vector
//		const Vector beamSegmentOffset = beamDirection * segmentLength;
//
//		Uint32 segmentIndex = 0;
//		for( ; i < maxParticle; ++i )
//		{
//			// Load particle
//			Particle& particle = *particleBuffer.GetParticleAt( i );
//
//			segmentIndex = i % m_numSegments + 1;
//
//			const Float step = (Float)segmentIndex/(Float)m_numSegments;
//
//			particle.m_position = emitterPos + beamSegmentOffset * (Float)segmentIndex + ( particle.m_velocity ) * ( ( ( i < maxParticle - 1 ) ? 1.0f : 0.0f ) * EvaluateFloat( m_cone, step, 1.0f ) );
//
//			// Update if not dead
//			if ( *(Uint32*)&particle.m_lifeSpanInv )
//			{
//				// Update particle life
//				particle.m_life += updateContext.m_timeDelta;
//
//				// Kill the particle if it's time come to pass
//				Float lifeFrac = particle.m_life * particle.m_lifeSpanInv;
//				if ( lifeFrac >= 1.0f )
//				{
//					// Kill
//					particle.m_life = 1.0f / particle.m_lifeSpanInv;
//					particle.m_lifeSpanInv = 0.0f;
//
//					particleBuffer.Dealloc( i );
//					i--;
//					numParticles--;
//
//					beamsCount = numParticles / m_numSegments;
//					clampedCount = numParticles - numParticles % m_numSegments;
//
//					continue;
//				}
//			}
//		}
//		maxParticle += m_numSegments;
//		maxParticle = ( maxParticle > clampedCount ) ? clampedCount : maxParticle;
//	}	
//}

// Generate particle render data
IRenderParticleData* CParticleDrawerBeam::GenerateRenderData( const IParticleData* data, const IParticleData::RenderingContext& renderingContext ) const
{
	// Get particle data
	CParticleData< BeamParticle >* beamParticleData = ( CParticleData< BeamParticle >* ) data;
	const CParticleBuffer< BeamParticle >& buffer = beamParticleData->GetBuffer();
	
	const Uint32 count = buffer.GetNumParticles();

	const Int32 beamsCount = count / m_numSegments;

	const Uint32 clampedCount = count - count % m_numSegments;

	const Vector emitterPos = renderingContext.m_component->GetWorldPosition();

	if ( beamsCount > 0 )
	{
		// Create render data buffer
		IRenderParticleData* renderData = GRender->CreateParticleData( PDT_Beam, clampedCount );
		if ( renderData )
		{
			// Get the data
			RenderBeamParticle* outBuffer = ( RenderBeamParticle* ) renderData->GetData();

			// Fill particles
			Vector3 tempN1Pos, tempN2Pos;
			RenderBeamParticle* writePtr = outBuffer;
			
			Float texFraction = 1.0f / m_numSegments;

			Uint32 iParticle = 0;
			Uint32 maxParticle = m_numSegments;
			// Group particles by beam
			for ( Int32 j=0; j<beamsCount; ++j )
			{
				const BeamParticle* particle = buffer.GetParticleAt( iParticle );

				Float lastTexCoord = 0.0f;
				Uint8 lastAlpha = 0;

				tempN1Pos = emitterPos;
				tempN2Pos = emitterPos;

				for ( ; iParticle<maxParticle; ++iParticle, ++writePtr )
				{
					const BeamParticle* particle = buffer.GetParticleAt( iParticle );

					writePtr->m_position[0] = particle->m_position.A[0];
					writePtr->m_position[1] = particle->m_position.A[1];
					writePtr->m_position[2] = particle->m_position.A[2];

					writePtr->m_N1Pos[0] = tempN1Pos.A[0];
					writePtr->m_N1Pos[1] = tempN1Pos.A[1];
					writePtr->m_N1Pos[2] = tempN1Pos.A[2];

					writePtr->m_N2Pos[0] = tempN2Pos.A[0];
					writePtr->m_N2Pos[1] = tempN2Pos.A[1];
					writePtr->m_N2Pos[2] = tempN2Pos.A[2];


					Float particleLength = (particle->m_position - tempN1Pos).Mag();
					Float texCoord = lastTexCoord + particleLength * m_texturesPerUnit;	
					writePtr->m_texCoord = texCoord;
					writePtr->m_lastTexCoord = lastTexCoord;
					lastTexCoord = texCoord;

					tempN2Pos = tempN1Pos;
					tempN1Pos = particle->m_position;

					Uint8 alpha = (Uint8) ::Clamp<Float>( particle->m_color.A[3] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[0] = (Uint8) ::Clamp< Float >( particle->m_color.A[0] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[1] = (Uint8) ::Clamp< Float >( particle->m_color.A[1] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[2] = (Uint8) ::Clamp< Float >( particle->m_color.A[2] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[3] = alpha;
					writePtr->m_lastAlpha = lastAlpha;
					lastAlpha = alpha;

					writePtr->m_frame = particle->m_frame;

					writePtr->m_stretch = particle->m_size.X;
				}
				maxParticle += m_numSegments;
			}

			// Generate vertices, this can be done here or on the GPU to improve CPU usage, for now this is done here
			renderData->GenerateVertexData();
			return renderData;
		}
	}

	// Data not created
	return NULL;
}

#endif