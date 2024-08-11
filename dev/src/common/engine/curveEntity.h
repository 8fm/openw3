/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "renderVertices.h"
#include "entity.h"
#include "multiCurve.h"
#include "component.h"
#include "curveEntitySpawner.h"

class CCurveComponent;
class CCurveControlPointEntity;
class CCurveControlPointComponent;
class CCurveTangentControlPointEntity;
class CCurveTangentControlPointComponent;
struct SMultiCurve;
enum ECurveInterpolationMode : CEnum::TValueType;
enum ECurveManualMode : CEnum::TValueType;

//! Curve accessor helper class; handy if SMultiCurve (being just a struct, not CObject) can change its memory address
class ICurveAccessor
{
public:
	virtual ~ICurveAccessor() {}
	virtual SMultiCurve* Get() = 0;
};

//! Default curve accessor
class CDefaultCurveAccessor : public ICurveAccessor
{
public:
	CDefaultCurveAccessor( SMultiCurve* curve ) :
		m_curve( curve )
	{}

	virtual SMultiCurve* Get() override
	{
		return m_curve;
	}

private:
	SMultiCurve* m_curve;
};

//! Listener for any curve selection changes
class ICurveSelectionListener
{
public:
	virtual ~ICurveSelectionListener() {}
	virtual void OnCurveSelectionChanged( CCurveEntity* curveEntity, const TDynArray< Uint32 >& selectedControlPointIndices ) = 0;
};

//! Helper entity used for editing the curve
class CCurveEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CCurveEntity, CEntity, 0 );

public:
	CCurveEntity();
	virtual ~CCurveEntity();

	// Overrides from CEntity
	virtual void OnTick( Float timeDelta ) override;
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnSelectionChanged() override;
	virtual void OnDestroyed( CLayer* layer ) override;

#ifndef NO_EDITOR
	CCurveEntitySpawner* GetCurveEntitySpawner();
#endif

	//! Creates new editable curve entity
	static CCurveEntity* CreateEditor( CWorld* world, ICurveAccessor* curveAccessor, Bool showControlPoints );
	//! Creates new editable curve entity
	static CCurveEntity* CreateEditor( CWorld* world, SMultiCurve* curve, Bool showControlPoints );
	//! Deletes curve 3D editor for a given curve (if one exists); returns true if editor was found
	static Bool DeleteEditor( CWorld* world, SMultiCurve* curve );
	//! Deletes curve 3D editors - either only invalidated ones i.e. ones that return invalid (NULL) curve or all of them
	static void DeleteEditors( CWorld* world, Bool onlyDeleteInvalidated = true );
	//! Refreshes editable curve entity (after curve change) if it exists
	static CCurveEntity* RefreshEditor( CWorld* world, SMultiCurve* curve );
	//! Finds existing curve entity that edits given curve
	static CCurveEntity* FindCurveEditor( CWorld* world, SMultiCurve* curve );
	//! Gets edited curve
	RED_INLINE SMultiCurve* GetCurve() const { return m_curveAccessor->Get(); }
	//! Toggles on/off curve edit mode (with control points and tangent control points)
	void EnableEditMode( Bool enable );

	// Control point manipulation
	CCurveControlPointEntity* AddControlPointAfter( CCurveControlPointEntity* controlPointEntity );
	CCurveControlPointEntity* AddControlPointBefore( CCurveControlPointEntity* controlPointEntity );
	CCurveControlPointEntity* AppendControlPointWorld( const Vector& worldPos );
	void SelectAllControlPoints();
	void SelectPreviousControlPoint( CCurveControlPointEntity* controlPoint );
	void SelectNextControlPoint( CCurveControlPointEntity* controlPoint );
	void SelectControlPointByIndex( Uint32 index, Bool resetSelection = false );

	// Curve properties manipulation
	Bool IsCurveLoopingEnabled() const;
	void SetCurveLooping( Bool loop );
	Bool IsPositionRenderingEnabled() const { return m_showCurvePosition; }
	void EnablePositionRendering( Bool enabled ) { m_showCurvePosition = enabled; }
	Bool IsRotationRenderingEnabled() const { return m_showCurveRotation; }
	void EnableRotationRendering( Bool enabled ) { m_showCurveRotation = enabled; }
	Bool IsDebugObjectCurveRunEnabled() const { return m_enableDebugObjectCurveRun; }
	void EnableDebugObjectCurveRun( Bool enable ) { m_enableDebugObjectCurveRun = enable; m_debugObjectCurveRunTime = 0.0f; }
	Bool IsTimeVisualizationEnabled() const { return m_visualizeCurveTime; }
	void EnableTimeVisualization( Bool enabled ) { m_visualizeCurveTime = enabled; }
	void EnableAutomaticTimeByDistanceRecalculation( Bool enable );
	Bool IsAutomaticTimeByDistanceRecalculationEnabled();
	void RecalculateTimeByDistance( Bool selectionOnly = false );
	void EnableAutomaticTimeRecalculation( Bool enable );
	Bool IsAutomaticTimeRecalculationEnabled();
	void RecalculateTime();
	void EnableAutomaticRotationFromDirectionRecalculation( Bool enable );
	Bool IsAutomaticRotationFromDirectionRecalculationEnabled();
	void RecalculateRotationFromDirection();
	ECurveInterpolationMode GetPositionInterpolationMode();
	void SetPositionInterpolationMode( ECurveInterpolationMode mode );
	ECurveManualMode GetPositionManualMode();
	void SetPositionManualMode( ECurveManualMode mode );
	ECurveInterpolationMode GetRotationInterpolationMode();
	void SetRotationInterpolationMode( ECurveInterpolationMode mode );
	void RecalculateBezierTangents();

	Bool CanDeleteControlPoint();
	void DeleteControlPoint( CCurveControlPointEntity* controlPointEntity );
	void SetControlPointPosition( CCurveControlPointEntity* controlPointEntity, const Vector& pos );
	CCurveControlPointEntity* AddControlPointWorld( const Vector& worldPos );
	CCurveControlPointEntity* AddControlPoint( Uint32 index, const EngineTransform& transform );
	TDynArray<CCurveControlPointEntity*>& GetControlPoints() { return m_controlPointEntities; }
	Uint32 GetControlPointIndex( CCurveControlPointEntity* controlPointEntity );
	void SetControlPointTime( Uint32 index, Float time );

	static void SetHoverControlPoint( CEntity* entity );
	static CEntity* GetHoverControlPoint();

	void SetDrawOnTop( Bool drawOnTop ) { m_drawOnTop = drawOnTop; }
	Bool GetDrawOnTop() const { return m_drawOnTop; }

	Bool GetDrawArrowControlPoints() const { return m_drawArrowControlPoints; }

	void UpdateControlPointTransforms();
	void OnCurveChanged();
	void NotifySelectionChange();

	static void RegisterSelectionListener( ICurveSelectionListener* listener ) { m_selectionListeners.PushBackUnique( listener ); }
	static void UnregisterSelectionListener( ICurveSelectionListener* listener ) { m_selectionListeners.RemoveFast( listener ); }

