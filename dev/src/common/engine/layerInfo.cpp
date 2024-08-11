/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "layerInfo.h"
#include "envProbeComponent.h"
#include "world.h"
#include "game.h"
#include "meshComponent.h"
#include "lightComponent.h"
#include "spotLightComponent.h"
#include "pointLightComponent.h"
#include "staticMeshComponent.h"
#include "decalComponent.h"
#include "pathComponent.h"
#include "soundAmbientAreaComponent.h"
#include "pathlibNavmeshComponent.h"
#include "triggerAreaComponent.h"
#include "stickerComponent.h"
#include "layerGroup.h"
#include "selectionManager.h"

#include "../core/jobGenericJobs.h"
#include "../core/loadingJobManager.h"
#include "../core/depot.h"
#include "../core/dependencySaver.h"
#include "../core/gameSave.h"
#include "../core/feedback.h"
#include "baseEngine.h"
#include "component.h"
#include "entity.h"
#include "layersEntityCompatibilityChecker.h"

#define LAYER_INFO_MAGIC		'LYRI'

IMPLEMENT_ENGINE_CLASS( CLayerInfo );
IMPLEMENT_RTTI_ENUM( ELayerType );
IMPLEMENT_RTTI_ENUM( ELayerBuildTag );
IMPLEMENT_RTTI_ENUM( ELayerMergedContent );

RED_DEFINE_STATIC_NAME( CWayPointComponent );
RED_DEFINE_STATIC_NAME( LayerInfoBuildTagChanged );
RED_DEFINE_STATIC_NAME( layerBuildTag );

// Shared layer build tag colors
#ifndef NO_EDITOR
namespace LayerBuildTagColors
{
	Color None				( 0xFA, 0x58, 0x58, 0xFF );
	Color Ignored			( 0xB4, 0x5F, 0x41, 0xFF );
	Color EnvOutdoor		( 0x81, 0xDA, 0xF5, 0xFF );
	Color EnvIndoor			( 0xCE, 0xED, 0xC9, 0xFF );
	Color Quest				( 0xF4, 0xFA, 0x58, 0xFF );
	Color Communities		( 0x86, 0xB4, 0x04, 0xFF );
	Color EnvUnderground	( 0xFF, 0xBF, 0x08, 0xFF );
	Color Audio				( 0xD4, 0xD5, 0xEE, 0xFF );
	Color Nav				( 0x00, 0x22, 0x00, 0xFF );
	Color Dlc				( 0xF8, 0xF0, 0xF8, 0xFF );
	const Color& GetColorFor( ELayerBuildTag tag )
	{
		switch ( tag )
		{
		case LBT_Ignored		: return Ignored;
		case LBT_EnvOutdoor		: return EnvOutdoor;
		case LBT_EnvIndoor		: return EnvIndoor;
		case LBT_Quest			: return Quest;
		case LBT_Communities	: return Communities;
		case LBT_EnvUnderground	: return EnvUnderground;
		case LBT_Audio			: return Audio;
		case LBT_Nav			: return Nav;
		case LBT_DLC			: return Dlc;
		default					: return None;
		}
	}
};
#endif

static inline Bool IsBasedOn( CObject* obj, CName className )
{
	return obj->IsA( SRTTI::GetInstance().FindClass( className ) );
}

LayerLoadingContext::LayerLoadingContext()
	: m_stats( nullptr )
	, m_queueEvent( true )
	, m_loadHidden( false )
{};

CLayerInfo::CLayerInfo()
    : m_world( nullptr )
    , m_layer( nullptr )
	, m_layerGroup( nullptr )
    , m_editorCopy( nullptr )
    , m_loadingToken( nullptr )
	, m_layerType( LT_AutoStatic )
    , m_isVisible( true )
	, m_requestUnload( false )
	, m_layerBuildTag( LBT_None )
	, m_layerMergeContentMode( LMC_Auto )
{
	// Register as listener of some editor related events
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( EnginePerformancePlatformChanged ), this );
#endif
}

CLayerInfo::CLayerInfo( CWorld* world, CLayerGroup* layerGroup, const String& depotFilePath )
	: m_world( nullptr )
    , m_layer( nullptr )
	, m_layerGroup( nullptr )
    , m_editorCopy( nullptr )
    , m_loadingToken( nullptr )
	, m_layerType( LT_AutoStatic )
	, m_layerMergeContentMode( LMC_Auto )
    , m_isVisible( true )
	, m_requestUnload( false )
#ifndef NO_DEBUG_PAGES
	, m_debugErrorFlag( false )
#endif
{
	// Initialize layer settings
	Init( world, layerGroup, depotFilePath );

	// Register as listener of some editor related events
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( EnginePerformancePlatformChanged ), this );
#endif
}

CLayerInfo::~CLayerInfo()
{
	// Make sure we're not on the world update list
	if ( GetWorld() )
	{
		GetWorld()->RemoveFromUpdateList( this );
	}
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( this );
#endif
}

void CLayerInfo::GetHierarchyPath( String &path, Bool omitRoot /* = false */ ) const
{
    path = GetShortName();
    CLayerGroup *layerGroup = GetLayerGroup();
    while ( layerGroup )
    {
		// omit the last group (with "World" name)
		if ( omitRoot && layerGroup->GetParentGroup() == NULL ) break;

        path = layerGroup->GetName() + TXT("\\") + path;
        layerGroup = layerGroup->GetParentGroup();
    }
}

