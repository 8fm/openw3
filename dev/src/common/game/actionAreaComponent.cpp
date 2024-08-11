/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "actionAreaComponent.h"
#include "../../common/engine/behaviorGraphNotifier.h"
#include "../../common/core/gatheredResource.h"
#include "../engine/tickManager.h"
#include "../engine/dynamicLayer.h"
#include "../engine/renderFrame.h"

namespace
{
	CGatheredResource resExplorationBlendEntity( TXT("gameplay\\globals\\exploration\\blendcharacter.w2ent"), RGF_NotCooked );
	CGatheredResource resExplorationAnimSet1( TXT("characters\\templates\\witcher\\animation\\obstacle.w2anims"), RGF_Startup );
	CGatheredResource resExplorationAnimSet2( TXT("characters\\templates\\witcher\\animation\\exploration_animset.w2anims"), RGF_Startup );
}

IMPLEMENT_ENGINE_CLASS( CActionAreaComponent );
IMPLEMENT_ENGINE_CLASS( CActionAreaBlendActor );
IMPLEMENT_ENGINE_CLASS( CAnimDef );
IMPLEMENT_ENGINE_CLASS( SAnimShift );
IMPLEMENT_RTTI_BITFIELD( EAllowedActorGroups );
RED_DEFINE_STATIC_NAME( OnUseExploration );

Matrix UnscaleMatrix( const Matrix & matrix, const Vector & inScale )
{
	Vector scale = inScale;

	if ( scale.X == 0.f ) scale.X = 1.f;
	if ( scale.Y == 0.f ) scale.Y = 1.f;
	if ( scale.Z == 0.f ) scale.Z = 1.f;

	if ( scale.X == 1.f && scale.Y == 1.f && scale.Z == 1.f )
		return matrix;

	scale.X = 1.f / scale.X;
	scale.Y = 1.f / scale.Y;
	scale.Z = 1.f / scale.Z;

	Matrix res = matrix;
	res.SetScale44( scale );
	return res;
}

Matrix UnscaleMatrix( const Matrix & matrix )
{
	return UnscaleMatrix( matrix, matrix.GetScale33() );
}

Matrix UnscaleMatrixInv( const Matrix & matrix, const Vector & inScale )
{
	Vector scale = inScale;

	if ( scale.X == 0.f ) scale.X = 1.f;
	if ( scale.Y == 0.f ) scale.Y = 1.f;
	if ( scale.Z == 0.f ) scale.Z = 1.f;

	if ( scale.X == 1.f && scale.Y == 1.f && scale.Z == 1.f )
		return matrix;

	scale.X = 1.f / scale.X;
	scale.Y = 1.f / scale.Y;
	scale.Z = 1.f / scale.Z;

	Matrix res = matrix;
	res.SetPreScale44( scale );
	return res;
}

Matrix UnscaleMatrixInv( const Matrix & matrix )
{
	return UnscaleMatrixInv( matrix, matrix.GetPreScale33() );
}

CActionAreaComponent::CActionAreaComponent()
	: m_allowedGroups( AAG_Player | AAG_TallNPCs | AAG_Ghost )
	, m_totalTransformation( 0.f, 0.f, 0.f )
	, m_totalTransformationIsKnown( true )
	, m_editorData( NULL )
	, m_walkToSideDistance( 0.f )
	, m_walkToBackDistance( 0.f )
	, m_walkToFrontDistance( 0.f )
	, m_fullTransformation( Matrix::IDENTITY )
{
	m_height = 0.5f;
	m_color = Color::LIGHT_BLUE;
}

namespace RenderingUtils
{
	void RenderArrow( CRenderFrame* frame, const Color & wireFrameColor, const Color & color, const TDynArray<Vector> & points, Bool overlay = false );

	void RenderPolyline( CRenderFrame* frame, const Color & color, const TDynArray<Vector> & points, Bool closed );
}

void CActionAreaComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	// Generate bounding boxes
	if ( flags == SHOW_Bboxes )
	{
		frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::LIGHT_YELLOW );
	}

	if ( flags == SHOW_Areas )
	{
		Box box( Vector( -0.5f, -0.5f, 0.f ), Vector( 0.5f, 0.5f, CalcHeightWS() ) );

		{ // Draw internal action area
			Matrix localToWorld = GetLocalToWorld();
			Vector axisX  = localToWorld.TransformVector( Vector::EX * 0.5f );
			Vector axisY  = localToWorld.TransformVector( Vector::EY * 0.5f );
			Vector center = localToWorld.GetTranslation();
			
			TAreaPoints worldPoints;
			worldPoints.PushBack( center - axisX - axisY );
			worldPoints.PushBack( center + axisX - axisY );
			worldPoints.PushBack( center + axisX + axisY );
			worldPoints.PushBack( center - axisX + axisY );

			DrawArea( frame, worldPoints );
		}

		// Draw 'walk to action' area
		if ( ( m_allowedGroups & AAG_Player ) != 0 )
		{
			if ( m_walkToSideDistance > 0.f || m_walkToBackDistance > 0.f || m_walkToFrontDistance > 0.f )
			{
				box.Min.X -= m_walkToSideDistance;
				box.Min.Y -= m_walkToBackDistance;
				box.Max.X += m_walkToSideDistance;
				box.Max.Y += m_walkToFrontDistance;

				frame->AddDebugBox( box, GetLocalToWorld(), Color::LIGHT_GREEN, false );
			}
		}
	}

	if ( flags == SHOW_Exploration )
	{
		if ( m_editorData == NULL )
		{
			m_editorData = new EditorOnlyData();
		}

		// Draw start and end point validity icons
		if ( GGame->GetActiveWorld() == GetLayer()->GetWorld() )
		{
			if ( m_editorData->m_offMeshLinks.Empty() )
			{
				CalculateOffMeshLinks( );
				//m_editorData->CacheOffMeshLinks( );
			}

#ifndef NO_COMPONENT_GRAPH
			m_editorData->RenderLinkValidity( frame, GetWorldPosition(), m_hitProxyId );
#endif
		}

		// Fill parabola only when needed
		if ( m_editorData->m_localParabolaPoints.Empty() )
		{
			InvalidateParabola( true );
		}

		// Draw exploration parabola
#ifndef NO_COMPONENT_GRAPH
		Color color = Color::GREEN;
		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
			color = m_hitProxyId.GetColor();
		}
		else if ( ! IsSelected() )
		{
			color.R /= 2;
			color.G /= 2;
			color.B /= 2;
		}
#else
		Color color( m_color.R / 2, m_color.G / 2, m_color.B / 2, m_color.A );
#endif

		RenderingUtils::RenderPolyline( frame, color, m_editorData->m_worldParabolaPoints, false );
		RenderingUtils::RenderArrow( frame, color, color, m_editorData->m_worldParabolaArrowPoints );
	}
}

/************************************************************************/
/* Attachment, detachment and other low-level stuff                     */
/************************************************************************/
void CActionAreaComponent::ReFillLocalPoints( Bool fillWorldPoints )
{
	m_localPoints.Clear();
	m_localPoints.PushBack( Vector( -0.5f - m_walkToSideDistance, -0.5f - m_walkToBackDistance, 0.0f ) );
	m_localPoints.PushBack( Vector( +0.5f + m_walkToSideDistance, -0.5f - m_walkToBackDistance, 0.0f ) );
	m_localPoints.PushBack( Vector( +0.5f + m_walkToSideDistance, +0.5f + m_walkToFrontDistance, 0.0f ) );
	m_localPoints.PushBack( Vector( -0.5f - m_walkToSideDistance, +0.5f + m_walkToFrontDistance, 0.0f ) );

	m_worldPoints.Clear();
	
	if ( fillWorldPoints )
	{
		Matrix localToWorld = GetLocalToWorld();
		m_worldPoints.Resize( m_localPoints.Size() );
		for ( Uint32 i=0; i < m_localPoints.Size(); ++i )
		{
			m_worldPoints[i] = localToWorld.TransformPoint( m_localPoints[i] );
		}

		OnUpdateBounds();
	}

	// Mark compiled trigger shape as dirty (not up to date)
	InvalidateAreaShape();
}

void CActionAreaComponent::RefreshInPathEngine()
{
	ASSERT( IsAttached() );

	if ( IsEnabled() )
	{
		// register the link
		//world->GetPathEngineWorld()->UnregisterOffMeshConnection( this );
		//world->GetPathEngineWorld()->RegisterOffMeshConnection( this, GetFriendlyName() );
	}

	if ( m_editorData )
	{
		m_editorData->m_offMeshLinks.Clear();
	}
}

