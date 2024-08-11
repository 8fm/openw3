/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "renderFrame.h"
#include "viewport.h"
#include "curveEntity.h"
#include "curveControlPointEntity.h"
#include "curveTangentControlPointEntity.h"
#include "renderVertices.h"
#include "renderFrame.h"
#include "pathComponent.h"
#include "world.h"
#include "layer.h"
#include "selectionManager.h"
#include "dynamicLayer.h"
#include "tickManager.h"
#include "hitProxyId.h"

namespace RenderingUtils
{
	void CreateArrow( const Vector & startPoint, const Vector & endPoint, TDynArray<Vector> & outPoints, Float length = 0.2f, Float sideSize = 0.05f );
	void RenderArrow( CRenderFrame* frame, const Color& wireframeColor, const Color & color, const TDynArray<Vector> & points, Bool overlay = false );
}

static const Float g_maxLineLength = 0.3f;
static const Float g_maxTimeLineLength = 0.01f;
static const Float g_lineWidth = 0.02f;

CEntity* CCurveEntity::m_hoverControlPointEntity = nullptr;

TDynArray< ICurveSelectionListener* > CCurveEntity::m_selectionListeners;

IMPLEMENT_ENGINE_CLASS( CCurveEntity );

CCurveEntity::CCurveEntity()
	: m_enableDebugObjectCurveRun( false )
	, m_visualizeCurveTime( false )
	, m_drawOnTop( false )
	, m_drawArrowControlPoints( false )
#ifndef NO_EDITOR
	, m_curveEntitySpawner( nullptr )
#endif
{
}

CCurveEntity::~CCurveEntity()
{
}

void CCurveEntity::OnDestroyed( CLayer* layer )
{
	TBaseClass::OnDestroyed( layer );

	if ( m_curveAccessor )
	{
		delete m_curveAccessor;
		m_curveAccessor = nullptr;
	}

#ifndef NO_EDITOR
	if( m_curveEntitySpawner )
	{
		m_curveEntitySpawner->Discard();
		m_curveEntitySpawner = nullptr;
	}
#endif // NO_EDITOR

	EnableEditMode( false );
}

void CCurveEntity::EnableAutomaticTimeByDistanceRecalculation( Bool enable )
{
	GetCurve()->EnableAutomaticTimeByDistanceRecalculation( enable );
	if ( enable )
	{
		UpdateControlPointTransforms();
	}
	OnCurveChanged();
}

Bool CCurveEntity::IsAutomaticTimeByDistanceRecalculationEnabled()
{
	return GetCurve()->IsAutomaticTimeByDistanceRecalculationEnabled();
}

void CCurveEntity::RecalculateTimeByDistance( Bool selectionOnly )
{
	TDynArray< Uint32 > indices;
	if ( selectionOnly )
	{
		for ( Uint32 i = 0; i < m_controlPointEntities.Size(); i++ )
		{
			if ( m_controlPointEntities[ i ]->IsSelected() )
			{
				indices.PushBack( i );
			}
		}
	}
	GetCurve()->RecalculateTimeByDistance( selectionOnly ? &indices : NULL );
	UpdateControlPointTransforms();
	OnCurveChanged();
}

void CCurveEntity::EnableAutomaticTimeRecalculation( Bool enable )
{
	GetCurve()->EnableAutomaticTimeRecalculation( enable );
	if ( enable )
	{
		UpdateControlPointTransforms();
	}
	OnCurveChanged();
}

Bool CCurveEntity::IsAutomaticTimeRecalculationEnabled()
{
	return GetCurve()->IsAutomaticTimeRecalculationEnabled();
}

void CCurveEntity::RecalculateTime()
{
	GetCurve()->RecalculateTime();
	UpdateControlPointTransforms();
	OnCurveChanged();
}
void CCurveEntity::EnableAutomaticRotationFromDirectionRecalculation( Bool enable )
{
	GetCurve()->EnableAutomaticRotationFromDirectionRecalculation( enable );
	if ( enable )
	{
		UpdateControlPointTransforms();
	}
	OnCurveChanged();
}

Bool CCurveEntity::IsAutomaticRotationFromDirectionRecalculationEnabled()
{
	return GetCurve()->IsAutomaticRotationFromDirectionRecalculationEnabled();
}

void CCurveEntity::RecalculateRotationFromDirection()
{
	GetCurve()->RecalculateRotationFromDirection();
	UpdateControlPointTransforms();
	OnCurveChanged();
}

