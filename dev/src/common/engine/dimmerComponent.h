/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "drawableComponent.h"
#include "renderSettings.h"

class IRenderProxy;

enum EDimmerType : CEnum::TValueType
{
	DIMMERTYPE_Default,
	DIMMERTYPE_InsideArea,
	DIMMERTYPE_OutsideArea,
};

BEGIN_ENUM_RTTI( EDimmerType );
	ENUM_OPTION( DIMMERTYPE_Default );
	ENUM_OPTION( DIMMERTYPE_InsideArea );
	ENUM_OPTION( DIMMERTYPE_OutsideArea );
END_ENUM_RTTI();

/// Dimmer component
class CDimmerComponent : public CDrawableComponent
{
	DECLARE_ENGINE_CLASS( CDimmerComponent, CDrawableComponent, 0 );

protected:
	EDimmerType					m_dimmerType;
	Bool						m_isAreaMarker;
	Float						m_ambientLevel;
	Float						m_marginFactor;
	Float						m_autoHideDistance;

public:
	CDimmerComponent();
	virtual ~CDimmerComponent();

	virtual Float GetAutoHideDistance() const;
	virtual Float GetDefaultAutohideDistance() const;
	virtual Float GetMaxAutohideDistance() const;

#ifdef USE_UMBRA
	virtual Uint32 GetOcclusionId() const override;
#endif // USE_UMBRA

public:
	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

#ifdef USE_UMBRA
	virtual Bool ShouldBeCookedAsOcclusionData() const { return true; }
	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds );
#endif

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	virtual Uint32 GetMinimumStreamingDistance() const override;

	// Update world space bounding box
	virtual void OnUpdateBounds();
	
	// OnSerialize
	virtual void OnSerialize( IFile& file );

public:
	EDimmerType GetDimmerType() const { return m_dimmerType; }
	Bool IsAreaMarker() const { return m_isAreaMarker; }

	void SetDimmerType( EDimmerType type ) { m_dimmerType = type; }
	void SetAreaMarker( Bool val ) { m_isAreaMarker = val; }

	Float GetAmbientLevel() const { return m_ambientLevel; }
	Float GetMarginFactor() const { return m_marginFactor; }

	void SetAmbientLevel( Float val ) { m_ambientLevel = val; }
	void SetMarginFactor( Float val ) { m_marginFactor = val; }
};

BEGIN_CLASS_RTTI( CDimmerComponent );
	PARENT_CLASS( CDrawableComponent );
	PROPERTY_EDIT( m_isAreaMarker, TXT("Area marker") );
	PROPERTY_EDIT( m_dimmerType, TXT("Dimmer type") );
	PROPERTY_EDIT( m_ambientLevel, TXT("Ambient level (for interior markers value -1 means that the same ambient as for interior volume based markers should be used)") );
	PROPERTY_EDIT( m_marginFactor, TXT("Margin factor") );
	PROPERTY_EDIT( m_autoHideDistance, TXT( "Auto hide distance. -1 will set default value from ini file" ) );
END_CLASS_RTTI();
