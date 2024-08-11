/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "resource.h"
#include "namesRegistry.h"
#include "events.h"
#include "dependencyMapper.h"
#include "directory.h"
#include "thumbnail.h"
#include "cooker.h"
#include "depot.h"
#include "scriptStackFrame.h"
#include "dependencyLoader.h"
#include "loadingJobManager.h"

IMPLEMENT_ENGINE_CLASS( CResource );

CResource::CResource()
#ifndef NO_EDITOR
	: m_saveForbidden( false )
#endif
{
}

CResource::~CResource()
{
}

void CResource::CleanupSourceData()
{
	// Delete all PC source data

#ifndef NO_RESOURCE_IMPORT
	m_importFile = String::EMPTY;
#endif
}

#ifndef NO_EDITOR_RESOURCE_SAVE

void CResource::OnResourceSavedInEditor()
{
}

#endif

#ifndef NO_RESOURCE_COOKING

void CResource::OnCook( class ICookerFramework& cooker )
{
	CObject::OnCook( cooker );

	// find matching cooker 
	const ECookingPlatform platform = cooker.GetPlatform();
	ICooker* baseCooker = ICooker::FindCooker( GetClass(), platform );
	if ( baseCooker )
	{
		ICooker::CookingOptions options( platform );
		options.m_resource = this;

		if ( !baseCooker->DoCook( cooker, options ) )
		{
			cooker.CookingError( this, TXT("Custom cooker '%ls' failed"), 
				baseCooker->GetClass()->GetName().AsChar() );
		}
	}
}

#endif

void CResource::OnAllHandlesReleased()
{
	// Do nothing for a resource that does not come from a file
	if ( m_file && !GIsClosing ) // hack: the engine shutdown is still a huge mess
	{
		// We were just told that this resource is not longer used in any THandle
		// Make sure we WONT create new handles the the resource (unless we call a direct Load on it)
		EnableHandleProtection();

		// Add this resource to quarantine
		m_file->MoveToQuarantine();
	}
}

void CResource::OnResourceReused()
{
}

String CResource::GetFriendlyName() const
{
	String name;

	if ( GetFile() )
	{
		name += GetClass()->GetName().AsString();
		name += TXT(" \"");
		name += GetFile()->GetDepotPath();
		name += TXT("\"");
	}
	else
	{
		name += TXT("Unnamed ");
		name += GetClass()->GetName().AsString();
	}

	return name;
}

String CResource::GetDepotPath() const
{
	if ( GetFile() )
	{
		return GetFile()->GetDepotPath();
	}
	else
	{
		return String::EMPTY;
	}
}

Red::System::DateTime CResource::GetFileTime() const
{
	if ( GetFile() )
	{
		return GetFile()->GetFileTime();
	}
	else
	{
		return Red::System::DateTime();
	}
}

Bool CResource::PreventCollectingResource() const
{
	if ( GIsEditor )
	{
		return GetFile() && GetFile()->IsModified();
	}

	return false;
}

Bool CResource::PreventFullUnloading() const
{
	return false;
}

void CResource::GenerateDependencies( TDynArray< CResource* >& neededResources )
{
	// Generate full dependencies
	DependencyMappingContext context( this );
	CDependencyMapper mapper( context );

	// Extract imports
	for ( Uint32 i=0; i<mapper.m_imports.Size(); i++ )
	{
		const CDependencyMapper::ImportInfo& info = mapper.m_imports[i];
		neededResources.PushBack( info.m_resource );
	}
}

#ifndef NO_EDITOR
void CResource::GetDependentResourcesPaths( TDynArray< DependencyInfo >& resourcesPaths, const TDynArray< String >& alreadyProcessed, Bool recursive /* = true */ ) const
{
	// Collect the path of current resource
	if ( !alreadyProcessed.Exist( GetDepotPath() ) )
	{
		resourcesPaths.PushBackUnique( DependencyInfo( GetDepotPath() ) );
	}

	Uint32 pathsIndex = 0, pathsSize = resourcesPaths.Size();
	while( pathsIndex < pathsSize )		// resourcesPaths changes its Size in this loop
	{
		LoadDependenciesPaths( resourcesPaths[ pathsIndex ].m_path, resourcesPaths, alreadyProcessed );
		++pathsIndex;

		if ( recursive )
		{
			pathsSize = resourcesPaths.Size();
		}
	}
}