ECurveInterpolationMode CCurveEntity::GetPositionInterpolationMode()
{
	return GetCurve()->GetPositionInterpolationMode();
}

void CCurveEntity::SetPositionInterpolationMode( ECurveInterpolationMode mode )
{
	GetCurve()->SetPositionInterpolationMode( mode );
	UpdateControlPointTransforms();
	OnCurveChanged();
}

ECurveInterpolationMode CCurveEntity::GetRotationInterpolationMode()
{
	return GetCurve()->GetRotationInterpolationMode();
}

void CCurveEntity::SetRotationInterpolationMode( ECurveInterpolationMode mode )
{
	GetCurve()->SetRotationInterpolationMode( mode );
	UpdateControlPointTransforms();
	OnCurveChanged();
}
ECurveManualMode CCurveEntity::GetPositionManualMode()
{
	return GetCurve()->GetPositionManualMode();
}

void CCurveEntity::SetPositionManualMode( ECurveManualMode mode )
{
	GetCurve()->SetPositionManualMode( mode );
	UpdateControlPointTransforms();
	OnCurveChanged();
}

void CCurveEntity::RecalculateBezierTangents()
{
	GetCurve()->RecalculateBezierTangents();
	UpdateControlPointTransforms();
	OnCurveChanged();
}

Bool CCurveEntity::IsCurveLoopingEnabled() const
{
	return GetCurve()->IsLooping();
}

void CCurveEntity::SetCurveLooping( Bool loop )
{
	GetCurve()->SetLooping( loop );
	UpdateControlPointTransforms();
	OnCurveChanged();
}

void CCurveEntity::SelectAllControlPoints()
{
	CWorld* world = GetLayer()->GetWorld();
	world->GetSelectionManager()->DeselectAll();
	for ( auto it = m_controlPointEntities.Begin(); it != m_controlPointEntities.End(); ++it )
	{
		world->GetSelectionManager()->Select( *it );
	}
}

void CCurveEntity::GetCurveAbsoluteTransform( EngineTransform& result )
{
	GetCurve()->GetAbsoluteTransform( result );
}

void CCurveEntity::GetCurveAbsoluteMatrix( Matrix& result )
{
	GetCurve()->GetAbsoluteMatrix( result );
}

CCurveEntity* CCurveEntity::CreateEditor( CWorld* world, SMultiCurve* curve, Bool showControlPoints )
{
	return CreateEditor( world, new CDefaultCurveAccessor( curve ), showControlPoints );
}

CCurveEntity* CCurveEntity::CreateEditor( CWorld* world, ICurveAccessor* curveAccessor, Bool showControlPoints )
{
	ASSERT( world->GetDynamicLayer() );

	CCurveEntity* entity = FindCurveEditor( world, curveAccessor->Get() );
	if ( !entity )
	{
		EntitySpawnInfo info;
		info.m_entityClass = ClassID<CCurveEntity>();
		entity = Cast<CCurveEntity>( world->GetDynamicLayer()->CreateEntitySync( info )  );
		entity->Setup( curveAccessor );
	}
	else
	{
		delete curveAccessor;
	}

	if ( showControlPoints )
	{
		entity->EnableEditMode( true );
	}

	return entity;
}

Bool CCurveEntity::DeleteEditor( CWorld* world, SMultiCurve* curve )
{
	if ( CCurveEntity* curveEntity = FindCurveEditor( world, curve ) )
	{
		curveEntity->Destroy();
		return true;
	}

	return false;
}

void CCurveEntity::GetAllEditors( CWorld* world, TDynArray<CCurveEntity*>& curveEntities )
{
	curveEntities.ClearFast();

	if ( !world->GetDynamicLayer() )
	{
		return;
	}

	const LayerEntitiesArray& entities = world->GetDynamicLayer()->GetEntities();
	for ( auto it = entities.Begin(); it != entities.End(); ++it ) 
	{
		if ( CCurveEntity* curveEntity = Cast<CCurveEntity>( *it ) )
		{
			curveEntities.PushBack( curveEntity );
		}
	}
}

void CCurveEntity::DeleteEditors( CWorld* world, Bool onlyDeleteInvalidated )
{
	TDynArray<CCurveEntity*> curveEntities;
	GetAllEditors( world, curveEntities );

	// Delete all invalidated curve entities

	for ( auto it = curveEntities.Begin(); it != curveEntities.End(); ++it ) 
	{
		if ( !onlyDeleteInvalidated || !(*it)->GetCurve() )
		{
			(*it)->Destroy();
		}
	}
}

