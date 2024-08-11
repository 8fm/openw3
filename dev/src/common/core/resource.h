/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "softHandle.h"
#include "object.h"
#include "datetime.h"

class ResourceLoadingContext;
class CGatheredResource;
class CDirectory;
class CDiskFile;
class CDependencyLoader;

#ifndef NO_EDITOR
static Char* SMergeableResourceExtensions[] =
{
	TXT("csv"),
};
#endif

#ifdef NO_EDITOR

#define DECLARE_ENGINE_RESOURCE_CLASS( __class, __base, __ext, __desc )	\
	DECLARE_ENGINE_CLASS( __class, __base, 0 )	\
	public: virtual const Char* GetExtension() const	{ return TXT(__ext); } \
	static const Char* GetFileExtension()				{ return TXT(__ext); } \
	private:

#else

#define DECLARE_ENGINE_RESOURCE_CLASS( __class, __base, __ext, __desc )	\
	DECLARE_ENGINE_CLASS( __class, __base, 0 )	\
	public: virtual const Char* GetExtension() const	{ return TXT(__ext); } \
	static const Char* GetFileExtension()				{ return TXT(__ext); } \
	virtual const Char* GetFriendlyDescription() const	{ return TXT(__desc); } \
	static const Char* GetFriendlyFileDescription()		{ return TXT(__desc); } \
	private:

#endif

/* 
Resource reload priority
	0 - unreloadable
	10 - normal (default)
	20 - layer
	30 - entity
	40 - world
	50 - env layer 
*/
typedef Uint32 ResourceReloadPriority;

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
/* Operation to carry when trying to submit a resource */
enum EResourceSubmitOperation
{
	RSO_Default=0,		// Default operation: submit the resource file
	RSO_ReturnSuccess,	// Do nothing and return success (assumes the handler did a submission by itself - useful for submitting dependencies)
	RSO_ReturnFailure	// Do nothing and return failure
};
#endif

class IVCMergeableResource
{
public:
	IVCMergeableResource();
};

/********************************/
/* Base resource class			*/
/********************************/
class CResource : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CResource, CObject );

	friend class CDiskFile;
	friend class CBaseEngine;
	friend class CDependencyLoader;
	friend class ResourceLoadingTask;

private:
	CDiskFile*			m_file;					// Source file, not valid for cooked resources
	Bool				m_isLoading;			// Is this resource being loaded right now ?
	Bool				m_isMissing;			// Is this resource just created as a resource stub ?

#ifndef NO_RESOURCE_IMPORT

protected:
	CDateTime			m_importFileTimeStamp;	// Timestamp of the file this resource was imported from
	String				m_importFile;			// Path to the import file

public:
	// Get depot file for this resource
	RED_MOCKABLE RED_INLINE CDiskFile* GetFile() const { return m_file; }

	// Get import file name
	RED_INLINE const String& GetImportFile() const { return m_importFile; }

#else

public:
	// Get depot file for this resource
	RED_INLINE CDiskFile* GetFile() const { return m_file; }

#endif

	// Unbinds file for this resource
	void UnbindFile( );

public:
    CResource();
	virtual ~CResource();

	// Is this resource being loaded right now ? Be careful with resources like that
	RED_FORCE_INLINE const Bool IsLoading() const { return m_isLoading; }

	// Is this resource just created as a resource stub for missing resource ?
	RED_FORCE_INLINE const Bool IsMissing() const { return m_isMissing; }

#ifndef NO_RESOURCE_COOKING
	//! Cook resource for use in game
	//! By default this function searches for matching ICooker helper class and uses it to convert the resource (old path)
	//! Also, by default, the function CleanupSourceData() is called
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

	//! On finalize, unregister stuff
	virtual void OnFinalize();

	//! Called when all THandles to this IReferencable are gone
	virtual void OnAllHandlesReleased();

	//! This resource is being re imported
	virtual void OnResourceReused();

	//! Debug stuff, verify object internal structure, called from time to time in debug builds
	virtual void OnDebugValidate();
	
#ifndef NO_DATA_VALIDATION
	//! Validate this resource using the data error reporter
	virtual void OnCheckDataErrors() const;

	//! Full validation of resource - only heavy duty stuff, done in offline tools
	virtual void OnFullValidation( const String& additionalContext ) const;
#endif

	//! Eg. render resource creation
	virtual void ForceFullyLoad(){};

public:
	// Generate dependency map for this resource, slow
	void GenerateDependencies( TDynArray< CResource* >& neededResources );

#ifndef NO_EDITOR
	struct DependencyInfo
	{
		String m_path;
		String m_parentPath;

		DependencyInfo( const String& path )
			: m_path( path ) 
		{}

		DependencyInfo( const String& path, const String& parent )
			: m_path( path )
			, m_parentPath( parent )
		{}

		Bool operator ==( const DependencyInfo& a )
		{
			return a.m_path == m_path;
		}
	};
	// Collect resource dependencies' paths
	void GetDependentResourcesPaths( TDynArray< DependencyInfo >& resourcesPaths, const TDynArray< String >& alreadyProcessed, Bool recursive = true ) const;
	
	void DisableSaving( Bool value ) { m_saveForbidden = value; }
private:
	Bool m_saveForbidden;
#endif

#ifndef NO_EDITOR_RESOURCE_SAVE

public:
	//! Called just before this resource is saved in editor
	virtual void OnResourceSavedInEditor();

#endif

	Bool IsModified() const;

#ifndef NO_RESOURCE_IMPORT

public:
	// Save resource using existing disk file
	Bool Save();

	// Save resource as specific file in given directory
	Bool SaveAs( CDirectory* directory, const String& fileName, Bool noSourceControl = false );

	// Set import file name
	void SetImportFile( const String& importFile );

	// Check if there is a newer version of the file to import
	Bool CheckNewerImportFileExists() const;

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

