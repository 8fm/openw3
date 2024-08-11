/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "materialCooker.h"
#include "shaderCompiler.h"
#include "../../common/gpuApiUtils/gpuApiShaderParsing.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/diskFile.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/renderer/renderHelpers.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/shaderCache.h"
#include "../../common/engine/staticShaderCache.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/engine/materialCompilerDefines.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/materialRootBlock.h"
#include "../../common/engine/particleSystem.h"
#include "../../common/engine/particleEmitter.h"
#include "../../common/engine/particleDrawer.h"
#include "../../common/engine/hairworksHelpers.h"
#include "wccStats.h"

static Red::Threads::CMutex st_statsMutex;


CMaterialCooker::CMaterialCooker()
#ifdef USE_NVIDIA_FUR
	: m_hairSDK( nullptr )
#endif
{
}

CMaterialCooker::~CMaterialCooker()
{
#ifdef USE_NVIDIA_FUR
	RED_ASSERT( m_hairSDK == nullptr, TXT("Still have hairworks SDK. Should have called FinishFurCooking!") );
	HairWorksHelpers::ShutdownSDK( m_hairSDK );
#endif
}


void CMaterialCooker::BuildFastFXMap( TFastFXMap& map )
{
	const String fxDirPath( TXT("fx\\") );

	// Find all particle systems in fx/ folder and iterate their emitters along with vertex factories required
	CDirectory* fxDirectory = GDepot->FindPath( fxDirPath );

	TDynArray< String > particleSystemsPaths;
	fxDirectory->FindResourcesByExtension( CParticleSystem::GetFileExtension(), particleSystemsPaths );

	for ( auto path : particleSystemsPaths )
	{
		ResourceLoadingContext context;
		CResource* psResource = GDepot->LoadResource( path, context );
		RED_ASSERT( psResource );
		CParticleSystem* particleSystem = SafeCast< CParticleSystem >( psResource );
		RED_ASSERT( particleSystem );

		const TDynArray< CParticleEmitter* >& emitters = particleSystem->GetEmitters();
		for ( auto emitter : emitters )
		{
			RED_ASSERT( emitter );
			if ( emitter->IsEnabled() && emitter->GetMaterial() && emitter->GetMaterial()->GetMaterialDefinition() )
			{
				IMaterialDefinition* matDef = emitter->GetMaterial()->GetMaterialDefinition();
				CMaterialGraph* matGraph = Cast< CMaterialGraph >( matDef );
				if ( matGraph )
				{
					const String& materialPath = matGraph->GetDepotPath();

					// FastFX processes only materials from "fx" directory
					if ( materialPath.BeginsWith( fxDirPath ) )
					{
						// Verify what material context we need to compile for.
						const EParticleVertexDrawerType drawerType = emitter->GetParticleDrawer()->GetVertexDrawerType();
						const EMaterialVertexFactory mvf = ParticleVertexDrawerToMaterialVertexFactory( drawerType );

						map[ materialPath ].PushBackUnique( mvf );
					}
				}
			}
		}

		psResource->Discard();
	}
}

Bool CMaterialCooker::CompileMaterial( CMaterialGraph* materialGraph, const MaterialCookingOptions& options, Processors::Entries::CMaterialProcessorEntry* entry /*=nullptr*/ )
{
	// No material graph to compile
	if ( !materialGraph )
	{
		WARN_WCC( TXT("No material graph to compile") );
		return false;
	}
	// No shader cache
	if ( !options.m_shaderCacheCooker || !options.m_staticShaderCacheCooker )
	{
		WARN_WCC( TXT("Unable to cook material '%ls' because there's no shader cache!!!"), materialGraph->GetFriendlyName().AsChar() );
		return true;
	}
	// No blocks ?
	if ( materialGraph->GraphGetBlocks().Empty() )
	{
		WARN_WCC( TXT("Unable to cook material '%ls' because it's empty!!!"), materialGraph->GetFriendlyName().AsChar() );
		return true;
	}

	// Update parameter list
	materialGraph->UpdateParametersList();
	materialGraph->UpdateRenderingParams();

	// Collect techniques to compile
	TDynArray< TechniqueToCompile* > techniquesToCompile;
	CollectTechniques( materialGraph, techniquesToCompile, options.m_materialVertexFactories );

	// stats
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_statsMutex );
		extern CMaterialShadersStats GMaterialShadersStats;
		GMaterialShadersStats.GenerateShaderStat( materialGraph->GetFriendlyName(), techniquesToCompile.Size() );
	}

	// Compile techniques
	LOG_WCC( TXT("'%s': %d techniques to compile"), materialGraph->GetFriendlyName().AsChar(), techniquesToCompile.SizeInt() );
	for ( auto techniqueToCompile : techniquesToCompile )
	{
		Bool shouldBeSaved = false;
		if ( CompileShaderTechnique( options, materialGraph, *techniqueToCompile, &shouldBeSaved ) )
		{
			if ( entry )
			{
				entry->m_doSave |= shouldBeSaved;
			}
		}
	}

	// Cleanup 
	techniquesToCompile.ClearPtr();

	// Done
	return true;
}

