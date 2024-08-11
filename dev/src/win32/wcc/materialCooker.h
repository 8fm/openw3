#pragma once

#include "..\..\common\matcompiler\build.h"

#include "../../common/core/cooker.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/materialCompilerDefines.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/staticShaderCache.h"
#include "../../common/engine/shaderCache.h"

#include "wccStats.h"

class IShaderCache;
class SChangelist;
class CFurMeshResource;

namespace
{
	void* Allocate( Uint32 size )
	{
		return RED_MEMORY_ALLOCATE( MemoryPool_Strings, MC_Temporary, size );
	}

	void Free( void* ptr )
	{
		if ( ptr )
		{
			RED_MEMORY_FREE( MemoryPool_Strings, MC_Temporary, ptr );
		}
	}
}

enum eShaderTypeFlags
{
	VertexShader		 = FLAG(0),
	PixelShader			 = FLAG(1),
	GeometryShader		 = FLAG(2),
	HullShader			 = FLAG(3),
	DomainShader		 = FLAG(4),
	ComputeShader		 = FLAG(5),
	ShaderTypeFlagsMax	 = FLAG(6),
};

struct SMaterialCookingContext
{
	const AnsiChar*				m_shaderFileNameAnsi;	//!< File name of the source shader
	String						m_shaderFileName;
	CMaterialCompilerDefines	m_definitions;			//!< A set of definitions to be inserted
	GpuApi::eShaderType			m_shaderType;			//!< Type of the shader that we are cooking
	GpuApi::eBufferChunkType	m_bufferChunkType;		//!< If != BCT_Max then the shader is a StreamOut shader with this stream output format
	Uint32						m_shadersMask;			//!< A mask representing what shaders are being used in this material (eg.: VS + PS, VS + HS + DS + PS, etc.)
	Bool						m_dumpAssembly;
	Bool						m_isStaticShader;
};

struct SMaterialCookingOutput
{
	DataBuffer					m_shaderData;		//!< Output shader bytecode
	Uint64						m_shaderHash;		//!< Output shader hash (hashed code and defines)
	Uint64						m_shaderCodeCRC;	//!< Output shader CRC (hash from unrolled code)
	Uint64						m_fileNameHash;

	SMaterialCookingOutput()
		: m_shaderData( DataBuffer( TDataBufferAllocator< MC_BufferShader >::GetInstance() ) )
		, m_shaderHash( 0 )
		, m_fileNameHash( 0 )
	{}
};

struct MaterialCookingOptions
{
	IShaderCache* 						m_shaderCacheCooker;
	IStaticShaderCache*					m_staticShaderCacheCooker;
	String								m_furShaderCachePath;
	ECookingPlatform	 				m_platform;
	String				 				m_dumpFileName;
	String 				 				m_dumpDirPath;
	Bool								m_dumpAssembly;
	SChangelist*		 				m_changelist;
	Bool								m_doResaveCRC;
	Bool				 				m_collectStats;
	TDynArray< EMaterialVertexFactory >	m_materialVertexFactories;

	MaterialCookingOptions()
		: m_shaderCacheCooker( nullptr )
		, m_staticShaderCacheCooker( nullptr )
		, m_platform( (ECookingPlatform)0 )
		, m_dumpAssembly( false )
		, m_changelist( nullptr )
		, m_doResaveCRC( false )
		, m_collectStats( false )
	{}
};

struct TechniqueToCompile
{
	RenderingContext			m_renderingContext;
	MaterialRenderingContext	m_materialContext;

	TechniqueToCompile()
		: m_materialContext( m_renderingContext )
	{
	}

	TechniqueToCompile( ERenderingPass renderPass )
		: m_materialContext( m_renderingContext )
	{
		m_renderingContext.m_pass = renderPass;
	}

	TechniqueToCompile( ERenderingPass renderPass, EMaterialVertexFactory vertexFactory )
		: m_materialContext( m_renderingContext )
	{
		m_renderingContext.m_pass = renderPass;
		m_materialContext.m_vertexFactory = vertexFactory;
	}
};

// forward declarations
namespace Processors
{
	namespace Entries
	{
		struct CMaterialProcessorEntry;
		struct CTechniqueProcessorEntry;
	}
	class CMaterialProcessor;
	class CTechniqueProcessor;
}

class CMaterialCooker
{
public:
	typedef THashMap< String, TDynArray< EMaterialVertexFactory > > TFastFXMap;

	enum ECompilationResult
	{
		ECR_CompilationSuccessful	= 0,
		ECR_CompilationFailed		= 1,
		ECR_FoundInCache			= 2,
	};

	static String CookingPlatformToString( ECookingPlatform platform )
	{
		Char* platformStr = TXT("Unknown");
		switch ( platform )
		{
		case PLATFORM_PC: 
			platformStr = TXT( "PC" );
			break;
#ifndef WCC_LITE
		case PLATFORM_PS4: 
			platformStr = TXT( "PS4" );
			break;
		case PLATFORM_XboxOne: 
			platformStr = TXT( "XBoxOne" );
			break;
#endif
		}
		return platformStr;
	}

public:
	static void				BuildFastFXMap( TFastFXMap& map );

public:

	CMaterialCooker();
	~CMaterialCooker();

	Bool					CompileMaterialMultithreaded( CMaterialGraph* graph, const MaterialCookingOptions& options, Processors::Entries::CMaterialProcessorEntry* entry = nullptr );
	Bool					CompileMaterial( CMaterialGraph* graph, const MaterialCookingOptions& options, Processors::Entries::CMaterialProcessorEntry* entry = nullptr );
	Bool					CompileStaticShaders( const MaterialCookingOptions& options );
	Bool					CompileShaderTechnique( const MaterialCookingOptions& options, CMaterialGraph* material, TechniqueToCompile& technique, Bool* shouldBeSaved = nullptr );