void CActionAreaComponent::OnPropertyPostChange( IProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );
	if ( prop->GetName() == TXT("walkToBackDistance") )
	{
		if ( m_walkToBackDistance < 0 )
			m_walkToBackDistance = 0;
		ReFillLocalPoints( true );
	}
	else if( prop->GetName() == TXT("walkToFrontDistance") )
	{
		if( m_walkToFrontDistance < 0 )
			m_walkToFrontDistance = 0;
		ReFillLocalPoints( true );
	}
	else if ( prop->GetName() == TXT("walkToSideDistance") )
	{
		if ( m_walkToSideDistance < 0 )
			m_walkToSideDistance = 0;
		ReFillLocalPoints( true );
	}
}

void CActionAreaComponent::SetEnabled( Bool enabled )
{
	EnableOffMeshConnection( enabled );
	EnableTriggerArea( enabled );
}

void CActionAreaComponent::SetAllowedGroupFlag( EAllowedActorGroups group, Bool flag )
{
	if( flag )
		m_allowedGroups |= group;
	else
		m_allowedGroups &= (~group);
}

void CActionAreaComponent::EnableTriggerArea( Bool enable )
{
	if ( enable != m_isEnabled )
	{
		SetShouldSave( true );
	}

	if ( IsAttached() && enable )
	{
		// player shit
		CPlayer* player = GCommonGame->GetPlayer();
		if ( player )
		{
			player->DeactivateActionArea( this );
		}
	}

	TBaseClass::SetEnabled( enable );
}

void CActionAreaComponent::EnableOffMeshConnection( Bool enable )
{
	m_isConnectionEnabled = enable;
	ASSERT( false, TXT("NOT YET REIMPLEMENTED") );
	//if ( IsAttached() && GetLayer() && GetLayer()->GetWorld() && GetLayer()->GetWorld()->GetPathEngineWorld() )
	//{
	//	GetLayer()->GetWorld()->GetPathEngineWorld()->UpdateOffMeshConnection( this );
	//}
}

void CActionAreaComponent::OnAttached( CWorld* world )
{
	// TODO: remove when VER_EXPLORATION_AREA_FRONT_IS_EY is outdated
	if ( m_worldPoints.Size() != m_localPoints.Size() )
	{
		LOG_GAME( TXT("Exploration '%ls' is outdated"), GetFriendlyName().AsChar() );

		ForceUpdateTransformNodeAndCommitChanges();

		Matrix localToWorld = GetLocalToWorld();
		m_worldPoints.Resize( m_localPoints.Size() );
		for ( Uint32 i=0; i<m_localPoints.Size(); i++ )
		{
			m_worldPoints[i] = localToWorld.TransformPoint( m_localPoints[i] );
		}
		OnUpdateBounds();
	}

	TBaseClass::OnAttached( world );

	// Get start point and end point if it is not already known
	if ( !m_totalTransformationIsKnown )
	{
		UpdateTotalTransformation();
	}

#ifndef NO_EDITOR
	if ( ! GGame->IsActive() )
	{
		m_editorData = new EditorOnlyData();
	}
#endif

	CacheOffMeshLinks( world );
	m_isConnectionEnabled = IsEnabled();
	m_nextLinkIdx = 0;
	ToggleNextLink();

	//CPathEngineWorld* peWorld = world->GetPathEngineWorld();
	//if( peWorld )
	//{
	//	peWorld->RegisterOffMeshConnection( this, GetFriendlyName() );
	//	peWorld->UpdateOffMeshConnection( this );
	//}
	
	// Add to editor fragment group
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Bboxes );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Exploration );
}

void CActionAreaComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// Remove from tick groups
	world->GetTickManager()->Remove( this );

	if ( m_editorData )
	{
		delete m_editorData;
		m_editorData = NULL;
	}

	//if ( world->GetPathEngineWorld() )
	//{
	//	world->GetPathEngineWorld()->UnregisterOffMeshConnection( this );
	//}

	CPlayer * player = GCommonGame->GetPlayer();
	if ( player )
	{
		GCommonGame->GetPlayer()->DeactivateActionArea( this );
	}

	// Remove from editor fragment group
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Bboxes );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Exploration );
}

void CActionAreaComponent::CacheOffMeshLinks( CWorld* world )
{
	CalculateOffMeshLinks( );
}

void CActionAreaComponent::UpdateTotalTransformation()
{
	CalculateTotalShift();
	RefreshInPathEngine();
	InvalidateParabola( false );
}


void CActionAreaComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CActionAreaComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	if ( ! this->IsAttached() )
		return;

	// Refresh if not in game
	if ( !GGame->IsActive() )
	{
		RefreshInPathEngine();
		InvalidateParabola( false );
	}
}

