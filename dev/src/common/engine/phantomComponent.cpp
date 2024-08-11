/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "phantomComponent.h"
#include "../physics/physicsSimpleBodyWrapper.h"
#include "../physics/compiledCollision.h"
#include "../physics/physicsEngine.h"
#include "mesh.h"
#include "meshEnum.h"
#include "collisionMesh.h"
#include "collisionCache.h"
#include "hardAttachment.h"
#include "phantomAttachment.h"
#include "renderFrame.h"
#include "entity.h"
#include "world.h"
#include "layer.h"
#include "../physics/physicsWorld.h"
#include "../physics/physicsSettings.h"
#include "../core/scriptStackFrame.h"
#include "../core/dataError.h"
#include "physicsDataProviders.h"

IMPLEMENT_ENGINE_CLASS( CPhantomComponent );
IMPLEMENT_RTTI_ENUM( EPhantomShape );
RED_DEFINE_STATIC_NAME( Phantom )

const CName emptyCName = CNAME( Empty );

CPhantomComponent::CPhantomComponent()
: m_wrapper()
, m_collisionGroupName( CName::NONE )
, m_shapeType( PS_EntityBounds )
, m_shapeDimensions( 0, 0 ,0 )
, m_activated( false )
, m_onTriggerEnteredScriptEvent( emptyCName )
, m_onTriggerExitedScriptEvent( emptyCName )
, m_useInQueries( false )
, m_activateParentObject( nullptr )
{
}

CPhantomComponent::~CPhantomComponent()
{
}

void CPhantomComponent::Activate()
{
	Activate( GetParentNode() );	
}

CNode * CPhantomComponent::GetParentNode()
{
	CHardAttachment* attachment = GetTransformParent();
	if( attachment && attachment->IsA< CPhantomAttachment >() )
	{
		return attachment->GetParent();
	}
	else
	{
		return GetEntity();
	}
}

void CPhantomComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	Activate();

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_PhantomShapes );
}

void CPhantomComponent::OnParentAttachmentAdded( IAttachment* attachment )
{
	// Pass to the base class
	TBaseClass::OnParentAttachmentAdded( attachment );
}

void CPhantomComponent::OnParentAttachmentBroken( IAttachment* attachment )
{
	// Pass to the base class
	TBaseClass::OnParentAttachmentBroken( attachment );
}

void CPhantomComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	Deactivate();

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_PhantomShapes );
}

void CPhantomComponent::Activate( CNode* parentObject )
{
#ifndef RED_FINAL_BUILD
	if( SPhysicsSettings::m_dontCreateTriggers ) return;
#endif

	if ( m_activated )
	{
		return;
	}

	CPhysicsWorld* physicsWorld = nullptr;
	CWorld* world = GetWorld();
	if( !world || !world->GetPhysicsWorld( physicsWorld ) || !parentObject )
	{
		return;
	}
	
	m_activateParentObject = parentObject;

	if ( !m_useInQueries )
	{
		IPhysicalCollisionTriggerCallback* callback = parentObject->QueryPhysicalCollisionTriggerCallback();
		if ( !callback && m_onTriggerEnteredScriptEvent == emptyCName && m_onTriggerExitedScriptEvent == emptyCName )
		{
			return;
		}
	}

	// verify shape size
	m_shapeDimensions.X = Clamp< Float > ( m_shapeDimensions.X, 0.05f, FLT_MAX );
	m_shapeDimensions.Y = Clamp< Float > ( m_shapeDimensions.Y, 0.05f, FLT_MAX );
	m_shapeDimensions.Z = Clamp< Float > ( m_shapeDimensions.Z, 0.05f, FLT_MAX );

	if( m_localToWorld.GetTranslationRef() == Vector::ZEROS )
	{
		m_localToWorld = Matrix::IDENTITY;
	}

	if( m_meshCollision )
	{
		String meshPath = m_meshCollision->GetDepotPath();

		CDiskFile* file = GDepot->FindFile( meshPath );
		if( file )
		{
			Red::System::DateTime time = file->GetFileTime();
		
			GCollisionCache->FindCompiled_Async( this, meshPath, time );
		}
	}
	else
	{
		SetupPhysicsWrapper();
	}

	m_activated = true;
}