static void ExtractShortLayerName( const String& depotFilePath, String& shortName )
{
	// Filename starts after last '\\' character
	size_t fileStartPos;
	Bool startPosFound = depotFilePath.FindCharacter( TXT('\\'), fileStartPos, true );
	++fileStartPos;

	// Extract short layer name
	size_t fileDotPos;
	depotFilePath.FindCharacter( TXT('.'), fileDotPos, true );

	if ( !startPosFound ) 
	{
		fileDotPos = depotFilePath.GetLength();
	}
	shortName = depotFilePath.MidString( fileStartPos, fileDotPos - fileStartPos );
}

void CLayerInfo::Init( CWorld* world, CLayerGroup* parentLayerGroup, const String& depotFilePath )
{
	ASSERT( m_world.Get() == nullptr );
	ASSERT( m_layerGroup == nullptr );
	ASSERT( world != nullptr );
	ASSERT( parentLayerGroup != nullptr );

	// Setup world pointer
	m_world = world;
	m_layerGroup = parentLayerGroup;

	// Extract short name
	ExtractShortLayerName( depotFilePath, m_shortName );

	// Convert to local path if necessary
	if ( !GDepot->ConvertToLocalPath( depotFilePath, m_depotFilePath ) )
	{
		m_depotFilePath = depotFilePath;
	}
}

void CLayerInfo::InitNoWorld( const String &depotFilePath )
{
	CFilePath path( depotFilePath );
	m_shortName = path.GetFileName();

	// Convert to local path if necessary
	if ( GDepot->ConvertToLocalPath( depotFilePath, m_depotFilePath ) )
	{
		LOG_ENGINE( TXT("'%ls' absolute not local file path"), depotFilePath.AsChar() );
	}
	else
	{
		m_depotFilePath = depotFilePath;
	}
}

void CLayerInfo::PullLayerGroupsAndLayerInfo()
{
	m_hasEmbeddedLayerInfo = true;
}

void CLayerInfo::SetLayerBuildTag( const ELayerBuildTag& val )
{
	MarkModified();
	m_layerBuildTag = val;
}

void CLayerInfo::SetLayerType( const ELayerType& type )
{
	MarkModified();
	m_layerType = type;
}

void CLayerInfo::SetStreamingLayer( Bool val )
{
	m_streamingLayer = val;
}

Bool CLayerInfo::IsStreamingLayer() const
{
	return m_streamingLayer;
}

void CLayerInfo::UpdatePath()
{
	if ( m_layerGroup )
	{
		CFilePath oldPath( m_depotFilePath );
		m_depotFilePath = m_layerGroup->GetDepotPath() + oldPath.GetFileNameWithExt();
	}
}

void CLayerInfo::OnPreSave()
{
	ISerializable::OnPreSave();

	// Restore GUID
	if ( GetGUID().IsZero() )
	{
		WARN_ENGINE( TXT("Layer '%ls' had invalid GUID. Restoring."), GetDepotPath().AsChar() );
		m_guid = CGUID::Create();
	}
}

void CLayerInfo::OnSerialize( IFile& file )
{
	ISerializable::OnSerialize( file );

	// Restore GUID
	if ( file.IsReader() && GetGUID().IsZero() )
	{
		WARN_ENGINE( TXT("Loaded layer '%ls' with invalid GUID. Restoring."), GetDepotPath().AsChar() );
		m_guid = CGUID::Create();
	}

	// Store cooked data
	if ( m_hasEmbeddedLayerInfo && !file.IsGarbageCollector() )
	{
		file << m_layerGroup;
	}

	// GC link, not serialized directly to prevent from static loading
	if ( file.IsGarbageCollector() )
	{
		file << m_world;
		file << m_layer;
		file << m_layerGroup;
		file << m_layerToDestroy;
	}
}

#ifndef NO_EDITOR_WORLD_SUPPORT

Bool CLayerInfo::Create()
{
	// Create layer if not already created
	if ( !m_layer )
	{
		// World is not loaded from file, not able to determine loading location
		ASSERT( GetWorld()->GetFile() );

		// Find or create directory for layer then save the file
		CDirectory* directory = GDepot->CreatePath( m_depotFilePath );
		if ( !directory )
		{
			return false;
		}

		// Bind layer info to layer
		m_layer = CreateObject<CLayer>();
		m_layer->SetLayerInfo( this );
		m_guid = CGUID::Create();

		// Layer info cannot be saved in no_resource_import builds, even if it is created
#ifndef NO_RESOURCE_IMPORT
		// Save file
		if ( !m_layer->SaveAs( directory, m_shortName ) )
		{
			return false;
		}
#endif

		// Attach
		ConditionalAttachToWorld();
	}
	else
	{
		WARN_ENGINE(TXT("LayerInfo alread holds a layer; skipping creating a new one;"));
	}

	return true;
}

void CLayerInfo::ForceVisible( const Bool forceVisible )
{
	// this is a hack and should be logged
	if ( m_forceVisible != forceVisible )
	{
		/*LOG_ENGINE( TXT("Force visible flag for layer '%ls' set to %ls"), 
			GetDepotPath().AsChar(), forceVisible ? TXT("true") : TXT("false") );*/

		m_forceVisible = forceVisible;
	}
}

