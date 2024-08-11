/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "multiCurve.h"
#include "component.h"
#include "../core/curveSimple.h"


class IMovePathPlannerQuery;

/// Editable path component
class CPathComponent : public CComponent
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CPathComponent, CComponent );

private:
	SMultiCurve						m_curve;
	TDynArray< Float >				m_curveCachedDistances;
	Float							m_curveTotalDistance;

	SSimpleCurve					m_speedCurve;
	
public:
	CPathComponent();

	// Find edge closest to given point
	Int32 FindClosestEdge( const Vector& point );

	Float GetAlphaOnEdge( const Vector& point, Int32 edgeIdx, Float epsilon = 0.001f );

	Vector GetClosestPointOnPath( const Vector& point, Int32 &engeIdx, Float &edgeAlpha, Float epsilon = 0.001f );
	Vector GetClosestPointOnPath( const Vector& point, Float epsilon = 0.001f );
	Float  GetDistanceToPath    ( const Vector& point, Float epsilon = 0.001f );

	Vector	GetNextPointOnPath			( Int32 &edgeIdx, Float &edgeAlpha, Float distance, Bool &isEndOfPath, Float epsilon = 0.001f );
	Vector	GetNextPointOnPath			( const Vector& point, Float distance, Bool &isEndOfPath, Float epsilon = 0.001f );
	Vector	GetPathPoint				( Int32 edgeIdx, Float edgeAlpha );
	Vector	CorrectPathPoint			( const Vector& point, Int32& inOutEdgeIdx, Float& inOutEdgeAlpha, Float epsilon = 0.01f );
	Bool	CalculateSpeedMult			( Int32 edgeIdx, Float edgeAlpha, Float& outSpeedMult );
	Float	CalculateT					( Int32 edgeIdx, Float edgeAlpha );
	Bool	HasSpeedCurve				()const;
	Float	CalculateDistanceFromStart	( Int32 edgeIdx, Float edgeAlpha );

	Vector GetBeginning();

	SMultiCurve& GetCurve() { return m_curve; }

#ifndef NO_EDITOR
	// Editor only stuff
	virtual void EditorOnTransformChangeStart() override;
	// Editor only stuff
	virtual void EditorOnTransformChanged() override;
	// Editor only stuff
	virtual void EditorOnTransformChangeStop() override;
#endif

	virtual Bool ShouldGenerateEditorFragments( CRenderFrame* frame ) const override;

	/// Component interface
	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )override;

protected:
	void GetInterpolationControlPoints( Vector& p1, Vector& p2, Vector& p3, Vector& p4, Int32 edgeIdx );
	void InitCachedData();

	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo ) override;
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnSelectionChanged() override;
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;
#ifndef RED_FINAL_BUILD
	virtual void OnSerialize( IFile& file ) override;
#endif

#ifndef NO_RESOURCE_COOKING	
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

private:
#ifndef RED_FINAL_BUILD
	void RefactorPath();
#endif

	void funcFindClosestEdge( CScriptStackFrame& stack, void* result );
	void funcGetAlphaOnEdge( CScriptStackFrame& stack, void* result );
	void funcGetClosestPointOnPath( CScriptStackFrame& stack, void* result );
	void funcGetClosestPointOnPathExt( CScriptStackFrame& stack, void* result );
	void funcGetDistanceToPath( CScriptStackFrame& stack, void* result );
	void funcGetNextPointOnPath( CScriptStackFrame& stack, void* result );
	void funcGetNextPointOnPathExt( CScriptStackFrame& stack, void* result );
	void funcGetWorldPoint( CScriptStackFrame& stack, void* result );
	void funcGetPointsCount( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CPathComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT("Curve"), TXT("MultiCurveEditor3D") )
	PROPERTY_EDIT( m_speedCurve, TXT("In the case of an npc in a MoveAlongPath scripted action, the npc's speed along the path is mult by the value of this curve") )
	NATIVE_FUNCTION( "FindClosestEdge", funcFindClosestEdge );
	NATIVE_FUNCTION( "GetAlphaOnEdge", funcGetAlphaOnEdge );
	NATIVE_FUNCTION( "GetClosestPointOnPath", funcGetClosestPointOnPath );
	NATIVE_FUNCTION( "GetClosestPointOnPathExt", funcGetClosestPointOnPathExt );
	NATIVE_FUNCTION( "GetDistanceToPath", funcGetDistanceToPath );
	NATIVE_FUNCTION( "GetNextPointOnPath", funcGetNextPointOnPath );
	NATIVE_FUNCTION( "GetNextPointOnPathExt", funcGetNextPointOnPathExt );
	NATIVE_FUNCTION( "GetWorldPoint", funcGetWorldPoint );
	NATIVE_FUNCTION( "GetPointsCount", funcGetPointsCount );
END_CLASS_RTTI();
		 
