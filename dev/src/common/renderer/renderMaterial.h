/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderDynamicResource.h"
#include "renderHelpers.h"
#include "renderResourceIterator.h"
#include "../engine/materialDefinition.h"

class IRenderElement;
class CRenderShader;
struct MaterialEntry;
class MaterialRenderingContext;
class CHLSLMaterialShaderCompiler;
class CHLSLMaterialCompiler;

struct SCompiledTechniqueParams
{
	Uint32						m_contextID;
	GpuApi::BufferRef			m_vertexParamsBuffer;
	GpuApi::BufferRef			m_pixelParamsBuffer;
};

/// Material parameters
class CRenderMaterialParameters : public IDynamicRenderResource, public TRenderResourceListWithCache< CRenderMaterialParameters >
{
	DECLARE_RENDER_RESOURCE_ITERATOR;

public:
	TDynArray< Uint8, MC_RenderData >	m_pixelData;		// Compiled parameters
	TDynArray< Uint8, MC_RenderData >	m_vertexData;		// Compiled parameters
	TDynArray< IRenderResource* >		m_textures;			// Referenced textures
	CRenderMaterialParameters*			m_batchNext;		// Next in batch
	IRenderElement*						m_batchList;		// Per material batch list

	// Cube texture overrides
	Uint8								m_dataCubeTextureOffset; // Cube texture offset in m_data
	Uint8								m_textureArrayCubeTextureOffset; // Cube texture offset in m_textures

	Bool								m_isMasked;

protected:
	TDynArray< SCompiledTechniqueParams, MC_RenderData > m_techniqueParams;

	static THashMap< Uint32, GpuApi::BufferRef >	m_sharedParamBuffers;
	static Red::Threads::CMutex						m_sharedParamBuffersMutex;
public:
	static GpuApi::BufferRef GetSharedParamBuffer( const void* data, Uint32 dataSize );
	static void SafeReleaseSharedParamBuffer( GpuApi::BufferRef& buffer );
	
protected:
	CRenderMaterialParameters( const IMaterial* material );
	CRenderMaterialParameters( const CRenderMaterialParameters& paramsToCopy );

public:
	virtual ~CRenderMaterialParameters();

	RED_FORCE_INLINE Bool IsMasked() const { return m_isMasked; }

	// Create material parameters
	static CRenderMaterialParameters* Create( const IMaterial* material, Uint64 partialRegistrationHash );

	// Cube texture replacement in material parameters - for instances of meshes with the same material
	void ReplaceCubeTexture( CCubeTexture* cubeTexture );

	// Has cube map?
	Bool HasCubeTexture();

	// Describe resource
	virtual CName GetCategory() const;

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;

	// On device lost/reset
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();

public:
	void CompileDataBuffer( const IMaterialDefinition* definition, const IMaterial* material, EMaterialShaderTarget shaderTarget );

	void Bind( Uint32 contextID, ERenderShaderType shaderType ) const;

	GpuApi::BufferRef GetParamsBuffer( Uint32 contextID, ERenderShaderType shaderType ) const;
	void SetParamsBuffer( Uint32 contextID, ERenderShaderType shaderType, GpuApi::BufferRef bufferRef );

protected:
	//! Setup parameters. Assumed to only be called during construction.
	void SetupParameters( const IMaterial* material );

};

/// Render material flags
enum ERenderMaterialFlags
{
	RMF_TwoSided					= FLAG( 0 ),
	RMF_TwoSidedLighting			= FLAG( 1 ),	//!< not used anymore
	RMF_ColorShift					= FLAG( 2 ),
	RMF_EffectParam0				= FLAG( 3 ),
	RMF_EffectParam1				= FLAG( 4 ),
	RMF_Emissive					= FLAG( 5 ),
	RMF_Forward						= FLAG( 6 ),
	RMF_AccumRefracted				= FLAG( 7 ),
	RMF_Overlay						= FLAG( 9 ),	//!< not used anymore
	RMF_OverlaySecondPass			= FLAG( 10 ),	//!< not used anymore
	RMF_MimicMaterial				= FLAG( 11 ),
	RMF_UsesFoliageColor			= FLAG( 12 ),
	RMF_Skin						= FLAG( 13 ),
	RMF_CanUseOnMeshes				= FLAG( 14 ),
	RMF_CanUseOnParticles			= FLAG( 15 ),
	RMF_ReflectiveMasked			= FLAG( 16 ),
	RMF_Eye							= FLAG( 17 ),
	RMF_SkinnedInstancing			= FLAG( 18 ),
	RMF_CanUseOnMorphMeshes			= FLAG( 19 ),
};