void CLayerInfo::CreateBasic()
{
	m_layer = CreateObject< CLayer >();
	m_layer->SetLayerInfo( this );
	m_guid = CGUID::Create();
}

Bool CLayerInfo::Create( CLayer *layer )
{
	// Create layer if not already created
	if ( !m_layer )
	{
		// World is not loaded from file, not able to determine loading location
		ASSERT( GetWorld()->GetFile() );

		// Find or create directory for layer then save the file
		CDirectory* directory = GDepot->CreatePath( m_depotFilePath );
		if ( !directory )
		{
			return false;
		}

		// Bind layer info to layer
		m_layer = layer;
		m_layer->SetLayerInfo( this );

		// Append layer info
		AppendLayerInfoObject();

		// Attach
		ConditionalAttachToWorld();
	}
	else
	{
		WARN_ENGINE(TXT("LayerInfo alread holds a layer; skipping creating a new one;"));
	}

	return true;
}

#endif

Bool CLayerInfo::RequestLoad( class CGenericCountedFence* loadingFence /*= nullptr*/ )
{
	// if the layer is not visible, it should not be loaded
	if ( GetMergedVisiblityFlag() == false )
	{
		return false;
	}

	// Reset unload flag
	m_requestUnload = false;
	m_layerToDestroy = NULL;	// TODO: can leak

	// Already loaded
	if ( m_layer )
	{
		return true;
	}

	// Loading right now
	if ( !m_loadingToken )
	{
		m_loadingToken = new CJobLoadResource( m_depotFilePath );
		if ( !m_loadingToken->GetResource() )
		{
			SJobManager::GetInstance().Issue( m_loadingToken );
			GetWorld()->AddToUpdateList( this );
		}
	}

	CResource * resource =  m_loadingToken->GetResource();
	// Already loaded
	if ( resource )
	{
		// Bind layer info to layer
		m_layer = SafeCast< CLayer >( resource );			
		m_layer->SetLayerInfo( this );

		// Attach to world after loading
		ConditionalAttachToWorld();

		// Release async loading token
		m_loadingToken->Release();
		m_loadingToken = NULL;
	}
	

	// Async loading started
	return true;
}

void CLayerInfo::FinishAsyncLoading()
{
	if ( m_loadingToken != nullptr )
	{
		LayerLoadingContext context;
		context.m_loadHidden = true;
		SyncLoad( context );
	}
}

void CLayerInfo::FinishLoadingJob()
{
	if ( m_loadingToken != nullptr )
	{
		// wait for the job to finish
		while ( !m_loadingToken->HasEnded() )
		{
			Red::Threads::YieldCurrentThread();
		}

		m_loadingToken->Release();
		m_loadingToken = nullptr;
	}
}

Bool GCrapDoNotAttach = false;

Bool CLayerInfo::SyncLoad( LayerLoadingContext& context )
{
	// if the layer is not visible, it should not be loaded
	if ( GetMergedVisiblityFlag() == false && !context.m_loadHidden )
	{
		return false;
	}

	// Reset unload flag
	m_requestUnload = false;

	// Already loaded
	if ( m_layer )
	{
		return true;
	}

	// We were async loading this crap, finish it now
	FinishLoadingJob();

	// Load resource
	ResourceLoadingContext loadingContext;
	loadingContext.m_stats = context.m_stats;
	m_layer = LoadResource< CLayer >( m_depotFilePath, loadingContext );
	if ( !m_layer )
	{
		WARN_ENGINE( TXT("Unable to sync load layer '%ls'"), m_shortName.AsChar() );
		return false;
	}

	// Loaded while pending destroy, reset
	if ( m_layerToDestroy )
	{
		ASSERT( m_layerToDestroy == m_layer.Get() );
		m_layerToDestroy = NULL;
	}

	// Bind layer to layer info
	m_layer->SetLayerInfo( this );

	// Attach to world after loading
	if ( !GCrapDoNotAttach )
	{
		ConditionalAttachToWorld();
	}

	// This will ensure that the streaming system is entirely refreshed, and the correct layers are loaded in the editor.
	m_world->RequestStreamingUpdate();
	
	// Layer loaded event
#ifndef NO_EDITOR_EVENT_SYSTEM
	if ( ! (GetLayerGroup() && GetLayerGroup()->IsSystemGroup() ) )
	{
		if ( context.m_queueEvent )
		{
			EDITOR_QUEUE_EVENT( CNAME( LayerLoaded ), CreateEventData( this ) );
		}
		else
		{
			EDITOR_DISPATCH_EVENT( CNAME( LayerLoaded ), CreateEventData( this ) );
		}
	}
#endif

	// Layer was loaded
	return true;
}

