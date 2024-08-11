/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "contentManifest.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
struct SContentFile;

//////////////////////////////////////////////////////////////////////////
// SContentInfo
//////////////////////////////////////////////////////////////////////////
struct SContentInfo : Red::System::NonCopyable
{
	SContentInfo( RuntimePackageID packageID, CName chunkName, const String& mountPath, const TDynArray< const SContentFile* > contentFiles, Bool isMod )
		: m_packageID( packageID )
		, m_chunkName( chunkName )
		, m_mountPath( mountPath )
		, m_contentFiles( contentFiles )
		, m_isMod( isMod )
	{}

	const RuntimePackageID					m_packageID;	//<! Potentially for matching up DLC license changes or knowing if a DLC ( != BASE_RUNTIME_PACKAGE_ID )
	const CName								m_chunkName;	//<! For abuse currently, try not to avoid using. Name is only unique for a given packageID. E.g., can have a dlc0 in different packages.
	const String&							m_mountPath;	//<! Absolute directory path that contentFile paths are relative to.
	const TDynArray< const SContentFile* >	m_contentFiles; //<! List of content files in this package.
	const Bool								m_isMod;		//<! Is this from a mod? Abuse like no tomorrow, please.
};

//////////////////////////////////////////////////////////////////////////
// IContentListener
//////////////////////////////////////////////////////////////////////////
class IContentListener
{
public:
	virtual const Char* GetName() const=0;

	virtual void OnContentAvailable( const SContentInfo& contentInfo ) {}
	virtual void OnPackageAvailable( RuntimePackageID packageID ) {}

public:
	IContentListener() {}
	virtual ~IContentListener() {}
};
