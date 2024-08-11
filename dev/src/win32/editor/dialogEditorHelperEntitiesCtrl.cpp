
#include "build.h"
#include "dialogEditorHelperEntitiesCtrl.h"
#include "dialogEditor.h"
#include "..\..\common\engine\curveEntity.h"
#include "..\..\common\engine\curveControlPointEntity.h"
#include "..\..\common\engine\curveControlPointEntity.h"
#include "..\..\common\engine\curveTangentControlPointEntity.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/world.h"
#include "../../common/engine/tickManager.h"
#include "../../common/game/storySceneUtils.h"

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CEdSceneHelperComponent );

CEdSceneHelperComponent::CEdSceneHelperComponent()
	: m_visible( true )
{

}

void CEdSceneHelperComponent::InitShape( const CEdSceneHelperShapeSettings* s )
{
	if ( s )
	{
		m_settings = *s;
	}
}

void CEdSceneHelperComponent::SetVisible( Bool flag )
{
	m_visible = flag;

	if ( !m_visible && IsSelected() )
	{
		SCENE_ASSERT( GetEntity()->GetLayer()->GetWorld()->GetSelectionManager() );
		if ( GetEntity()->GetLayer()->GetWorld()->GetSelectionManager() )
		{
			GetEntity()->GetLayer()->GetWorld()->GetSelectionManager()->Deselect( this );
		}
	}
}

void CEdSceneHelperComponent::ToggleInteractive()
{
	m_settings.m_isInteractive = !m_settings.m_isInteractive;
}

void CEdSceneHelperComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_VisualDebug );
}

void CEdSceneHelperComponent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_VisualDebug );

	TBaseClass::OnDetached( world );
}

void CEdSceneHelperComponent::SetRawRefPlacement( const Vector* pos, const EulerAngles* rot, const Vector* s )
{
	if ( pos )
	{
		m_refTransform.SetPosition( *pos );
	}
	if ( rot )
	{
		m_refTransform.SetRotation( *rot );
	}
	if ( s )
	{
		m_refTransform.SetScale( *s );
	}
}

void CEdSceneHelperComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	if ( !m_visible )
	{
		return;
	}

	// Get local-to-world transform without scale
	Matrix localToWorld;
	GetLocalToWorld( localToWorld );

	Matrix localToWorldNoScale;
	Vector scale;
	localToWorld.ExtractScale( localToWorldNoScale, scale );
	localToWorldNoScale.SetTranslation( localToWorld.GetTranslation() );

	// Get bounding box
	const Box& boundingBox = m_settings.m_box;

	// Hit proxy mode
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
#ifndef NO_COMPONENT_GRAPH
		if ( m_settings.m_isBox )
		{
			frame->AddDebugSolidBox( boundingBox, localToWorldNoScale, m_hitProxyId.GetColor(), true );
		}
		
		if ( m_settings.m_isBone )
		{
			Matrix parentL2W;
			m_refTransform.CalcLocalToWorld( parentL2W );
			frame->AddDebugBone( localToWorldNoScale, parentL2W, m_hitProxyId.GetColor(), true, m_settings.m_boneScale );
		}
#endif
		return;
	}

	// Bright when selected
	Color color = m_settings.m_color;
	if ( !IsSelected() )
	{
		color.R /= m_settings.m_selectionColorDiv;
		color.G /= m_settings.m_selectionColorDiv;
		color.B /= m_settings.m_selectionColorDiv;
	}

	if ( m_settings.m_isBox )
	{
		ERenderingSortGroup sortGroup;
		if ( m_settings.m_isInteractive )
		{
			sortGroup = RSG_Sprites;
		}
		else
		{
			sortGroup = m_settings.m_overlayMode ? RSG_DebugOverlay : RSG_DebugUnlit;
		}

		frame->AddDebugSolidBox( boundingBox, localToWorldNoScale, color, sortGroup );
		frame->AddDebugBox( boundingBox, localToWorldNoScale, m_settings.m_wireFrameColor, sortGroup );
	}

	if ( m_settings.m_isBone )
	{
		Matrix parentL2W;
		m_refTransform.CalcLocalToWorld( parentL2W );
		frame->AddDebugBone( localToWorldNoScale, parentL2W, color, m_settings.m_overlayMode, m_settings.m_boneScale );
	}

	if ( !GetEntity()->GetName().Empty() && ( !m_settings.m_nameOnlyWhenSelected || ( m_settings.m_nameOnlyWhenSelected && IsSelected() ) ) )
	{
		frame->AddDebugText( localToWorld.GetTranslation(), GetEntity()->GetName(), 10, 1 );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CEdSceneHelperEntity );