/************************************************************************/
/* Component update                                                     */
/************************************************************************/
void CActionAreaComponent::EnteredArea( CComponent* component )
{
	CPlayer* player = GCommonGame->GetPlayer();
	if ( ! component || component->GetEntity() != player )
	{
		return;
	}

	if( !player->IsInCombat() || ( m_allowedGroups & AAG_PlayerInCombat ) != 0 )
	{
		if ( IsAvailableForAgent( PEAT_Player ) )
		{
			player->ActivateActionArea( this );
		}
	}
	else
	{
		CallEvent( CNAME(OnPlayerInCombatEnteredArea), true );
        if ( IsAvailableForAgent( PEAT_Player ) )
        {
            GGame->GetActiveWorld()->GetTickManager()->AddToGroup( this, TICK_PostPhysics );
        }
	}
}

void CActionAreaComponent::ExitedArea( CComponent* component )
{
	CPlayer* player = GCommonGame->GetPlayer();
	if ( ! component || component->GetEntity() != player )
	{
		return;
	}

	if ( IsAvailableForAgent( PEAT_Player ) )
	{
		player->DeactivateActionArea( this );
	}

	if ( player->IsInCombat() && (m_allowedGroups & AAG_PlayerInCombat) == 0 )
	{
		CallEvent( CNAME(OnPlayerInCombatEnteredArea), false );
	}

    if ( IsAvailableForAgent( PEAT_Player ) )
    {
        GGame->GetActiveWorld()->GetTickManager()->RemoveFromGroup( this, TICK_PostPhysics );
    }
}

void CActionAreaComponent::OnTickPostPhysics( Float timeDelta )
{
	PC_SCOPE( ActionAreaComponent_OnTickPostPhysics )

    CPlayer* player = GCommonGame->GetPlayer();
    if ( ! player->IsInCombat() )
    {
        player->ActivateActionArea( this );
        GGame->GetActiveWorld()->GetTickManager()->RemoveFromGroup( this, TICK_PostPhysics );
    }
}

void CActionAreaComponent::TriggerExploration( CPlayer * player )
{	
	THandle< CActionAreaComponent > explorationArea( this );
	Bool res = ( CR_EventSucceeded == player->CallEvent( CNAME( OnUseExploration ), explorationArea ) );
	if ( res )
	{
		player->DeactivateActionArea( this );
	}
}

Bool CActionAreaComponent::CanBeAutoTriggered( CPlayer * player ) const
{
	// If player is in the trigger exploration may be started manually (slide should be started)
	if ( IsInteractionTriggered() )
	{
		return false;
	}

	// Check heading condition for automatic exploration
	Float diff = DistanceBetweenAnglesAbs( player->GetWorldYaw(), GetWorldYaw() );
	return diff < GetAngleTolerance();
}

Bool CActionAreaComponent::IsAvailableForAgent( EPathEngineAgentType agentType ) const
{
	switch( agentType )
	{
	case PEAT_Player:
		{
			return ( m_allowedGroups & AAG_Player ) == AAG_Player;
		}

	case PEAT_TallNPCs:
		{
			return ( m_allowedGroups & AAG_TallNPCs ) == AAG_TallNPCs;
		}

	case PEAT_ShortNPCs:
		{
			return ( m_allowedGroups & AAG_ShortNPCs ) == AAG_ShortNPCs;
		}

	case PEAT_Monsters:
		{
			return ( m_allowedGroups & AAG_Monsters ) == AAG_Monsters;
		}

	case PEAT_Ghost:
		{
			return ( m_allowedGroups & AAG_Ghost ) == AAG_Ghost;
		}

	default:
		{
			ASSERT( false && TXT( "Unknown agent type" ) );
			return false;
		}
	}
}

/************************************************************************/
/* Editor animation tunning                                             */
/************************************************************************/

