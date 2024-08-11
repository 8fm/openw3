/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderProxyDrawable.h"

class CRenderQuery;

/// Rendering proxy for mesh
class CRenderProxy_Flare : public IRenderProxyDrawable
{
private:
	struct SFlareState
	{
		SFlareState ();
		~SFlareState ();

		Bool IsInit() const;
		Bool Init( Bool useMultipleQueryObjects );
		void Shut();
		void SimulateAlpha( const SFlareParameters &params, Float timeDelta );
		void Update( const SFlareParameters &params, Float timeDelta );
		CRenderQuery* RequestQueryToDraw( Bool fullShapePass );
		void NotifyOcclusionTestsActive( Bool isActive );

		Bool						m_useMultipleQueryObjects;
		Float						m_currentAlpha;			//!< Flare alpha
		Float						m_currentAlphaSpeed;	//!< Alpha change speed
		Float						m_targetAlpha;			//!< Flare target alpha
		Int32						m_occlusionTestActiveState;
		CRenderQuery*				m_queryRefFull;			//!< Occlusion query ref
		CRenderQuery*				m_queryRefPart;			//!< Occlusion query ref
	};

protected:
	CRenderMaterialParameters*	m_materialParams;		//!< Material parameters
	CRenderMaterial*			m_material;				//!< Material
	SFlareParameters			m_parameters;			//!< Parameters
	SFlareState					m_state;				//!< State
	Int32						m_activeFlareIndex;		//!< Active flare index (in sync with scene)
	
public:
	static Vector	CalculateFlareSourcePosition( const CRenderProxy_Flare &flare, const CRenderFrameInfo &info );
	static Matrix	CalculateFlareOcclusionMatrix( const CRenderProxy_Flare &flare, const CRenderFrameInfo &info );
	static Float	GetFlareCategoryScaleBase( EFlareCategory category );
	static Float	GetFlareCategorySize( EFlareCategory category, const CRenderFrameInfo &info );

public:
	CRenderProxy_Flare( const RenderProxyInitInfo& initInfo );
	virtual ~CRenderProxy_Flare();

	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	//! Collect elements for rendering
	virtual void CollectElements( CRenderCollector& collector ) override;

public:
	//! Detach from scene
	virtual void DetachFromScene( CRenderSceneEx* scene ) override;

	virtual Bool Register( CRenderElementMap* reMap ) override;
	virtual Bool Unregister( CRenderElementMap* reMap );

public:
	//! Get active flare index
	void SetActiveFlareIndex( Int32 activeFlareIndex ) { m_activeFlareIndex = activeFlareIndex; }

	//! Get last visible frame
	RED_INLINE Int32 GetActiveFlareIndex() const { return m_activeFlareIndex; }

public:
	//! Return alpha
	Float GetCurrentAlpha() const { return m_state.m_currentAlpha; }

	//! Draws query geometry
	void DrawOcclusion( CRenderSceneEx* scene, Bool fullShapePass, const CRenderFrameInfo& info );

	//! Draws occlusion shape
	void DrawOcclusionShape( const CRenderFrameInfo& info );

	//! Update
	void Update( CRenderSceneEx* scene, const CRenderCamera &camera, const CFrustum &frustum, Float timeDelta );

public:
	//! Get parameters
	const SFlareParameters& GetParameters() const { return m_parameters; }

	//! Set parameters
	void SetParameters( const SFlareParameters* parameters ) { m_parameters = *parameters; }

	//! Get material
	CRenderMaterial* GetMaterial() const { return m_material; }

	//! Get material parameters
	CRenderMaterialParameters* GetMaterialParameters() const { return m_materialParams; }
};