IMPLEMENT_ENGINE_CLASS( CEdSceneHelperEntityForLightEvent );

IMPLEMENT_ENGINE_CLASS( CEdSceneHelperEntityForDurationLookat );

CEdSceneHelperEntity::CEdSceneHelperEntity()
	: m_callback( nullptr )
	, m_offsetWS( Vector::ZERO_3D_POINT )
	, m_visible( true )
{

}

CEdSceneHelperEntity::~CEdSceneHelperEntity()
{
	if ( m_callback )
	{
		delete m_callback;
	}
}

void CEdSceneHelperEntity::Init( IEdSceneHelperEntityCallbackObject* callback, CEdSceneEditor* editorCallback )
{
	SCENE_ASSERT( !m_callback );
	m_callback = callback;
	m_editorCallback = editorCallback;
}

Bool CEdSceneHelperEntity::GetId( CGUID& id ) const
{
	if ( m_callback && m_callback->HasId() )
	{
		id = m_callback->GetId();
		return true;;
	}
	return false;
}


void CEdSceneHelperEntity::SelectHelper()
{
	RepositionHelper();
	m_visible = true;
	for ( ComponentIterator< CEdSceneHelperComponent > it( this ); it; ++it )
	{
		(*it)->SetVisible( m_visible );
	}
	if ( IsAttached() && GetLayer() && GetLayer()->GetWorld() )
	{
		GetLayer()->GetWorld()->GetTickManager()->AddEntity( this );
	}
}

void CEdSceneHelperEntity::DeselectHelper()
{
	m_offsetWS = Vector::ZERO_3D_POINT;
	m_visible = false;
	for ( ComponentIterator< CEdSceneHelperComponent > it( this ); it; ++it )
	{
		(*it)->SetVisible( m_visible );
	}
	if ( IsAttached() && GetLayer() && GetLayer()->GetWorld() )
	{
		GetLayer()->GetWorld()->GetTickManager()->RemoveEntity( this );
	}
}

void CEdSceneHelperEntity::RepositionHelper()
{
	Vector posSS( Vector::ZERO_3D_POINT );
	EulerAngles rotSS( EulerAngles::ZEROS );

	if ( m_callback )
	{	
		m_callback->GetTransform( posSS, rotSS );
		posSS.W = 1.f;
	}
	else
	{
		// TODO
		posSS = GetWorldPosition();
		rotSS = GetWorldRotation();
	}

	Matrix toWorldSpace;
	if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_SceneFrame )
	{
		EngineTransform sceneToWorld = m_editorCallback->OnHelperEntity_GetSceneToWorld();
		sceneToWorld.CalcLocalToWorld( toWorldSpace );
	}
	else if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_ActorFrame )
	{
		if( CActor* actor = m_frameSettings.m_parentActor.Get() )
		{
			actor->GetLocalToWorld( toWorldSpace );				
		}
	}
	else if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_BoneFrame )
	{
		toWorldSpace = CalculateL2WForLocalFrame();
	}
	else if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_SlotFrame )
	{
		toWorldSpace = CalculateL2WForSlotFrame();
	}
	else if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_WorldFrame )
	{
		toWorldSpace = Matrix::IDENTITY;
	}

	Matrix matRotWS = rotSS.ToMatrix() * toWorldSpace;

	const EulerAngles rotWS = matRotWS.ToEulerAngles();
	const Vector posWS = toWorldSpace.TransformPoint( posSS );

	Vector pointOnScreenWS( Vector::ZERO_3D_POINT );
	const Bool enableFloatingHelper = m_editorCallback->OnHelperEntity_FloatingHelpersEnabled();
	const Bool isPointOnScreen = m_editorCallback->OnHelperEntity_IsPointOnScreen( posWS, pointOnScreenWS );
	if ( enableFloatingHelper && !isPointOnScreen )
	{
		m_offsetWS = pointOnScreenWS - posWS;
	}
	else
	{
		m_offsetWS = Vector::ZERO_3D_POINT;
	}

	Move( posWS + m_offsetWS, rotWS );	
}