CCurveEntity* CCurveEntity::FindCurveEditor( CWorld* world, SMultiCurve* curve )
{
	if ( !world->GetDynamicLayer() )
	{
		return NULL;
	}

	const LayerEntitiesArray& entities = world->GetDynamicLayer()->GetEntities();
	for ( auto it = entities.Begin(); it != entities.End(); ++it ) 
	{
		if ( CCurveEntity* curveEntity = Cast<CCurveEntity>( *it ) )
		{
			if ( curveEntity->GetCurve() == curve )
			{
				return curveEntity;
			}
		}
	}
	return NULL;
}

CCurveControlPointEntity* CCurveEntity::CreateControlPointEntity( Uint32 index )
{
	EntitySpawnInfo info;
	info.m_entityClass = ClassID<CCurveControlPointEntity>();

	CCurveControlPointEntity* controlPointEntity = Cast<CCurveControlPointEntity>( GetLayer()->CreateEntitySync( info )  );
	controlPointEntity->Setup( this, index );

	UpdateControlPointTransform( controlPointEntity, index );

	return controlPointEntity;
}

void CCurveEntity::EnableEditMode( Bool enable )
{
	if ( enable && m_controlPointEntities.Size() == 0 )
	{
		// Create entity for each control point

		const Uint32 numControlPoints = GetCurve()->Size();
		m_controlPointEntities.Reserve( numControlPoints );
		for ( Uint32 i = 0; i < numControlPoints; i++ )
		{
			m_controlPointEntities.PushBack( CreateControlPointEntity( i ) );
		}

		// Hide control points for all other curves in the world

		const LayerEntitiesArray& entities = GetLayer()->GetEntities();
		for ( auto it = entities.Begin(); it != entities.End(); ++it ) 
		{
			if ( CCurveEntity* curveEntity = Cast<CCurveEntity>( *it ) )
			{
				if ( curveEntity != this )
				{
					curveEntity->EnableEditMode( false );
				}
			}
		}
	}
	else if ( !enable && m_controlPointEntities.Size() > 0 )
	{
		// Delete control point entities

		for ( unsigned int i = 0; i < m_controlPointEntities.Size(); i++ )
		{
			m_controlPointEntities[i]->Destroy();
		}
		m_controlPointEntities.Clear();

		// Turn on default visualization mode

		m_enableDebugObjectCurveRun = false;
		m_visualizeCurveTime = false;
		m_showCurveRotation = false;
		m_showCurvePosition = true;
	}
}

void CCurveEntity::Setup( ICurveAccessor* curveAccessor )
{
	SMultiCurve* curve = curveAccessor->Get();
	CNode* curveParent = curve->GetParent();

	ASSERT( curve->IsEditableIn3D() );

	m_curveAccessor = curveAccessor;

	m_showCurvePosition = true;
	m_showCurveRotation = false;
	m_drawArrowControlPoints = curve->GetCurveType() == ECurveType_Vector && ( curveParent && curveParent->IsA< CPathComponent >() );

	// Create curve component to be used for drawing the actual curve and transforming it

	CCurveComponent* curveComponent = CreateObject<CCurveComponent>();
	curveComponent->Setup( curve->GetShowFlags() );
	AddComponent( curveComponent );

	// Set up relative transformation

	UpdateCurveEntityTransform();
}

void CCurveEntity::UpdateControlPointTransforms()
{
	for ( Uint32 i = 0; i < m_controlPointEntities.Size(); i++ )
	{
		UpdateControlPointTransform( m_controlPointEntities[i], i );
	}
}

void CCurveEntity::UpdateControlPointTransform( CCurveControlPointEntity* controlPointEntity, Uint32 index )
{
	// Update transformation of the control point

	EngineTransform worldTransform;
	GetCurve()->GetAbsoluteControlPointTransform( index, worldTransform );

	controlPointEntity->SetPosition( worldTransform.GetPosition() );
	controlPointEntity->SetRotation( worldTransform.GetRotation() );
	controlPointEntity->SetScale( worldTransform.GetScale() );

	// Update tangent control points

	controlPointEntity->RefreshTangentControlPoints( index );
}

Uint32 CCurveEntity::GetControlPointIndex( CCurveControlPointEntity* controlPointEntity )
{
	for ( Uint32 i = 0; i < m_controlPointEntities.Size(); i++ )
	{
		if ( m_controlPointEntities[i] == controlPointEntity )
		{
			return i;
		}
	}

	return (Uint32) -1;
}