Bool CMaterialCooker::CompileMaterialMultithreaded( CMaterialGraph* materialGraph, const MaterialCookingOptions& options, Processors::Entries::CMaterialProcessorEntry* entry /*=nullptr*/ )
{
	// No material graph to compile
	if ( !materialGraph )
	{
		WARN_WCC( TXT("No material graph to compile") );
		return false;
	}
	// No shader cache
	if ( !options.m_shaderCacheCooker || !options.m_staticShaderCacheCooker )
	{
		WARN_WCC( TXT("Unable to cook material '%ls' because there's no shader cache!!!"), materialGraph->GetFriendlyName().AsChar() );
		return true;
	}
	// No blocks ?
	if ( materialGraph->GraphGetBlocks().Empty() )
	{
		WARN_WCC( TXT("Unable to cook material '%ls' because it's empty!!!"), materialGraph->GetFriendlyName().AsChar() );
		return true;
	}

	// Update parameter list
	materialGraph->UpdateParametersList();
	materialGraph->UpdateRenderingParams();

	// Collect techniques to compile
	TDynArray< TechniqueToCompile* > techniquesToCompile;
	CollectTechniques( materialGraph, techniquesToCompile, options.m_materialVertexFactories );
	LOG_WCC( TXT("'%s': %u techniques to compile"), materialGraph->GetFriendlyName().AsChar(), techniquesToCompile.Size() );

	TDynArray< Processors::Entries::CTechniqueProcessorEntry* > techniqueEntries;
	for ( auto ttc : techniquesToCompile )
	{
		Processors::Entries::CTechniqueProcessorEntry* entry = new Processors::Entries::CTechniqueProcessorEntry();
		entry->m_graph = materialGraph;
		entry->m_technique = ttc;
		techniqueEntries.PushBack( entry );
	}

	// Compile techniques
	Processors::CTechniqueProcessor processor( this, options );
	auto params = CParallelForTaskSingleArray< Processors::Entries::CTechniqueProcessorEntry*, Processors::CTechniqueProcessor >::SParams::Create();
	{
		params->m_array				= techniqueEntries.TypedData();
		params->m_numElements		= techniqueEntries.Size();
		params->m_processFunc		= &Processors::CTechniqueProcessor::Compute;
		params->m_processFuncOwner	= &processor;
		params->m_priority			= TSP_Critical;
		params->SetDebugName		( TXT("MaterialCooker") );
	}
	params->ProcessNow();
	params->Release();

	// Cleanup 
	techniqueEntries.Clear();
	techniquesToCompile.ClearPtr();

	// Done
	return true;
}

////////////////////////////////////////////////////////

CTechniqueLister::CTechniqueLister( TDynArray< TechniqueToCompile* >& techniques, TSortedArray< Uint32 >& alreadyCreatedIds, Bool instanced, ERenderingPass renderPass, EMaterialVertexFactory vertexFactory, Bool isDiscardPass )
	: m_techniques( techniques )
	, m_alreadyCreatedIds( alreadyCreatedIds )
	, m_isInstanced( instanced )
	, m_renderPass( renderPass )
	, m_vertexFactory( vertexFactory )
	, m_isDiscardPass( isDiscardPass )
{
	m_extraStreams.PushBack( false );
	m_vertexCollapse.PushBack( false );
	m_skinning.PushBack( false );
	m_particleInstancing.PushBack( false );
	m_uvDissolve.PushBack( false );
	m_uvDissolveSeparateUV.PushBack( false );
}