void CEdSceneHelperEntity::Move( const Vector& posWS, const EulerAngles& rotWS )
{
	SetRawPlacement( &posWS, &rotWS, nullptr );

	ForceUpdateTransformNodeAndCommitChanges();
	ForceUpdateBoundsNode();
	if( CWorld* world = GetLayer()->GetWorld() )
	{
		world->GetSelectionManager()->RefreshPivot();
	}
}

void CEdSceneHelperEntity::SetColor( const Color& color )
{
	for ( ComponentIterator< CEdSceneHelperComponent > it( this ); it; ++it )
	{
		(*it)->SetColor( color );
	}
}

void CEdSceneHelperEntity::SetRawRefPlacement( const Vector* pos, const EulerAngles* rot, const Vector* s )
{
	for ( ComponentIterator< CEdSceneHelperComponent > it( this ); it; ++it )
	{
		(*it)->SetRawRefPlacement( pos, rot, s );
	}
}



Matrix CEdSceneHelperEntity::CalculateL2WForLocalFrame() const
{
	ASSERT( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_BoneFrame );

	if ( CAnimatedComponent* ac = m_frameSettings.m_parentComp.Get() )
	{
		return StorySceneUtils::CalcL2WForAttachedObject( ac, m_frameSettings.m_boneName, m_frameSettings.m_attachmentFlags );
	}
	return Matrix::IDENTITY;
}

Matrix CEdSceneHelperEntity::CalculateL2WForSlotFrame() const
{
	ASSERT( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_SlotFrame );

	Matrix res =  Matrix::IDENTITY;
	if ( CEntity* ent = m_frameSettings.m_slotOwner.Get() )
	{
		if( const CEntityTemplate* templ = ent->GetEntityTemplate() )
		{
			if ( const EntitySlot* entitySlot = templ->FindSlotByName( m_frameSettings.m_boneName, true ) )		
			{
				entitySlot->CalcMatrix( ent, res, nullptr );
			}
		}		
	}
	return res;
}

Matrix CEdSceneHelperEntity::CalculateToLocalSpaceMat() const 
{
	Matrix toLocalSpace( Matrix::IDENTITY );
	if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_SceneFrame )
	{
		EngineTransform sceneToWorld = m_editorCallback->OnHelperEntity_GetSceneToWorld();
		sceneToWorld.CalcWorldToLocal( toLocalSpace );
	}
	else if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_ActorFrame )
	{
		if( CActor* actor = m_frameSettings.m_parentActor.Get() )
		{
			actor->GetWorldToLocal( toLocalSpace );				
		}
	}
	else if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_BoneFrame )
	{
		toLocalSpace = CalculateL2WForLocalFrame().FullInverted();			
	}
	else if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_SlotFrame )
	{
		toLocalSpace = CalculateL2WForSlotFrame().FullInverted();			
	}
	else if( m_frameSettings.m_frameType == SSceneHelperReferenceFrameSettings::RF_WorldFrame )
	{
		toLocalSpace = Matrix::IDENTITY;
	}
	return toLocalSpace;
}

void CEdSceneHelperEntity::SetPosition( const Vector& position )
{
	Vector newPosition( position );
	newPosition.W = 1.f;

	if ( m_callback )
	{
		Matrix toLocalSpace = CalculateToLocalSpaceMat();

		const Vector prevPosWS = GetPosition()-m_offsetWS;
		const Vector newPosWS = newPosition-m_offsetWS;

		const Vector prevPosSS = toLocalSpace.TransformPoint( prevPosWS );
		Vector newPosSS = toLocalSpace.TransformPoint( newPosWS );

		m_callback->OnSetPositionFromPreview( prevPosSS, newPosSS );

		if ( m_callback->HasId() && m_editorCallback )
		{
			m_editorCallback->OnHelperEntity_RefreshProperty( m_callback->GetId(), true, false );
		}
	}

	TBaseClass::SetPosition( newPosition );
}

void CEdSceneHelperEntity::SetRotation( const EulerAngles& newRotWS )
{
	if ( m_callback )
	{
		Matrix toLocalSpace = CalculateToLocalSpaceMat();

		EulerAngles oldRotWS = GetWorldRotation();

		Matrix oldMatRotLS = oldRotWS.ToMatrix() * toLocalSpace;
		Matrix newMatRotLS = newRotWS.ToMatrix() * toLocalSpace;

		EulerAngles oldRotLS = oldMatRotLS.ToEulerAngles();
		EulerAngles newRotLS = newMatRotLS.ToEulerAngles();

		m_callback->OnSetRotationFromPreview( oldRotLS, newRotLS );

	
		if ( m_callback->HasId() && m_editorCallback )
		{
			m_editorCallback->OnHelperEntity_RefreshProperty( m_callback->GetId(), false, true );
		}
	}

	TBaseClass::SetRotation( newRotWS );
}