void CCurveEntity::SetControlPointTime( Uint32 index, Float time )
{
	GetCurve()->SetControlPointTime( index, time );
	UpdateControlPointTransforms();
}

void CCurveEntity::OnControlPointMoved( CCurveControlPointEntity* controlPointEntity )
{
	// Get the index of the control point

	const Int32 index = GetControlPointIndex( controlPointEntity );
	if ( index == -1 )
	{
		return;
	}

	// Update control point coordinates

	EngineTransform localTransform;
	ToLocalTransform( controlPointEntity->GetTransform(), localTransform );
	GetCurve()->SetControlPointTransform( index, localTransform );

	// Update control points after curve modification (some things might have been changed automatically, e.g. due to automatic rotation recalculation)
	
	UpdateControlPointTransforms();
	OnCurveChanged();
}

void CCurveEntity::SetControlPointPosition( CCurveControlPointEntity* controlPointEntity, const Vector& pos )
{
	controlPointEntity->SetPosition( pos );
	OnControlPointMoved( controlPointEntity );
}

Bool CCurveEntity::CanDeleteControlPoint()
{
	// Make sure there's always at least 2 control points in the curve
	return GetCurve()->Size() > 2;
}

void CCurveEntity::DeleteControlPoint( CCurveControlPointEntity* controlPointEntity )
{
	// Make sure there's always at least 2 control points in the curve
	if ( !CanDeleteControlPoint() )
	{
		return;
	}

	OnControlPointDeleted( controlPointEntity );
	controlPointEntity->Destroy();
}

void CCurveEntity::OnControlPointDeleted( CCurveControlPointEntity* controlPointEntity )
{
	// Get the index of the control point

	const Int32 index = GetControlPointIndex( controlPointEntity );
	if ( index == -1 )
	{
		return;
	}

	// Remove control point

	GetCurve()->RemoveControlPoint( index );
	m_controlPointEntities.Remove( controlPointEntity );
	OnCurveChanged();

	// Select previous control point

	GetLayer()->GetWorld()->GetSelectionManager()->DeselectAll();
	GetLayer()->GetWorld()->GetSelectionManager()->Select( m_controlPointEntities[ Clamp<Uint32>( index, 0, m_controlPointEntities.Size() - 1 ) ] );
}

void CCurveEntity::OnTangentControlPointMoved( CCurveControlPointEntity* controlPointEntity, CCurveTangentControlPointEntity* tangentControlPointEntity, Uint32 tangentIndex )
{
	// Get the index of the control point

	const Int32 index = GetControlPointIndex( controlPointEntity );
	if ( index == -1 )
	{
		return;
	}

	// Get local space tangent

	EngineTransform localTangentControlPointTransform, localControlPointTransform;
	ToLocalTransform( tangentControlPointEntity->GetTransform(), localTangentControlPointTransform );
	ToLocalTransform( controlPointEntity->GetTransform(), localControlPointTransform );

	const Vector tangent = localTangentControlPointTransform.GetPosition() - localControlPointTransform.GetPosition();

	// Update tangent

	GetCurve()->SetControlPointTangent( index, tangentIndex, tangent );

	// Update control points after curve modification (some things might have been changed automatically, e.g. due to automatic rotation recalculation)

	UpdateControlPointTransforms();

	OnCurveChanged();
}

CCurveControlPointEntity* CCurveEntity::AddControlPointAfter( CCurveControlPointEntity* controlPointEntity )
{
	const Int32 index = GetControlPointIndex( controlPointEntity );
	if ( index != -1 )
	{
		return AddControlPointWorld( index + 1 );
	}
	return NULL;
}

CCurveControlPointEntity* CCurveEntity::AppendControlPointWorld( const Vector& worldPos )
{
	return AddControlPointWorld( m_controlPointEntities.Size(), &worldPos );
}

CCurveControlPointEntity* CCurveEntity::AddControlPointBefore( CCurveControlPointEntity* controlPointEntity )
{
	const Int32 index = GetControlPointIndex( controlPointEntity );
	if ( index != -1 )
	{
		return AddControlPointWorld( index );
	}
	return NULL;
}