Bool CLayerInfo::SyncUnload()
{
	// Reset unload flag
	m_requestUnload = false;

	// Cancel any loading that is pending
	if ( m_loadingToken )
	{
		// Discard loading token, we will perform sync loading anyways
		m_loadingToken->Cancel();
		m_loadingToken->Release();
		m_loadingToken = NULL;

		// Wait for end of the loading
		GEngine->FlushJobs();
		SJobManager::GetInstance().FlushPendingJobs();
	}

	// Destroy layer
	if ( m_layer )
	{
		// Detach layer to remove all created proxies in rendering/physics
		if ( m_layer->IsAttached() )
		{
			// Detach layer
			m_layer->DetachFromWorld();
		}

		// Detach layer from layer info
		m_layer->SetLayerInfo( NULL );

		// Unlink layer pointer
		m_layerToDestroy = m_layer.Get();
		m_layer = NULL;
		GetWorld()->AddToUpdateList( this );
		
#ifndef NO_EDITOR_EVENT_SYSTEM
		if ( !( this->GetLayerGroup() && this->GetLayerGroup()->IsSystemGroup() ) )
		{
			EDITOR_QUEUE_EVENT( CNAME( LayerUnloaded ), CreateEventData( this ) );
		}
#endif
	}

	// Unloaded
	return true;
}

#ifndef NO_EDITOR
Bool CLayerInfo::SyncLoadUnattached()
{
	ResourceLoadingContext loadingContext;
	m_layer = LoadResource< CLayer >( m_depotFilePath, loadingContext );
	if ( !m_layer )
	{
		WARN_ENGINE( TXT("Unable to sync load layer '%ls'"), m_shortName.AsChar() );
		return false;
	}
	m_layer->SetLayerInfo( this );
	return true;
}

Bool CLayerInfo::SyncUnloadUnattached()
{
	if ( m_layer )
	{
		m_layer->SetLayerInfo( nullptr );

		m_layer->GetFile()->Unload();
		m_layer = nullptr;
	}

	return true;
}
#endif // NO_EDITOR

Bool CLayerInfo::UpdateLoadingState( Bool sync /*=false*/ )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("LAYERS"), TXT("Slow UpdateLoadingState") );

	// Unload requested
	if ( m_requestUnload )
	{
		// Cancel pending load
		if ( m_loadingToken )
		{
			m_loadingToken->Cancel();
			m_loadingToken->Release();
			m_loadingToken = NULL;
		}

		// If we have loaded layer detach it
		if ( m_layer )
		{
			// Detach layer to remove all created proxies in rendering/physics
			if ( m_layer->IsAttached() )
			{
				// Detach layer
				m_layer->DetachFromWorld();
			}

			// Detect problematic case
			if ( m_layer->HasFlag( OF_Root ) )
			{
				WARN_ENGINE( TXT("Early unload of layer '%ls'"), m_shortName.AsChar() );
			}

			// Detach layer from layer info
			m_layer->SetLayerInfo( NULL );

			// Unlink layer pointer
			m_layerToDestroy = m_layer.Get();
			m_layer = NULL;
		}

		// Emit event
		EDITOR_QUEUE_EVENT( CNAME( LayerUnloaded ), CreateEventData( this ) );

		// Unloaded
		m_requestUnload = false;
	}

	// Make sure we're not referencing to an already destroyed layer
	if ( m_layerToDestroy && ( m_layerToDestroy->HasFlag( OF_Discarded ) ||
							   m_layerToDestroy->HasFlag( OF_Finalized ) ) )
	{
		m_layerToDestroy = NULL;
	}

	// Destroy layer
	if ( m_layerToDestroy )
	{
		// No components can be attached
		Bool hasAttachedEntities = m_layerToDestroy->GetNumAttachedComponents() > 0;
		if ( !hasAttachedEntities )
		{
			// Notify layer entities
			m_layerToDestroy->NotifyUnloading();

			// Discard data on layer
			if ( m_layerToDestroy->GetFile() )
			{
				m_layerToDestroy->GetFile()->Unload();
			}
			else
			{
				m_layerToDestroy->Discard();
			}

			// Reset
			m_layerToDestroy = NULL;
		}
	}

	// Loading
	if ( m_loadingToken )
	{
		// Error or canceled task
		if ( m_loadingToken->HasEnded() && !m_loadingToken->HasFinishedWithoutErrors() )
		{
			WARN_ENGINE( TXT("Async loading of layer '%ls' failed"), m_shortName.AsChar() );
			m_loadingToken->Release();
			m_loadingToken = NULL;

			EDITOR_QUEUE_EVENT( CNAME( LayerLoadFailed ), CreateEventData( this ) );

			return false;
		}

		// This is soooooo hacky, but sync parameter is only used in Umbra tome generation
		if ( sync )
		{
			while ( !m_loadingToken->HasEnded() )
			{
				Red::Threads::SleepOnCurrentThread( 200 );
			}
		}

		// Valid ?
		if ( m_loadingToken->HasFinishedWithoutErrors() )
		{
			// Grab layer
			m_layer = Cast< CLayer >( m_loadingToken->GetResource() );
			if ( !m_layer )
			{
				WARN_ENGINE( TXT("Async loading of layer '%ls' loaded no data"), m_shortName.AsChar() );
				m_loadingToken->Release();
				m_loadingToken = NULL;

				EDITOR_QUEUE_EVENT( CNAME( LayerLoadFailed ), CreateEventData( this ) );

				return false;
			}

			// Bind layer to layer info
			m_layer->SetLayerInfo( this );

			// Release task
			m_loadingToken->Release();
			m_loadingToken = NULL;

			// Attach to world after loading
			ConditionalAttachToWorld();

			// Layer loaded event
#ifndef NO_EDITOR_EVENT_SYSTEM
			if ( ! (GetLayerGroup() && GetLayerGroup()->IsSystemGroup() ) )
			{
				EDITOR_QUEUE_EVENT( CNAME( LayerLoaded ), CreateEventData( this ) );
			}
#endif
		}
	}

	Bool removedElement = false;
	if ( !m_requestUnload && !m_layerToDestroy && !m_loadingToken )
	{
		// no need to update further
		GetWorld()->RemoveFromUpdateList( this ); //remove from updating
		removedElement = true;;
	}

	return removedElement;
}

