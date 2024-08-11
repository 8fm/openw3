/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "component.h"

// Changing this without care may corrupt some stuff !!
#define NUM_NORMALBLEND_AREAS 16

class CNormalBlendComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CNormalBlendComponent, CComponent, 0 );

protected:
	THandle< IMaterial >		m_sourceMaterial;
	THandle< ITexture >			m_sourceNormalTexture;

	THandle< CMaterialInstance >	m_normalBlendMaterial;
	TDynArray< Vector >				m_normalBlendAreas;

	INormalBlendDataSource* m_dataSource;

	Bool					m_useMainTick;

protected:
	Bool					m_isConnectedToMesh;

#ifndef NO_EDITOR
	/// HACK: When a new normalBlendMaterial is set, we want to map the attached materials' parameters to the normal-blend
	/// material's parameters. However, after the OnPropertyPostChange handler for normalBlendMaterial is done executing,
	/// all render proxies are re-created
	Bool m_shouldRemapParameters;

	/// Find as many parameter matches between our normal-blend material and the materials we are replacing, to save
	/// artists from having to set everything. The matching operates according to name and type (exact matches).
	void MapMaterialParameters();
#endif
	void ClearWeights();
	void UpdateWeights();

	/// Get and setup a child NB Attachment. The cached attachment will simply be the first child attachment, if it is a
	/// CNormalBlendAttachment. If the first child is not NB Attachment, then nothing can be cached. We will attempt to match
	/// material parameters between the attached mesh and the normalblend material. The normalblend material and areas will
	/// be given to the cached attachment.
	void CacheNBAttachment();
	void ResetCachedNBAttachment();

	/// If we've previously cached a child NB Attachment, this will return it.
	CNormalBlendAttachment* GetCachedNBAttachment();

	/// Check if we've cached a child NB Attachment.
	Bool HasCachedNBAttachment() const;

	/// Check if we have a child NB Attachment that could be cached.
	Bool CheckCanCacheNBAttachment() const;

public:
	CNormalBlendComponent();
	virtual ~CNormalBlendComponent();

	virtual void OnPropertyPreChange( IProperty* property );
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual void OnChildAttachmentAdded( IAttachment* attachment );
	virtual void OnChildAttachmentBroken( IAttachment* attachment );

	virtual void OnAppearanceChanged( Bool added ) override;
	virtual void OnStreamIn() override;

	void OnRenderProxiesChanged();

	void SetAreas( const TDynArray< Vector >& areas );

public: 
	virtual void OnTickPostUpdateTransform( Float dt );

	void UpdateDataManually( const Float* data );

	virtual bool UsesAutoUpdateTransform() override { return false; }
};

BEGIN_CLASS_RTTI( CNormalBlendComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_INLINED( m_dataSource, TXT("Controls how the blend weights are generated") );
	PROPERTY_EDIT( m_useMainTick, TXT("Use main tick, dataSource can use mainThreadUpdate") );
	PROPERTY_EDIT( m_sourceMaterial, TXT("Base material that will be replaced with the normal-blend material") );
	PROPERTY_EDIT( m_sourceNormalTexture, TXT("Only materials using this texture will be replaced with the normal-blend material") );
	PROPERTY_INLINED( m_normalBlendMaterial, TXT("Normal-blend material, will replace the materials used by the mesh") );
	PROPERTY_EDIT( m_normalBlendAreas, TXT("Normal-blend areas. Don't add/remove, there must be 16 areas.") );
END_CLASS_RTTI();
