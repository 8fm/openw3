/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if 0
class CParticleDrawerFacingTrail : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerFacingTrail, IParticleDrawer, 0);

protected:
	Float	m_trailWidth;
	Bool	m_textureTiling;
	Float	m_texturesPerUnit;
	Bool	m_dynamicTexCoords;

public:
	CParticleDrawerFacingTrail()
		: m_trailWidth( 0.25f )
		, m_texturesPerUnit( 0.5f )
		, m_dynamicTexCoords( false )
	{

	}

	void InitParticles( IParticleData* data, Uint32 firstIndex, Uint32 count ) const
	{
		CParticleData< TrailParticle >* trailParticleData = ( CParticleData< TrailParticle >* ) data;
		CParticleBuffer< TrailParticle >& buffer = trailParticleData->GetBuffer();
		TrailParticle* newParticle = buffer.GetParticleAt( firstIndex );
		IParticleData::ParticleDrawerCache& cache = data->GetDrawerCache();
		if ( cache.m_emissionStopped ) 
		{
			cache.m_lastPosition = newParticle->m_position;
			cache.m_lastTextCoord = 0.0f;
			cache.m_emissionStopped = false;
		}
		if (!m_dynamicTexCoords && count)
		{
			for (Uint32 i=0; i<count; ++i)
			{
				TrailParticle* newParticle = buffer.GetParticleAt( i + firstIndex );
				Float particleLength = (newParticle->m_position - cache.m_lastPosition).Mag();
				Float texCoord = cache.m_lastTextCoord + particleLength * m_texturesPerUnit;
				newParticle->m_prevTexCoord = cache.m_lastTextCoord;
				newParticle->m_texCoord = texCoord;
				cache.m_lastTextCoord = texCoord;
				cache.m_lastPosition = newParticle->m_position;
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
			// Create render data buffer
			IRenderParticleData* renderData = GRender->CreateParticleData( PDT_FacingTrail, buffer.GetNumParticles() );
			if ( renderData )
			{
				// Get the data
				const TrailParticle* particle = buffer.GetParticleAt( 0 );
				RenderFacingTrailParticle* outBuffer = ( RenderFacingTrailParticle* ) renderData->GetData();

				// Fill particles
				Vector3 tempN1Pos, tempN2Pos;
				Float lastTexCoord = 0.0f;
				Uint8 lastAlpha = 0;
				RenderFacingTrailParticle* writePtr = outBuffer;
				tempN1Pos = particle->m_position;
				tempN2Pos = particle->m_position;
				Float texFraction = 1.0f/count;
				Float fullTrailLength = 0.0f;
				for ( Uint32 i=0; i<count; i++, writePtr++ )
				{
					const TrailParticle* particle = buffer.GetParticleAt( i );

					writePtr->m_position[0] = particle->m_position.A[0];
					writePtr->m_position[1] = particle->m_position.A[1];
					writePtr->m_position[2] = particle->m_position.A[2];

					writePtr->m_N1Pos[0] = tempN1Pos.A[0];
					writePtr->m_N1Pos[1] = tempN1Pos.A[1];
					writePtr->m_N1Pos[2] = tempN1Pos.A[2];

					writePtr->m_N2Pos[0] = tempN2Pos.A[0];
					writePtr->m_N2Pos[1] = tempN2Pos.A[1];
					writePtr->m_N2Pos[2] = tempN2Pos.A[2];

					if ( m_dynamicTexCoords )
					{
						if ( m_textureTiling )
						{
							Float particleLength = (particle->m_position - tempN1Pos).Mag();
							Float texCoord = lastTexCoord + particleLength * m_texturesPerUnit;	
							writePtr->m_texCoord = texCoord;
							writePtr->m_lastTexCoord = lastTexCoord;
							lastTexCoord = texCoord;
						}
						else
						{
							writePtr->m_lastTexCoord = lastTexCoord;
							Float texCoord = lastTexCoord + texFraction;
							writePtr->m_texCoord = texCoord;
							lastTexCoord = texCoord;
						}
					}
					else 
					{
						writePtr->m_texCoord = particle->m_texCoord;
						writePtr->m_lastTexCoord = particle->m_prevTexCoord;
					}
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

					writePtr->m_stretch = m_trailWidth;
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

BEGIN_CLASS_RTTI( CParticleDrawerFacingTrail );
PARENT_CLASS( IParticleDrawer );
PROPERTY_EDIT( m_trailWidth, TXT( "Width of the trail" ) )
PROPERTY_EDIT( m_textureTiling, TXT( "Tile texture on trail" ) )
PROPERTY_EDIT( m_texturesPerUnit, TXT( "Textures per length unit in world space" ) )
PROPERTY_EDIT( m_dynamicTexCoords, TXT( "Texture coordinates follow emitter" ) )
END_CLASS_RTTI();


#endif