Bool CLayerInfo::RequestUnload( Bool purgeLayerStorage /* = false */ )
{
	if ( purgeLayerStorage )
	{
		RED_LOG( Save, TXT("Purging storage for layer %ls"), GetDepotPath().AsChar() );
		m_storage.PurgeAndCloseStorage();
	}

	// Request layer unload
	if ( m_layer )
	{
		// Request unload
		if ( !m_requestUnload )
		{
			EDITOR_DISPATCH_EVENT( CNAME( LayerUnloading ), CreateEventData( this ) );

			m_requestUnload = true;
			GetWorld()->AddToUpdateList( this );
		}

		// Layer will be unloaded
		return true;
	}

	// Not loaded
	return false;
}

#ifndef NO_EDITOR_WORLD_SUPPORT

void CLayerInfo::GetLatest()
{
	CDiskFile *f = GDepot->FindFile( GetDepotPath() );
	if (f) f->Sync();

	String layerPath;
	GDepot->GetAbsolutePath( layerPath );
	layerPath += GetDepotPath();

	if ( !GFileManager->GetFileSize( layerPath ) )
	{
		if (f)
		{
			if (IsLoaded())
			{
				f->Rebind(NULL); // why is it here for ?

				SyncUnload();

				CLayerGroup* parentGroup = GetLayerGroup();
				parentGroup->RemoveLayer( this );
			}

			if (f->GetDirectory()) f->GetDirectory()->DeleteFile(*f);

			// Inform world
			m_world->SynchronizeLayersRemove(m_world->GetWorldLayers());
		}
		else
		{
			m_world->Reload(true);
		}
	}
	else
	{	
		if (IsLoaded())
		{
			m_layer->Reload(true);
		}
	}
}

void CLayerInfo::Refresh( Bool confirm /* = true */ )
{
	CDiskFile *file = GDepot->FindFile( GetDepotPath() );
	if ( file )
	{
		if ( IsLoaded() && !file->IsModified())
		{
			// hack required by selection manager - some of the objects from the world might be deleted 
			// and yet selection manager doesn't notice that
			GGame->GetActiveWorld()->GetSelectionManager()->DeselectOnLayer( m_layer.Get() );

			m_layer->Reload( confirm );
		}
	}
}

void CLayerInfo::Reload()
{
	CDiskFile *file = GDepot->FindFile( GetDepotPath() );
	if ( file )
	{
		if ( !IsLoaded() )
		{
			LayerLoadingContext loadingContext;
			SyncLoad( loadingContext );
		}
		
		// hack required by selection manager - some of the objects from the world might be deleted 
		// and yet selection manager doesn't notice that
		GGame->GetActiveWorld()->GetSelectionManager()->DeselectOnLayer( m_layer.Get() );

		m_layer->Reload(true);
	}
}

Bool CLayerInfo::Save()
{
	// Unable to save layer that is loading
	if ( m_loadingToken )
	{
		WARN_ENGINE( TXT("Unable to save layer '%ls' that is being loaded"), m_shortName.AsChar() );
		return false;
	}

	// Save only if there's something to save and we can
#ifndef NO_RESOURCE_IMPORT
	if ( m_layer )
	{
		if ( m_layer->IsModified() )
		{
			if ( !CLayersEntityCompatibilityChecker::CheckLayersEntitiesCompatibility( this, TDynArray< String >() ) )
			{
				return false;
			}
		}

		// We can use direct Save because layers are always mapped to their CDiskFiles when they are added
		return m_layer->Save();
	}
#endif

	// Not loaded, nothing to save
	return true;
}

Bool CLayerInfo::Remove( Bool confirm )
{
	// Find layer's disk file
	CDiskFile* file = GDepot->FindFile( m_depotFilePath );
	if ( file )
	{
		if ( file->Delete( confirm, false ) )
		{
			if (IsLoaded())
			{
				SyncUnload();
			}

			// Remove from group
			CLayerGroup* parentGroup = GetLayerGroup();
			parentGroup->RemoveLayer( this );

			// Layer must be unloaded
			ASSERT( !m_layer );

			// Let the others know
			EDITOR_QUEUE_EVENT( CNAME( LayerRemoved ), CreateEventData( this ) );
			return true;
		}
	}
	else
	{
		// Remove from group
		CLayerGroup* parentGroup = GetLayerGroup();
		parentGroup->RemoveLayer( this );
		return true;
	}

	// Unable to delete
	WARN_ENGINE( TXT("Unable to delete layer '%ls'"), m_depotFilePath.AsChar() );
	return false;
}

