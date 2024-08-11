/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "ioTagResolver.h"
#include "gatheredResource.h"
#include "xmlFileReader.h"
#include "depot.h"
#include "scopedPtr.h"

CFileSystemPriorityResovler GFileSysPriorityResovler;

CFileSystemPriorityResovler::CFileSystemPriorityResovler()
	: m_context( eIOContext_Boot )
	, m_initialized( false )
{
}

Bool CFileSystemPriorityResovler::InitializePriorityTables()
{
	// load the file
#if defined(RED_PLATFORM_ORBIS)
	const String xmlPath = GFileManager->GetBaseDirectory() + TXT("bin/config/io_priority_table.xml");
#elif defined(RED_PLATFORM_DURANGO)
	const String xmlPath = GFileManager->GetBaseDirectory() + TXT("config/io_priority_table.xml");
#else
	const String xmlPath = GFileManager->GetBaseDirectory() + TXT("config/io_priority_table.xml");
#endif
	Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( xmlPath, FOF_AbsolutePath | FOF_MapToMemory ) );
	if ( !file )
	{
		ERR_CORE( TXT("Missing IO priority table. Build data is corrupted.") );
		return false;
	}

	// parse xml
	Red::TScopedPtr< CXMLReader > xml( new CXMLFileReader( *file ) );
	if ( !xml )
	{
		ERR_CORE( TXT("Unable to parse IO priority table. Build data is corrupted.") );
		return false;
	}

	// build name->tag mapping
	THashMap< StringAnsi, EIOTag > tagNames;
	tagNames.Reserve(eIOTag_MAX);
	for ( Uint32 i=0; i<eIOTag_MAX; ++i )
	{
		const AnsiChar* name = Helper::GetIOTagName( (EIOTag) i );
		tagNames[ name ] = (EIOTag) i;
	}

	// build name->context mapping
	THashMap< StringAnsi, EIOContext > contextNames;
	contextNames.Reserve(eIOContext_MAX);
	for ( Uint32 i=0; i<eIOContext_MAX; ++i )
	{
		const AnsiChar* name = Helper::GetIOContextName( (EIOContext) i );
		contextNames[ name ] = (EIOContext) i;
	}

	// priority names
	THashMap< String, Int8 > priorityNames;
	priorityNames[ TXT("normal") ] = Red::IO::eAsyncPriority_Normal;
	priorityNames[ TXT("low") ] = Red::IO::eAsyncPriority_Low;
	priorityNames[ TXT("high") ] = Red::IO::eAsyncPriority_High;
	priorityNames[ TXT("veryhigh") ] = Red::IO::eAsyncPriority_VeryHigh;
	priorityNames[ TXT("critical") ] = Red::IO::eAsyncPriority_Critical;

	// reset memory
	Red::MemorySet( &m_tables, 0xCC, sizeof(m_tables) );

	// process file
	if ( xml->BeginNode( TXT("tables") ) )
	{
		while ( xml->BeginNode( TXT("context") ) )
		{
			String name;
			if ( !xml->AttributeT( TXT("name"), name ) )
			{
				ERR_CORE( TXT("Missing context name"), name.AsChar() );
				return false;
			}

			// find context ID
			EIOContext contextID = eIOContext_MAX;
			if ( !contextNames.Find( UNICODE_TO_ANSI( name.AsChar() ), contextID ) )
			{
				ERR_CORE( TXT("Unknown context name '%ls'"), name.AsChar() );
				return false;
			}

			// start loading groups
			while ( xml->BeginNode( TXT("tag") ) )
			{
				String tagName;
				if ( !xml->AttributeT( TXT("name"), tagName ) )
				{
					ERR_CORE( TXT("Missing tag name in context '%ls'"), name.AsChar() );
					return false;
				}

				// find tag ID
				EIOTag tagID = eIOTag_MAX;
				if ( !tagNames.Find( UNICODE_TO_ANSI( tagName.AsChar() ), tagID ) )
				{
					ERR_CORE( TXT("Unknown tag name '%ls'"), tagName.AsChar() );
					return false;
				}

				// already set ?
				if ( m_tables[ contextID ][ tagID ] != 0xCC )
				{
					ERR_CORE( TXT("Priority value for tag '%ls' in context '%ls' is already set"), 
						tagName.AsChar(), name.AsChar() );
					return false;
				}

				// get priority value
				String priorityName;
				if ( !xml->AttributeT( TXT("priority"), priorityName ) )
				{
					ERR_CORE( TXT("Missing priority value for tag '%ls' in context '%ls'"), 
						tagName.AsChar(), name.AsChar() );
					return false;
				}

				// resolve priority name
				Int8 priority = -1;
				if ( !priorityNames.Find( priorityName, priority ) )
				{
					ERR_CORE( TXT("Unknown priority name '%ls'"), priorityName.AsChar() );
					return false;
				}


				// set priority value
				m_tables[ contextID ][ tagID ] = priority;

				// close current node, go to next one
				xml->EndNode();
			}

			// close current node, go to next one
			xml->EndNode();
		}
	}

	// fill missing values
	for ( Uint32 i=0; i<eIOContext_MAX; ++i )
	{
		for ( Uint32 j=0; j<eIOTag_MAX; ++j )
		{
			if ( m_tables[i][j] == 0xCC )
			{
				const AnsiChar* contextName = Helper::GetIOContextName( (EIOContext) i );
				const AnsiChar* tagName = Helper::GetIOContextName( (EIOContext) i );

				WARN_CORE( TXT("Loading priority for tag '%hs' in contenxt '%hs' not set. Defaulting to Normal Priority."),
					tagName, contextName );

				m_tables[i][j] = Red::IO::eAsyncPriority_Normal;
			}
		}
	}

	// stats
	LOG_CORE( TXT("Loading priorities initialized. Current mode: %hs."), Helper::GetIOContextName(m_context) );
	m_initialized = true;

	// loaded
	return true;
}