CCurveControlPointEntity* CCurveEntity::AddControlPointWorld( const Vector& worldPos )
{
	Vector localPos;
	GetCurve()->ToLocalPosition( worldPos, localPos );

	const Uint32 newControlPointIndex = GetCurve()->AddControlPoint( localPos );

	CCurveControlPointEntity* newControlPoint = CreateControlPointEntity( (Int32) newControlPointIndex );
	m_controlPointEntities.Insert( newControlPointIndex, newControlPoint );

	UpdateControlPointTransforms();
	OnCurveChanged();

	GetLayer()->GetWorld()->GetSelectionManager()->DeselectAll();
	GetLayer()->GetWorld()->GetSelectionManager()->Select( newControlPoint );

	return newControlPoint;
}

CCurveControlPointEntity* CCurveEntity::AddControlPoint( Uint32 index, const EngineTransform& transform )
{
	const Uint32 newControlPointIndex = GetCurve()->AddControlPointAt( index, transform );

	CCurveControlPointEntity* newControlPoint = CreateControlPointEntity( (Int32) newControlPointIndex );
	m_controlPointEntities.Insert( newControlPointIndex, newControlPoint );

	UpdateControlPointTransforms();
	OnCurveChanged();

	GetLayer()->GetWorld()->GetSelectionManager()->DeselectAll();
	GetLayer()->GetWorld()->GetSelectionManager()->Select( newControlPoint );

	return newControlPoint;
}

CCurveControlPointEntity* CCurveEntity::AddControlPointWorld( Uint32 index, const Vector* worldPos )
{
	Vector localPos;
	if ( worldPos )
	{
		GetCurve()->ToLocalPosition( *worldPos, localPos );
	}

	GetCurve()->AddControlPointAt( index, worldPos ? &localPos : NULL );

	CCurveControlPointEntity* newControlPoint = CreateControlPointEntity( index );
	m_controlPointEntities.Insert( index, newControlPoint );

	UpdateControlPointTransforms();
	OnCurveChanged();

	GetLayer()->GetWorld()->GetSelectionManager()->DeselectAll();
	GetLayer()->GetWorld()->GetSelectionManager()->Select( newControlPoint );

	return newControlPoint;
}

Bool CCurveEntity::UpdateCurveEntityTransform()
{
	EngineTransform expectedTransform;
	GetCurveAbsoluteTransform( expectedTransform );

	if ( !( expectedTransform == GetTransform() ) )
	{
		SetPosition( expectedTransform.GetPosition() );
		SetRotation( expectedTransform.GetRotation() );
		SetScale( expectedTransform.GetScale() );
		return true;
	}

	return false;
}

void CCurveEntity::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );

	// Force relative transform update (to keep this entity and whole curve in sync with owner entity)

	if ( UpdateCurveEntityTransform() )
	{
		UpdateControlPointTransforms();
	}

	// Update debug object curve run

	SMultiCurve* curve = GetCurve();
	if ( m_enableDebugObjectCurveRun )
	{
		m_debugObjectCurveRunTime += timeDelta;
		if ( m_debugObjectCurveRunTime > curve->GetTotalTime() )
		{
			m_debugObjectCurveRunTime = 0.0f;
		}
	}

	// Update tangent points (show / hide them)

	const Uint32 numControlPoints = m_controlPointEntities.Size();
	for ( Uint32 i = 0; i < numControlPoints; i++ )
	{
		m_controlPointEntities[ i ]->RefreshTangentControlPoints( i );
	}
}

CCurveEntity* CCurveEntity::RefreshEditor( CWorld* world, SMultiCurve* curve )
{
	if ( CCurveEntity* curveEntity = FindCurveEditor( world, curve ) )
	{
		if ( curveEntity->m_controlPointEntities.Size() == curve->Size() ) // No points added or removed? Just update transforms
		{
			curveEntity->UpdateControlPointTransforms();
			return curveEntity;
		}
		else // Points added or removed - recreate the editor
		{
			const Bool hasControlPointsShown = !curveEntity->m_controlPointEntities.Empty();
			DeleteEditor( world, curve );
			return CreateEditor( world, curve, hasControlPointsShown );
		}
	}
	return NULL;
}

void CCurveEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	if ( !IsInGame() )
	{
		world->GetTickManager()->AddEntity( this );	
	}
}

void CCurveEntity::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	if ( !IsInGame() )
	{
		world->GetTickManager()->RemoveEntity( this );
	}
}

