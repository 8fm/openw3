/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/class.h"
#include "../../common/core/rttiSystem.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/staticShaderCache.h"
#include "../../common/engine/shaderCache.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/furMeshResource.h"
#include "cookDataBase.h"
#include "cookDataBaseHelper.h"
#include "cacheBuilderCommandlet.h"
#include "materialCooker.h"
#include "baseCacheBuilder.h"

//----------------------

class CShaderCacheBuilder : public IResourceBasedCacheBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CShaderCacheBuilder );

public:
	CShaderCacheBuilder();
	~CShaderCacheBuilder();

	// interface
	virtual void GetExtensions( TDynArray< String >& outExtensionList, const ECookingPlatform platform ) const override;
	virtual Bool Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions ) override;
	virtual Bool Process( const CCookerDataBase& db, const TDynArray< CCookerDataBase::TCookerDataBaseID >& filesToProcess ) override;
	virtual Bool ProcessResource( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, CResource* resource ) override;
	virtual Bool Save() override;

	// description
	virtual const Char* GetName() const override { return TXT("shaders"); }
	virtual const Char* GetDescription() const override { return TXT("Compile shaders for used materials"); }

private:
	IShaderCache*				m_shaderCacheCooker;
	IStaticShaderCache*			m_staticShaderCacheCooker;

	Bool						m_compileStaticShaders;
	Bool						m_compileFurShaders;
	MaterialCookingOptions		m_options;
	CMaterialCooker				m_materialCooker;
	CMaterialCooker::TFastFXMap	m_fastFXMap;

	typedef THashMap< CCookerDataBase::TCookerDataBaseID, Bool > TReferenceMap;
	TReferenceMap				m_isReferencedByMesh;
};

BEGIN_CLASS_RTTI( CShaderCacheBuilder )
	PARENT_CLASS( IResourceBasedCacheBuilder );
END_CLASS_RTTI()

//----------------------

IMPLEMENT_ENGINE_CLASS( CShaderCacheBuilder );

CShaderCacheBuilder::CShaderCacheBuilder()
	: m_compileStaticShaders( false )
	, m_compileFurShaders( false )
	, m_shaderCacheCooker( nullptr )
	, m_staticShaderCacheCooker( nullptr )
{
}

CShaderCacheBuilder::~CShaderCacheBuilder()
{
}

void CShaderCacheBuilder::GetExtensions( TDynArray< String >& outExtensionList, const ECookingPlatform platform ) const
{
	// material graphs
	outExtensionList.PushBack( ResourceExtension< CMaterialGraph >() );

	// also fur resources, so we can build the hairworks shaders
	outExtensionList.PushBack( ResourceExtension< CFurMeshResource >() );

	// TODO: once we move the fx files to depot we can process them as normal files
	//outExtensionList.PushBack( TXT("fx") );
}

