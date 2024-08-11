/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "resourcePath.h"
#include "handleMap.h"
#include "resource.h"
#include "class.h"

/// Gathering flags
enum EResourceGatheringFlags
{
	RGF_NotCooked		= FLAG( 0 ),		//!< Resource is NOT cooked ( by default resources are cooked )
	RGF_Startup			= FLAG( 1 ),		//!< Resource is cooked in startup packages ( by default resources are cooked into separate files )
};

/// Gathered resource represents a "string" path to a resource that is collected during cooking.
class CGatheredResource
{
public:
	CGatheredResource( const Char* path, Uint32 flags );
	~CGatheredResource();

	//! Get resource path
	RED_INLINE const CResourcePath& GetPath() const { return m_path; }

	//! Get resource
	RED_INLINE THandle< CResource > GetResource() const { return m_resource; }

	//! Is the resource loaded ?
	RED_INLINE Bool IsLoaded() const { return m_resource.Get() != NULL; }

	//! Is this resource cooked ?
	RED_INLINE Bool IsCooked() const { return 0 == ( m_flags & RGF_NotCooked ); }

	//! Is this resource included in the startup package ?
	RED_INLINE Bool IsStartup() const { return 0 != ( m_flags & RGF_Startup ); }

	//! Load resource, very touchy to any errors
	Bool Load();

	//! Release internal reference to resource, if it's the last one resource will be unloaded
	void Release();

	//! Load and get the resource
	template< class T >
	RED_INLINE THandle< T > LoadAndGet()
	{
		Load();
		THandle< CResource > res = GetResource();
		return Cast< T >( res );
	}

public:
	//! Get the resource list
	static TDynArray< CGatheredResource* >& GetGatheredResources();

	//! Unload all gathered resources
	static void ReleaseAllGatheredResources();

protected:
	CResourcePath			m_path;				//!< Path to resource
	THandle< CResource >	m_resource;			//!< Loaded resource
	Uint32					m_flags;			//!< Gather flags
	Bool					m_isInvalid;		//!< Resource failed to load
};