void CCurveEntity::OnSelectionChanged()
{
	if ( IsSelected() )
	{
		EnableEditMode( true );
		NotifySelectionChange();

#if 0 // Disabled because this fucked up CSelectionManager state

		// Deselect this and select the owner

		SMultiCurve* curve = GetCurve();
		CNode* curveParent = curve->GetParent();
		if ( !curveParent )
		{
			return;
		}

		GetLayer()->GetWorld()->GetSelectionManager()->DeselectAll();
		GetLayer()->GetWorld()->GetSelectionManager()->Select( curveParent );

#endif
	}

	TBaseClass::OnSelectionChanged();
}

//---------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CCurveComponent );

CCurveComponent::CCurveComponent()
{
}

CCurveComponent::~CCurveComponent()
{
}

void CCurveComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, m_showFlags );
}

void CCurveComponent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, m_showFlags );
	TBaseClass::OnDetached( world );
}

Bool CCurveComponent::ShouldGenerateEditorFragments( CRenderFrame* frame ) const
{
	CCurveEntity* curveEntity = static_cast<CCurveEntity*>( GetEntity() );
	SMultiCurve* curve = curveEntity->GetCurve();

	EngineTransform transform;
	curve->GetAbsoluteTransform( transform );
	Matrix worldToCurve;
	transform.CalcWorldToLocal( worldToCurve );
	Uint32 nearestCP = curve->FindNearestControlPoint( worldToCurve.TransformPoint( frame->GetFrameInfo().m_camera.GetPosition() ) );

	Vector currentPos;
	curve->GetAbsoluteControlPointPosition( nearestCP, currentPos );

	Float dist = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
	Float distFromCam = frame->GetFrameInfo().m_camera.GetPosition().DistanceSquaredTo( currentPos );
	if ( distFromCam < dist )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CCurveComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Get the curve to draw

	CCurveEntity* curveEntity = static_cast<CCurveEntity*>( GetEntity() );
	SMultiCurve* curve = curveEntity->GetCurve();

	// Colors for X/Y/Z axis and the curve

	Color colors[4] =
	{
		Color(255, 50, 50, 200),
		Color(50, 255, 50, 200),
		Color(0, 50, 255, 200),
		curve->GetColor()
	};

#ifndef NO_COMPONENT_GRAPH
	const Bool isHitProxyFragment = frame->GetFrameInfo().m_renderingMode == RM_HitProxies;
	if ( isHitProxyFragment )
	{
		colors[0] = colors[1] = colors[2] = colors[3] = GetHitProxyID().GetColor();
	}
#endif

	// Generate verts for segments between each consecutive control point pair

	const Float arrowLength = 0.4f;
	const Float arrowSideSize = 0.15f;

	Float prevTime = curve->GetControlPointTime( 0 );
	EngineTransform prevTransform;
	curve->GetAbsoluteControlPointTransform( 0, prevTransform );

	TDynArray<DebugVertex> positionVerts;
	TDynArray<DebugVertex> rotationVerts;
	TDynArray<Uint16> rotationIndices;

	const Uint32 numControlPoints = curve->Size();
	const Uint32 numEndControlPoints = numControlPoints + ( curve->IsLooping() ? 1 : 0 );

	for ( Uint32 i = 1; i < numEndControlPoints; i++ )
	{
		// Get next transform

		const Uint32 endControlPointIndex = i < numControlPoints ? i : 0;

		Float nextTime;
		if ( i < numControlPoints )
		{
			nextTime = curve->GetControlPointTime( endControlPointIndex );
		}
		else
		{
			nextTime = curve->GetTotalTime();
		}
		EngineTransform nextTransform;
		curve->GetAbsoluteControlPointTransform( endControlPointIndex, nextTransform );

		// Get the distance between control points

		Int32 numLines;
		if ( curveEntity->m_visualizeCurveTime )
		{
			const Float timeBetweenControlPoints = nextTime - prevTime;
			numLines = timeBetweenControlPoints <= 0.0f ? 1 : (Int32) Red::Math::MCeil( timeBetweenControlPoints / g_maxTimeLineLength );
			numLines = Min( numLines, 64 );
		}
		else
		{
			const Float distanceBetweenControlPoints = prevTransform.GetPosition().DistanceTo( nextTransform.GetPosition() );
			numLines = distanceBetweenControlPoints <= 0.0f ? 1 : (Int32) Red::Math::MCeil( distanceBetweenControlPoints / g_maxLineLength );
			numLines = Min( numLines, 16 );
		}

		// Draw the lines between two control points

		if ( numLines > 0 )
		{
			const Float subTimeStep = (nextTime - prevTime) / (Float) numLines;

			Float subTime = prevTime;

			for ( int j = 0; j < numLines; j++, subTime += subTimeStep )
			{
				const Float subTimeWrapped = Min( subTime, curve->GetTotalTime() );

				EngineTransform subTransform;
				curve->GetAbsoluteTransform( subTimeWrapped, subTransform );

				// Generate position vertex data

				if ( curveEntity->m_showCurvePosition )
				{
					const Uint32 colorIndex = curveEntity->m_visualizeCurveTime ? ( positionVerts.Size() & 1 ) : 3;

					DebugVertex vertex;
					vertex.Set( subTransform.GetPosition(), colors[ colorIndex ] );
					positionVerts.PushBack( vertex );
				}

				// Generate rotation vertex data

				if ( curveEntity->m_showCurveRotation && curve->HasRotation() )
				{
					DebugVertex start, end;

					start.Set( subTransform.GetPosition(), colors[0] );
					end.Set( subTransform.GetPosition() + subTransform.GetRotation().TransformVector( Vector( g_maxLineLength, 0, 0, 0 ) ), colors[0] );
					rotationIndices.PushBack( (Uint16) rotationVerts.Size() ); rotationVerts.PushBack( start );
					rotationIndices.PushBack( (Uint16) rotationVerts.Size() ); rotationVerts.PushBack( end );

					start.Set( subTransform.GetPosition(), colors[1] );
					end.Set( subTransform.GetPosition() + subTransform.GetRotation().TransformVector( Vector( 0, g_maxLineLength, 0, 0 ) ), colors[1] );
					rotationIndices.PushBack( (Uint16) rotationVerts.Size() ); rotationVerts.PushBack( start );
					rotationIndices.PushBack( (Uint16) rotationVerts.Size() ); rotationVerts.PushBack( end ); 

					start.Set( subTransform.GetPosition(), colors[2] );
					end.Set( subTransform.GetPosition() + subTransform.GetRotation().TransformVector( Vector( 0, 0, g_maxLineLength, 0 ) ), colors[2] );
					rotationIndices.PushBack( (Uint16) rotationVerts.Size() ); rotationVerts.PushBack( start );
					rotationIndices.PushBack( (Uint16) rotationVerts.Size() ); rotationVerts.PushBack( end );
				}
			}
		}

		// Draw direction arrow

		if ( i + 1 == numControlPoints && !curveEntity->GetDrawArrowControlPoints() )
		{
			Vector veryPrevPosition;
			curve->GetAbsolutePosition( nextTime - 0.01f, veryPrevPosition );

			TDynArray<Vector> arrowPoints;
			RenderingUtils::CreateArrow( veryPrevPosition, nextTransform.GetPosition(), arrowPoints, arrowLength, arrowSideSize );
			RenderingUtils::RenderArrow( frame, Color( 255, 0, 255, 255 ), Color( 255, 0, 255, 150 ), arrowPoints, curveEntity->GetDrawOnTop() );
		}

		// Go to the next control point

		prevTransform = nextTransform;
		prevTime = nextTime;
	}

	// Finalize drawing curve position

	if ( curveEntity->m_showCurvePosition )
	{
		// Append the last vertex

		const Uint32 colorIndex = curveEntity->m_visualizeCurveTime ? ( positionVerts.Size() & 1 ) : 3;

		DebugVertex vertex;
		vertex.Set( prevTransform.GetPosition(), colors[ colorIndex ] );
		positionVerts.PushBack( vertex );

		// Draw

		frame->AddDebugFatLines( &(*positionVerts.Begin()), positionVerts.Size(), g_lineWidth, curveEntity->GetDrawOnTop() );
	}

	// Finalize drawing curve rotation

	if ( curveEntity->m_showCurveRotation && curve->HasRotation() )
	{
		frame->AddDebugIndexedLines( &(*rotationVerts.Begin()), rotationVerts.Size(), &(*rotationIndices.Begin()), rotationIndices.Size(), curveEntity->GetDrawOnTop() );
	}

	// Draw debug curve run preview

	if ( curveEntity->m_enableDebugObjectCurveRun )
	{
		EngineTransform localDebugObjectTransform;
		curve->GetTransform( curveEntity->m_debugObjectCurveRunTime, localDebugObjectTransform );
		Matrix worldDebugObjectMatrix;
		curveEntity->ToWorldMatrix( localDebugObjectTransform, worldDebugObjectMatrix );

		const Box boundingBox = Box( Vector::ZEROS, 0.15f );

		frame->AddDebugBox( boundingBox, worldDebugObjectMatrix, Color(255, 50, 50, 50), curveEntity->GetDrawOnTop() );
		frame->AddDebugSolidBox( boundingBox, worldDebugObjectMatrix, Color(255, 50, 50, 150), curveEntity->GetDrawOnTop() );
	}
}