void CResource::LoadDependenciesPaths( const String& resourcePath, TDynArray< DependencyInfo >& resourcesPaths, const TDynArray< String >& alreadyProcessed ) const
{
	// Find depot file
	CDiskFile* file = GDepot->FindFileUseLinks( resourcePath, 0 );
	if ( file )
	{
		// Load file and its dependencies
		IFile* reader = file->CreateReader();
		if ( reader )
		{
			CDependencyLoader loader( *reader, NULL );
			TDynArray< FileDependency > dependencies;
			if ( loader.LoadDependencies( dependencies, true ) )
			{
				// Collect all dependencies paths
				for ( Uint32 i = 0; i < dependencies.Size(); ++i )
				{
					if ( !alreadyProcessed.Exist( dependencies[i].m_depotPath ) )
					{
						resourcesPaths.PushBackUnique( DependencyInfo( dependencies[i].m_depotPath, file->GetDepotPath() ) );
					}
				}
			}
			delete reader;
		}
	}
}
#endif

Red::Threads::AtomicOps::TAtomic32	GResourceRuntimeIndex = 1;

const Uint32 CResource::AllocateRuntimeIndex()
{
	return Red::Threads::AtomicOps::Increment32( &GResourceRuntimeIndex );
}

Bool CResource::MarkModified()
{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	// Give the file opportunity to checkout itself
	if ( m_file )
	{
		ASSERT( IsTransient() == false );
		return m_file->MarkModified();
	}
#endif

	// Pass to object base class
	return TBaseClass::MarkModified();
}

Bool CResource::CanModify()
{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	// Give the file opportunity to checkout itself
	if ( m_file )
	{
		ASSERT( IsTransient() == false );
		return m_file->IsCheckedOut() || m_file->IsLocal() || m_file->IsAdded();
	}
#endif

	// Pass to object base class
	return TBaseClass::CanModify();
}

#ifndef NO_RESOURCE_IMPORT

void CResource::SetImportFile( const String& importFile )
{
	if ( CanModify() )
	{
		m_importFile = importFile;
		m_importFileTimeStamp = GFileManager->GetFileTime( importFile );
		MarkModified();
	}
}

Bool CResource::CheckNewerImportFileExists() const
{
	// Check if there exists a file with never version
	CDateTime importFileTimeStamp( GFileManager->GetFileTime( m_importFile ) );
	if ( importFileTimeStamp.IsValid() && importFileTimeStamp > m_importFileTimeStamp )
	{
		return true;
	}

	// Not found
	return false;
}

Bool CResource::Save()
{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	EDITOR_DISPATCH_EVENT( CNAME( ResourcePreSave ), CreateEventData( this ) );
	
	if ( m_saveForbidden )
	{
		return false;
	}

	ASSERT( m_file );
	return m_file->Save();
#else
	return false;
#endif	//!NO_FILE_SOURCE_CONTROL_SUPPORT
}

class SJobManagerLocker
{
public:
	SJobManagerLocker()
	{
		SJobManager::GetInstance().Lock();
		SJobManager::GetInstance().FlushProcessingJobs();
	}

	~SJobManagerLocker()
	{
		SJobManager::GetInstance().Unlock();
	}
};