void CPhantomComponent::SetupPhysicsWrapper()
{
	if( CWorld* world = GetWorld() )
	{
		CPhysicsWorld* physicsWorld = nullptr;
		if( world->GetPhysicsWorld( physicsWorld ) )
		{
			CPhysicsEngine::CollisionMask collisionMask = ( m_collisionGroupName != CName::NONE ) ?  GPhysicEngine->GetCollisionTypeBit( m_collisionGroupName ) : 0;
			CPhysicsEngine::CollisionMask collisionGroup = GPhysicEngine->GetCollisionTypeBit( m_triggeringCollisionGroupNames );
			if( collisionGroup == 0  )
			{
				collisionGroup = GPhysicEngine->GetWithAllCollisionMask(); 
			}

            ////////////////////////////////////////////////////////////////////////////
            //// HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
            //// Exclude boat collision groups if this phantom component belongs to pc_aard entity
            //// We should never add Aard impulse to boat
            //CForceFieldEntity* forceField = nullptr;
            //CObject* parentObj = GetParent();
            //if( parentObj != nullptr && parentObj->IsA<CForceFieldEntity>() )
            //{
            //    forceField = (CForceFieldEntity*)parentObj;
            //}

            //CEntity* parentEntity = nullptr;
            //if( forceField != nullptr && forceField->m_parentEntityTemplate != nullptr )
            //{
            //    parentEntity = forceField->m_parentEntityTemplate;
            //}

            //const String aard( TXT("pc_aard") );
            //size_t index = 0;
            //if( parentEntity != nullptr && String( parentEntity->GetName().AsChar() ).FindSubstring( aard, index, false, 0 ) )
            //{
            //    TDynArray< CName > excludeNames;
            //    excludeNames.PushBack(CName( TXT("Boat")            ) );
            //    excludeNames.PushBack(CName( TXT("BoatSide")        ) );
            //    excludeNames.PushBack(CName( TXT("BoatDestruction") ) );
            //    excludeNames.PushBack(CName( TXT("BoatDocking")     ) );

            //    const CPhysicsEngine::CollisionMask removeGroup = GPhysicEngine->GetCollisionTypeBit( excludeNames );
            //    collisionGroup &= ~removeGroup;
            //}
            //// HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
            ////////////////////////////////////////////////////////////////////////////

			if( m_compiledCollision )
			{
				m_wrapper = physicsWorld->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), collisionMask, collisionGroup, m_compiledCollision, nullptr, nullptr );
			}
			else switch( m_shapeType )
			{
			case PS_Sphere:
				{
					m_wrapper = physicsWorld->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), collisionMask, collisionGroup, m_compiledCollision, nullptr, &m_shapeDimensions.X );
					break;
				}
			case PS_Box:
			case PS_EntityBounds:
			default:
				{
					m_wrapper = physicsWorld->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), collisionMask, collisionGroup, m_compiledCollision, &m_shapeDimensions, nullptr );
					break;
				}
			}
		}
	}

	if( !m_wrapper ) return;
	m_wrapper->SwitchToKinematic( true );
	if( m_useInQueries ) return; // don't setup callbacks

	if( m_onTriggerEnteredScriptEvent != CNAME( Empty ) )
	{
		if( m_eventsCalledOnComponent )
		{
			m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusFound, this, m_onTriggerEnteredScriptEvent );
		}
		else
		{
			m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusFound, GetEntity(), m_onTriggerEnteredScriptEvent );
		}
	}

	if( m_onTriggerExitedScriptEvent != CNAME( Empty ) )
	{
		if( m_eventsCalledOnComponent )
		{
			m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusLost, this, m_onTriggerExitedScriptEvent );
		}
		else
		{
			m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusLost, GetEntity(), m_onTriggerExitedScriptEvent );
		}
	}

	if( m_activateParentObject )
	{
		m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnTriggerFocusFound, m_activateParentObject->QueryPhysicalCollisionTriggerCallback(), m_activateParentObject );
		m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnTriggerFocusLost, m_activateParentObject->QueryPhysicalCollisionTriggerCallback(), m_activateParentObject );
	}
}