	Bool					InitFurCooking();
	Bool					LoadExistingFurShaders( const String& absolutePath );
	Bool					CompileFurShader( CFurMeshResource* fur );
	Bool					FinishFurCooking( const MaterialCookingOptions& options );

private:
	void					CollectTechniques( CMaterialGraph* material, TDynArray< TechniqueToCompile* >& techniques, const TDynArray< EMaterialVertexFactory >& mvfactories = TDynArray< EMaterialVertexFactory >() );
	ECompilationResult		CompileShader( const MaterialCookingOptions& options, const AnsiChar* code, const SMaterialCookingContext& context, SMaterialCookingOutput& output );
	Bool					CompileStaticShader( const MaterialCookingOptions& options, const String& shaderFileName, const CMaterialCompilerDefines& localDefines, const TDynArray< CMaterialCompilerDefines >& systemDefines, Uint32 shaderTypeFlags, GpuApi::eBufferChunkType bct );

	static const Char* GetShaderName( eShaderTypeFlags shaderType );
	void CheckContext( const MaterialRenderingContext& context );

	CStaticShadersStats  m_shaderCacheStats;

#ifdef USE_NVIDIA_FUR
	class GFSDK_HairSDK*		m_hairSDK;
#endif
};

namespace Processors
{
	namespace Entries
	{
		struct CMaterialProcessorEntry
		{
		public:
			CMaterialGraph*			m_graph;
			MaterialCookingOptions	m_options;
			Bool					m_doSave;
		};

		struct CTechniqueProcessorEntry
		{
		public:
			CMaterialGraph*			m_graph;
			TechniqueToCompile*		m_technique;
		};
	}

	class CMaterialProcessor
	{
	public:
		CMaterialCooker*		m_cooker;
		TDynArray< String >*	m_cookedMaterials;

		Red::Threads::CMutex	m_arrayMutex;

	public:
		CMaterialProcessor( CMaterialCooker* cooker, TDynArray< String >* cookedMaterials = nullptr )
			: m_cooker( cooker )
			, m_cookedMaterials( cookedMaterials )
		{ }

		CMaterialProcessor& operator=( const CMaterialProcessor& other )
		{
			m_cooker			= other.m_cooker;
			m_cookedMaterials	= other.m_cookedMaterials;
			return *this;
		}

		void Compute( Entries::CMaterialProcessorEntry*& entry )
		{
			entry->m_doSave = false;
			m_cooker->CompileMaterial( entry->m_graph, entry->m_options, entry );

			// report back the name of compiled material
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_arrayMutex );
			if ( m_cookedMaterials )
			{
				const String& depotPath = entry->m_graph->GetDepotPath();
				if ( depotPath.BeginsWith( TXT("fx\\") ) )
				{
					// better to cook all other materials in all configurations
					m_cookedMaterials->PushBack( depotPath );
				}
			}			
		}
	};

	class CTechniqueProcessor
	{
	public:
		CMaterialCooker*		m_cooker;
		MaterialCookingOptions	m_options;
		Red::Threads::CMutex	m_arrayMutex;

	public:
		CTechniqueProcessor( CMaterialCooker* cooker, const MaterialCookingOptions& options )
			: m_cooker( cooker )
			, m_options( options )
		{ }

		CTechniqueProcessor& operator=( const CTechniqueProcessor& other )
		{
			m_cooker			= other.m_cooker;
			m_options			= other.m_options;
			return *this;
		}

		void Compute( Entries::CTechniqueProcessorEntry*& entry )
		{
			m_cooker->CompileShaderTechnique( m_options, entry->m_graph, *entry->m_technique );
		}
	};
};

class CTechniqueLister
{
public:
	CTechniqueLister( TDynArray< TechniqueToCompile* >& techniques, TSortedArray< Uint32 >& alreadyCreatedIds, Bool instanced, ERenderingPass renderPass, EMaterialVertexFactory vertexFactory, Bool isDiscardPass );
	void GenerateTechniques( Bool supportsExtraStreams, Bool supportsVertexCollapse, Bool supportsUVDissolve );

private:
	TechniqueToCompile* CreateTechniqueInternal( Bool hasExtraStreams, Bool hasVertexCollapse, Bool supportsUVDissolveSeparateUV );
	void CreateTechnique( Bool hasExtraStreams, Bool hasVertexCollapse, Bool supportsUVDissolveSeparateUV );
	void CreateInstancedTechnique( Bool hasExtraStreams, Bool hasVertexCollapse, Bool supportsParticlesInstancing, Bool supportsUVDissolveSeparateUV );

private:
	TDynArray< TechniqueToCompile* >&	m_techniques;
	TSortedArray< Uint32 >&				m_alreadyCreatedIds;

	Bool								m_isInstanced;
	ERenderingPass						m_renderPass;
	EMaterialVertexFactory				m_vertexFactory;
	Bool								m_isDiscardPass;
	Bool								m_supportsDismemberment;

	TDynArray< Bool >					m_dissolve;
	TDynArray< Bool >					m_extraStreams;
	TDynArray< Bool >					m_vertexCollapse;
	TDynArray< Bool >					m_skinning;
	TDynArray< Bool >					m_particleInstancing;
	TDynArray< Bool >					m_uvDissolve;
	TDynArray< Bool >					m_uvDissolveSeparateUV;
};