Bool CShaderCacheBuilder::Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions )
{
	// initialize base builder
	if ( !IResourceBasedCacheBuilder::Initialize( outputFilePath, platform, additonalOptions ) )
		return false;

	// additional option to ignore static shader processing
	m_compileStaticShaders = additonalOptions.HasOption( TXT("staticshaders") );


	String staticShaderCacheFilename;
	String shaderCacheFilename;
	String furShaderCacheFilename;
	switch( platform )
	{
	case ECookingPlatform::PLATFORM_PC:
		{
			staticShaderCacheFilename = TXT("staticshader.cache");
			shaderCacheFilename = TXT("shader.cache");
			furShaderCacheFilename = TXT("furshader.cache");
			break;
		}
#ifndef WCC_LITE
	case ECookingPlatform::PLATFORM_PS4:
		{
			staticShaderCacheFilename = TXT("staticshaderps4.cache");
			shaderCacheFilename = TXT("shaderps4.cache");
			break;
		}
	case ECookingPlatform::PLATFORM_XboxOne:
		{
			staticShaderCacheFilename = TXT("staticshaderxboxone.cache");
			shaderCacheFilename = TXT("shaderxboxone.cache");
			break;
		}
#endif
	default:
		HALT( "Unsupported platform" );
	}

 	String shaderInputPath = String( TXT( "" ) );
	String shaderOutputPath = outputFilePath + shaderCacheFilename;
	String staticShaderInputPath = String( TXT( "" ) );
	String staticShaderOutputPath = outputFilePath + staticShaderCacheFilename;


	// Prepare for fur shader cooking, if we have a proper filename (if no filename, then not supported for the platform).
	m_compileFurShaders = !furShaderCacheFilename.Empty();
	if ( m_compileFurShaders && !m_materialCooker.InitFurCooking() )
	{
		ERR_WCC( TXT("Failed to initialize fur cooking. Fur shaders will not be cooked.") );
		m_compileFurShaders = false;
	}
	if ( m_compileFurShaders )
	{
		m_options.m_furShaderCachePath = outputFilePath + furShaderCacheFilename;
	}


	// Attempt to make an incremental build using the cache from previous one.
	if( additonalOptions.HasOption( TXT( "useoldcache" ) ) )
	{
		shaderInputPath = ( GFileManager->GetDataDirectory( ) + shaderCacheFilename ).ToLower( );
		staticShaderInputPath = ( GFileManager->GetDataDirectory( ) + staticShaderCacheFilename ).ToLower( );

		if ( m_compileFurShaders )
		{
			m_materialCooker.LoadExistingFurShaders( GFileManager->GetDataDirectory( ) + furShaderCacheFilename );
		}
	}

	// Initialize cooker with previous cache, if any.
	m_shaderCacheCooker = IShaderCache::CreateReadWrite( shaderInputPath );
	m_staticShaderCacheCooker = IStaticShaderCache::CreateReadWrite( staticShaderInputPath );

	// Set output path in the cookers.
	m_shaderCacheCooker->SetAbsolutePath( shaderOutputPath );
	m_staticShaderCacheCooker->SetAbsolutePath( staticShaderOutputPath );

	// When cooking, always save the results to output folder.
	m_shaderCacheCooker->ForceDirty( );
	m_staticShaderCacheCooker->ForceDirty( );

	m_options.m_platform = platform;
	m_options.m_shaderCacheCooker = m_shaderCacheCooker;
	m_options.m_staticShaderCacheCooker = m_staticShaderCacheCooker;
	m_options.m_dumpFileName = String::EMPTY;
	m_options.m_dumpDirPath = GFileManager->GetBaseDirectory() + TXT("shadersDump\\");
	m_options.m_dumpAssembly = false;

	CMaterialCooker::BuildFastFXMap( m_fastFXMap );

	return true;
}