void CCurveEntity::SetHoverControlPoint( CEntity* entity )
{
	m_hoverControlPointEntity = entity;
}

CEntity* CCurveEntity::GetHoverControlPoint()
{
	return m_hoverControlPointEntity;
}

void CCurveEntity::ToWorldMatrix( const EngineTransform& localTransform, Matrix& result )
{
	GetCurve()->ToAbsoluteMatrix( localTransform, result );
}

void CCurveEntity::ToWorldTransform( const EngineTransform& localTransform, EngineTransform& result )
{
	GetCurve()->ToAbsoluteTransform( localTransform, result );
}

void CCurveEntity::ToLocalMatrix( const EngineTransform& worldTransform, Matrix& result )
{
	GetCurve()->ToLocalMatrix( worldTransform, result );
}

void CCurveEntity::ToLocalTransform( const EngineTransform& worldTransform, EngineTransform& result )
{
	GetCurve()->ToLocalTransform( worldTransform, result );
}

void CCurveEntity::OnCurveChanged()
{
	SMultiCurve* curve = GetCurve();

#ifndef NO_EDITOR
	// Notify listeners
	SMultiCurve::PostCurveChanged( curve );
#endif // !NO_EDITOR

	// Mark layer as modified

	CNode* curveParent = curve->GetParent();
	if ( curveParent )
	{
		CLayer* layer = curveParent->GetLayer();
		if ( layer )
		{
			layer->MarkModified();
		}
	}
}