void CActionAreaComponent::CalculateTotalShift()
{
	// Create ghost and retrieve info from animation
	CName preAnim  = GetAnimationPre( AAG_Player );
	CName loopAnim = GetAnimationLoop( AAG_Player );
	CName postAnim = GetAnimationPost( AAG_Player );

	// delete old animations
	for ( TDynArray< CAnimDef* >::iterator it = m_animations.Begin(); it != m_animations.End(); ++it )
	{
		if ( *it )
		{
			(*it)->Discard();
		}
	}
	m_animations.Clear();

	if ( !preAnim.Empty() )
	{
		m_animations.PushBack( ::CreateObject< CAnimDef >( this )->Initialize( preAnim, NULL ) );
	}

	if ( !loopAnim.Empty() )
	{
		Uint32 count = GetLoopCount();
		for ( Uint32 i = 0; i < count; ++i )
		{
			m_animations.PushBack( ::CreateObject< CAnimDef >( this )->Initialize( loopAnim, m_animations.Empty() ? NULL : m_animations.Back() ) );
		}
	}

	if ( !postAnim.Empty() )
	{
		m_animations.PushBack( ::CreateObject< CAnimDef >( this )->Initialize( postAnim, m_animations.Empty() ? NULL : m_animations.Back() ) );
	}

	Matrix totalShift = Matrix::IDENTITY;
	if ( !m_animations.Empty() )
	{
		totalShift = m_animations.Back()->GetTotalTransform();
	}
	SetTotalTransformation( totalShift );
}
#ifndef NO_EDITOR
Bool CActionAreaComponent::OnEditorBeginSpriteEdit( TDynArray< CEntity* > &spritesToEdit )
{
	ASSERT( !m_editorData->m_blendActors.Size() );

	// spawn ghosts
	Bool hasShiftsDefined = false;
	Uint32 shiftsCount = 0;
	CAnimDef* shiftAnimDef = NULL;
	Matrix localToWorldTransform = UnscaleMatrix( GetLocalToWorld() );

	// aquire a layer on which the ghosts will be spawned
	CLayer* layer = GetLayer();
	if ( !layer )
	{
		layer = GGame->GetActiveWorld()->GetDynamicLayer();
	}

	if ( !layer )
	{
		// there's no layer to edit the shit on - break out
		return false;
	}


	for ( TDynArray< CAnimDef* >::iterator animIt = m_animations.Begin(); animIt != m_animations.End(); ++animIt )
	{
		(*animIt)->SpawnGhosts( layer, localToWorldTransform, m_editorData->m_blendActors );

		Uint32 currAnimShiftsCount = (*animIt)->GetShiftsCount();
		if ( currAnimShiftsCount > 0 )
		{
			shiftAnimDef = *animIt;
		}

		shiftsCount += currAnimShiftsCount;
		hasShiftsDefined |= (*animIt)->GetTotalShift().Mag3() > 1e-3;
	}

	// if there are no shifts defined, and the animation contains only a single shift definition - attempt to 
	// snap the transformation to the underlying ground
	if ( !hasShiftsDefined && shiftsCount == 1 && shiftAnimDef )
	{
		Matrix groundTransform;
		GetGroundTransform( m_fullTransformation, groundTransform );
		TDynArray< Vector > shifts;
		shifts.PushBack( groundTransform.GetTranslation() - m_fullTransformation.GetTranslation() );

		shiftAnimDef->SetShifts( shifts );

		Matrix newTransofrm = m_animations.Back()->GetTotalTransform();
		SetTotalTransformation( newTransofrm );
		RefreshInPathEngine();
		InvalidateParabola( false );
	}

	// memorize the goals and initialize their transformations
	for ( Uint32 i = 0; i < m_editorData->m_blendActors.Size(); ++i )
	{
		spritesToEdit.PushBack( m_editorData->m_blendActors[i] );
		m_editorData->m_blendActors[ i ]->UpdatePosition( localToWorldTransform );
	}

	return true;
}

void CActionAreaComponent::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	ASSERT( vertexIndex >= 0 );
	ASSERT( (Uint32) vertexIndex < m_editorData->m_blendActors.Size() );

	if ( m_editorData->m_lockBlendActorUpdates )
	{
		return;
	}

	Red::System::ScopedFlag< Bool > safeLock( m_editorData->m_lockBlendActorUpdates = true, false );
	Matrix unscaledLocalToWorld = UnscaleMatrix( GetLocalToWorld() );
	CActionAreaBlendActor* editedActor = m_editorData->m_blendActors[ vertexIndex ];

	if ( !MarkModified() || !editedActor->SetBlendPosition( unscaledLocalToWorld, wishedPosition ) )
	{
		allowedPosition = oldPosition;
		return;
	}

	allowedPosition = wishedPosition;

	// update positions of sprites
	Uint32 count = m_editorData->m_blendActors.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_editorData->m_blendActors[ i ]->UpdatePosition( unscaledLocalToWorld );
	}

	// update the total transformation
	SetTotalTransformation( m_animations.Back()->GetTotalTransform() );
	RefreshInPathEngine();
	InvalidateParabola( false );
}

void CActionAreaComponent::OnEditorEndSpriteEdit()
{
	for ( Uint32 i=0; i < m_editorData->m_blendActors.Size(); ++i )
	{
		CEntity* actor = m_editorData->m_blendActors[ i ];
		actor->Destroy();
	}
	m_editorData->m_blendActors.Clear();
}