void CTechniqueLister::GenerateTechniques( Bool supportsExtraStreams, Bool supportsVertexCollapse, Bool supportsUVDissolve )
{
	if ( m_isInstanced )
	{
		CreateInstancedTechnique(	false /*EXTRASTREAMS*/,
									false /*VERTEXCOLLAPSE*/,
									true /*PARTICLE_INSTANCING*/,
									false /*UV_DISSOLVE_SEPARATE_UV*/ );

		if ( supportsUVDissolve )
		{
			CreateInstancedTechnique( false, false, true, true /*UV_DISSOLVE_SEPARATE_UV*/ );
		}
		
	}

	if ( supportsExtraStreams )		m_extraStreams.PushBack( true );
	if ( supportsVertexCollapse )	m_vertexCollapse.PushBack( true );
	if ( supportsUVDissolve )		m_uvDissolveSeparateUV.PushBack( true );

	for ( Bool extraStreams : m_extraStreams )
		for ( Bool vertexCollapse : m_vertexCollapse )
			for ( Bool uvDissolveSeparateUV : m_uvDissolveSeparateUV )
			{
				if ( m_isInstanced )
				{
					CreateInstancedTechnique( extraStreams, vertexCollapse, false /*PARTICLE_INSTANCING*/, uvDissolveSeparateUV );
				}
				else
				{
					CreateTechnique( extraStreams, vertexCollapse, uvDissolveSeparateUV );
				}
			}
}

TechniqueToCompile* CTechniqueLister::CreateTechniqueInternal( Bool hasExtraStreams, Bool hasVertexCollapse, Bool supportsUVDissolveSeparateUV )
{
	TechniqueToCompile* technique = new TechniqueToCompile( m_renderPass, m_vertexFactory );
	technique->m_materialContext.m_discardingPass = m_isDiscardPass;
	technique->m_materialContext.m_hasExtraStreams = hasExtraStreams;
	technique->m_materialContext.m_hasVertexCollapse = hasVertexCollapse;
	technique->m_materialContext.m_uvDissolveSeparateUV = supportsUVDissolveSeparateUV;
	return technique;
}

void CTechniqueLister::CreateTechnique( Bool hasExtraStreams, Bool hasVertexCollapse, Bool supportsUVDissolveSeparateUV )
{
	TechniqueToCompile* technique = CreateTechniqueInternal( hasExtraStreams, hasVertexCollapse, supportsUVDissolveSeparateUV );
	Uint32 contextID = technique->m_materialContext.CalcID();
	if ( !m_alreadyCreatedIds.Exist( contextID ) )
	{
		//LOG_WCC( TXT("    [%d] -> '%s'"), contextID, technique->m_materialContext.ToString().AsChar() );
		m_alreadyCreatedIds.Insert( contextID );
		m_techniques.PushBack( technique );
	}
	else
	{
		// technique already there, delete the unnecessary one
		//LOG_WCC( TXT("    DUPLICATE [%d] -> '%s'"), contextID, technique->m_materialContext.ToString().AsChar() );
		delete technique;
		technique = nullptr;
	}
}