Bool CResource::SaveAs( CDirectory* directory, const String& simpleFileName, Bool noSourceControl )
{
	RED_UNUSED( noSourceControl );
	ASSERT( directory );
	ASSERT( !simpleFileName.Empty() );

	// IT IS MANTADORY TO FLUSH all streaming jobs before we start saving
	SJobManager::GetInstance().FlushPendingJobs();
	//SJobManagerLocker magicLocker; <- less safe, we may have a dead lock inside the save function

	// Append default resource extension
	String fileName = simpleFileName;
	if ( !simpleFileName.ContainsSubstring( TXT(".") ) )
	{
		fileName += TXT(".");
		fileName += GetExtension();
	}

	// Make sure we won't overwrite existing file with resource already loaded
	CDiskFile* existingFile = directory->FindLocalFile( fileName );
	if ( existingFile && existingFile->GetResource() && existingFile->GetResource() != this )
	{
		WARN_CORE( TXT("Unable to save '%ls' because it will overwrite already loaded resource"), fileName.AsChar() );
		return false;
	}

	// New file is being saved
	if ( !existingFile )
	{
		if ( m_file )
		{
			// this should probably get applied also to last case
			m_file->Bind( NULL );
		}
		// Create new disk file definition
		m_file = new CDiskFile( directory, fileName, this );
		// we were setting the file's resource directly here, but setting it through the constructor 
		// calls Bind() and sets the internal loading flag properly
		//m_file->m_resource = this;

		// Save the file
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		if ( !m_file->Save() )
		{
			// Saving failed, cleanup CDiskFile
			delete m_file;
			m_file = NULL;
			return false;
		}
#endif	//!NO_FILE_SOURCE_CONTROL_SUPPORT

		// Saving succeeded, register new file in CDirectory
		directory->AddFile( m_file );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		// Add file to version control
		if ( !noSourceControl )
		{
			m_file->Add();
		}

		m_file->GetStatus();
#endif	//!NO_FILE_SOURCE_CONTROL_SUPPORT
	}

	// The easiest case - saving to the existing file
	else if ( existingFile == m_file )
	{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		// Just save it
		return m_file->Save();
#endif	//!NO_FILE_SOURCE_CONTROL_SUPPORT
	}

	// We can be saving to different, but EXISTING file ( overriding it )
	else
	{
		CDiskFile* otherFile = existingFile;
		CDiskFile* originalFile = m_file;

		// Override file/resource mapping before saving
		ASSERT( otherFile->GetResource() == NULL );
		// same as in ( !existingFile ) case - using Rebind to set file's resource
		// sets also file's internal loading flag properly
		//otherFile->m_resource = this;
		otherFile->Rebind( this );

		if ( originalFile )
		{
			originalFile->m_resource = NULL;
		}
		m_file = otherFile;

		// Save the file
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
		if ( !m_file->Save() )
		{
			// Saving failed, restore previous state
			otherFile->m_resource = NULL;
			if ( originalFile )
			{
				originalFile->m_resource = this;
			}
			m_file = originalFile;
			return false;
		}
#endif	//!NO_FILE_SOURCE_CONTROL_SUPPORT
	}

	// If we got so far is means saving was successful
	return true;
}

#endif	//NO_RESOURCE_IMPORT

void CResource::GetAdditionalInfo( TDynArray< String >& ) const
{
}

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

Bool CResource::Reload(Bool confirm)
{
	if (!GetReloadPriority()) return false;

	if ( confirm )
	{
		EDITOR_QUEUE_EVENT( CNAME( FileReloadConfirm ), CreateEventData( this ) );
	}
	else if ( m_file )
	{
		m_file->Unmodify();
		m_file->Reload();
	}

	return true;
}

#endif	//NO_FILE_SOURCE_CONTROL_SUPPORT

void CResource::OnDebugValidate()
{
	// Pass to base class
	TBaseClass::OnDebugValidate();

	// Make sure file links are valid
	if ( m_file )
	{
		// Check mapping
		ASSERT( m_file->GetResource() == this );

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

		// Check thumbnail pointer
		TDynArray< CThumbnail* > thumbnails = m_file->GetThumbnails();
		for ( Uint32 i = 0; i < thumbnails.Size(); ++i )
		{
			ASSERT( IsValidObject( thumbnails[i] ) );
		}

#endif

	}
}

#ifndef NO_DATA_VALIDATION
void CResource::OnCheckDataErrors() const
{
}

void CResource::OnFullValidation( const String& /*additionalContext*/ ) const
{
}
#endif

void CResource::funcGetPath( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( GetFile() )
	{
		RETURN_STRING( GetDepotPath() );
	}
	else
	{
		HALT(  "No file" );
		RETURN_STRING( String::EMPTY );
	}

}

void CResource::OnFinalize()
{
	// Unbind file
	UnbindFile( );

	// Disable handle reference counting
	TBaseClass::OnFinalize();
}

Bool CResource::IsModified() const 
{ 
#ifndef NO_RESOURCE_IMPORT
	if ( m_file ) 
		return m_file->IsModified(); 
	return false;
#else
	return false;
#endif
}
// Unbinds file for this resource
void CResource::UnbindFile( )
{
	if ( m_file )
	{
		m_file->Unbind( this );
		m_file = NULL;
	}
}
