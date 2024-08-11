/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once 

#include "materialDefinition.h"
#include "graphContainer.h"
#include "renderFrame.h"

class IMaterialParameterSupplier;
class CMaterialParameter;
class CMaterialBlock;
class CMaterialGraphEngineValue;

/// Graph based material
class CMaterialGraph : public IMaterialDefinition, public IGraphContainer
{
	DECLARE_ENGINE_RESOURCE_CLASS( CMaterialGraph, IMaterialDefinition, "w2mg", "Material graph" );

protected:
	TDynArray< CGraphBlock* >				m_blocks;						//!< Material blocks/descriptions
	TDynArray< CMaterialParameter* >		m_pixelParameterBlocks;			//!< Material parameter blocks 
	TDynArray< CMaterialParameter* >		m_vertexParameterBlocks;		//!< Material parameter blocks 
	ERenderingSortGroup						m_sortGroup;					//!< Best sort group to use when rendering this material
	ERenderingBlendMode						m_blendMode;					//!< Blend mode when rendering transparent objects	
	Uint32									m_paramMask;					//!< Parameter mask ( calculated from used engine values )
	Bool									m_isTwoSided;					//!< This material is two sided
	Bool									m_isEmissive;					//!< This material is emissive
	Bool									m_isMasked;						//!< This material is alpha tested
	Bool									m_canOverrideMasked;			//!< Material instances can enabled/disable masking
	Bool									m_isForward;					//!< This material uses forward rendering
	Bool									m_isAccumulativelyRefracted;	//!< This material accumulatively refracted
	Bool									m_isReflectiveMasked;			//!< This material is reflective masked
	Bool									m_isMimicMaterial;				//!< This material is mimic related
	Bool									m_isVolumeRendering;			//!< This material is used for rendering proxy volumes only
	Bool									m_isWaterBlended;
	Bool									m_isEye;						//!< This material is eye
	mutable CRCMap							m_crcMap;						//!< CRC of the HLSL code (VS & PS) generated from the graph for the context

private:
	mutable Red::Threads::CMutex			m_crcMapMutex;

public:
	// Get sort group to use with this material
	RED_INLINE ERenderingSortGroup GetRenderingSortGroup() const { return m_sortGroup; }

	// Get blend mode to use with this material
	RED_INLINE ERenderingBlendMode GetRenderingBlendMode() const { return m_blendMode; }

	// Get material parameter mask
	RED_INLINE Uint32 GetRenderingFragmentParameterMask() const { return m_paramMask; }

	// Is this material two sided ?
	RED_INLINE Bool IsTwoSided() const { return m_isTwoSided; }

	// Is this material using emissive ?
	RED_INLINE Bool IsEmissive() const { return m_isEmissive; }

	// Is this material using mask ?
	RED_INLINE Bool IsMasked() const { return m_isMasked; }

#ifndef NO_EDITOR
	Bool HasAnyBlockParameters() const override { return !m_blocks.Empty(); }
#endif

	// Can an instance using this definition change whether it's masked or not?
	RED_INLINE Bool CanInstanceOverrideMasked() const { return m_canOverrideMasked; }

	// Is this material mimic related ?
	RED_INLINE Bool IsMimicMaterial() const { return m_isMimicMaterial; }

	// Is this material using forward rendering ?
	RED_INLINE Bool IsForwardRendering() const { return m_isForward; }

	// Is this material skin ? [Deprecated]
	RED_INLINE Bool IsSkin() const { return false; }

	// Is this material eye
	RED_INLINE Bool IsEye() const { return m_isEye; }

	// Is this material using accumulative refraction ?
	RED_INLINE Bool IsAccumulativelyRefracted() const { return m_isAccumulativelyRefracted; }	

	// Is this material reflective masked
	RED_INLINE Bool IsReflectiveMasked() const { return m_isReflectiveMasked; }

	RED_INLINE void ClearCRC() { m_crcMap.Clear(); }

	// Is this material used for rendering volumes only
	virtual Bool IsUsedByVolumeRendering() const override { return m_isVolumeRendering || m_sortGroup == RSG_Volumes; }

	virtual Bool TryGetCRC( Uint32 contextId, Uint64& vsCRC, Uint64& psCRC ) const override;

	virtual Uint64 GetVSCRC( Uint32 contextId ) const override
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_crcMapMutex );
		return m_crcMap[ contextId ].m_first;
	}
	virtual Uint64 GetPSCRC( Uint32 contextId ) const override
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_crcMapMutex );
		return m_crcMap[ contextId ].m_second;
	}

	virtual const CRCMap& GetCRCMap() const override { return m_crcMap; }

public:
	// Handle serialized type mismatch
	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;

	// Serialize material
	virtual void OnSerialize( IFile& file ) override;

#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

	// Serialize material
	virtual void OnPostLoad( ) override;

	// Compile material
	virtual void Compile( IMaterialCompiler* compiler ) const override;
	virtual void CacheCRCs( IMaterialCompiler* compiler ) override;

	//! Update list of parameters
	void UpdateParametersList();

	//! Update rendering parameters ( sort group and such )
	virtual void UpdateRenderingParams() override;

	virtual Bool SupportsContext( const MaterialRenderingContext& context ) const override;

	CMaterialRootBlock* GetRootBaseBlock() const;

	Bool IsTerrainMaterial() const;

#endif

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	// Reload
	virtual Bool Reload( Bool confirm );
#endif

public:
	virtual void GetAllParameterNames( TDynArray< CName >& outNames ) const override;

	//! Write material parameter data
	virtual Bool WriteParameterRaw( const CName& name, const void* data, Bool recreateResource = true ) override;

	//! Read parameter data
	virtual Bool ReadParameterRaw( const CName& name, void* data ) const override;

	//! Find graph parameter
	virtual CMaterialParameter* FindParameter( const CName& name, Bool caseSensitive = true );

public:
	//! Get object that owns the graph
	virtual CObject *GraphGetOwner() override;

	//! Get list of blocks
	virtual TDynArray< CGraphBlock* >& GraphGetBlocks() override { return m_blocks; }
	virtual const TDynArray< CGraphBlock* >& GraphGetBlocks() const override { return m_blocks; }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Graph structure has been modified
	virtual void GraphStructureModified() override;

	//! Does this graph supports given class
	virtual Bool GraphSupportsBlockClass( CClass *blockClass ) const override;

	//!! Get background offset
	virtual Vector GraphGetBackgroundOffset() const override;

	//! Set background offset
	virtual void GraphSetBackgroundOffset( const Vector& offset ) override;

#endif
};

BEGIN_CLASS_RTTI( CMaterialGraph );
	PARENT_CLASS( IMaterialDefinition );
	PROPERTY( m_sortGroup );
	PROPERTY( m_blendMode );
	PROPERTY( m_offset );
	PROPERTY( m_blocks );
	PROPERTY( m_pixelParameterBlocks );
	PROPERTY( m_vertexParameterBlocks );
	PROPERTY( m_paramMask );
	PROPERTY( m_isTwoSided );
	PROPERTY( m_isEmissive );
	PROPERTY( m_isMasked );
	PROPERTY( m_canOverrideMasked );
	PROPERTY( m_isForward );
	PROPERTY( m_isAccumulativelyRefracted );
	PROPERTY( m_isReflectiveMasked );
	PROPERTY( m_isEye );
	PROPERTY( m_isMimicMaterial );	
END_CLASS_RTTI();