void CEdSceneHelperEntity::SetScale( const Vector& newScale )
{
	if ( m_callback )
	{
		Matrix toLocalSpace = CalculateToLocalSpaceMat();

		Vector prevScale = m_localToWorld.GetScale33();

		Vector prevScaleSS = toLocalSpace.TransformPoint( prevScale );
		Vector newScaleSS = toLocalSpace.TransformPoint( newScale );

		m_callback->OnSetScaleFromPreview( prevScaleSS, newScaleSS );


		if ( m_callback->HasId() && m_editorCallback )
		{
			m_editorCallback->OnHelperEntity_RefreshProperty( m_callback->GetId(), false, true );
		}
	}

	TBaseClass::SetScale( newScale );
}

void CEdSceneHelperEntity::SetFrameSettings( const SSceneHelperReferenceFrameSettings & set )
{
	m_frameSettings = set;	
}

void CEdSceneHelperEntity::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );

	RepositionHelper();
}

//////////////////////////////////////////////////////////////////////////

CEdSceneHelperEntitiesCtrl::~CEdSceneHelperEntitiesCtrl()
{
	SCENE_ASSERT( m_helpers.Size() == 0 );
}

void CEdSceneHelperEntitiesCtrl::Init( CEdSceneEditor* mediator )
{
	m_mediator = mediator;
}

void CEdSceneHelperEntitiesCtrl::Destroy()
{
	DestroyAllHelpers();
}

void CEdSceneHelperEntitiesCtrl::Refresh()
{
	DestroyAllHelpers();
}

void CEdSceneHelperEntitiesCtrl::OnWidgetPostChange( const CNode* node )
{
	for ( Uint32 i=0; i<m_helpers.Size(); ++i )
	{
		if ( CEdSceneHelperEntity* e = m_helpers[ i ].Get() )
		{
			for ( BaseComponentIterator it( e ); it; ++it )
			{
				if ( (*it) == node )
				{
					e->RepositionHelper();
					break;
				}
			}
		}
	}
}

Bool CEdSceneHelperEntitiesCtrl::Exist( const CGUID& id ) const
{
	for ( Uint32 i=0; i<m_helpers.Size(); ++i )
	{
		if ( CEdSceneHelperEntity* e = m_helpers[ i ].Get() )
		{
			CGUID eId;
			if ( e->GetId( eId ) && id == eId )
			{
				return true;
			}
		}
	}

	return false;
}

Bool CEdSceneHelperEntitiesCtrl::Select( const CGUID& id )
{
	for ( Uint32 i=0; i<m_helpers.Size(); ++i )
	{
		if ( CEdSceneHelperEntity* e = m_helpers[ i ].Get() )
		{
			CGUID eId;
			if ( e->GetId( eId ) && id == eId )
			{
				e->SelectHelper();
				return true;
			}
		}
	}

	return false;
}

void CEdSceneHelperEntitiesCtrl::DeselectAllHelpers()
{
	for ( Uint32 i=0; i<m_helpers.Size(); ++i )
	{
		if ( CEdSceneHelperEntity* e = m_helpers[ i ].Get() )
		{
			e->DeselectHelper();
		}
	}
}

Bool CEdSceneHelperEntitiesCtrl::Contains( const CComponent* c ) const
{
	for ( Uint32 i=0; i<m_helpers.Size(); ++i )
	{
		if ( CEntity* e = m_helpers[ i ].Get() )
		{
			for ( BaseComponentIterator it( e ); it; ++it )
			{
				if ( *it == c )
				{
					return true;
				}
			}
		}
	}

	return false;
}

Bool CEdSceneHelperEntitiesCtrl::Contains( const CGUID& id ) const
{
	for ( Uint32 i=0; i<m_helpers.Size(); ++i )
	{
		if ( CEdSceneHelperEntity* e = m_helpers[ i ].Get() )
		{
			CGUID eId;
			if ( e->GetId( eId ) && id == eId )
			{
				return true;
			}
		}
	}

	return false;
}

CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::InternalCreateHelper( const Vector& pos, const EulerAngles& rot, const CEdSceneHelperShapeSettings* s, CClass* customClass )
{
	EntitySpawnInfo info;
	info.m_spawnPosition = pos;
	info.m_spawnRotation = rot;
	if ( customClass )
	{
		info.m_entityClass = customClass;
	}
	else
	{
		info.m_entityClass = ClassID< CEdSceneHelperEntity >();
	}	
	CEdSceneHelperEntity* e = Cast< CEdSceneHelperEntity >( m_mediator->OnHelperEntity_GetWorld()->GetDynamicLayer()->CreateEntitySync( info ) );	
	if ( e )
	{
		CEdSceneHelperComponent* c = Cast< CEdSceneHelperComponent >( e->CreateComponent( ClassID< CEdSceneHelperComponent >(), SComponentSpawnInfo() ) );
		if ( c )
		{
			c->InitShape( s );
		}

		m_helpers.PushBack( e );
	}
	return e;
}

CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::CreateHelper( const CComponent* c, const CEdSceneHelperShapeSettings* s )
{
	return InternalCreateHelper( c->GetEntity()->GetWorldPosition(), c->GetEntity()->GetWorldRotation(), s );
}

CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::CreateHelper( const EngineTransform& transform, const CEdSceneHelperShapeSettings* s )
{
	return InternalCreateHelper( transform.GetPosition(), transform.GetRotation(), s );
}

CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::CreateHelper( const CGUID& id, Vector& vec, const CEdSceneHelperShapeSettings* s )
{
	CEdSceneHelperEntity* e = InternalCreateHelper( vec, EulerAngles::ZEROS, s );
	e->Init( new CEdSceneHelperEntityCallbackObject_VectorEvent( id, vec ), m_mediator );
	return e;
}

CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::CreateHelper( const CGUID& id, EngineTransform& transform, const CEdSceneHelperShapeSettings* s )
{
	CEdSceneHelperEntity* e = InternalCreateHelper( transform.GetPosition(), transform.GetRotation(), s );
	e->Init( new CEdSceneHelperEntityCallbackObject_TransformEvent( id, transform ), m_mediator );
	return e;
}

CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::CreateHelper( const CGUID& id, EngineTransform& transform, Bool& dirtyFlag, const CEdSceneHelperShapeSettings* s )
{
	CEdSceneHelperEntity* e = InternalCreateHelper( transform.GetPosition(), transform.GetRotation(), s, CEdSceneHelperEntityForLightEvent::GetStaticClass() );
	e->Init( new CEdSceneHelperEntityCallbackObject_TransformWithDirtyFlagEvent( id, transform, dirtyFlag ), m_mediator );
	return e;
}


CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::CreateHelperForLightEvent( const CGUID& id, EngineTransform& transform, const CEdSceneHelperShapeSettings* s /*= nullptr */ )
{
	CEdSceneHelperEntity* helper = InternalCreateHelper( transform.GetPosition(), transform.GetRotation(), s, CEdSceneHelperEntityForLightEvent::GetStaticClass() );
	helper->Init( new CEdSceneHelperEntityCallbackObject_LightPropEvent( id, transform ), m_mediator );
	return helper;
}

CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::CreateHelperForDurationLookatEvent( const CGUID& id,  Vector& pos1, Vector& pos2, const CEdSceneHelperShapeSettings* s /*= nullptr */ )
{
	//TODO need to create helper able to control two points, currently two helpers are mapped to the same GUID 
	//and only single helper is returned
	//
	CreateHelper( id, pos1 );
	return CreateHelper( id, pos2 );
}


CEdSceneHelperEntity* CEdSceneHelperEntitiesCtrl::FindHelperById( const CGUID& id )
{
	for ( Int32 i=m_helpers.SizeInt()-1; i>=0; --i )
	{
		if ( CEdSceneHelperEntity* e = m_helpers[ i ].Get() )
		{
			CGUID eId;
			if ( e->GetId( eId ) && eId == id )
			{
				return e;
			}
		}
	}
	return nullptr;
}

void CEdSceneHelperEntitiesCtrl::DestroyHelpers( const CEntity* entity )
{
	// TODO
}