void CTechniqueLister::CreateInstancedTechnique( Bool hasExtraStreams, Bool hasVertexCollapse, Bool supportsParticlesInstancing, Bool supportsUVDissolveSeparateUV )
{
	TechniqueToCompile* technique = CreateTechniqueInternal( hasExtraStreams, hasVertexCollapse, supportsUVDissolveSeparateUV );
	technique->m_materialContext.m_useInstancing = true;
	technique->m_materialContext.m_useParticleInstancing = supportsParticlesInstancing;

	Uint32 contextID = technique->m_materialContext.CalcID();
	if ( !m_alreadyCreatedIds.Exist( contextID ) )
	{
		//LOG_WCC( TXT("    [%d] -> '%s'"), contextID, technique->m_materialContext.ToString().AsChar() );
		m_alreadyCreatedIds.Insert( contextID );
		m_techniques.PushBack( technique );
	}
	else
	{
		// technique already there, delete the unnecessary one
		//LOG_WCC( TXT("    DUPLICATE [%d] -> '%s'"), contextID, technique->m_materialContext.ToString().AsChar() );
		delete technique;
		technique = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////

void CMaterialCooker::CollectTechniques( CMaterialGraph* material, TDynArray< TechniqueToCompile* >& techniques, const TDynArray< EMaterialVertexFactory >& mvfactories )
{
	if ( !material )
	{
		return;
	}

	TSortedArray< Uint32 > alreadyCreatedIds;

	// Terrain material, very simple case
	if ( material->IsTerrainMaterial() )
	{
		for ( Uint8 discard = 0; discard <= 1; ++discard )
		{
			for ( Uint8 pregenNorms = 0; pregenNorms <= 1; ++pregenNorms )
			{
				TechniqueToCompile* technique;
				technique = new TechniqueToCompile( RP_GBuffer, MVF_TesselatedTerrain );
				technique->m_materialContext.m_useInstancing = true;
				technique->m_materialContext.m_discardingPass = discard == 1;
				technique->m_materialContext.m_pregeneratedMaps = pregenNorms == 1;
				technique->m_materialContext.m_lowQuality = false;
				techniques.PushBack( technique );

				TechniqueToCompile* technique2;
				technique2 = new TechniqueToCompile( RP_GBuffer, MVF_TerrainSkirt );
				technique2->m_materialContext.m_useInstancing = true;
				technique2->m_materialContext.m_discardingPass = discard == 1;
				technique2->m_materialContext.m_pregeneratedMaps = pregenNorms == 1;
				technique2->m_materialContext.m_lowQuality = true;
				techniques.PushBack( technique2 );
			}
		}
		return;
	}

	// Always compile in no lighting mode
	TDynArray< ERenderingPass > renderPasses;

	Bool isDeferred = false;
	Bool isForward = false;
	Bool canBeMasked = false;
	Bool canBeNotMasked = !canBeMasked;

	CMaterialRootBlock* rootBlock = material->GetRootBaseBlock();
	if ( rootBlock )
	{
		canBeNotMasked |= rootBlock->CanOverrideMasked();
		canBeMasked = rootBlock->IsMasked();
		isDeferred = rootBlock->IsDeferred();
		isForward = rootBlock->IsForward();
		rootBlock->GetValidPasses( renderPasses );
	}

	// Collect vertex factories - ALL now, should be filtered in future
	TDynArray< EMaterialVertexFactory > vertexFactories;
	if ( mvfactories.Empty() ) // Guess vertex factories
	{
		if ( material->CanUseOnMeshes() || material->CompileAllTechniques() || material->CanUseOnCollapsableObjects() )
		{
			vertexFactories.PushBack( MVF_MeshStatic );
			if ( material->CanUseSkinning() )
			{
				vertexFactories.PushBack( MVF_MeshSkinned );
			}
		}

		if ( material->CanUseOnDestructionMeshes() )
		{
			vertexFactories.PushBack( MVF_MeshDestruction );
		}
		
		if ( material->CanUseOnApexMeshes() || material->CompileAllTechniques() )
		{
			vertexFactories.PushBack( MVF_ApexWithoutBones );
			vertexFactories.PushBack( MVF_ApexWithBones ); // we need skinning for both destruction and cloth
		}

		// Particles are supported only when non deferred shader is used
		if ( ( !isDeferred && material->CanUseOnParticles() ) || material->CompileAllTechniques() )
		{
			vertexFactories.PushBack( MVF_ParticleBilboard );
			vertexFactories.PushBack( MVF_ParticleParallel );
			vertexFactories.PushBack( MVF_ParticleMotionBlur );
			vertexFactories.PushBack( MVF_ParticleSphereAligned );
			vertexFactories.PushBack( MVF_ParticleVerticalFixed );
			vertexFactories.PushBack( MVF_ParticleTrail );
			vertexFactories.PushBack( MVF_ParticleFacingTrail );
			vertexFactories.PushBack( MVF_ParticleBeam );
			vertexFactories.PushBack( MVF_ParticleScreen );
		}
	}
	else // Vertex factories provided
	{
		vertexFactories.PushBack( mvfactories );
	}

	Bool supportsVertexCollapse = material->CanUseOnCollapsableObjects() || material->CompileAllTechniques();

	// Generate all rendering passes
	for ( Uint8 instancing = 0; instancing <= 1; ++instancing )
	{
		for ( auto renderPass : renderPasses )
		{
			if ( renderPass == RP_HitProxies )
			{
				continue;
			}

			for ( auto vertexFactory : vertexFactories )
			{
				const Bool supportsDismemberment	= DoesMaterialVertexFactorySupportClippingEllipse( vertexFactory );
				const Bool supportsDissolve			= DoesMaterialVertexFactorySupportDissolve( vertexFactory );
				Bool requiresSkinning				= false;
				Bool supportsExtraStreams			= false;

				Bool supportsUVDissolve				= false;

				if ( vertexFactory == MVF_MeshSkinned  || vertexFactory == MVF_MeshDestruction )				supportsExtraStreams = true;
				if ( vertexFactory == MVF_MeshStatic )				supportsExtraStreams = true;
				if ( vertexFactory == MVF_ApexWithoutBones )		supportsExtraStreams = true;
				if ( vertexFactory == MVF_ApexWithBones ) 			supportsExtraStreams = true;
				if ( vertexFactory == MVF_MeshSkinned || vertexFactory == MVF_MeshDestruction )				requiresSkinning = true;
				if ( vertexFactory == MVF_ApexWithBones )			requiresSkinning = true;

				if ( vertexFactory == MVF_MeshSkinned || vertexFactory == MVF_MeshStatic )
				{
					supportsUVDissolve = material->CanUseOnMorphMeshes() || material->CompileAllTechniques();
				}

				Bool supportsInstancing = instancing > 0 && (!requiresSkinning || material->CanUseSkinnedInstancing());

				Bool canBeDiscard = canBeMasked || supportsDissolve || supportsDismemberment || supportsUVDissolve;
				Bool canBeNotDiscard = canBeNotMasked;

				for ( Uint8 discard = 0; discard <= 1; ++discard )
				{
					if ( discard == 0 && !canBeNotDiscard )
					{
						continue;
					}
					if ( discard == 1 && !canBeDiscard )
					{
						continue;
					}

					CTechniqueLister techniqueLister( techniques, alreadyCreatedIds, supportsInstancing, renderPass, vertexFactory, discard == 1 );
					techniqueLister.GenerateTechniques( supportsExtraStreams, supportsVertexCollapse, supportsUVDissolve );
				}
			}
		}
	}
}

static Bool AddShader( IStaticShaderCache* staticShaderCache, Uint64 hash, const DataBuffer& shaderData )
{
	StaticShaderEntry* entry = new StaticShaderEntry();
	entry->m_hash = hash;
	entry->m_data = shaderData;
	staticShaderCache->AddShader( hash, entry );
}

static Bool AddShader( IShaderCache* shaderCache, Uint64 hash, const DataBuffer& shaderData )
{
	ShaderEntry* entry = new ShaderEntry();
	entry->m_hash = hash;
	entry->SetData( shaderData );
	return shaderCache->AddShader( hash, entry ) == IShaderCache::eResult_Valid;
}

Bool CMaterialCooker::CompileShaderTechnique( const MaterialCookingOptions& options, CMaterialGraph* material, TechniqueToCompile& technique, Bool* shouldBeSaved /*=nullptr*/ )
{
	if ( !material || !material->GetFile() )
	{
		return false;
	}

	// Create material compiler
	CHLSLMaterialCompiler compiler( technique.m_materialContext, options.m_platform );
	compiler.GenerateDefaultCode();

	// Compile material
	material->Compile( &compiler );

	// Sort interpolators (so they don't depend on the graph structure)
	compiler.SortAndOutputInterpolators();

	// Finalize data connections
	compiler.CreateDataConnections();

	// Generate CRC parts for final code.
	material->CacheCRCs( &compiler );

	const Uint32 contextId = technique.m_materialContext.CalcID();

	// Generate final CRC.
	Uint64 materialCRC = 0;
	Uint64 includesCRC = CalculateDirectoryCRC( GpuApi::GetShaderIncludePath() );

	Uint32 materialPathHash = Red::System::CalculateAnsiHash32LowerCase( material->GetDepotPath().AsChar() );
	Uint64 materialHash = CalculateMaterialHash( materialPathHash, contextId );

	MaterialEntry* materialEntry = nullptr;
	IShaderCache::EResult result = options.m_shaderCacheCooker->GetMaterial( materialHash, materialEntry );

	// Check if current CRCs have a match in previous cache, if any.
	Bool invalidate = result != IShaderCache::eResult_Valid;
	if ( materialEntry )
	{
		Bool isUpToDate = materialCRC == materialEntry->m_crc;
		isUpToDate &= includesCRC == materialEntry->m_includesCRC;
		invalidate = !isUpToDate;
	}

	if ( !invalidate )
	{
		// up-to-date
		return true;
	}

	if ( shouldBeSaved )
	{
		*shouldBeSaved = true;
	}

	String tempFilename = CFilePath( material->GetFile()->GetFileName() ).GetFileName() + technique.m_materialContext.ToString();
	String message = String::Printf( TXT("Compiled '%s', hash: [%") RED_PRIWu64 TXT("]" ), tempFilename.AsChar(), materialHash );

	// Prepare material defines
	CMaterialCompilerDefines defines;
	defines.InitFromMaterialStates( technique.m_materialContext );
	
	Uint64 psHash = 0;
	Uint64 vsHash = 0;
	Uint64 dsHash = 0;
	Uint64 hsHash = 0;

	const String& friendlyName = material->GetFriendlyName();
	const String& shaderFileName = material->GetFile()->GetFileName();
	const AnsiChar* shaderFileNameAnsi = UNICODE_TO_ANSI( material->GetFile()->GetFileName().AsChar() );
	RED_ASSERT( shaderFileNameAnsi );

	// Setup common material cooking context params
	SMaterialCookingContext context;
	context.m_shaderFileName		= shaderFileName;
	context.m_shaderFileNameAnsi	= shaderFileNameAnsi;
	context.m_definitions			= defines;
	context.m_shadersMask			= VertexShader | PixelShader;
	context.m_dumpAssembly			= options.m_dumpAssembly;
	context.m_isStaticShader		= false;

	if ( material->IsTerrainMaterial() &&  technique.m_materialContext.m_vertexFactory != MVF_TerrainSkirt )
	{
		context.m_shadersMask |= HullShader | DomainShader;
	}

	extern CMaterialShadersStats GMaterialShadersStats;
	// PS
	{
		CStringPrinter code;
		compiler.m_pixelShader->GetFinalCode( code );

		context.m_shaderType = GpuApi::PixelShader;
		SMaterialCookingOutput output;
		ECompilationResult compilationResult = CompileShader( options, code.AsChar(), context, output );
		switch ( compilationResult )
		{
		case ECR_CompilationSuccessful:
			psHash = output.m_shaderHash;
			materialCRC = ACalcBufferHash64Merge( &output.m_shaderCodeCRC, sizeof( output.m_shaderCodeCRC ), materialCRC );
			message += String::Printf( TXT(", PS_CRC: [%") RED_PRIWu64 TXT("]" ), output.m_shaderCodeCRC );
			AddShader( options.m_shaderCacheCooker, psHash, output.m_shaderData );
			if( options.m_collectStats )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_statsMutex );
				GMaterialShadersStats.AddShaderStat( friendlyName.AsChar(), GpuApi::PixelShader, output.m_shaderData.GetSize() );
			}
			break;
		case ECR_CompilationFailed:
			RED_LOG_ERROR( Shaders, TXT("Error compiling PixelShader for shader file '%s', techniqueID: '%d'"), shaderFileName.AsChar(), technique.m_materialContext.CalcID() );
			break;
		case ECR_FoundInCache:
			psHash = output.m_shaderHash;
			RED_LOG( Shaders, TXT("PixelShader for shader file '%ls', techniqueID: '%d' found in cache"), shaderFileName.AsChar(), technique.m_materialContext.CalcID() );
			break;
		}
	}

	// VS
	{
		CStringPrinter code;
		compiler.m_vertexShader->GetFinalCode( code );
		
		context.m_shaderType = GpuApi::VertexShader;
		SMaterialCookingOutput output;
		ECompilationResult compilationResult = CompileShader( options, code.AsChar(), context, output );
		switch ( compilationResult )
		{
		case ECR_CompilationSuccessful:
			vsHash = output.m_shaderHash;
			materialCRC = ACalcBufferHash64Merge( &output.m_shaderCodeCRC, sizeof( output.m_shaderCodeCRC ), materialCRC );
			message += String::Printf( TXT(", VS_CRC: [%") RED_PRIWu64 TXT("]" ), output.m_shaderCodeCRC );
			AddShader( options.m_shaderCacheCooker, vsHash, output.m_shaderData );
			if( options.m_collectStats )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_statsMutex );
				GMaterialShadersStats.AddShaderStat( friendlyName.AsChar(), GpuApi::VertexShader, output.m_shaderData.GetSize() );
			}
			break;
		case ECR_CompilationFailed:
			RED_LOG_ERROR( Shaders, TXT("Error compiling VertexShader for shader file '%s', techniqueID: '%d'"), shaderFileName.AsChar(), technique.m_materialContext.CalcID() );
			break;
		case ECR_FoundInCache:
			vsHash = output.m_shaderHash;
			RED_LOG( Shaders, TXT("VertexShader for shader file '%ls', techniqueID: '%d' found in cache"), shaderFileName.AsChar(), technique.m_materialContext.CalcID() );
			break;
		}
	}

	if ( material->IsTerrainMaterial() && compiler.GetContext().m_vertexFactory != MVF_TerrainSkirt )
	{
		// HS
		{
			CStringPrinter code;
			compiler.m_vertexShader->GetFinalCode( code );

			context.m_shaderType = GpuApi::HullShader;
			SMaterialCookingOutput output;
			ECompilationResult compilationResult = CompileShader( options, code.AsChar(), context, output );
			switch ( compilationResult )
			{
			case ECR_CompilationSuccessful:
				hsHash = output.m_shaderHash;
				materialCRC = ACalcBufferHash64Merge( &output.m_shaderCodeCRC, sizeof( output.m_shaderCodeCRC ), materialCRC );
				message += String::Printf( TXT(", HS_CRC: [%") RED_PRIWu64 TXT("]" ), output.m_shaderCodeCRC );
				AddShader( options.m_shaderCacheCooker, hsHash, output.m_shaderData );
				if( options.m_collectStats )
				{
					Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_statsMutex );
					GMaterialShadersStats.AddShaderStat( friendlyName.AsChar(), GpuApi::HullShader, output.m_shaderData.GetSize() );
				}
				break;
			case ECR_CompilationFailed:
				RED_LOG_ERROR( Shaders, TXT("Error compiling HullShader for shader file '%s', techniqueID: '%d'"), shaderFileName.AsChar(), technique.m_materialContext.CalcID() );
				break;
			case ECR_FoundInCache:
				hsHash = output.m_shaderHash;
				RED_LOG( Shaders, TXT("HullShader for shader file '%ls', techniqueID: '%d' found in cache"), shaderFileName.AsChar(), technique.m_materialContext.CalcID() );
				break;
			}
		}

		// DS
		{
			CStringPrinter code;
			compiler.m_vertexShader->GetFinalCode( code );

			context.m_shaderType = GpuApi::DomainShader;
			SMaterialCookingOutput output;
			ECompilationResult compilationResult = CompileShader( options, code.AsChar(), context, output );
			switch ( compilationResult )
			{
			case ECR_CompilationSuccessful:
				dsHash = output.m_shaderHash;
				materialCRC = ACalcBufferHash64Merge( &output.m_shaderCodeCRC, sizeof( output.m_shaderCodeCRC ), materialCRC );
				message += String::Printf( TXT(", DS_CRC: [%") RED_PRIWu64 TXT("]" ), output.m_shaderCodeCRC );
				AddShader( options.m_shaderCacheCooker, dsHash, output.m_shaderData );
				if( options.m_collectStats )
				{
					Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_statsMutex );
					GMaterialShadersStats.AddShaderStat( friendlyName.AsChar(), GpuApi::DomainShader, output.m_shaderData.GetSize() );
				}
				break;
			case ECR_CompilationFailed:
				RED_LOG_ERROR( Shaders, TXT("Error compiling DomainShader for shader file '%s', techniqueID: '%d'"), shaderFileName.AsChar(), technique.m_materialContext.CalcID() );
				break;
			case ECR_FoundInCache:
				dsHash = output.m_shaderHash;
				RED_LOG( Shaders, TXT("DomainShader for shader file '%ls', techniqueID: '%d' found in cache"), shaderFileName.AsChar(), technique.m_materialContext.CalcID() );
				break;
			}
		}
	}

	message += String::Printf( TXT(", materialCRC: [%") RED_PRIWu64 TXT("]" ), materialCRC );

	CHLSLMaterialShaderCompiler* vsCompiler = static_cast< CHLSLMaterialShaderCompiler* >( compiler.GetVertexShaderCompiler() );
	RED_ASSERT( vsCompiler );
	CHLSLMaterialShaderCompiler* psCompiler = static_cast< CHLSLMaterialShaderCompiler* >( compiler.GetPixelShaderCompiler() );
	RED_ASSERT( psCompiler );

	if ( !materialEntry )
	{
		materialEntry = new MaterialEntry();
	}

	materialEntry->m_hash = materialHash;
	materialEntry->m_crc = materialCRC;
	materialEntry->m_includesCRC = includesCRC;
	materialEntry->m_contextId = contextId;
	materialEntry->m_path = material->GetDepotPath();

	materialEntry->SetShader( GpuApi::PixelShader, psHash );
	materialEntry->SetShader( GpuApi::VertexShader, vsHash );
	materialEntry->SetShader( GpuApi::HullShader, hsHash );
	materialEntry->SetShader( GpuApi::DomainShader, dsHash );

	materialEntry->m_vsSamplerStates = vsCompiler->m_samplerStates;
	materialEntry->m_psSamplerStates = psCompiler->m_samplerStates;
	materialEntry->m_vsUsedParameters = compiler.m_usedVertexParameters;
	materialEntry->m_psUsedParameters = compiler.m_usedPixelParameters;

	if( options.m_shaderCacheCooker->AddMaterial( materialHash, materialEntry ) == IShaderCache::eResult_Valid && options.m_collectStats )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_statsMutex );
		// Sampler states size
		GMaterialShadersStats.AddMaterialGenericStat( friendlyName.AsChar(), materialEntry->m_path.DataSize() );
		GMaterialShadersStats.AddMaterialStat( friendlyName.AsChar(), eMaterialStat::VSSamplerStates, materialEntry->m_vsSamplerStates.Size(), materialEntry->m_vsUsedParameters.Size(), materialEntry->m_vsSamplerStates.DataSize(), materialEntry->m_vsUsedParameters.DataSize() );
		GMaterialShadersStats.AddMaterialStat( friendlyName.AsChar(), eMaterialStat::PSSamplerStates, materialEntry->m_psUsedParameters.Size(), materialEntry->m_psUsedParameters.Size(), materialEntry->m_psSamplerStates.DataSize(), materialEntry->m_psUsedParameters.DataSize() );
	}

	// If the entry exists in the cache it will not be marked as
	// dirty, even if it has changed, so we need to force it here.
	options.m_shaderCacheCooker->ForceDirty( );

	// Compiled
	return true;
}