/// Material
class CRenderMaterial : public CRenderMaterialParameters
{
public:
	static TDynArray< CRenderMaterial* >	st_currentlyRecompilingMaterials;	//!< Materials that are currently recompiling (have issued recompilation jobs)
	static Red::Threads::CMutex				st_recompileMutex;					//!< Mutex for accessing the recompiling materials list

public:
	typedef TDynArray< TPair< Uint32, GpuApi::SamplerStateRef > > TSamplerStateArray;

public:
	/// Info about parameter used by rendering technique
	class UsedParameter
	{
	public:
		IMaterialDefinition::EParameterType	m_type;					//!< Type of parameter
		Uint8								m_offset;				//!< Offset in parameter table
		Uint8								m_register;				//!< Register we should bind the parameter to

	public:
		RED_INLINE UsedParameter()
			: m_type( IMaterialDefinition::PT_None )
			, m_offset( 0 )
		{};

		RED_INLINE UsedParameter( IMaterialDefinition::EParameterType type, Uint8 offset, Uint8 registerIndex )
			: m_type( type )
			, m_offset( offset )
			, m_register( registerIndex )
		{
		};
	};
	typedef TDynArray< UsedParameter, MC_RenderData > TUsedParameterArray;

	// Compiled material version for specific material context
	class CompiledTechnique
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_RenderData );

	protected:
		CRenderShader*				m_pixelShader;
		CRenderShader*				m_vertexShader;
		CRenderShader*				m_hullShader;
		CRenderShader*				m_domainShader;
		TUsedParameterArray			m_vertexParameters;
		TUsedParameterArray			m_pixelParameters;
		TSamplerStateArray			m_psSamplerStates;			//!< Sampler states to bind for PS stage
		TSamplerStateArray			m_vsSamplerStates;			//!< Sampler states to bind for VS stage

	public:
		CompiledTechnique( CRenderShader* pixelShader, CRenderShader* vertexShader, const TUsedParameterArray& pixelParameters, const TUsedParameterArray& vertexParameters, const TSamplerStateArray& psSamplerStates, const TSamplerStateArray& vsSamplerStates, CRenderShader* hullShader = NULL, CRenderShader* domainShader = NULL );
		~CompiledTechnique();

		// Bind shader and parameters
		void Bind( const MaterialRenderingContext& context );
		Bool BindParams( const MaterialRenderingContext& context, const CRenderMaterialParameters* params, Float distance );
		
		// Unbind parameters ( textures mostly )
		void UnbindParams();

	protected:
		Bool SetParamsForShaderType( const CRenderMaterialParameters* params, const TSamplerStateArray& samplerStates, ERenderShaderType shaderType, Uint32 contextID, Float distance );
	};

	class MaterialCompilationJob : public CTask
	{
	protected:
		static Red::Threads::CAtomic< Uint32 > m_currentlyProcessingCount;

	public:
		MaterialCompilationJob( CRenderMaterial* material, const MaterialRenderingContext& context );
		virtual ~MaterialCompilationJob();

		RED_INLINE Uint32 GetContextID() const { return m_context->CalcID(); }

	public:
		RED_INLINE CompiledTechnique*	GetCompiledTechnique() const	{ return m_technique; }

		static Uint32 GetCurrentlyProcessingCount() { return m_currentlyProcessingCount.GetValue(); }
		static void IncreaseProcessingCount() { m_currentlyProcessingCount.Increment(); }
		static CompiledTechnique* DoCompilation( CRenderMaterial* material, const MaterialRenderingContext& context );

	protected:
		virtual void Run();

#ifndef NO_DEBUG_PAGES
	public:
		//! Get short debug info
		virtual const Char* GetDebugName() const { return TXT("Material compilation."); }

		//! Get debug color
		virtual Uint32 GetDebugColor() const { return Color::RED.ToUint32(); }
#endif

	private:
		MaterialRenderingContext*	m_context;
		RenderingContext*			m_renderingContext;
		CompiledTechnique*			m_technique;
		CRenderMaterial*			m_material;
	};

private:
	static Red::Threads::CMutex						m_techniquesMutex;			//!< Mutex for compiled techniques.

	Uint32											m_engineMaterialHash;

	// DO NOT ACCESS
	THandle< IMaterialDefinition >					m_engineMaterial;			//!< Source material
	String											m_engineMaterialName;		//!< Name of the source material ( debug )
#ifndef NO_ASYNCHRONOUS_MATERIALS
	THashMap< Uint32, MaterialCompilationJob* >		m_compilationJobs;			//!< Asynchronous compilation jobs
#endif // NO_ASYNCHRONOUS_MATERIALS
	CRCMap											m_crcMap;

public:
	THashMap< Uint32, CompiledTechnique* >			m_techniques;				//!< Compiled material versions
	THashMap< Uint32, Bool >						m_techniquesValid;			//!< Did the technique succeeded compiling
	IMaterialDefinition::TParameterArray			m_pixelParameters;			//!< Dynamic parameters
	IMaterialDefinition::TParameterArray			m_vertexParameters;			//!< Dynamic parameters
	ERenderingSortGroup								m_sortGroup;				//!< Cached sort group
	Uint32											m_flags;					//!< Cached flags

