/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if 0
class CParticleDrawerTrail : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerTrail, IParticleDrawer, 0);

protected:
	Bool m_textureTiling;
	Float m_texturesPerUnit;
	Bool m_dynamicTexCoords;
	Bool m_HQtextureStretch;

public:
	CParticleDrawerTrail()
		: m_texturesPerUnit( 0.5f )
		, m_dynamicTexCoords( false )
		, m_HQtextureStretch( true )
	{

	}

	void InitParticles( IParticleData* data, Uint32 firstIndex, Uint32 count ) const
	{
		CParticleData< TrailParticle >* trailParticleData = ( CParticleData< TrailParticle >* ) data;
		CParticleBuffer< TrailParticle >& buffer = trailParticleData->GetBuffer();

		Uint32 numParticles = buffer.GetNumParticles();
		TrailParticle* particle = buffer.GetParticleAt( firstIndex );

		IParticleData::ParticleDrawerCache& cache = data->GetDrawerCache();
		if ( cache.m_emissionStopped ) 
		{
			cache.m_lastPosition = particle->m_position;
			cache.m_lastTextCoord = 0.0f;
			cache.m_emissionStopped = false;
		}
		if (!m_dynamicTexCoords && count)
		{
			for (Uint32 i=0; i<count; ++i)
			{
				TrailParticle* particle = buffer.GetParticleAt( i + firstIndex );

				Float particleLength = (particle->m_position - cache.m_lastPosition).Mag();
				//LOG_ENGINE( TXT("position: %s"), ToString( newParticles[i].m_position ) );
				//LOG_ENGINE( TXT("cached last position: %s"), ToString( cache.m_lastPosition ) );
				//LOG_ENGINE( TXT("particle length: %f"), particleLength );
				Float texCoord = cache.m_lastTextCoord + particleLength * m_texturesPerUnit;
				//LOG_ENGINE( TXT("tex coord: %f"), texCoord );
				particle->m_prevTexCoord = cache.m_lastTextCoord;
				particle->m_texCoord = texCoord;
				cache.m_lastTextCoord = texCoord;
				cache.m_lastPosition = particle->m_position;
			}
		}
	}

	// Generate particle render data
	virtual IRenderParticleData* GenerateRenderData( const IParticleData* data, const IParticleData::RenderingContext& renderingContext ) const
	{
		// Get particle data
		CParticleData< TrailParticle >* trailParticleData = ( CParticleData< TrailParticle >* ) data;
		const CParticleBuffer< TrailParticle >& buffer = trailParticleData->GetBuffer();
		const Uint32 count = buffer.GetNumParticles();
		if ( count )
		{
			// Create render data buffer, add 1 particle for the subframe patch
			IRenderParticleData* renderData = GRender->CreateParticleData( PDT_Trail, buffer.GetNumParticles() + 1 );
			if ( renderData )
			{
				// Get the data
				const TrailParticle* particle = buffer.GetParticleAt( 0 );
				RenderTrailParticle* outBuffer = ( RenderTrailParticle* ) renderData->GetData();

				// Fill particles
				Vector3 tempLastPos;
				Vector3 emitterAxis, lastEmitterAxis;
				Float lastTexCoord = 0.0f;
				Uint8 lastAlpha = 0;
				RenderTrailParticle* writePtr = outBuffer;
				writePtr->m_lastPos[0] = particle->m_position.A[0];
				writePtr->m_lastPos[1] = particle->m_position.A[1];
				writePtr->m_lastPos[2] = particle->m_position.A[2];
				lastEmitterAxis = particle->m_emitterAxis;
				tempLastPos = particle->m_position;
				Float texFraction = 1.0f/count;
				Float fullTrailLength = 0.0f;
				for ( Uint32 i = 0; i < count; i++, writePtr++ )
				{
					particle = buffer.GetParticleAt( i );

					writePtr->m_position[0] = particle->m_position.A[0];
					writePtr->m_position[1] = particle->m_position.A[1];
					writePtr->m_position[2] = particle->m_position.A[2];

					writePtr->m_emitterAxis[0] = particle->m_emitterAxis.A[0];
					writePtr->m_emitterAxis[1] = particle->m_emitterAxis.A[1];
					writePtr->m_emitterAxis[2] = particle->m_emitterAxis.A[2];

					writePtr->m_lastPos[0] = tempLastPos.A[0];
					writePtr->m_lastPos[1] = tempLastPos.A[1];
					writePtr->m_lastPos[2] = tempLastPos.A[2];

					writePtr->m_lemitterAxis[0] = lastEmitterAxis.A[0];
					writePtr->m_lemitterAxis[1] = lastEmitterAxis.A[1];
					writePtr->m_lemitterAxis[2] = lastEmitterAxis.A[2];

					lastEmitterAxis = particle->m_emitterAxis;

					if ( m_dynamicTexCoords )
					{
						if ( m_textureTiling )
						{
							Float particleLength = (particle->m_position - tempLastPos).Mag();
							Float texCoord = lastTexCoord + particleLength * m_texturesPerUnit;	
							writePtr->m_texCoord = texCoord;
							writePtr->m_lastTexCoord = lastTexCoord;
							lastTexCoord = texCoord;
						}
						else
						{
							if ( m_HQtextureStretch )
							{
								Float particleLength = (particle->m_position - tempLastPos).Mag();
								writePtr->m_particleLen = particleLength;
								fullTrailLength += particleLength;
							}
							else
							{
								writePtr->m_lastTexCoord = lastTexCoord;
								Float texCoord = lastTexCoord + texFraction;
								writePtr->m_texCoord = texCoord;
								lastTexCoord = texCoord;
							}	
						}
					}
					else 
					{
						writePtr->m_texCoord = particle->m_texCoord;
						writePtr->m_lastTexCoord = particle->m_prevTexCoord;
					}
					tempLastPos = particle->m_position;

					Uint8 alpha = (Uint8) ::Clamp<Float>( particle->m_color.A[3] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[0] = (Uint8) ::Clamp< Float >( particle->m_color.A[0] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[1] = (Uint8) ::Clamp< Float >( particle->m_color.A[1] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[2] = (Uint8) ::Clamp< Float >( particle->m_color.A[2] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[3] = alpha;
					writePtr->m_lastAlpha = lastAlpha;
					lastAlpha = alpha;

					writePtr->m_frame = particle->m_frame;

					writePtr->m_stretch = particle->m_size.X / 2.0f;
				}

				if (m_HQtextureStretch && !m_textureTiling && m_dynamicTexCoords )
				{
					RenderTrailParticle* readWritePtr = outBuffer;
					lastTexCoord = 0.0f;
					Float oneByFullTrailLength;
					if ( fullTrailLength>0.0f )
					{
						oneByFullTrailLength = 1.0f / fullTrailLength;
					}
					else
					{
						oneByFullTrailLength = 0.0f;
					}

					for (Uint32 i=0 ; i<count; ++i)
					{
						readWritePtr[i].m_lastTexCoord = lastTexCoord;
						readWritePtr[i].m_texCoord = lastTexCoord + readWritePtr[i].m_particleLen * oneByFullTrailLength;
						lastTexCoord = readWritePtr[i].m_texCoord;
					}
				}

				const Matrix& currentTransform = renderingContext.m_component->GetLocalToWorld();
				const Vector& currentPos = currentTransform.GetTranslation();
				const Vector& currentAxis = currentTransform.GetRow( 2 );
				// TEMPSHIT: Spawn a patch between current component position and the newest particle
				const TrailParticle* first = buffer.GetParticleAt( count - 1 );

				// Get the addition render particle for the patch
				RenderTrailParticle* ptrPrev = &outBuffer[ count-1 ];
				RenderTrailParticle* ptrLast = &outBuffer[ count ];

				{	
					const Float lastTexCoordToStretch = ptrPrev->m_texCoord;
					const Float texLengthToStretch = MAbs( ptrPrev->m_texCoord - ptrPrev->m_lastTexCoord );
					const Float effectiveLastTexCoord = ptrPrev->m_lastTexCoord + texLengthToStretch / 2.0f;
					ptrPrev->m_texCoord = effectiveLastTexCoord;

					ptrLast->m_position[0] = currentPos.A[0];
					ptrLast->m_position[1] = currentPos.A[1];
					ptrLast->m_position[2] = currentPos.A[2];

					ptrLast->m_emitterAxis[0] = currentAxis.A[0];
					ptrLast->m_emitterAxis[1] = currentAxis.A[1];
					ptrLast->m_emitterAxis[2] = currentAxis.A[2];

					ptrLast->m_lastPos[0] = first->m_position.A[0];
					ptrLast->m_lastPos[1] = first->m_position.A[1];
					ptrLast->m_lastPos[2] = first->m_position.A[2];

					ptrLast->m_lemitterAxis[0] = first->m_emitterAxis.A[0];
					ptrLast->m_lemitterAxis[1] = first->m_emitterAxis.A[1];
					ptrLast->m_lemitterAxis[2] = first->m_emitterAxis.A[2];

					Uint8 alpha = (Uint8) ::Clamp<Float>( first->m_color.A[3] * 255.0f, 0.0f, 255.0f );
					ptrLast->m_color[0] = (Uint8) ::Clamp< Float >( first->m_color.A[0] * 255.0f, 0.0f, 255.0f );
					ptrLast->m_color[1] = (Uint8) ::Clamp< Float >( first->m_color.A[1] * 255.0f, 0.0f, 255.0f );
					ptrLast->m_color[2] = (Uint8) ::Clamp< Float >( first->m_color.A[2] * 255.0f, 0.0f, 255.0f );
					ptrLast->m_color[3] = alpha;
					ptrLast->m_lastAlpha = alpha;

					ptrLast->m_frame = first->m_frame;

					ptrLast->m_stretch = first->m_size.X / 2.0f;

					ptrLast->m_texCoord = lastTexCoordToStretch;
					ptrLast->m_lastTexCoord = effectiveLastTexCoord;
				}

				// Generate vertices, this can be done here or on the GPU to improve CPU usage, for now this is done here
				renderData->GenerateVertexData();
				return renderData;
			}
		}

		// Data not created
		return NULL;
	}

	void OnEmissionBreak( IParticleData * data ) const
	{
		data->GetDrawerCache().m_emissionStopped = true;
	}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType() const { return PT_Trail; }
};

BEGIN_CLASS_RTTI( CParticleDrawerTrail );
PARENT_CLASS( IParticleDrawer );
PROPERTY_EDIT( m_textureTiling, TXT( "Tile texture on trail" ) )
PROPERTY_EDIT( m_texturesPerUnit, TXT( "Textures per length unit in world space" ) )
PROPERTY_EDIT( m_dynamicTexCoords, TXT( "Texture coordinates follow emitter" ) )
PROPERTY_EDIT( m_HQtextureStretch, TXT( "Weighted texture distribution in no tiling mode" ) )
END_CLASS_RTTI();


#endif