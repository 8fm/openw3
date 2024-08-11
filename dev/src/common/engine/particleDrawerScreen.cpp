/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if 0
class CParticleDrawerScreen : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerScreen, IParticleDrawer, 0);

protected:
	// Generate particle render data
	virtual IRenderParticleData* GenerateRenderData( const IParticleData* data, const IParticleData::RenderingContext& renderingContext ) const
	{
		// Get particle data
		CParticleData< SimpleParticle >* simpleParticleData = ( CParticleData< SimpleParticle >* ) data;
		CParticleBuffer< SimpleParticle >& buffer = simpleParticleData->GetBuffer();
		const Uint32 count = buffer.GetNumParticles();
		if ( count )
		{
			// Create render data buffer
			IRenderParticleData* renderData = GRender->CreateParticleData( PDT_Screen, buffer.GetNumParticles() );
			if ( renderData )
			{
				// Get the data
				RenderSimpleParticle* outBuffer = ( RenderSimpleParticle* ) renderData->GetData();

				// Fill particles
				RenderSimpleParticle* writePtr = outBuffer;
				for ( Uint32 i=0; i<count; i++, writePtr++ )
				{
					const SimpleParticle* particle = buffer.GetParticleAt( i );
					writePtr->m_position[0] = particle->m_position.A[0];
					writePtr->m_position[1] = particle->m_position.A[1];
					writePtr->m_position[2] = particle->m_position.A[2];
					writePtr->m_frame = particle->m_frame;
					writePtr->m_rotation = particle->m_rotation;
					writePtr->m_color[0] = (Uint8) ::Clamp< Float >( particle->m_color.A[0] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[1] = (Uint8) ::Clamp< Float >( particle->m_color.A[1] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[2] = (Uint8) ::Clamp< Float >( particle->m_color.A[2] * 255.0f, 0.0f, 255.0f );
					writePtr->m_color[3] = (Uint8) ::Clamp< Float >( particle->m_color.A[3] * 255.0f, 0.0f, 255.0f );
					writePtr->m_size[0] = particle->m_size.X;
					writePtr->m_size[1] = particle->m_size.Y;
					writePtr->m_uv[0] = 0.0f;//( particle->m_flags & FLAG_FLIP_U ) ? 0.0f : 1.0f;
					writePtr->m_uv[1] = 0.0f;//( particle->m_flags & FLAG_FLIP_V ) ? 0.0f : 1.0f;
				}

				// Generate vertices, this can be done here or on the GPU to improve CPU usage, for now this is done here
				renderData->GenerateVertexData();
				return renderData;
			}
		}

		// Data not created
		return NULL;
	}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType() const { return PT_Simple; }
};

BEGIN_CLASS_RTTI( CParticleDrawerScreen );
PARENT_CLASS( IParticleDrawer );
END_CLASS_RTTI();


#endif