void CPhantomComponent::OnCompiledCollisionFound( CompiledCollisionPtr collision )
{
	m_compiledCollision = collision;
	SetupPhysicsWrapper();
}

void CPhantomComponent::OnCompiledCollisionInvalid()
{
#ifndef NO_RESOURCE_IMPORT

	String meshPath = m_meshCollision->GetDepotPath();

	CDiskFile* file = GDepot->FindFile( meshPath );

	if( file )
	{
		const CCollisionMesh* collisionMesh = m_meshCollision->GetCollisionMesh();

		// This should be call only in editor.
		GCollisionCache->Compile_Sync( m_compiledCollision, collisionMesh, meshPath, file->GetFileTime() );
		OnCompiledCollisionFound( m_compiledCollision );
	}

#endif
}

void CPhantomComponent::Deactivate()
{
	if ( !m_activated )
	{
		return;
	}

	if ( m_wrapper )
	{
		m_wrapper->Release();
		m_wrapper = NULL;
	}

	if( !m_compiledCollision )
	{
		GCollisionCache->CancelFindCompiled_Async( this );
		m_activateParentObject = nullptr;
	}

	m_activated = false;
}

void CPhantomComponent::OnAppearanceChanged( Bool added )
{
	// Reactivate with proper data
	if ( added )
	{
		if ( m_activated )
		{
			Deactivate();
		}
		Activate();
	}
}

void CPhantomComponent::OnStreamIn()
{
	// Reactivate with proper data
	if ( IsUsedInAppearance() )
	{
		if ( m_activated )
		{
			Deactivate();
		}
		Activate();
	}
}

void CPhantomComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag == SHOW_PhantomShapes )
	{
		switch( m_shapeType )
		{
		case PS_Sphere:	
			{
				frame->AddDebugSphere( Vector::ZERO_3D_POINT, m_shapeDimensions.X, GetLocalToWorld(), Color::RED );
				break;
			}
		case PS_Box:
		case PS_EntityBounds:
		default:
			{
				Box box( -m_shapeDimensions / 2, m_shapeDimensions / 2 );
				frame->AddDebugBox( box, GetLocalToWorld(), Color::RED );
				break;
			}
		}
	}
}

void CPhantomComponent::DefineManualy( EPhantomShape type, const Vector& dimensions )
{
	m_shapeType = type;
	m_shapeDimensions = dimensions;
}

void CPhantomComponent::Fill( const THandle< CPhantomComponent >& other )
{
	const EngineTransform& transform = other.Get()->GetTransform();

	SetPosition( transform.GetPosition() );
	SetRotation( transform.GetRotation() );
	SetScale( transform.GetScale() );
	ForceUpdateTransformNodeAndCommitChanges();

	m_meshCollision = other.Get()->m_meshCollision;
	m_triggeringCollisionGroupNames = other.Get()->m_triggeringCollisionGroupNames;
}

void CPhantomComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CPhantomComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	if ( m_wrapper && m_activated && prevLocalToWorld != GetLocalToWorld() )
	{
		m_wrapper->SetFlag( PRBW_PoseIsDirty, true );
	}
}

void CPhantomComponent::onTriggerEntered( const STriggeringInfo& info )
{
	m_triggeringInfo.PushBackUnique( info );
}