void CEdSceneHelperEntitiesCtrl::DestroyHelpers( const CGUID& id )
{
	for ( Int32 i=m_helpers.SizeInt()-1; i>=0; --i )
	{
		if ( CEdSceneHelperEntity* e = m_helpers[ i ].Get() )
		{
			CGUID eId;
			if ( e->GetId( eId ) && eId == id )
			{
				e->Destroy();
				m_helpers.RemoveAtFast( i );
			}
		}
		else
		{
			m_helpers.RemoveAtFast( i );
		}
	}
}

void CEdSceneHelperEntitiesCtrl::DestroyAllHelpers()
{
	for ( Uint32 i=0; i<m_helpers.Size(); ++i )
	{
		if ( CEntity* e = m_helpers[ i ].Get() )
		{
			e->Destroy();		// NOTE: replaced e->Discard() with e->Destroy() since it was causing problems
								// when switching preview mode in CWorldEditorFragmentsFilter::GenerateEditorFragments()
		}
	}
	m_helpers.Clear();
}

void CEdSceneHelperEntitiesCtrl::UpdateRawPlacement( const CGUID& id, const EngineTransform& t )
{
	CEdSceneHelperEntity* entity = FindHelperById( id );
	if ( entity )
	{
		entity->SetRawPlacement( &t.GetPosition(), &t.GetRotation(), &t.GetScale() );
	}
}

void CEdSceneHelperEntitiesCtrl::UpdateRawPlacement( const CGUID& id, const EngineTransform& t, const EngineTransform& refT )
{
	CEdSceneHelperEntity* entity = FindHelperById( id );
	if ( entity )
	{
		entity->SetRawPlacement( &t.GetPosition(), &t.GetRotation(), &t.GetScale() );
		entity->SetRawRefPlacement( &refT.GetPosition(), &refT.GetRotation(), &refT.GetScale() );
	}
}

void CEdSceneHelperEntitiesCtrl::HandleSelection( CWorld* world )
{
	if ( m_isHandlingSelection )
	{
		return;
	}

	m_isHandlingSelection = true;
	{
		CSelectionManager* selectionMgr = world->GetSelectionManager();
		CSelectionManager::CSelectionTransaction transaction(*selectionMgr);

		TDynArray< CNode* > nodes;
		selectionMgr->GetSelectedNodes( nodes );

		Bool hasSomethingFromScene = false;
		for ( Uint32 i=0; i < nodes.Size(); i++ )
		{
			CComponent* tc = Cast< CComponent >( nodes[i] );
			if ( tc )
			{
				hasSomethingFromScene = hasSomethingFromScene || IsSomethingFromScene( tc );
				hasSomethingFromScene = hasSomethingFromScene || IsSomethingFromCurveEntity( tc );
				hasSomethingFromScene = hasSomethingFromScene || IsHelperFromScene( tc );
			}			
		}

		if ( !RIM_IS_KEY_DOWN( IK_LControl ) && selectionMgr->GetSelectionCount() > 0 )
		{
			m_mediator->OnHelperEntity_ClearSelection();
			m_mediator->OnHelperEntity_SetEditString( String::EMPTY );
			if ( hasSomethingFromScene )
			{
				selectionMgr->DeselectAll();
			}			
		}

		for ( Uint32 i=0; i < nodes.Size(); i++ )
		{
			CComponent* tc = Cast< CComponent >( nodes[i] );
			if ( tc )
			{
				if ( IsSomethingFromCurveEntity( tc ) || IsHelperFromScene( tc ) )
				{
					if ( tc->IsSelected() && !RIM_IS_KEY_DOWN( IK_LShift ) )
					{
						selectionMgr->Deselect( tc );
					}
					else if ( !tc->IsSelected() )
					{
						selectionMgr->Select( tc );
						
						if ( CEdSceneHelperEntity* he = Cast< CEdSceneHelperEntity >( tc->GetEntity() ) )
						{
							he->SelectHelper();
						}
					}
				}
				else if ( !tc->IsSelected() && IsSomethingFromScene( tc ) )
				{
					CNode* node = SelectObjectFromScene( tc );
					if ( node )
					{
						selectionMgr->Select( node );
					}
				}
				else if ( !tc->IsSelected() )
				{
					CNode* node = SelectObjectFromWorld( tc );
					if ( node )
					{
						selectionMgr->Select( node );
					}
				}
			}
		}

		m_mediator->OnHelperEntity_SelectionChanged();
	}
	m_isHandlingSelection = false;
}


Bool CEdSceneHelperEntitiesCtrl::IsSomethingFromScene( const CComponent* c ) const
{
	return m_mediator->OnHelperEntity_IsSomethingFromScene( c );
}