Bool CLayerInfo::Rename( const String& newName )
{
	// Cannot rename to-be streamed, unstreamed, loaded, unloaded, etc layer
	if ( m_requestUnload )
	{
		return false;
	}

	// Try to load the layer (we need that to stream the data in so that they are
	// saved later under the proper filenames)
	Bool wasLoaded = IsLoaded();
	if ( !m_layer )
	{
		LayerLoadingContext loadingContext;
		if ( !SyncLoad( loadingContext ) )
		{
			return false;
		}
	}
	if ( !m_layer )
	{
		return false;
	}

	// Make sure we can modify the layer
	if ( !m_layer->MarkModified() )
	{
		return false;
	}

	// Rename the file
	String depotRoot;
	GDepot->GetAbsolutePath( depotRoot );
	CFilePath currentPath( depotRoot + GetDepotPath() );
	if ( !m_layer->GetFile()->Rename( newName, currentPath.GetExtension() ) )
	{
		return false;
	}

	// Update names
	String previousName = m_shortName;
	m_shortName = newName;
	m_depotFilePath = m_layer->GetFile()->GetDepotPath();

#ifndef NO_RESOURCE_IMPORT
	// Save the layer
	m_layer->Save();
#endif

	// If the layer wasn't loaded, unload it
	if ( !wasLoaded )
	{
		RequestUnload();
	}

	return true;
}

#endif

void CLayerInfo::Show( Bool visible )
{
	if ( visible != m_isVisible )
	{
		// Change visibility flag
		m_isVisible = visible;

		// If layer list loaded change attachment state
		if ( IsLoaded() )
		{
			ConditionalAttachToWorld();	
		}

		// Let the others know
		EDITOR_QUEUE_EVENT( CNAME( LayerVisibilityChanged ), CreateEventData( this ) );
	}
}

//void CLayerInfo::CheckStaticLayer()
//{
//#ifndef NO_EDITOR
//	if ( GetLayerType() == LT_AutoStatic ) // check the layer's contents to set it to static
//	{
//		if ( m_layer )
//		{
//			// Assume static
//			m_isStatic = true;
//
//			// Terrain and path layers are always static
//			if ( m_layer->GetLayerInfo() )
//			{
//				// Paths...
//				if ( m_layer->GetLayerInfo()->GetShortName().EqualsNC( TXT("Paths") ) )
//				{
//					return;
//				}
//
//				// Lores tarrain...
//				if ( m_layer->GetLayerInfo()->GetShortName().EqualsNC( TXT("lores terrain") ) )
//				{
//					return;
//				}
//
//				// Highres terrain
//				if ( m_layer->GetLayerInfo()->GetShortName().BeginsWith( TXT("tile ") ) )
//				{
//					return;
//				}
//			}
//
//			// Check entities on the layer
//			const LayerEntitiesArray& entities = m_layer->GetEntities();
//			for ( Uint32 i=0; i<entities.Size(); i++ )
//			{
//				CheckStaticLayerEntity( entities[i] );
//
//				// Not static
//				if ( !m_isStatic )
//				{
//					break;
//				}
//			}
//	#ifdef _DEBUG
//			// Info
//			if ( m_isStatic )
//			{
//				LOG_ENGINE( TXT("*** Layer '%ls' assumed as static"), m_shortName.AsChar() );
//			}
//	#endif
//		}
//	}
//	else if ( GetLayerType() == LT_NonStatic ) // never set it to static
//	{
//		m_isStatic = false;
//	}
//#endif
//}

//void CLayerInfo::CheckStaticLayerEntity( CEntity* entity )
//{
//	if ( !m_isStatic ) return;
//
//	// Check components
//	const TDynArray< CComponent* >& components = entity->GetComponents();
//	for ( Uint32 j=0; j<components.Size(); j++ )
//	{
//		CComponent* component = components[j];
//		if ( component )
//		{
//			// Only small subset of components can be assumed static
//			if ( !component->IsExactlyA< CMeshComponent >() && 
//				!component->IsExactlyA< CLightComponent >() &&
//				!component->IsExactlyA< CSpotLightComponent >() &&
//				!component->IsExactlyA< CPointLightComponent >() &&
//				!component->IsExactlyA< CStaticMeshComponent >() && 
//				!component->IsExactlyA< CEnvProbeComponent >() && 
//				!component->IsExactlyA< CDecalComponent >() && 
//				!component->IsExactlyA< CPathComponent >() && 
//				!IsBasedOn( component, CNAME( CSoundAmbientAreaComponent ) ) &&
//				!IsBasedOn( component, CNAME( CWayPointComponent ) ) &&
//				!IsBasedOn( component, CNAME( CNavmeshComponent ) ) &&
//				!IsBasedOn( component, CNAME( CNavmeshGenerationRootComponent ) ) &&
//				!IsBasedOn( component, CNAME( CTriggerAreaComponent ) ) &&
//				!IsBasedOn( component, CNAME( CStickerComponent ) ) )
//			{
//				LOG_ENGINE( TXT("*** Layer '%ls' non static because of '%ls'"), m_shortName.AsChar(), component->GetFriendlyName().AsChar() );
//				m_isStatic = false;
//				break;
//			}
//		}
//	}
//}