void CPhantomComponent::onTriggerExited( const STriggeringInfo& info )
{
	if( !info.TriggeredBodyWasRemoved() )
	{
		m_triggeringInfo.RemoveFast( info );
	}
	else for( Uint32 i = 0; i != m_triggeringInfo.Size(); ++i )
	{
		if( m_triggeringInfo[ i ].m_triggeredBodyId == info.m_triggeredBodyId )
		{
			m_triggeringInfo.RemoveAtFast( i );
			break;
		}
	}
}

void CPhantomComponent::funcActivate( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Activate( GetEntity() );
}

void CPhantomComponent::funcDeactivate( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Deactivate();
}

void CPhantomComponent::funcGetTriggeringCollisionGroupNames( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, names, TDynArray< CName >() );
	FINISH_PARAMETERS;
	names = m_triggeringCollisionGroupNames;
}

void CPhantomComponent::funcGetNumObjectsInside( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( m_triggeringInfo.Size() );
}

void CPhantomComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("onTriggerFocusFoundScriptEvent") )
	{
		if( !GetEntity()->FindEvent( m_onTriggerEnteredScriptEvent ) )
		{
			ASSERT( false, TXT( "Parent object havent got %s event in scripts "), m_onTriggerEnteredScriptEvent.AsString().AsChar() );
			m_onTriggerEnteredScriptEvent = CNAME( Empty );
		}
	}

	if ( property->GetName() == TXT("onTriggerFocusLostScriptEvent") )
	{
		if( !GetEntity()->FindEvent( m_onTriggerExitedScriptEvent ) )
		{
			ASSERT( false, TXT( "Parent object havent got %s event in scripts "), m_onTriggerExitedScriptEvent.AsString().AsChar() );
			m_onTriggerExitedScriptEvent = CNAME( Empty );
		}
	}

	if ( property->GetName() == TXT("shapeType") )
	{
		if( m_shapeType == PS_EntityBounds )
		{
			Box box = GetEntity()->CalcBoundingBox();
			m_shapeDimensions = box.CalcExtents();
		}
	}

}

void CPhantomComponent::SetEnabled( Bool enabled )
{
	TBaseClass::SetEnabled( enabled );
	if( !m_wrapper ) return;
	if( m_useInQueries ) return; // don't setup callbacks
	if( enabled )
	{
		CNode* parentObject = nullptr;
		CHardAttachment* attachment = GetTransformParent();
		if( attachment && attachment->IsA< CPhantomAttachment >() )
		{
			parentObject = attachment->GetParent();
		}
		else
		{
			parentObject = GetEntity();
		}

		if ( parentObject == nullptr && m_onTriggerEnteredScriptEvent == CNAME( Empty ) && m_onTriggerExitedScriptEvent == CNAME( Empty ))
		{
			return;
		}
		else
		{
			if( m_onTriggerEnteredScriptEvent != CNAME( Empty ) )
			{
				if( m_eventsCalledOnComponent )
				{
					m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusFound, this, m_onTriggerEnteredScriptEvent );
				}
				else
				{
					m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusFound, GetEntity(), m_onTriggerEnteredScriptEvent );
				}
			}

			if( m_onTriggerExitedScriptEvent != CNAME( Empty ) )
			{
				if( m_eventsCalledOnComponent )
				{
					m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusLost, this, m_onTriggerExitedScriptEvent );
				}
				else
				{
					m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusLost, GetEntity(), m_onTriggerExitedScriptEvent );
				}
			}

			if( parentObject )
			{
				m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnTriggerFocusFound, parentObject->QueryPhysicalCollisionTriggerCallback(), parentObject );
				m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnTriggerFocusLost, parentObject->QueryPhysicalCollisionTriggerCallback(), parentObject );
			}
		}
	}
	else
	{
		m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusFound, 0, CName() );
		m_wrapper->SetScriptCallback( CPhysicsWrapperInterface::EPSCT_OnTriggerFocusLost, 0, CName() );

		m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnTriggerFocusFound, nullptr, nullptr );
		m_wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnTriggerFocusLost, nullptr, nullptr );
	}
}