Bool CShaderCacheBuilder::Process( const CCookerDataBase& db, const TDynArray< CCookerDataBase::TCookerDataBaseID >& filesToProcess )
{
	// Process static shaders
	if ( m_compileStaticShaders )
	{
		// Create local material compiler
		if ( !m_materialCooker.CompileStaticShaders( m_options ) )
		{
			ERR_WCC( TXT("Failed to compile static shaders") );
			if ( !m_ignoreErrors )
			{
				return false;
			}
		}
	}

	for ( auto& dbId : filesToProcess )
	{
		m_isReferencedByMesh.Insert( dbId, false );
	}

	StringAnsi fxString( "fx\\" );

	const CName materialGraphClassName = CMaterialGraph::GetStaticClass()->GetName();
	TDynArray< const CClass* > meshClass;
	meshClass.PushBack( CMesh::GetStaticClass() );
	db.QueryResources( [&]( const CCookerDataBase& db, const CCookerResourceEntry& entry )
	{ 
		TDynArray< CCookerResourceEntry > hardDependencies;
		TDynArray< CCookerResourceEntry > softDependencies;
		TDynArray< CCookerResourceEntry > inplaceDependencies;

		const CCookerDataBase::TCookerDataBaseID entryId = db.GetFileEntry( entry.GetFileId() );
		if ( !db.GetFileDependencies( entryId, hardDependencies, softDependencies, inplaceDependencies ) )
		{
			return false;
		}

		// loop through dependencies
		for ( CCookerResourceEntry& hardDependency : hardDependencies )
		{
			const CCookerDataBase::TCookerDataBaseID dependencyId = db.GetFileEntry( hardDependency.GetFileId() );

			// get dependency class
			CName resourceClassName;
			if ( !db.GetFileResourceClass( dependencyId, resourceClassName ) )
			{
				continue;
			}
			
			// check if dependency is a material graph
			if ( resourceClassName == materialGraphClassName )
			{
				/*
				if ( hardDependency.GetFilePath().BeginsWith( fxString ) )
				{
					StringAnsi warning = StringAnsi::Printf( "Material '%s' referenced by mesh '%s' via HARD DEPENDENCY", hardDependency.GetFilePath().AsChar(), entry.GetFilePath().AsChar() );
					LOG_ENGINE( ANSI_TO_UNICODE( warning.AsChar() ) );
				}
				*/
				
				RED_ASSERT( m_isReferencedByMesh.KeyExist( dependencyId ) );
				m_isReferencedByMesh[ dependencyId ] = true;
			}
		}

		// loop through dependencies
		for ( CCookerResourceEntry& softDependency : softDependencies )
		{
			const CCookerDataBase::TCookerDataBaseID dependencyId = db.GetFileEntry( softDependency.GetFileId() );

			// get dependency class
			CName resourceClassName;
			if ( !db.GetFileResourceClass( dependencyId, resourceClassName ) )
			{
				continue;
			}

			// check if dependency is a material graph
			if ( resourceClassName == materialGraphClassName )
			{
				/*
				if ( softDependency.GetFilePath().BeginsWith( fxString ) )
				{
					StringAnsi warning = StringAnsi::Printf( "Material '%s' referenced by mesh '%s' via SOFT DEPENDENCY", softDependency.GetFilePath().AsChar(), entry.GetFilePath().AsChar() );
					LOG_ENGINE( ANSI_TO_UNICODE( warning.AsChar() ) );
				}
				*/

				RED_ASSERT( m_isReferencedByMesh.KeyExist( dependencyId ) );
				m_isReferencedByMesh[ dependencyId ] = true;
			}
		}

		return true;
	}, CookDataBaseHelper::PerClassFilter( meshClass ) );

	// Process material files
	return IResourceBasedCacheBuilder::Process( db, filesToProcess );
}

Bool CShaderCacheBuilder::ProcessResource( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, CResource* resource )
{
	if ( m_compileFurShaders && resource->IsA< CFurMeshResource >() )
	{
		return m_materialCooker.CompileFurShader( Cast< CFurMeshResource >( resource ) );
	}

	// Material graph processing
	if ( !resource->IsA< CMaterialGraph >() )
	{
		return false;
	}

	CMaterialGraph* materialGraph = Cast< CMaterialGraph >( resource );
	const String& materialPath = materialGraph->GetDepotPath();

	// setup vertex factories for FastFX path
	TDynArray< EMaterialVertexFactory > vertexFactoriesForMaterial;
	if ( m_fastFXMap.Find( materialPath, vertexFactoriesForMaterial ) )
	{
		/*
		if ( m_isReferencedByMesh[ dbId ] )
		{
			LOG_ENGINE( TXT("Material '%ls' found in FastFX path but referenced by mesh"), materialPath.AsChar() );
		}
		*/
		m_options.m_materialVertexFactories = vertexFactoriesForMaterial;
	}
	else
	{
		// clear vertex factories if not in FastFX mode
		m_options.m_materialVertexFactories.Clear();
	}

	if ( m_isReferencedByMesh[ dbId ] )
	{
		// for materials that are referenced by any mesh via a dependency we can't use FastFX path
		// clear predefined vertex factories
		m_options.m_materialVertexFactories.Clear();
	}
	return m_materialCooker.CompileMaterialMultithreaded( materialGraph, m_options );
}

Bool CShaderCacheBuilder::Save()
{
	// flush shader cache to file
	if ( m_shaderCacheCooker )
	{
		m_shaderCacheCooker->Flush();

		delete m_shaderCacheCooker;
		m_shaderCacheCooker = nullptr;
	}

	if ( m_staticShaderCacheCooker )
	{
		m_staticShaderCacheCooker->Flush();

		delete m_staticShaderCacheCooker;
		m_staticShaderCacheCooker = nullptr;
	}

	if ( m_compileFurShaders )
	{
		m_materialCooker.FinishFurCooking( m_options );
	}

	return true;
}

