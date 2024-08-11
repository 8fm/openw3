/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redSystem/crt.h"

typedef Red::Threads::AtomicOps::TAtomic32 RuntimePackageID;

const RuntimePackageID INVALID_RUNTIME_PACKAGE_ID	= 0;
const RuntimePackageID BASE_RUNTIME_PACKAGE_ID		= 1;

const Char* const MANIFEST_FILE_NAME = TXT("content.xml");

namespace Helper
{
	RuntimePackageID AllocRuntimePackageID();
}

//////////////////////////////////////////////////////////////////////////
// SContentFile
//////////////////////////////////////////////////////////////////////////
struct SContentFile
{
	StringAnsi			m_path;
	Uint64				m_size;
	Uint64				m_crc;
	Bool				m_isPatch;

	SContentFile()
		: m_size( 0 )
		, m_crc( 0 )
		, m_isPatch( false )
	{}

	SContentFile( const StringAnsi& path, Uint64 size, Uint64 crc, Bool isPatch )
		: m_path( path )
		, m_size( size )
		, m_crc( crc )
		, m_isPatch( isPatch )
	{}
};

//////////////////////////////////////////////////////////////////////////
// SContentChunk
//////////////////////////////////////////////////////////////////////////
struct SContentChunk
{
	CName							m_chunkLabel;
	TDynArray< SContentFile >		m_contentFiles;
};

//////////////////////////////////////////////////////////////////////////
// SContentLocalizedText
//////////////////////////////////////////////////////////////////////////
struct SContentLocalizedText
{
	String	m_language;
	String	m_value;

	Bool operator==( const SContentLocalizedText& rhs ) const { return m_language == rhs.m_language && m_value == rhs.m_value; }
};

//////////////////////////////////////////////////////////////////////////
// SContentPack
//////////////////////////////////////////////////////////////////////////
struct SContentPack
{
	CName								m_id;
	CName								m_dependency;
	Red::System::DateTime				m_timestamp;
	Uint32								m_minVersion;
	Uint32								m_priority;

	TDynArray< SContentChunk >			m_contentChunks;

	SContentPack()
		: m_minVersion( 0 )
		, m_priority( 0 )
	{}

	Bool operator==(const SContentPack& rhs ) const { return m_id == rhs.m_id; }
};

//////////////////////////////////////////////////////////////////////////
// SContentManifest
//////////////////////////////////////////////////////////////////////////
struct SContentManifest
{
	SContentPack		m_contentPack;
	//TDynArray<...>	m_blacklistDlcs; FIXME: move into installer.dat with platform specific label IDs?

	SContentManifest()
	{}
};

//////////////////////////////////////////////////////////////////////////
// SContentPackInstance
//////////////////////////////////////////////////////////////////////////
struct SContentPackInstance
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );

	enum EPackageStatus
	{
		ePackageStatus_Pending,
		ePackageStatus_Ready,
		ePackageStatus_LoadFailed,
	};

	SContentPackInstance( RuntimePackageID packageID, const String& mountPath )
		: m_mountPath( mountPath )
		, m_packageID( packageID )
		, m_packageStatus( ePackageStatus_Pending )
		, m_isLicensed( true )
		, m_autoAttachChunks( false )
		, m_isHidden( false )
		, m_isMod( false )
	{}

	SContentPackInstance( const SContentPack& contentPack, RuntimePackageID packageID, const String& mountPath )
		: m_contentPack( contentPack )
		, m_mountPath( mountPath )
		, m_packageID( packageID )
		, m_packageStatus( ePackageStatus_Ready )
		, m_isLicensed( true )
		, m_autoAttachChunks( false )
		, m_isHidden( false )
		, m_isMod( false )
	{}

	SContentPackInstance( SContentPack&& contentPack, RuntimePackageID packageID, const String& mountPath )
		: m_contentPack( Move( contentPack ) )
		, m_mountPath( mountPath )
		, m_packageID( packageID )
		, m_packageStatus( ePackageStatus_Ready )
		, m_isLicensed( true )
		, m_autoAttachChunks( false )
		, m_isHidden( false )
		, m_isMod( false )
	{}

	void SetAutoAttachChunks( Bool autoAttachChunks )
	{
		m_autoAttachChunks = autoAttachChunks;
	}

	void SetHidden( Bool isHidden )
	{
		m_isHidden = isHidden;
	}

	void SetMod( Bool isMod )
	{
		m_isMod = isMod;
	}

	SContentPack		m_contentPack;
	String				m_mountPath;
	RuntimePackageID	m_packageID;
	EPackageStatus		m_packageStatus;
	Bool				m_isLicensed:1;
	Bool				m_autoAttachChunks:1;
	Bool				m_isHidden:1;
	Bool				m_isMod:1;
};

//////////////////////////////////////////////////////////////////////////
// EContentPackageEventType
//////////////////////////////////////////////////////////////////////////
enum EContentPackageEventType
{
	eContentPackageEventType_Invalid,
	eContentPackageEventType_NewPackageMounted,
	eContentPackageEventType_NewChunkInstalled,
	eContentPackageEventType_LicenseChanged,
};

enum EContentPackageMountFlags
{
	eContentPackageMountFlags_None				= 0,
	eContentPackageMountFlags_AutoAttachChunks	= FLAG(0),
	eContentPackageMountFlags_IsHidden			= FLAG(1),
};

//////////////////////////////////////////////////////////////////////////
// SContentPackageEvent
//////////////////////////////////////////////////////////////////////////
struct SContentPackageEvent
{
	EContentPackageEventType			m_eventType;
	String								m_mountPath;
	RuntimePackageID					m_packageID;

	union
	{
		Uint32						m_reserved;
		Uint32						m_chunkNameIndex;
		Uint32						m_mountFlags;
		Bool						m_isLicensed;
	};

	SContentPackageEvent()
		: m_eventType( eContentPackageEventType_Invalid )
		, m_packageID( INVALID_RUNTIME_PACKAGE_ID )
		, m_reserved( 0 )
	{}

	SContentPackageEvent( RuntimePackageID packageID, const Char* mountPath, Uint32 mountFlags )
		: m_eventType( eContentPackageEventType_NewPackageMounted )
		, m_packageID( packageID )
		, m_mountPath( mountPath )
		, m_mountFlags( mountFlags )
	{}

	SContentPackageEvent( RuntimePackageID packageID, Bool isLicensed )
		: m_eventType( eContentPackageEventType_LicenseChanged )
		, m_packageID( packageID )
		, m_isLicensed( isLicensed )
	{}

	SContentPackageEvent( RuntimePackageID packageID, const CName& chunkName )
		: m_eventType( eContentPackageEventType_NewChunkInstalled )
		, m_packageID( packageID )
		, m_chunkNameIndex( chunkName.GetIndex() )
	{}

	CName AsChunkName() const 
	{ 
		RED_FATAL_ASSERT( m_eventType == eContentPackageEventType_NewChunkInstalled, "Wrong event type %u (expected %u)", m_eventType, eContentPackageEventType_NewChunkInstalled );
		return CName( m_chunkNameIndex );
	}
};

namespace Helper
{
	Bool CreateContentChunk( SContentChunk& outContentChunk, Red::System::DateTime& outMaxFileTimestamp, CName chunkName, const TDynArray< String >& absolutePaths, const String& baseDirectory,
		Bool allFilesMustSucceed = true, Bool skipCrc = false );

	void SortFilesForFilter( SContentChunk& outContentChunk );
}