public:
	// Reload resource
	virtual Bool Reload( Bool confirm );

	// Called before the resource's file is added to repository
	virtual void OnBeforeAddToVersionControl() {}

	// Called before a check out - return false to fail the checkout
	virtual Bool OnBeforeCheckOut() { return true; }

	// Called after a check out - the succeed parameter has the checkout success or failure
	virtual void OnAfterCheckOut( Bool /*succeed*/ ) {}

	// Called before a submit - expected to return one of EResourceSubmitOperation (see the enum for details)
	// If interactive is false, the user will not be asked for a changelist description and the given one will be used
	virtual EResourceSubmitOperation OnBeforeSubmit( Bool, const String& ) { return RSO_Default; }
#endif

public:
	// Allocate resoruce runtime index - used in various resource base maps
	static const Uint32 AllocateRuntimeIndex();

	// Get priority
	virtual ResourceReloadPriority GetReloadPriority() { return 10; }

	// Mark resource as modified
	virtual Bool MarkModified();

	// Check if object can by modified
	virtual Bool CanModify();

	// Save
	virtual void OnSave() {}

	// Resource was pasted from clipboard
	virtual void OnPaste( Bool /*wasCopied*/ ) {}

	// Get resource friendly name
	virtual String GetFriendlyName() const;

	// Get resource depot path, use instead of GetFile()->GetDepotPath()
	virtual String GetDepotPath() const;

	// Get file time of the resource, warning: costly
	virtual Red::System::DateTime GetFileTime() const;

	// Get additional resource info, displayed in editor
	virtual void GetAdditionalInfo( TDynArray< String >& info ) const;

	// Should we prevent GC from collecting this resource
	virtual Bool PreventCollectingResource() const;

	// Should we prevent from losing changes to this resource when unloading
	virtual Bool PreventFullUnloading() const;

	//! Release source data ( cooking )
	virtual void CleanupSourceData();

public:
	// Get file extension used for this resource
	virtual const Char* GetExtension() const = 0;

#ifndef NO_EDITOR

	// Get human friendly description
	virtual const Char* GetFriendlyDescription() const = 0;

#endif

	// Get default resource used when resource of given type has not been found
	virtual CGatheredResource * GetDefaultResource() { return NULL; }

public:
	// Resource factory info
	template< class T >
	class FactoryInfo
	{
	public:
		CObject*		m_parent;		//!< Parent object, usually NULL
		T*				m_reuse;		//!< Existing resource objects to reuse

		RED_INLINE FactoryInfo()
			: m_parent( NULL )
			, m_reuse( NULL )
		{};

		// Create resource for resource factory
		RED_INLINE T* CreateResource() const
		{
			if ( m_reuse )
			{
				m_reuse->OnResourceReused();
				return m_reuse;
			}
			else
			{
				return ::CreateObject<T>( m_parent );
			}
		}
	};

public:
	// Compare policy
	struct CompareFunc
	{
		static RED_INLINE Bool Less( CResource* key1, CResource* key2 )
		{
			if (key1->GetReloadPriority() < key2->GetReloadPriority())
			{
				return true;
			}
			else if (key1->GetReloadPriority() > key2->GetReloadPriority())
			{
				return false;
			}
			else
			{
				if (key1 < key2)
				{
					return true;
				}
				else if (key1 > key2)
				{
					return false;
				}
				else
				{
					return false;
				}
			}
		}	
	};

private:
	void funcGetPath( CScriptStackFrame& stack, void* result );

#ifndef NO_EDITOR
	// Load resource dependencies using its loader
	void LoadDependenciesPaths( const String& resourcePath, TDynArray< DependencyInfo >& dependenciesPaths, const TDynArray< String >& alreadyProcessed ) const;
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( CResource );
	PARENT_CLASS( CObject );
#ifndef NO_RESOURCE_IMPORT
	PROPERTY_EDIT_NOT_COOKED( m_importFile, TXT("Source import path") );
	PROPERTY_RO_NOT_COOKED( m_importFileTimeStamp, TXT( "Last Modified" ) );
#endif
	NATIVE_FUNCTION( "GetPath", funcGetPath );
END_CLASS_RTTI();

RED_INLINE CClass* ResourceClassByExtension( const String & ext )
{
	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( CResource::GetStaticClass(), classes );

	for ( Uint32 i = 0; i < classes.Size(); ++i )
	{
		if ( ext.EqualsNC( classes[i]->GetDefaultObject< CResource >()->GetExtension() ) )
		{
			return classes[i];
		}
	}

	return NULL;
}

/// Get file extension used for particular resource class
template < class T >
RED_INLINE const Char* ResourceExtension() 
{	
	CClass *classDesc = ClassID<T>();
	CResource* zeroRes = classDesc->GetDefaultObject< CResource >();
	const Char *ext = zeroRes->GetExtension();
	return ext;
}

//! Compare
template< class T >
RED_INLINE Bool operator==( const TSoftHandle<T>& handle, const CResource* resource )
{
	return ( resource == NULL && handle.GetPath().Empty() ) || ( resource != NULL && handle.GetPath() == resource->GetDepotPath() );
}

/// Direct compare
template< class T >
RED_INLINE Bool operator==( const CResource* resource, const TSoftHandle<T>& handle )
{
	return handle == resource;
}

//! Not equal
template< class T >
RED_INLINE Bool operator!=( const TSoftHandle<T>& handle, const CResource* resource )
{
	return !( handle == resource );
}

template< class T >
RED_INLINE Bool operator!=( const CResource* resource, const TSoftHandle<T>& handle )
{
	return handle != resource;
}