Bool CLayerInfo::GetMergedVisiblityFlag() const
{
	if( m_forceVisible )
	{
		return true;
	}
	// Layer is not visible
	if ( !m_isVisible )
	{
		return false;
	}

	// Test groups, only in game
	if ( GGame->IsActive() )
	{
		// was to slow for some reason
		CLayerGroup* group = GetLayerGroup();
		if ( group && !group->IsVisible() )
		{
			return false;
		}
	}

	if (GIsCooker)
	{
		CLayerGroup* group = GetLayerGroup();
		if ( group && !group->IsVisibleOnStart() )
		{
			return false;
		}
	}

	// OK to show
	return true;
}

Bool CLayerInfo::MarkModified()
{
	if ( m_layer != nullptr )
	{
		return m_layer->MarkModified();
	}
	else
	{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		CDiskFile* file = GDepot->FindFile( m_depotFilePath );
		if ( file != nullptr )
		{
			return file->MarkModified();
		}
#endif
		return false;
	}
}

Bool GCanProcessLayerAttachments = true;

void CLayerInfo::LinkToWorld( CWorld* world )
{
	ASSERT( !m_world );
	m_world = world;
}

void CLayerInfo::ConditionalAttachToWorld()
{
	// Wait for a moment
	if ( !GCanProcessLayerAttachments )
	{
		return;
	}

	// No layer is loaded
	if ( !m_layer )
	{
		return;
	}

	// We are visible, attach to world
	const Bool isVisible = GetMergedVisiblityFlag();
	if ( isVisible )
	{
		if ( !m_layer->IsAttached() )
		{
			// Inform layer that it has been loaded
			m_layer->LayerLoaded();

			// Attach layer
			m_layer->AttachToWorld( GetWorld() );
		}
	}
	else
	{
		if ( m_layer->IsAttached() )
		{
			// Detach layer
			m_layer->DetachFromWorld();
		}
	}
}

#ifndef NO_EDITOR_WORLD_SUPPORT
Bool CLayerInfo::AppendLayerInfoObject( IFile& file )
{
	// Append layer info at the end
	// Note, this assumes this function is only called after RESAVING the layer, if not - the layer size will grow

	// move the to end of the file
	const Uint32 writePos = static_cast< Uint32 >( file.GetSize() );
	file.Seek( writePos );

	// Save the info
	DependencySavingContext context( this );
	CDependencySaver saver( file, NULL );
	if ( saver.SaveObjects( context ) )
	{
		// Write marker
		Uint32 marker[2] = { LAYER_INFO_MAGIC, writePos };
		file.Serialize( marker, sizeof(marker) );
	}
	else
	{
		// We were unable to save the layer info object
		WARN_ENGINE( TXT("Unable to save layer info with layer") );
	}

	return true;
}

Bool CLayerInfo::AppendLayerInfoObject( const String& fileAbsolutePath )
{
	IFile* file = GFileManager->CreateFileWriter( fileAbsolutePath, FOF_Append | FOF_AbsolutePath );
	if ( file )
	{
		AppendLayerInfoObject( *file );
		delete file;
		return true;
	}

	// Layer info not saved
	return false;
}

Bool CLayerInfo::AppendLayerInfoObject()
{	
	if ( m_layer )
	{
		return AppendLayerInfoObject( m_layer->GetFile()->GetAbsolutePath() );
	}

	// Layer info not saved
	return false;
}

void CLayerInfo::CreateLayerCopyForPIE()
{
	// Game cannot be active here
	ASSERT( !GGame->IsActive() );

	// We cannot be during async loading right now
	ASSERT( !m_loadingToken );

	// Always detach
	if ( m_layer && m_layer->IsAttached() )
	{
		m_layer->DetachFromWorld();
	}

	// Reset layer storage
	m_storage.ResetStorage();

	// Static layers don't require PIE copy
	// Env layers also do not require a copy regardless of their static flag
	if ( IsEnvironment() )
	{
		return;
	}

	// Create playable copy of layer
	if ( m_layer )
	{
		// Do not create a layer copy for file less layers
		if ( !m_layer->GetFile() )
		{
			RED_ASSERT( !m_layer->GetFile(), TXT("Layer '%ls' has no disk file. Probably it was not saved. It will not be loaded in the PIE."), GetDepotPath().AsChar() );
			return;
		}

		// OK, remember the layer object "hidden from the world"
		m_editorCopy = m_layer.Get();

		// Prevent removing editor copy of the layer
		m_editorCopy->AddToRootSet();

		// Cache layer data - this may or may not do a copy of the layer data - depends on the content and time stamp
		ASSERT( m_layer->GetFile() );
		String cacheFilePath;
		if ( m_layer->GetFile()->CacheSaveData( cacheFilePath ) )
		{
			// HACK!
			// Consulted with Dex from the Engine team.
			// The problem was: layer copy was not a full copy of the file. For instance it didn't contain layer info, therefore causing bugs.
			// As all of this code soon will be re-written by the Engine team (hopefully;), the temporary solution is to just write this layer info struct here.
			IFile* file = GFileManager->CreateFileWriter( cacheFilePath, FOF_Append | FOF_Buffered | FOF_AbsolutePath );
			if ( file )
			{
				const Uint64 headerStart = file->GetSize();

				file->Seek( headerStart );

				DependencySavingContext context( this );
				CDependencySaver saver( *file, nullptr );
				if ( saver.SaveObjects( context ) )
				{
					Uint32 marker[2] = { LAYER_INFO_MAGIC, Uint32( headerStart ) };
					file->Serialize( marker, sizeof( marker ) );
				}
				else
				{
					WARN_ENGINE( TXT("Unable to save layer info with layer") );
				}

				delete file;
			}
		}

		// Unbind resource from file
		m_layer->GetFile()->Rebind( nullptr );

		// Force reload
		m_layer = NULL;
	}
}