void CFileSystemPriorityResovler::SwitchConext( const EIOContext context )
{	
	if ( m_context != context )
	{
		LOG_CORE( TXT("IO context switched to %hs"), Helper::GetIOContextName(context) );
		m_context = context;
	}
}

const Red::IO::EAsyncPriority CFileSystemPriorityResovler::Resolve( const EIOTag tag ) const
{
	// no tables yet
	if ( !m_initialized )
		return Red::IO::eAsyncPriority_Normal;

	if ( tag == eIOTag_BundlePreload )
		return Red::IO::eAsyncPriority_Critical;

	// ASSERT
	RED_FATAL_ASSERT( m_context < eIOContext_MAX, "Invalid IO context" );
	RED_FATAL_ASSERT( tag < eIOTag_MAX, "Invalid IO tag" );
	return (Red::IO::EAsyncPriority) m_tables[ m_context ][ tag ];
}

//----

namespace Helper
{

	const AnsiChar* GetIOTagName( const EIOTag tag )
	{
		#define TEST(x) case x: return #x + 7;
		switch ( tag )
		{
			TEST(eIOTag_Generic);
			TEST(eIOTag_BundlePreload);
			TEST(eIOTag_TexturesLow);
			TEST(eIOTag_TexturesNormal);
			TEST(eIOTag_TexturesImmediate);
			TEST(eIOTag_MeshesLow);
			TEST(eIOTag_MeshesNormal);
			TEST(eIOTag_MeshesImmediate);
			TEST(eIOTag_AnimationsLow);
			TEST(eIOTag_AnimationsNormal);
			TEST(eIOTag_AnimationsImmediate);
			TEST(eIOTag_SoundNormal);
			TEST(eIOTag_SoundImmediate);
			TEST(eIOTag_CollisionNormal);
			TEST(eIOTag_CollisionImmediate);
			TEST(eIOTag_TerrainHeight);
			TEST(eIOTag_TerrainControl);
			TEST(eIOTag_TerrainColor);
			TEST(eIOTag_ResourceLow);
			TEST(eIOTag_ResourceNormal);
			TEST(eIOTag_ResourceImmediate);
			TEST(eIOTag_UmbraBuffer);
		}
		#undef TEST

		return "Unknown";
	}

	const AnsiChar* GetIOContextName( const EIOContext context )
	{
		#define TEST(x) case x: return #x + 11;
		switch ( context )
		{
			TEST(eIOContext_Boot);
			TEST(eIOContext_WorldLoading);
			TEST(eIOContext_WorldStreaming);
			TEST(eIOContext_SceneLoading);
		}
		#undef TEST

		return "Unknown";
	}

} // Helper

//----