Bool CActionAreaComponent::OnEditorBeginVertexEdit( TDynArray< Vector >& vertices, Bool& isClosed, Float& height )
{
	return false;
}

#endif

void CActionAreaComponent::GetGroundTransform( const Matrix& originalTransformation, Matrix& modifiedTransform ) const
{
	Matrix transform = originalTransformation * UnscaleMatrix( GetLocalToWorld() );
	modifiedTransform = originalTransformation;
	Vector pos = transform.GetTranslation();
	TraceResultPlacement result;
	if ( CTraceTool::StaticAgentPlacementTraceTest( GGame->GetActiveWorld(), pos, 0.4f, result ) )
	{
		ASSERT( result.m_height != INVALID_Z );
		ASSERT( result.m_isValid );

		Float delta = result.m_height - pos.Z;
		Vector newTranslation = originalTransformation.GetTranslation();
		newTranslation.Z += delta;
		modifiedTransform.SetTranslation( newTranslation );
	}
}


#ifndef NO_DATA_VALIDATION
void CActionAreaComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );
}
#endif // NO_DATA_VALIDATION

void CActionAreaComponent::GetEndpointTransformation( Matrix& outTransformation ) const
{
	if ( m_totalTransformationIsKnown )
	{
		outTransformation = m_fullTransformation;
	}
	else
	{
		outTransformation = Matrix::IDENTITY;
	}
}

Vector CActionAreaComponent::GetActionEndPosition( const Vector& startPosition ) const
{
	Vector actionStartPos = GetClosestActionPosition( startPosition, true );
	Vector localActionStartPos = GetWorldToLocal().TransformPoint( actionStartPos );

	Matrix transform;
	GetEndpointTransformation( transform );
	Vector localActionEndPos = transform.TransformPoint( localActionStartPos );
	return GetLocalToWorld().TransformPoint( localActionEndPos );
}

Vector CActionAreaComponent::GetClosestPosition( const Vector& userPos ) const
{
	return GetClosestActionPosition( userPos, true );
}

EulerAngles CActionAreaComponent::GetOrientation() const
{
	EulerAngles angle = GetWorldRotation();
	return angle;
}

void CActionAreaComponent::ToggleNextLink()
{
}

void CActionAreaComponent::NotifyConnectionUsage()
{
	ToggleNextLink();
}

///////////////////////////////////////////////////////////////////////////////

CAnimDef* CAnimDef::Initialize( const CName& animName, CAnimDef* parent )
{
	m_animName = animName;
	m_parent = parent;

	CSkeletalAnimationSetEntry* animation = GetAnimation();
	if ( animation )
	{
		CalculateTotalShiftForAnimation( *animation );
	}

	return this;
}

Matrix CAnimDef::GetParentTotalTransform() const
{
	if ( m_parent )
	{
		return m_parent->GetTotalTransform();
	}
	else
	{
		return Matrix::IDENTITY;
	}
}

void CAnimDef::SetPosition( Uint32 shiftIdx, const Vector& pos )
{
	Matrix transform = Matrix::IDENTITY;
	if ( shiftIdx == 0 && m_parent )
	{
		transform = m_parent->GetTotalTransform();
	}
	else if ( shiftIdx > 0 )
	{
		transform = GetShiftTransform( shiftIdx - 1 );
	}

	Vector shiftLocalPos = transform.Inverted().TransformPoint( pos );
	m_shifts[ shiftIdx ].m_transform.SetTranslation( shiftLocalPos ); 
}

Matrix CAnimDef::GetShiftTransform( Uint32 shiftIdx ) const
{
	Matrix mtx = Matrix::IDENTITY;
	for ( Uint32 i = 0; i <= shiftIdx; ++i )
	{
		mtx = m_shifts[i].m_transform * mtx;
	}

	if ( m_parent )
	{
		mtx = mtx * m_parent->GetTotalTransform();
	}
	return mtx;
}

Vector CAnimDef::GetTotalShift() const
{
	Matrix actualTransform = Matrix::IDENTITY;
	Matrix originalTransform = Matrix::IDENTITY;
	for ( TDynArray< SAnimShift >::const_iterator it = m_shifts.Begin(); it != m_shifts.End(); ++it )
	{
		actualTransform = it->m_transform * actualTransform;
		originalTransform = it->m_originalTransform * originalTransform;
	}
	Vector totalShift = actualTransform.GetTranslation() - originalTransform.GetTranslation();
	return totalShift;
}