private:
	static void GetAllEditors( CWorld* world, TDynArray<CCurveEntity*>& curveEntities );
	CCurveControlPointEntity* CreateControlPointEntity( Uint32 index );
	CCurveControlPointEntity* AddControlPointWorld( Uint32 index, const Vector* worldPos = NULL );
	void Setup( ICurveAccessor* curveAccessor );
	void OnControlPointMoved( CCurveControlPointEntity* controlPointEntity );
	void OnControlPointDeleted( CCurveControlPointEntity* controlPointEntity );
	void OnTangentControlPointMoved( CCurveControlPointEntity* controlPointEntity, CCurveTangentControlPointEntity* tangentControlPointEntity, Uint32 tangentIndex );
	void UpdateControlPointTransform( CCurveControlPointEntity* controlPointEntity, Uint32 index );
	void AddDebugDrawSegment( TDynArray<DebugVertex>& verts, const Vector& subPos, Float subTime );
	void GetCurveAbsoluteMatrix( Matrix& result );
	void GetCurveAbsoluteTransform( EngineTransform& result );
	EShowFlags GetShowFlags() const { return GetCurve()->GetShowFlags(); }
	Bool UpdateCurveEntityTransform();
	void ToWorldMatrix( const EngineTransform& transform, Matrix& result );
	void ToWorldTransform( const EngineTransform& transform, EngineTransform& result );
	void ToLocalMatrix( const EngineTransform& transform, Matrix& result );
	void ToLocalTransform( const EngineTransform& transform, EngineTransform& result );

	ICurveAccessor* m_curveAccessor;
	CCurveComponent* m_curveComponent;
	TDynArray< CCurveControlPointEntity* > m_controlPointEntities;
	static TDynArray< ICurveSelectionListener* > m_selectionListeners;

	Bool m_showCurvePosition;
	Bool m_showCurveRotation;
	Bool m_visualizeCurveTime;
	Bool m_drawArrowControlPoints;

	Bool m_enableDebugObjectCurveRun;
	Float m_debugObjectCurveRunTime;

	Bool m_drawOnTop;

	static CEntity* m_hoverControlPointEntity;

#ifndef NO_EDITOR
	CCurveEntitySpawner* m_curveEntitySpawner;
#endif

	friend class CCurveComponent;
	friend class CCurveControlPointComponent;
	friend class CCurveControlPointEntity;
	friend class CCurveTangentControlPointComponent;
	friend class CCurveTangentControlPointEntity;
};

BEGIN_CLASS_RTTI( CCurveEntity )
	PARENT_CLASS( CEntity )
	PROPERTY( m_curveComponent )
	PROPERTY( m_controlPointEntities )
#ifndef NO_EDITOR
	PROPERTY( m_curveEntitySpawner )
#endif
END_CLASS_RTTI()

//! Helper component used for editing the curve
class CCurveComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CCurveComponent, CComponent, 0 );

public:
	CCurveComponent();
	virtual ~CCurveComponent();
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual Bool ShouldGenerateEditorFragments( CRenderFrame* frame ) const override;
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

	void Setup( EShowFlags showFlags ) { m_showFlags = showFlags; }

private:
	EShowFlags m_showFlags;
};

BEGIN_CLASS_RTTI( CCurveComponent )
	PARENT_CLASS( CComponent )
END_CLASS_RTTI()