void CLayerInfo::RestoreAfterPIEUnload()
{
	ASSERT( !m_loadingToken );
	ASSERT( !m_requestUnload );

	// Always detach
	if ( m_layer && m_layer->IsAttached() )
	{
		m_layer->DetachFromWorld();
	}

	// Load layer if it was loaded before PIE has started
	if ( m_editorCopy )
	{
		ASSERT( !m_layer );

		// Get layer file
		CDiskFile* layerFile = GDepot->FindFile( m_depotFilePath );
		ASSERT( layerFile );
		ASSERT( !layerFile->IsLoaded() );

		// Restore
		layerFile->RemoveCacheSaveData();

		// Unbind resource from file
		layerFile->Rebind( nullptr );

		// Static layers don't require PIE restore
		if ( !IsEnvironment() )
		{
			// Rebind cached layer
			m_layer = m_editorCopy;
			layerFile->Rebind( m_editorCopy );			
		}

		m_editorCopy->RemoveFromRootSet();
		m_editorCopy = NULL;
	}
}

void CLayerInfo::RestoreAfterPIELoad()
{
	ASSERT( !m_loadingToken );
	ASSERT( !m_requestUnload );

	// Reattach
	ConditionalAttachToWorld();
}

void CLayerInfo::CalculateBoundingBox( Box& box )
{
	// Get entities in the layer
	TDynArray< CEntity* > entities;
	m_layer->GetEntities( entities );

	// Merge bounding boxes
	for ( Uint32 i=0; i<entities.Size(); ++i)
	{
		CEntity* entity = entities[i];
		box.AddBox( entity->CalcBoundingBox() );
	}
}

#endif // NO_EDITOR_WORLD_SUPPORT

#ifndef NO_EDITOR_EVENT_SYSTEM
void CLayerInfo::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	// Show/hide layer when in editor and PP mode is changed
	if ( name == CNAME( EnginePerformancePlatformChanged ) )
	{
		if ( GIsEditor && GGame && !GGame->IsActive() )
		{			
			ConditionalAttachToWorld();
		}
	}
}
#endif // NO_EDITOR_EVENT_SYSTEM

Bool CLayerInfo::CheckShouldSave()
{
	m_shouldSave = false;

	if ( !m_isVisible )
	{
		m_shouldSave = true;
		return true;
	}

	if ( m_storage.ShouldSave( m_layer.Get() ) )
	{
		m_shouldSave = true;
		return true;
	}

	return false;
}

void CLayerInfo::SaveState( IGameSaver* saver )
{
	#ifndef NO_SAVE_VERBOSITY
		static Uint32 cnt = 0;
		RED_LOG( Save, TXT("Saving layer %ld: %s%s"), cnt++, GetDepotPath().AsChar(), GetShortName().AsChar() ); 
	#endif

	CGameSaverBlock block( saver, CNAME(layerData) );

	// Store visibility flag
	const Bool layerIsVisible = m_isVisible;
	saver->WriteValue( CNAME(isVisible), layerIsVisible );

	// Store gameplay entity storage for this layer
	m_storage.UpdateAndStoreData( saver, m_layer.Get() );
}

void CLayerInfo::LoadState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(layerData) );

	// Restore visibility flag
	Bool layerIsVisible = false;
	loader->ReadValue( CNAME(isVisible), layerIsVisible );
	m_isVisible = layerIsVisible;

	// Restore gameplay entity storage for this layer
	m_storage.RestoreData( loader );
}

Uint32 CLayerInfo::CalcObjectDynamicDataSize() const
{
	if ( GetLayerStorage() )
	{
		return GetLayerStorage()->GetDataSize();
	}

	return 0;
}

// HACK: since PreChange cannot veto the change, we need to restore it later if
// the MarkModified failed. For this we use this TDynArray to store the
// value inside
static TDynArray< Uint8 > PropertyChangeFailureData;
static Bool PropertyChangeFailed;

void CLayerInfo::OnPropertyPreChange( IProperty* property )
{
	// Store previous value
	PropertyChangeFailureData.Clear();
	CMemoryFileWriter writer( PropertyChangeFailureData );
	property->GetType()->Serialize( writer, property->GetOffsetPtr( this ) );

	// Try to modify the layer infos
	PropertyChangeFailed = !MarkModified();
}

void CLayerInfo::OnPropertyPostChange( IProperty* property )
{
	// If the property change failed, just restore the previous value
	if ( PropertyChangeFailed )
	{
		CMemoryFileReader reader( PropertyChangeFailureData, 0 );
		property->GetType()->Serialize( reader, property->GetOffsetPtr( this ) );
	}

	// Reset state
	PropertyChangeFailed = false;
	PropertyChangeFailureData.Clear();

#ifndef NO_EDITOR
	// Check if the build tag was changed so we can emit the event
	if ( property->GetName() == CNAME( layerBuildTag ) )
	{
		EDITOR_QUEUE_EVENT( CNAME( LayerInfoBuildTagChanged ), nullptr );
	}
#endif
}