Matrix CAnimDef::GetTotalTransform() const
{

	Vector totalShift = GetTotalShift();
	Matrix mtx = m_totalTransform;
	mtx.SetTranslation( mtx.GetTranslation() + totalShift );
	if ( m_parent )
	{
		Matrix parentTransform = m_parent->GetTotalTransform();
		mtx = mtx * parentTransform;
	}

	return mtx;
}

void CAnimDef::SetupTrajectoryBlendPoints( TSortedArray< CSlotAnimationShiftingInterval >& outBlendPoints ) const
{
	CSkeletalAnimationSetEntry* animation = GetAnimation();
	if ( !animation )
	{
		return;
	}

	animation->GetTrajectoryBlendPoints( outBlendPoints );
	ASSERT ( outBlendPoints.Size() == m_shifts.Size() && TXT( "Exploration animation has changed, and an exploration area using it was not updated - correct that now" ) );
	if ( outBlendPoints.Size() != m_shifts.Size() )
	{
		return;
	}

	Matrix actualTransform = Matrix::IDENTITY;
	Matrix originalTransform = Matrix::IDENTITY;
	Matrix shift = Matrix::IDENTITY;
	Uint32 count = outBlendPoints.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		actualTransform = m_shifts[i].m_transform * actualTransform;
		originalTransform = m_shifts[i].m_originalTransform * originalTransform;

		shift = shift.Invert() * originalTransform.Inverted() * actualTransform;
		outBlendPoints[i].SetShift( originalTransform.TransformVector( shift.GetTranslation() ) );
	}
}

void CAnimDef::CalculateTotalShiftForAnimation( CSkeletalAnimationSetEntry& animation )
{
	// Get animation one loop transformation and duration
	m_totalTransform = Matrix::IDENTITY;
	m_duration = animation.GetDuration();
	if ( animation.GetAnimation()->HasExtractedMotion() )
	{
		animation.GetAnimation()->GetMovementAtTime( m_duration, m_totalTransform );
	}
	const CSkeletalAnimation* skAnimation = animation.GetAnimation();

	// Initialize/update base points and deltas AND collect times of blending
	TSortedArray< CSlotAnimationShiftingInterval > baseShifts;
	animation.GetTrajectoryBlendPoints( baseShifts );

	// start transformation
	m_shifts.Clear();

	Matrix prevLocalShiftTransform = Matrix::IDENTITY;
	for ( TSortedArray< CSlotAnimationShiftingInterval >::iterator it = baseShifts.Begin(); it != baseShifts.End(); ++it )
	{
		Float stopTime = it->GetStopTime();
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform transform;
		if ( skAnimation->HasExtractedMotion() )
		{
			transform = skAnimation->GetMovementAtTime( stopTime );
		}
		else
		{
			transform.setZero();
		}


		Matrix localShiftTransform;
		HavokTransformToMatrix( transform, &localShiftTransform );
		Matrix relShiftTransform = localShiftTransform * prevLocalShiftTransform.Inverted();
		prevLocalShiftTransform = localShiftTransform;
#else
		RedQsTransform transform;
		if ( skAnimation->HasExtractedMotion() )
		{
			transform = skAnimation->GetMovementAtTime( stopTime );
		}
		else
		{
			transform.SetZero();
		}


		Matrix localShiftTransform;
		RedMatrix4x4 conversionMatrix = transform.ConvertToMatrix();
		localShiftTransform = reinterpret_cast< const Matrix& >( conversionMatrix );
		Matrix relShiftTransform = localShiftTransform * prevLocalShiftTransform.Inverted();
		prevLocalShiftTransform = localShiftTransform;
#endif
		m_shifts.PushBack( SAnimShift( relShiftTransform, stopTime ) );
	}
}

void CAnimDef::SetShifts( const TDynArray< Vector >& shifts )
{
	//ASSERT( shifts.Size() == m_shifts.Size() && "Incompatible exploration animation shifts definitions." );
	if ( shifts.Size() != m_shifts.Size() )
	{
		return;
	}

	Uint32 count = m_shifts.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_shifts[i].m_transform = m_shifts[i].m_originalTransform;
		m_shifts[i].m_transform.SetTranslation( m_shifts[i].m_transform.GetTranslation() + shifts[i] );
	}
}