Bool CEdSceneHelperEntitiesCtrl::IsHelperFromScene( const CComponent* c ) const
{
	return Contains( c );
}

CNode* CEdSceneHelperEntitiesCtrl::SelectObjectFromScene( const CComponent* c )
{
	return m_mediator->OnHelperEntity_SelectObjectFromScene( c );
}

CNode* CEdSceneHelperEntitiesCtrl::SelectObjectFromWorld( const CComponent* c )
{
	return m_mediator->OnHelperEntity_SelectObjectFromWorld( c );
}

Bool CEdSceneHelperEntitiesCtrl::IsSomethingFromCurveEntity( const CComponent* c ) const
{
	return c->IsA< CCurveComponent >() || c->IsA< CCurveControlPointComponent >() || c->IsA< CCurveTangentControlPointComponent >();
}

void CEdSceneHelperEntitiesCtrl::CollectSelectedHelpers( TDynArray< CEdSceneHelperEntity* >& entities )
{
	const Uint32 num = m_helpers.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		CEdSceneHelperEntity* e = m_helpers[ i ].Get();
		if ( e && e->IsSelected() )
		{
			entities.PushBack( e );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CEdSceneHelperEntityCallbackObject_VectorEvent::CEdSceneHelperEntityCallbackObject_VectorEvent( const CGUID& id, Vector& vec )
	: m_id( id ), m_vec( vec )
{

}

void CEdSceneHelperEntityCallbackObject_VectorEvent::OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	m_vec = newPos;
}

void CEdSceneHelperEntityCallbackObject_VectorEvent::GetTransform( Vector& outPos, EulerAngles& outRot )
{
	outPos = m_vec;
	outRot = EulerAngles::ZEROS;
}

//////////////////////////////////////////////////////////////////////////

CEdSceneHelperEntityCallbackObject_TransformEvent::CEdSceneHelperEntityCallbackObject_TransformEvent( const CGUID& id, EngineTransform& transform )
	: m_id( id ), m_transform( transform )
{

}

void CEdSceneHelperEntityCallbackObject_TransformEvent::OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	m_transform.SetPosition( newPos );
}

void CEdSceneHelperEntityCallbackObject_TransformEvent::OnSetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	m_transform.SetRotation( newRot );
}

void CEdSceneHelperEntityCallbackObject_TransformEvent::OnSetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_transform.SetScale( newScale );
}

void CEdSceneHelperEntityCallbackObject_TransformEvent::GetTransform( Vector& outPos, EulerAngles& outRot )
{
	outPos = m_transform.GetPosition();
	outRot = m_transform.GetRotation();
}

//////////////////////////////////////////////////////////////////////////

CEdSceneHelperEntityCallbackObject_TransformWithDirtyFlagEvent::CEdSceneHelperEntityCallbackObject_TransformWithDirtyFlagEvent( const CGUID& id, EngineTransform& transform, Bool& dirtyFlag )
	: CEdSceneHelperEntityCallbackObject_TransformEvent( id, transform )
	, m_dirtyFlag( dirtyFlag )
{
}

void CEdSceneHelperEntityCallbackObject_TransformWithDirtyFlagEvent::OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos )
{
	m_transform.SetPosition( newPos );
	m_dirtyFlag = true;
}

void CEdSceneHelperEntityCallbackObject_TransformWithDirtyFlagEvent::OnSetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	m_transform.SetRotation( newRot );
	m_dirtyFlag = true;
}

void CEdSceneHelperEntityCallbackObject_TransformWithDirtyFlagEvent::OnSetScaleFromPreview( const Vector& prevScale, Vector& newScale )
{
	m_transform.SetScale( newScale );
	m_dirtyFlag = true;
}

void CEdSceneHelperEntityForLightEvent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Scenes );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Dimmers );
}

void CEdSceneHelperEntityForLightEvent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Scenes );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Dimmers );
	TBaseClass::OnDetached( world );
}

void CEdSceneHelperEntityForLightEvent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	if ( m_visible )
	{
		if( flags == SHOW_Scenes )
		{
			m_editorCallback->OnHelperEntity_GenerateFragmentsForLight( m_lightId, frame );
		}
		else if ( flags == SHOW_Dimmers )
		{
			m_editorCallback->OnHelperEntity_GenerateFragmentsForDimmers( m_lightId, frame );
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
