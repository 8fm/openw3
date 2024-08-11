/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "envProbeParams.h"
#include "component.h"
#include "../core/deferredDataBuffer.h"

class IRenderResource;
class ICookerFramework;

/// Environment Probe component, used for image based lighting data generation and storage.
class CEnvProbeComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CEnvProbeComponent, CComponent, 0 );

protected:
	const Uint32		m_debugId;
	Uint32				m_nestingLevel;
	DeferredDataBuffer	m_facesData;
	Vector				m_genOrigin;
	IRenderResource		*m_renderResource;
	Vector				m_areaMarginFactor;
	Vector				m_areaDisplace;
	Float				m_contribution;
	Float				m_effectIntensity;
	Bool				m_isParallaxCorrected;
	EngineTransform		m_parallaxTransform;
	SEnvProbeGenParams	m_genParams;

	EnvProbeDataSourcePtr	m_dataSource;

	TStaticArray< Uint32, ENVPROBEBUFFERTEX_MAX >	m_textureCacheHashes;		// For cooked texture data

protected:
	void CommitProbeParamsChangedCommand();

public:
	CEnvProbeComponent();
	~CEnvProbeComponent();

	/// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	/// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	/// Object serialization interface
	virtual void OnSerialize( IFile& file );

	/// On property missing
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );
	
	/// Update component transform
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;
	
	//! Property was changed in editor
	virtual void OnPropertyPostChange( IProperty* property );

	/// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

public:
#ifndef NO_RESOURCE_COOKING
	/// Cooking
	virtual void OnCook( ICookerFramework& cooker ) override;
#endif
#ifndef NO_TEXTURECACHE_COOKER
	void DumpToTextureCache( ECookingPlatform platform ) const;
	String GenerateTextureCachePath() const;
	Uint32 GenerateTextureCacheHash( eEnvProbeBufferTexture tex ) const;
private:
	Bool ExtractEnvProbesToTextureCache( ECookingPlatform platform, const BufferHandle& loadedData ) const;
public:
#endif

	Vector GetProbeOrigin() const;
	Bool SetFacesBuffers( const Vector &genOrigin, Uint32 dataSize, const void *dataBuffer );
	void CreateRenderResource( Bool needsRecreate );
	void ReleaseRenderResource();
	SEnvProbeParams BuildProbeParams() const;
	Bool IsParallaxCorrected() const;
	Matrix BuildAreaLocalToWorld() const;
	Matrix BuildParallaxLocalToWorld() const;

	const TStaticArray< Uint32, ENVPROBEBUFFERTEX_MAX >& GetDataSourceTextureCacheHashes() const { return m_textureCacheHashes; }
	const DeferredDataBuffer& GetDataSourceDataBuffer() const { return m_facesData; }

	const EnvProbeDataSourcePtr& GetDataSource() const { return m_dataSource; }
};

BEGIN_CLASS_RTTI( CEnvProbeComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_contribution, TXT("m_contribution") );
	PROPERTY_EDIT( m_nestingLevel, TXT("m_nestingLevel") );
	PROPERTY_EDIT( m_effectIntensity, TXT("m_effectIntensity") );
	PROPERTY_EDIT( m_areaMarginFactor, TXT("m_areaMarginFactor") );
	PROPERTY_EDIT( m_areaDisplace, TXT("m_areaDisplace") );
	PROPERTY_EDIT( m_isParallaxCorrected, TXT("m_isParallaxCorrected") );
	PROPERTY_EDIT( m_parallaxTransform, TXT("m_parallaxTransform") );
	PROPERTY_EDIT( m_genParams, TXT("Generation params") );
	PROPERTY( m_textureCacheHashes );
END_CLASS_RTTI();
