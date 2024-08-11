
#include "build.h"
#include "questGraphSocket.h"
#include "questStaticCameraSequence.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestStaticCameraSequenceBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestStaticCameraSequenceBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );
}

#endif

void CQuestStaticCameraSequenceBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_startTime;
	compiler << i_listener;
	compiler << i_duration;
	compiler << i_cameraIterator;
	compiler << i_cameras;
}

void CQuestStaticCameraSequenceBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_startTime ] = 0.f;
	instanceData[ i_listener ] = 0;
	instanceData[ i_duration ] = 0.f;
	instanceData[ i_cameraIterator ] = 0;

	ASSERT( instanceData[ i_cameras ].Size() == 0 );
}

void CQuestStaticCameraSequenceBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	ASSERT( GetListener( data ) );
	ASSERT( GetCurrCamera( data ) );
 
	// Check timeout
	Float startTime = data[ i_startTime ];
	Float duration = data[ i_duration ];

	if ( ( Float )GGame->GetEngineTime() - startTime > duration + m_maxWaitTimePerCamera )
	{
		Int32 index = data[ i_cameraIterator ] - 1;

		SCAM_ERROR( TXT("CQuestStaticCameraSequenceBlock: Timeout for static camera with tag '%ls' - index '%d'"), m_cameras[ index ].AsString().AsChar(), index );
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestStaticCameraSequenceBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if ( CollectCameras( data ) )
	{
		CreateListener( data );

		Bool ret = PlayNextCamera( data );
		ASSERT( ret );
	}
	else
	{
		ThrowErrorNonBlocking( data, CNAME( Out ), TXT("Couldn't collect all static cameras. See log.") );
	}
}

void CQuestStaticCameraSequenceBlock::OnDeactivate( InstanceBuffer& data ) const
{
	DeleteListener( data );

	TBaseClass::OnDeactivate( data );
}

CStaticCamera* CQuestStaticCameraSequenceBlock::FindCamera( const CName& tag ) const
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world && world->GetTagManager() )
	{
		return Cast< CStaticCamera >( world->GetTagManager()->GetTaggedEntity( tag ) );
	}

	return NULL;
}

Bool CQuestStaticCameraSequenceBlock::CollectCameras( InstanceBuffer& data ) const
{
	if ( m_cameras.Size() == 0 )
	{
		return false;
	}

	TCameraArray& cameras = data[ i_cameras ];
	
	ASSERT( cameras.Size() == 0 );

	for ( Uint32 i=0; i<m_cameras.Size(); ++i )
	{
		const CName& tag = m_cameras[ i ];

		if ( tag == CName::NONE )
		{
			SCAM_ERROR( TXT("CQuestStaticCameraSequenceBlock: Camera tag '%d' is empty"), i );
			return false;
		}

		CStaticCamera* cam = FindCamera( tag );
		if ( cam )
		{
			if ( !cam->IsRunning() )
			{
				cameras.PushBack( cam );
			}
			else
			{
				SCAM_ERROR( TXT("CQuestStaticCameraSequenceBlock: Camera with tag '%ls' is already running"), tag.AsString().AsChar() );
				return false;
			}
		}
		else
		{
			SCAM_ERROR( TXT("CQuestStaticCameraSequenceBlock: Couldn't find camera with tag '%ls'"), tag.AsString().AsChar() );
			return false;
		}
	}

	return cameras.Size() != 0;
}

Bool CQuestStaticCameraSequenceBlock::PlayNextCamera( InstanceBuffer& data ) const
{
	TCameraArray& cameras = data[ i_cameras ];
	Int32& it = data[ i_cameraIterator ];

	ASSERT( (Int32)cameras.Size() > it );

	// Get next camera
	CStaticCamera* nextCamera = cameras[ it ].Get();
	ASSERT( nextCamera );

	// Cached start time for timeout
	data[ i_startTime ] = ( Float )GGame->GetEngineTime();

	// Cached duration
	data[ i_duration ] = nextCamera->GetDuration();
	
	// Run camera
	Bool ret = nextCamera->Run( GetListener( data ) );

	// Check prev camera
	if ( it > 0 )
	{
		CStaticCamera* currCamera = GetCurrCamera( data );
		ASSERT( currCamera && !currCamera->IsRunning() );
	}

	// Inc iterator
	++it;

	return ret;
}

Bool CQuestStaticCameraSequenceBlock::IsSequenceFinished( InstanceBuffer& data ) const
{
	TCameraArray& cameras = data[ i_cameras ];
	Int32 index = data[ i_cameraIterator ];

	return index == (Int32)cameras.Size();
}

void CQuestStaticCameraSequenceBlock::OnStaticCameraStopped( InstanceBuffer& data, const CStaticCamera* camera ) const
{
	ASSERT( CheckCameraListener( data, camera ) );

	if ( IsSequenceFinished( data ) )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
	else
	{
		Bool ret = PlayNextCamera( data );
		ASSERT( ret );
	}
}

CStaticCamera* CQuestStaticCameraSequenceBlock::GetCurrCamera( InstanceBuffer& data ) const
{
	TCameraArray& cameras = data[ i_cameras ];
	Int32 index = data[ i_cameraIterator ];

	ASSERT( index - 1 < (Int32)cameras.Size() && index - 1 >= 0 );

	return cameras[ index - 1 ].Get();
}

Bool CQuestStaticCameraSequenceBlock::CheckCameraListener( InstanceBuffer& data, const CStaticCamera* camera ) const
{
	return GetCurrCamera( data ) == camera;
}

CQuestStaticCameraBlockListener* CQuestStaticCameraSequenceBlock::GetListener( InstanceBuffer& data ) const
{
	return reinterpret_cast< CQuestStaticCameraBlockListener* >( data[ i_listener ] );
}

void CQuestStaticCameraSequenceBlock::CreateListener( InstanceBuffer& data ) const
{
	CQuestStaticCameraBlockListener* listener = new CQuestStaticCameraBlockListener( this, data );
	data[ i_listener ] = reinterpret_cast< TGenericPtr >( listener );
}

void CQuestStaticCameraSequenceBlock::DeleteListener( InstanceBuffer& data ) const
{
	CQuestStaticCameraBlockListener* listener = GetListener( data );
	if ( listener )
	{
		delete listener;
		data[ i_listener ] = 0;
	}
}