void CCurveEntity::NotifySelectionChange()
{
	// Collect indices of all selected control points

	TDynArray< Uint32 > selectedIndices;
	for ( Uint32 i = 0; i < m_controlPointEntities.Size(); i++ )
	{
		if ( m_controlPointEntities[ i ]->IsSelected() )
		{
			selectedIndices.PushBack( i );
		}
	}

	// Notify all listeners

	for ( auto it = m_selectionListeners.Begin(); it != m_selectionListeners.End(); ++it )
	{
		(*it)->OnCurveSelectionChanged( this, selectedIndices );
	}
}

void CCurveEntity::SelectControlPointByIndex( Uint32 index, Bool resetSelection )
{
	CWorld* world = GetLayer()->GetWorld();
	if ( !world )
	{
		return;
	}

	if ( index >= m_controlPointEntities.Size() )
	{
		return;
	}

	if ( resetSelection )
	{
		world->GetSelectionManager()->DeselectAll();
	}
	world->GetSelectionManager()->Select( m_controlPointEntities[ index ] );
}

void CCurveEntity::SelectPreviousControlPoint( CCurveControlPointEntity* controlPoint )
{
	CWorld* world = GetLayer()->GetWorld();
	if ( !world )
	{
		return;
	}

	const Uint32 currentControlPointIndex = GetControlPointIndex( controlPoint );
	world->GetSelectionManager()->DeselectAll();
	world->GetSelectionManager()->Select( m_controlPointEntities[ currentControlPointIndex > 0 ? ( currentControlPointIndex - 1 ) : ( m_controlPointEntities.Size() - 1 ) ] );
}

void CCurveEntity::SelectNextControlPoint( CCurveControlPointEntity* controlPoint )
{
	CWorld* world = GetLayer()->GetWorld();
	if ( !world )
	{
		return;
	}

	const Uint32 currentControlPointIndex = GetControlPointIndex( controlPoint );
	world->GetSelectionManager()->DeselectAll();
	world->GetSelectionManager()->Select( m_controlPointEntities[ ( currentControlPointIndex + 1 < m_controlPointEntities.Size() ) ? ( currentControlPointIndex + 1 ) : 0 ] );

}

#ifndef NO_EDITOR
CCurveEntitySpawner* CCurveEntity::GetCurveEntitySpawner()
{
	if( m_curveEntitySpawner == nullptr )
	{
		m_curveEntitySpawner = new CCurveEntitySpawner();
		m_curveEntitySpawner->SetParent( this );
	}
	return m_curveEntitySpawner;
}
#endif // NO_EDITOR
