/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if 0
enum EMeshParticleOrientationMode
{
	MPOM_Normal,
	MPOM_MovementDirection
};

BEGIN_ENUM_RTTI( EMeshParticleOrientationMode );
ENUM_OPTION( MPOM_Normal );
ENUM_OPTION( MPOM_MovementDirection );
END_ENUM_RTTI();




class CParticleDrawerMesh : public IParticleDrawer
{
	DECLARE_ENGINE_CLASS(CParticleDrawerMesh, IParticleDrawer, 0);

protected:
	typedef TDynArray< THandle< CMesh > >	TMeshList

	TMeshList						m_meshes;				//<! Mesh resources
	EMeshParticleOrientationMode	m_orientationMode;	//<! Orientation mode for meshes
	Uint8							m_lightChannels;			//!< Light channels

	CParticleDrawerMesh() 
		: m_lightChannels( LC_Default )
	{
	}

	// Perform special setup
	virtual void InitParticles( IParticleData *data, Uint32 firstIndex, Uint32 count ) const
	{

	}

	// Enable drawer to reconfigure particles when emission is stopped
	virtual void OnEmissionBreak( IParticleData *data ) const
	{

	}

	// Generate particle render data	
	virtual IRenderParticleData* GenerateRenderData( const IParticleData* data, const IParticleData::RenderingContext& renderingContext ) const
	{
		if ( m_meshes.Empty() )
		{
			return NULL;
		}
		// Get particle data
		CParticleData< MeshParticle >* meshParticleData = ( CParticleData< MeshParticle >* ) data;
		const CParticleBuffer< MeshParticle >& buffer = meshParticleData->GetBuffer();
		const Uint32 count = buffer.GetNumParticles();
		if ( count )
		{
			// Create render data buffer
			IRenderParticleData* renderData = GRender->CreateParticleData( PDT_Mesh, buffer.GetNumParticles() );
			if ( renderData )
			{
				renderData->SetAdditionalDataInfo( RPDI_LightChannel, (Uint32)m_lightChannels );

				// Get the data
				RenderMeshParticle* outBuffer = ( RenderMeshParticle* ) renderData->GetData();
				switch( m_orientationMode )
				{
				case MPOM_Normal:
					GenerateFreelyOrientedParticles( buffer, outBuffer, renderingContext );
					break;
				case MPOM_MovementDirection:
					GenerateMovementOrientedParticles( buffer, outBuffer, renderingContext );
					break;
				default:
					ASSERT(0);
				}

				// Generate vertices, this can be done here or on the GPU to improve CPU usage, for now this is done here
				renderData->GenerateVertexData();
				return renderData;
			}
		}
		// Data not created
		return NULL;
	}

private:
	void GenerateMovementOrientedParticles( const CParticleBuffer< MeshParticle >& buffer, RenderMeshParticle* renderParticles, const IParticleData::RenderingContext& renderingContext ) const
	{
		// Fill particles
		const Matrix& componentTransform = renderingContext.m_component->GetLocalToWorld();
		RenderMeshParticle* writePtr = renderParticles;
		Uint32 count = buffer.GetNumParticles();
		Uint32 numMeshes = m_meshes.Size();
		for ( Uint32 i=0; i<count; i++, writePtr++ )
		{
			const MeshParticle* particle = buffer.GetParticleAt( i );

			// Set mesh for particle
			Uint32 meshIndex = min( (Uint32)particle->m_frame, numMeshes - 1 );
			CMesh* mesh = m_meshes[ meshIndex ];
			if ( !mesh )
			{
				writePtr->m_renderMesh = NULL;
				continue;
			}
			CRenderMesh* renderMesh = (CRenderMesh*)mesh->GetRenderResource();
			writePtr->m_renderMesh = renderMesh;
			Vector3 position = particle->m_position;
			// If simulation locality enabled, move particle according to particle component movement
			if ( renderingContext.m_isLocalSpaceSimulation )
			{
				position = componentTransform.TransformPoint( position );
			}

			Matrix& localToWorld = writePtr->m_localToWorld;

			writePtr->m_color.Set4( particle->m_color.X, particle->m_color.Y, particle->m_color.Z, particle->m_alpha );

			Float yaw = particle->m_rotation.Z * 2.0f * M_PI;
			Float pitch = particle->m_rotation.Y * 2.0f * M_PI;
			Float roll = particle->m_rotation.X * 2.0f * M_PI;

			Vector forward = particle->m_velocity.Normalized();
			Vector side = Vector::Cross( forward, Vector::EZ ).Normalized3();

			// Adjust pitch
			RotateVector( forward, side, pitch );

			// Get up
			Vector up = Vector::Cross( side, forward );

			// Adjust yaw
			RotateVector( forward, up, yaw );

			// Get side
			side = Vector::Cross( forward, up );

			// Adjust roll
			RotateVector( side, forward, roll );

			// Get up again
			up = Vector::Cross( side, forward );

			localToWorld.SetRows( side, forward, up, Vector::EW );

			localToWorld.SetTranslation( position );
			localToWorld.SetScale33( particle->m_size );
		}
	}