void CAnimDef::SpawnGhosts( CLayer* layer, const Matrix& originTransform, TDynArray< CActionAreaBlendActor* >& outGhosts )
{
	CEntityTemplate* actorTemplate = resExplorationBlendEntity.LoadAndGet< CEntityTemplate >();
	ASSERT( actorTemplate );

	Matrix transform = GetParentTotalTransform() * originTransform;
	CActionAreaBlendActor* actor = SpawnGhost( layer, transform, actorTemplate, m_animName, 0.0f );
	actor->Initialize( *this, -1 );
	outGhosts.PushBack( actor );

	Matrix shiftTransform = transform;
	for ( TDynArray< SAnimShift >::const_iterator it = m_shifts.Begin(); it != m_shifts.End(); ++it )
	{
		shiftTransform = it->m_transform * shiftTransform;
		actor = SpawnGhost( layer, shiftTransform, actorTemplate, m_animName, it->m_time );
		actor->Initialize( *this, PtrDiffToInt32( ( void* )( it - m_shifts.Begin() ) ) );
		outGhosts.PushBack( actor );
	}

	transform = GetTotalTransform() * originTransform;
	actor = SpawnGhost( layer, transform, actorTemplate, m_animName, m_duration );
	actor->Initialize( *this, -2 );
	outGhosts.PushBack( actor );
}

CActionAreaBlendActor* CAnimDef::SpawnGhost( CLayer* layer, const Matrix& transform, CEntityTemplate* witcherTemplate, const CName& animationName, Float animationTime ) const
{
	EntitySpawnInfo sinfo;
	sinfo.m_spawnPosition = transform.GetTranslation();
	sinfo.m_spawnRotation = transform.GetAxisY().ToEulerAngles();
	sinfo.m_template = witcherTemplate;
	sinfo.m_detachTemplate = false;
	CActionAreaBlendActor* actor = Cast< CActionAreaBlendActor >( layer->CreateEntitySync( sinfo ) );
	ASSERT( actor );

	// Force creation of animations
	if ( GGame->GetActiveWorld() )
	{
		GGame->GetActiveWorld()->DelayedActions();
	}

	// Play animation
	CAnimatedComponent* ac = actor->GetRootAnimatedComponent();
	if ( ac )
	{
		if ( !ac->PlayAnimationAtTimeOnSkeleton( animationName, animationTime ) )
		{
			LOG_GAME( TXT("Cannot play animation %s in sprite editor"), animationName.AsString().AsChar() );
		}
	}

	return actor;
}

CSkeletalAnimationSetEntry* CAnimDef::GetAnimation() const
{
	// Load animations
	CSkeletalAnimationSet* animSet1  = resExplorationAnimSet1.LoadAndGet< CSkeletalAnimationSet >();
	CSkeletalAnimationSet* animSet2  = resExplorationAnimSet2.LoadAndGet< CSkeletalAnimationSet >();
	ASSERT( animSet1 );
	ASSERT( animSet2 );

	CSkeletalAnimationSetEntry* animation = animSet1->FindAnimation( m_animName );
	if ( !animation )
	{
		animation = animSet2->FindAnimation( m_animName );
	}
	return animation;
}

///////////////////////////////////////////////////////////////////////////////

CActionAreaBlendActor::CActionAreaBlendActor()
	: m_animationDefinition( NULL )
	, m_shiftIdx( -3 )
{
}

void CActionAreaBlendActor::Initialize( CAnimDef& animationDefinition, Int32 shiftIdx )
{
	ASSERT( shiftIdx >= -2 && TXT( "Invalid blend actor shift idx" ) );

	m_animationDefinition = &animationDefinition;
	m_shiftIdx = shiftIdx;
}

Bool CActionAreaBlendActor::SetBlendPosition( const Matrix& originTransform, const Vector& pos )
{
	if ( m_shiftIdx < 0 )
	{
		return false;
	}

	m_animationDefinition->SetPosition( m_shiftIdx, originTransform.Inverted().TransformPoint( pos ) );

	return true;
}

void CActionAreaBlendActor::UpdatePosition( const Matrix& originTransform )
{
	Matrix transform = Matrix::IDENTITY;
	
	if ( m_shiftIdx == -1 )
	{
		transform = m_animationDefinition->GetParentTotalTransform() * originTransform;
	}
	else if ( m_shiftIdx == -2 )
	{ 
		transform = m_animationDefinition->GetTotalTransform() * originTransform;
	}
	else if ( m_shiftIdx >= 0 )
	{
		transform = m_animationDefinition->GetShiftTransform( m_shiftIdx ) * originTransform;
	}
	else
	{
		ASSERT( false && TXT( "Invalid blend actor shift idx" ) );
	}
	SetPosition( transform.GetTranslation() );
}

///////////////////////////////////////////////////////////////////////////////
