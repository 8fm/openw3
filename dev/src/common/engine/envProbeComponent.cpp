/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "envProbeComponent.h"
#include "renderCommands.h"
#include "renderFrame.h"
#include "renderResource.h"
#include "world.h"

IMPLEMENT_ENGINE_CLASS( CEnvProbeComponent );

static Uint32 GenerateEnvProbeDebugId()
{
	static Uint32 generator = 10;
	return generator++;
}

// *********************************

CEnvProbeComponent::CEnvProbeComponent()
	: m_debugId ( GenerateEnvProbeDebugId() )
	, m_nestingLevel ( 0 )
	, m_renderResource ( NULL )
	, m_effectIntensity ( 1 )
	, m_contribution ( 1 )
	, m_areaMarginFactor ( 0.1f, 0.1f, 0.1f, 1 )
	, m_areaDisplace ( 0, 0, 0, 1 )
	, m_isParallaxCorrected ( false )
	, m_genOrigin ( Vector::ZEROS )
{
	m_facesData.SetMemoryClass( MC_EnvProbeData );
}

CEnvProbeComponent::~CEnvProbeComponent()
{
	RED_FATAL_ASSERT( !m_renderResource, "Render resource for env probe not released. OnDetached not called?" );
	RED_FATAL_ASSERT( m_dataSource.Get() == nullptr, "Data source for env probe not released. OnDetached not called?" );
	ReleaseRenderResource(); //< release just in case
}

void CEnvProbeComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Base fragments
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Draw only if visible
	if ( flag == SHOW_EnvProbesInstances )
	{
		const Float displayRadius = 1.f;
		const Float gameTime = frame ? frame->GetFrameInfo().m_gameTime : 0.f;

#ifndef NO_COMPONENT_GRAPH
		frame->AddEnvProbe( gameTime, m_renderResource, GetProbeOrigin(), displayRadius, GetHitProxyID() );
#else
		frame->AddEnvProbe( gameTime, m_renderResource, GetProbeOrigin(), displayRadius, CHitProxyID() );
#endif

		const SEnvProbeParams probeParams = BuildProbeParams();
		const Matrix &localToWorld = probeParams.m_areaLocalToWorld;
		const Vector &margin = probeParams.m_areaMarginFactor;
		frame->AddDebugBox( Box ( Vector::ZEROS, 1.f ), Matrix(localToWorld).SetScale33( Vector::Clamp4(Vector::ONES - margin * margin.W, 0.f, 1.f ) ), Color::BLUE );
		frame->AddDebugBox( Box ( Vector::ZEROS, 1.f ), localToWorld, Color::CYAN );
		frame->AddDebugBox( Box ( Vector::ZEROS, 1.f ), BuildParallaxLocalToWorld(), Color::YELLOW );

		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_EnvProbeStats ) )
		{
			const Color textColor = Color::WHITE;
			const Color textBgColor = Color::BLACK;
			frame->AddDebugText( probeParams.m_probeOrigin + Vector ( 0, 0, 0.9f ), String::Printf( TXT("DebugId %u"), (Uint32)probeParams.m_debugId ), true, textColor, textBgColor );
		}
	}
}

Vector CEnvProbeComponent::GetProbeOrigin() const
{
	return GetLocalToWorld().GetTranslation();
}

void CEnvProbeComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_EnvProbesInstances );

	CreateRenderResource( false );
}

void CEnvProbeComponent::OnDetached( CWorld* world )
{
	ReleaseRenderResource();

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_EnvProbesInstances );

	TBaseClass::OnDetached( world );
}

void CEnvProbeComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( !file.IsCooker() && !IsCooked() && !file.IsGarbageCollector() && !file.IsMapper() )
	{
		// TODO : Proper file version conversion. For now this keeps the data on disk the same, without having to
		// bump file version.
		SerializeDeferredDataBufferAsLatentDataBufferData( file, m_facesData );
	}

	// ace_todo: 
	// Remove conditional after version resave
	// VER_CURRENT is now VER_CLASS_PROPERTIES_DATA_CLEANUP

	if ( file.IsReader() )
	{
		Uint32 version = 0;
		file << version;

		// Conditional load
		if ( 0 == version )
		{
			m_genOrigin = TBaseClass::GetLocalToWorld().GetTranslation(); //< already serialized (on the very beginning of this func)
			ASSERT( m_genOrigin == GetProbeOrigin() );
		}
		else if ( 1 == version )
		{
			file << m_genOrigin;
		}
		else
		{
			ASSERT( !"Invalid version" );
		}
	}
	else
	{
		Uint32 version = 1;
		file << version;

		// Write
		file << m_genOrigin;
	}
}

void CEnvProbeComponent::CommitProbeParamsChangedCommand()
{
	if ( m_renderResource )
	{
		( new CRenderCommand_EnvProbeParamsChanged( m_renderResource, BuildProbeParams() ) )->Commit();
	}
}

void CEnvProbeComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CEnvProbeComponent );

	// Pass to base class
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	// Update params
	CommitProbeParamsChangedCommand();
}

void CEnvProbeComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Update params
	if ( property->GetName() == TXT("contribution") ||
		 property->GetName() == TXT("effectIntensity") ||
		 property->GetName() == TXT("areaMarginFactor") ||
		 property->GetName() == TXT("areaDisplace") || 
		 property->GetName() == TXT("isParallaxCorrected") ||
		 property->GetName() == TXT("parallaxTransform") ||
		 property->GetName() == TXT("nestingLevel") ||
		 property->GetName() == TXT("genParams") )
	{
		CommitProbeParamsChangedCommand();
	}

	// Update face params
	if ( property->GetName() == TXT("faceParamsNegX") ||
		 property->GetName() == TXT("faceParamsPosX") ||
		 property->GetName() == TXT("faceParamsNegY") ||
		 property->GetName() == TXT("faceParamsPosY") ||
		 property->GetName() == TXT("faceParamsNegZ") ||
		 property->GetName() == TXT("faceParamsPosZ") )
	{
		ReleaseRenderResource();
		VERIFY( SetFacesBuffers( Vector::ZEROS, 0, NULL ) );
	
		CommitProbeParamsChangedCommand();
	}
}

Bool CEnvProbeComponent::SetFacesBuffers( const Vector &genOrigin, Uint32 dataSize, const void *dataBuffer )
{
	// Save genOrigin
	m_genOrigin = genOrigin;

	// Copy data to local buffer
	m_facesData.ReallocateBuffer( dataSize );
	BufferHandle writeHandle = m_facesData.AcquireBufferHandleForWritingSync();
	Red::System::MemoryCopy( writeHandle->GetData(), dataBuffer, dataSize );

	// Mark as modified
	MarkModified();

	//
	return true;
}

void CEnvProbeComponent::CreateRenderResource( Bool needsRecreate )
{
	RED_FATAL_ASSERT( !((nullptr != m_renderResource) && (nullptr == m_dataSource.Get())), "Internal integrity failed" );
	if ( NULL != m_renderResource && !needsRecreate )
	{
		return;
	}

	ReleaseRenderResource();
	
	m_dataSource = GRender->CreateEnvProbeDataSource( *this );
	m_renderResource = GRender->UploadEnvProbe( this );
}

void CEnvProbeComponent::ReleaseRenderResource()
{
	if ( NULL != m_renderResource )
	{
		m_renderResource->Release();
		m_renderResource = NULL;
	}

	if ( m_dataSource )
	{
		IEnvProbeDataSource::tScopedLock lock ( IEnvProbeDataSource::GetCommunicationLock() );

		m_dataSource->Invalidate();
		m_dataSource = NULL; //< auto ref counted
	}
}

Bool CEnvProbeComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

SEnvProbeParams CEnvProbeComponent::BuildProbeParams() const
{	
	return SEnvProbeParams ( m_debugId, m_nestingLevel, Max( 0.f, m_effectIntensity ), m_genOrigin, GetLocalToWorld().GetTranslation(), BuildAreaLocalToWorld(), BuildParallaxLocalToWorld(), m_contribution, m_areaMarginFactor, m_genParams );
}

Matrix CEnvProbeComponent::BuildAreaLocalToWorld() const
{
	Matrix areaLocalToWorld;
	if ( m_genParams.m_isInteriorFallback )
	{
		areaLocalToWorld.SetIdentity();
		areaLocalToWorld.SetScale33( 10 * 1000 );
	}
	else
	{
		areaLocalToWorld = GetLocalToWorld();
		areaLocalToWorld.SetTranslation( 
			areaLocalToWorld.GetTranslation() + 
			areaLocalToWorld.GetAxisX().Normalized3() * m_areaDisplace.X +
			areaLocalToWorld.GetAxisY().Normalized3() * m_areaDisplace.Y + 
			areaLocalToWorld.GetAxisZ().Normalized3() * m_areaDisplace.Z );
	}
	return areaLocalToWorld;
}

Matrix CEnvProbeComponent::BuildParallaxLocalToWorld() const
{
	Matrix parallaxMatrix;
	if ( m_isParallaxCorrected )
	{
		parallaxMatrix = BuildAreaLocalToWorld();
		parallaxMatrix.SetTranslation(
			parallaxMatrix.GetTranslation() +
			parallaxMatrix.GetAxisX() * m_parallaxTransform.GetPosition().X +
			parallaxMatrix.GetAxisY() * m_parallaxTransform.GetPosition().Y +
			parallaxMatrix.GetAxisZ() * m_parallaxTransform.GetPosition().Z );

		parallaxMatrix.SetScale33( m_parallaxTransform.GetScale() );

		Vector trans = parallaxMatrix.GetTranslation();
		Matrix rot = m_parallaxTransform.GetRotation().ToMatrix();
		parallaxMatrix.SetTranslation( 0.f, 0.f, 0.f );
		parallaxMatrix = Matrix::Mul( rot, parallaxMatrix );
		parallaxMatrix.SetTranslation( trans );
	}
	else
	{
		parallaxMatrix = GetLocalToWorld();
		parallaxMatrix.SetScale33( 9999.f );
	}

	return parallaxMatrix;
}