extern Bool GDebugShaders;

const Char* CMaterialCooker::GetShaderName( eShaderTypeFlags shaderType )
{
	switch ( shaderType )
	{
	case VertexShader:		return TXT("VertexShader");
	case PixelShader:		return TXT("PixelShader");
	case GeometryShader:	return TXT("GeometryShader");
	case HullShader:		return TXT("HullShader");
	case DomainShader:		return TXT("DomainShader");
	case ComputeShader:		return TXT("ComputeShader");
	}
	RED_ASSERT( false, TXT("Unknown shader type") );
	return TXT("");
}

static Bool HasShader( IStaticShaderCache* staticShaderCache, Uint64 hash )
{
	StaticShaderEntry* entry = nullptr;
	return staticShaderCache->GetShader( hash, entry );
}

static Bool HasShader( IShaderCache* shaderCache, Uint64 hash )
{
	ShaderEntry* entry = nullptr;
	return shaderCache->GetShader( hash, entry ) == IShaderCache::eResult_Valid;
}

CMaterialCooker::ECompilationResult CMaterialCooker::CompileShader( const MaterialCookingOptions& options, const AnsiChar* code, const SMaterialCookingContext& context, SMaterialCookingOutput& output )
{
	IShaderCompiler* compiler = IShaderCompiler::Create( options.m_platform, context, options.m_dumpFileName, options.m_dumpDirPath );
	if ( !compiler )
	{
		return ECR_CompilationFailed;
	}

	const TDynArray< GpuApi::ShaderDefine>& defines = compiler->GetDefines();
	output.m_fileNameHash = GpuApi::GetFilenameHash( context.m_shaderFileNameAnsi, defines.TypedData(), defines.Size(), context.m_shaderType );

	AnsiChar* preprocessedCode = nullptr;
	Uint32 preprocessedLength = 0;
	Bool ret = GpuApi::GetShaderHash( output.m_shaderHash, code, compiler->GetEntryPoint(), defines.TypedData(), defines.Size(), context.m_shaderFileNameAnsi, &preprocessedCode, &preprocessedLength, Allocate, Free );
	if ( !ret || !preprocessedCode )
	{
		return ECR_CompilationFailed;
	}

	output.m_shaderCodeCRC = Red::System::CalculateHash64SkipWhitespaces( preprocessedCode, preprocessedLength, 0 );

	if ( HasShader( options.m_shaderCacheCooker, output.m_shaderHash ) || HasShader( options.m_staticShaderCacheCooker, output.m_shaderHash ) )
	{
		Free( preprocessedCode );
		return ECR_FoundInCache;
	}

	ret = compiler->Compile( preprocessedCode, preprocessedLength, output.m_shaderData, output.m_shaderHash );
	Free( preprocessedCode );
	delete compiler;

	return ret ? ECR_CompilationSuccessful : ECR_CompilationFailed;
}