	void GenerateFreelyOrientedParticles( const CParticleBuffer< MeshParticle >& buffer, RenderMeshParticle* renderParticles, const IParticleData::RenderingContext& renderingContext ) const
	{
		// Fill particles
		const Vector& componentPosition = renderingContext.m_component->GetLocalToWorld().GetTranslation();
		RenderMeshParticle* writePtr = renderParticles;
		EulerAngles baseAngles = renderingContext.m_component->GetWorldRotation();
		EulerAngles angles;
		Uint32 count = buffer.GetNumParticles();
		Uint32 numMeshes = m_meshes.Size();
		for ( Uint32 i=0; i<count; i++, writePtr++ )
		{
			const MeshParticle* particle = buffer.GetParticleAt( i );

			// Set mesh for particle
			Uint32 meshIndex = min( (Uint32)particle->m_frame, numMeshes - 1 );
			CMesh* mesh = m_meshes[ meshIndex ];
			if ( !mesh )
			{
				writePtr->m_renderMesh = NULL;
				continue;
			}
			CRenderMesh* renderMesh = (CRenderMesh*)mesh->GetRenderResource();
			writePtr->m_renderMesh = renderMesh;

			Vector position = particle->m_position;
			// If simulation locality enabled, move particle according to particle component movement
			if ( renderingContext.m_isLocalSpaceSimulation )
			{
				position += componentPosition;
			}

			const Float particleNormalizedLife = particle->m_life * particle->m_lifeSpanInv;

			writePtr->m_color.Set4( particle->m_color.X, particle->m_color.Y, particle->m_color.Z, particle->m_alpha );

			Float roll = particle->m_rotation.X * 360.0f;
			Float yaw = particle->m_rotation.Z * 360.0f;
			Float pitch = particle->m_rotation.Y * 360.0f;

			angles = baseAngles;
			angles.Roll += roll;
			angles.Pitch += pitch;
			angles.Yaw += yaw;

			writePtr->m_localToWorld = angles.ToMatrix();
			writePtr->m_localToWorld.SetTranslation( position );
			writePtr->m_localToWorld.SetScale33( particle->m_size );
		}
	}

	// Get particle type suitable for this drawer
	virtual EParticleType GetParticleType() const { return PT_Mesh; }

public:
	virtual const TMeshList* GetMeshes() const { return &m_meshes; }
};

BEGIN_CLASS_RTTI( CParticleDrawerMesh );
PARENT_CLASS( IParticleDrawer );
//PROPERTY_EDIT( m_mesh, TXT("TEMP") );
PROPERTY_EDIT( m_meshes, TXT( "Meshes" ) );
PROPERTY_EDIT( m_orientationMode, TXT( "The way for setting mesh orientation during movement" ) );
PROPERTY_BITFIELD_EDIT( m_lightChannels, ELightChannel, TXT("Light channels") );
END_CLASS_RTTI();


#endif