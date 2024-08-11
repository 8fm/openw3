
#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/xmlFileReader.h"

/// Async collision cache validation tool - checks data format integrity
class CTestNGPlusDefinitionsCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CTestNGPlusDefinitionsCommandlet, ICommandlet, 0 );

public:
	CTestNGPlusDefinitionsCommandlet()
	{
		m_commandletName = CName( TXT("testdefs") );
		m_definitionsToTest.PushBack( DefinitionInfo( TXT("items"), TXT("item") ) );
		m_definitionsToTest.PushBack( DefinitionInfo( TXT("abilities"), TXT("ability") ) );
		m_definitionsToTest.PushBack( DefinitionInfo( TXT("loot_definitions"), TXT("loot") ) );
	}

	~CTestNGPlusDefinitionsCommandlet()
	{
	}

	// ICommandlet interface
	virtual const Char* GetOneLiner() const 
	{ 
		return TXT("Test NG+ items and abilities definitions"); 
	}

	virtual void PrintHelp() const
	{
		LOG_WCC( TXT("Usage:") );
		LOG_WCC( TXT("  testdefs -dir1=<path> -dir2=<path>") );
	}

	virtual bool Execute( const CommandletOptions& options )
	{
		String dir1, dir2;
		if ( !options.GetSingleOptionValue( TXT("dir1"), dir1 ) || !options.GetSingleOptionValue( TXT("dir2"), dir2 ) )
		{
			ERR_WCC( TXT("Expecting paths to two directories") );
			return false;
		}

		if ( !dir1.EndsWith( TXT("\\") ) )
		{
			dir1 += TXT("\\");
		}

		if ( !dir2.EndsWith( TXT("\\") ) )
		{
			dir2 += TXT("\\");
		}

		TDynArray< String > paths1;
		GFileManager->FindFiles( dir1, TXT( "*.xml" ), paths1, true );

		Bool allOk = true;
		for ( const String& path1 : paths1 )
		{
			ASSERT( path1.BeginsWith( dir1 ) );
			ASSERT( GFileManager->FileExist( path1 ) );
			String rel = path1.MidString( dir1.Size()-1 );
			String path2 = dir2 + rel;
			if ( GFileManager->FileExist( path2 ) )
			{
				if ( !CompareDefinitions( path1, path2 ) )
				{
					allOk = false;
				}
			}
			else
			{
				WARN_WCC( TXT("Missing definition file: '%s'"), path2.AsChar() );
				allOk = false;
			}
		}

		return allOk;
	}

private:

	struct DefinitionInfo
	{
		DefinitionInfo( const String& mainTag, const String& defTag)
			: m_mainTag( mainTag ), m_defTag( defTag )
			{ }

		String m_mainTag;
		String m_defTag;
	};

	TDynArray< DefinitionInfo > m_definitionsToTest;

	typedef THashMap< String/*defTag*/, THashSet< String >/*defNames*/ > Definitions;

	Bool ExtractDefinitions( CXMLFileReader& xml, Definitions& definitions )
	{
		if ( xml.BeginNode( TXT("redxml") ) )
		{
			if ( xml.BeginNode( TXT("definitions") ) )
			{
				while ( xml.BeginNextNode() )
				{
					String mainNodeName;
					xml.GetNodeName( mainNodeName );
					auto it = FindIf( m_definitionsToTest.Begin(), m_definitionsToTest.End(),
						[ &mainNodeName ]( const DefinitionInfo& def ) { return def.m_mainTag == mainNodeName; } );

					if ( it != m_definitionsToTest.End() )
					{
						THashSet< String > names;
						while ( xml.BeginNextNode() )
						{
							String defNodeName;
							xml.GetNodeName( defNodeName );
							if ( defNodeName == it->m_defTag )
							{
								String name;
								if ( xml.Attribute( TXT("name"), name ) )
								{
									names.Insert( name );
								}
								else
								{
									WARN_WCC( TXT("Missing 'name' attribute in '%s' definition"), defNodeName.AsChar() )
									return false;
								}
							}

							definitions[ defNodeName ] = names;

							xml.EndNode();
						}
					}

					xml.EndNode();
				}

				xml.EndNode();
			}

			xml.EndNode();
		}

		return true;
	}

	Bool CompareDefinitions( const String& path1, const Definitions& definitions1, const String& path2, const Definitions& definitions2 )
	{
		for ( const auto& def1 : definitions1 )
		{
			const String& defTag = def1.m_first;
			if ( auto def2 = definitions2.FindPtr( defTag ) )
			{
				const THashSet< String >& defNames1 = def1.m_second;
				const THashSet< String >& defNames2 = *def2;
				for ( const String& defName1 : defNames1 )
				{
					if ( !defNames2.Exist( defName1 ) )
					{
						WARN_WCC( TXT("No definition '%s' in '%s'"), defName1.AsChar(), path2.AsChar() );
						return false;
					}
				}
			}
			else
			{
				WARN_WCC( TXT("No definitions of '%s' in '%s'"), defTag.AsChar(), path2.AsChar() );
				return false;
			}
		}

		return true;
	}

	Bool CompareDefinitions( const String& path1, const String& path2 )
	{
		Red::TScopedPtr< IFile > reader1( GFileManager->CreateFileReader( path1, FOF_AbsolutePath ) );
		Red::TScopedPtr< IFile > reader2( GFileManager->CreateFileReader( path2, FOF_AbsolutePath ) );
		ASSERT( reader1 && reader2 );
		CXMLFileReader xml1( *reader1 );
		CXMLFileReader xml2( *reader2 );

		Definitions definitions1;
		if ( !ExtractDefinitions( xml1, definitions1 ) )
		{
			WARN_WCC( TXT("Error while extracting definitions from '%s'"), path1.AsChar() );
			return false;
		}

		Definitions definitions2;
		if ( !ExtractDefinitions( xml2, definitions2 ) )
		{
			WARN_WCC( TXT("Error while extracting definitions from '%s'"), path2.AsChar() );
			return false;
		}

		if ( !CompareDefinitions( path1, definitions1, path2, definitions2 ) )
		{
			return false;
		}

		if ( !CompareDefinitions( path2, definitions2, path1, definitions1 ) )
		{
			return false;
		}


		return true;
	}
};


BEGIN_CLASS_RTTI( CTestNGPlusDefinitionsCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CTestNGPlusDefinitionsCommandlet );