public:
	//! Is this material two sided ?
	RED_INLINE Bool IsTwoSided() const { return 0 != ( m_flags & RMF_TwoSided ); }

	//! Is this material using color shift ?
	RED_INLINE Bool IsUsingColorShift() const { return 0 != ( m_flags & RMF_ColorShift ); }

	//! Is this material using effect param 0
	RED_INLINE Bool IsUsingEffectParam0() const { return 0 != ( m_flags & RMF_EffectParam0 ); }

	//! Is this material using effect param 1
	RED_INLINE Bool IsUsingEffectParam1() const { return 0 != ( m_flags & RMF_EffectParam1 ); }

	//! Is this emissive material
	RED_INLINE Bool IsEmissive() const { return 0 != ( m_flags & RMF_Emissive ); }

	//! Is this forward material
	RED_INLINE Bool IsForward() const { return 0 != ( m_flags & RMF_Forward ); }

	//! Is this material accumulatively refracted
	RED_INLINE Bool IsAccumulativelyRefracted() const { return 0 != ( m_flags & RMF_AccumRefracted ); }

	//! Is this material reflective masked
	RED_INLINE Bool IsReflectiveMasked() const { return 0 != ( m_flags & RMF_ReflectiveMasked); }

	//! Is this material mimic related
	RED_INLINE Bool IsMimicMaterial() const { return 0 != ( m_flags & RMF_MimicMaterial ); }

	//! Material is using foliage color
	RED_INLINE Bool IsUsingFoliageColor() const { return 0 != ( m_flags & RMF_UsesFoliageColor ); }

	//! Material is skin
	RED_INLINE Bool IsSkin() const { return 0 != ( m_flags & RMF_Skin ); }

	//! Material is eye
	RED_INLINE Bool IsEye() const { return 0 != ( m_flags & RMF_Eye ); }

	RED_INLINE Bool CanUseOnMeshes() { return 0 != ( m_flags & RMF_CanUseOnMeshes ); }
	RED_INLINE Bool CanUseOnParticles() { return 0 != ( m_flags & RMF_CanUseOnParticles ); }
	RED_INLINE Bool CanUseSkinnedInstancing() { return 0 != ( m_flags & RMF_SkinnedInstancing ); }
	RED_INLINE Bool CanUseOnMorphMeshes() { return 0 != ( m_flags & RMF_CanUseOnMorphMeshes ); }

	//! Get cached sort group
	RED_INLINE ERenderingSortGroup GetRenderSortGroup() const { return m_sortGroup; }

protected:
	CRenderMaterial( const IMaterialDefinition* material );

public:
	virtual ~CRenderMaterial();

	// Describe resource
	virtual CName GetCategory() const;

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;

	// Get displayable name
	virtual String GetDisplayableName() const { return m_engineMaterialName; }

	// Create material
	static CRenderMaterial* Create( const IMaterialDefinition *definition, Uint64 partialRegistrationHash );

	// Bind material for given material context - binds shaders and buffers, call as rarely as possible
	Bool Bind( const MaterialRenderingContext& context, const CRenderMaterialParameters* params, Float distance );

	Bool BindShaders( const MaterialRenderingContext& context, CRenderMaterial::CompiledTechnique*& techniqueBound );
	Bool BindParams( const MaterialRenderingContext& context, const CRenderMaterialParameters* params, CRenderMaterial::CompiledTechnique* technique, Float distance );

	// Check if this material can be used to replace another material (e.g. for mimics).
	Bool CanReplace( CRenderMaterial* material );

#ifndef NO_ASYNCHRONOUS_MATERIALS
	Bool ClearCompilationJobs();
#endif // NO_ASYNCHRONOUS_MATERIALS

protected:
	// Setup from the definition. Assumed to only be called during construction.
	void SetupParameters( const IMaterialDefinition* material );

	// Compile rendering technique
	CompiledTechnique* CompileTechnique( const MaterialRenderingContext& context );

	CompiledTechnique* GetTechniqueFromCache( const MaterialRenderingContext& context );
	CompiledTechnique* CreateTechniqueFromCache( const MaterialEntry* entry, const MaterialRenderingContext& context, const String& filename );

	// Compile fallback technique ( represents a failed shader compilation )
	static CompiledTechnique* CompileFallback( const MaterialRenderingContext& context );

	void CreateSamplerStates( const TDynArray< SamplerStateInfo, MC_MaterialSamplerStates>& samplerStateInfos, TSamplerStateArray& outSamplerStates );
	void CreateSamplerStates( const MaterialEntry* materialEntry, TSamplerStateArray& outVSSamplerStates, TSamplerStateArray& outPSSamplerStates );

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	void CreateSamplerStates( CHLSLMaterialShaderCompiler* compiler, TSamplerStateArray& outSamplerStates );
	void CreateSamplerStates( CHLSLMaterialCompiler& compiler, TSamplerStateArray& outVSSamplerStates, TSamplerStateArray& outPSSamplerStates );

public:
	void ClearTechniquesFromCache();
#endif //NO_RUNTIME_MATERIAL_COMPILATION
};
