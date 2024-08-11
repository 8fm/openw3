#include "build.h"
#include "boatBodyComponent.h"
#include "../physics/physicsWrapper.h"
#include "..\engine\tickManager.h"
#include "../engine/skeleton.h"
//#include "../engine/renderFrame.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBoatBodyComponent );

//////////////////////////////////////////////////////////////////////////

const Float TIME_STEP = 1.0f;

CBoatBodyComponent::CBoatBodyComponent(void)
    : m_isInCutsceneMode( false )
    , m_cutsceneBoneIndex( -1 )
    , m_cutsceneAddedTick( false )
#ifdef CORRECT_BOAT_MASSES
    , m_firstTickMassChange( true )
#endif
	, m_delayedSwitchWrapperToKinematic( false )
	, m_delayedSwitchWrapperToDynamic( false )
//     , m_isFirstTick(true)
//     , m_timeout(0.0f)
{
}

//////////////////////////////////////////////////////////////////////////

void CBoatBodyComponent::OnAttached( CWorld* world )
{
    TBaseClass::OnAttached( world );

    CallEvent( CNAME( OnComponentAttached ) );

    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Meshes );

	GetWorld()->GetTickManager()->AddToGroup( this, TICK_Main );
#ifdef CORRECT_BOAT_MASSES
    if( m_firstTickMassChange )
        GetWorld()->GetTickManager()->AddToGroup( this, TICK_PrePhysics );
#endif

    CPhysicsWrapperInterface* bodyWrapper = GetPhysicsRigidBodyWrapper();
    if( bodyWrapper == nullptr )
        return;

    // Hack to prevent boat from sinking deep into water during first few frames
    if( bodyWrapper->IsKinematic() )
	{
        bodyWrapper->SwitchToKinematic( false );
		m_delayedSwitchWrapperToKinematic = false;
		m_delayedSwitchWrapperToDynamic = false;
	}

    // Set entity pose update from boat body wrapper
    bodyWrapper->SetFlag( PRBW_UpdateEntityPose, true );

    // Disable buoyancy
    if( !bodyWrapper->IsKinematic() )
    {        
        bodyWrapper->SetFlag( PRBW_DisableBuoyancy, true );
        bodyWrapper->SetFlag( PRBW_DisableGravity, true );
    }
    else // This should never occur, switch to kinematic was unsuccessful
        ASSERT(false, TXT("BoatBoaty is kintematic (keyframed)! Change it to dynamic!"));
}

//////////////////////////////////////////////////////////////////////////

void CBoatBodyComponent::OnDetached( CWorld* world )
{
    TBaseClass::OnDetached( world );
	GetWorld()->GetTickManager()->RemoveFromGroup( this, TICK_Main );

//	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Meshes );
}

//////////////////////////////////////////////////////////////////////////

// void CBoatBodyComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
// {
//     TBaseClass::OnGenerateEditorFragments( frame, flag );
// 
//     const Uint32 count = m_toMove.Size();
//     for( Uint32 i=0; i<count; ++i )
//     {
//         frame->AddDebugSphere( m_toMove[i].GetWorldPosition(), 0.3f, Matrix::IDENTITY, Color::RED );
//     }
// }

//////////////////////////////////////////////////////////////////////////

void CBoatBodyComponent::OnTick( Float timeDelta )
{
	if( m_delayedSwitchWrapperToKinematic || m_delayedSwitchWrapperToDynamic )
	{
		RED_ASSERT( !( m_delayedSwitchWrapperToDynamic && m_delayedSwitchWrapperToKinematic ), TXT( "Trying to switch to dynamic and kinematic at the same time" ) );
		CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
		if( wrapper )
		{
			wrapper->SwitchToKinematic( m_delayedSwitchWrapperToKinematic );
			m_delayedSwitchWrapperToKinematic = false;
			m_delayedSwitchWrapperToDynamic = false;
		}
	}

	TBaseClass::OnTick( timeDelta );

	// Transform component according to cutscene bone transformation
	if( m_isInCutsceneMode )
	{
		SetupTransformInCutscene();
	}

	for( auto i : m_childAttachments )
	{
		CAnimatedComponent* component = Cast< CAnimatedComponent >( i->GetChild() );
		if( !component ) continue;

		CPhysicsWrapperInterface* wrapper = component->GetPhysicsRigidBodyWrapper();
		if( !wrapper ) continue;

		wrapper->SwitchToKinematic( true );

		GetWorld()->GetTickManager()->RemoveFromGroup( this, TICK_Main );
		return;
	}

}

//////////////////////////////////////////////////////////////////////////

void CBoatBodyComponent::OnTickPrePhysics( Float timeDelta )
{
	if( m_delayedSwitchWrapperToKinematic || m_delayedSwitchWrapperToDynamic )
	{
		RED_ASSERT( !( m_delayedSwitchWrapperToDynamic && m_delayedSwitchWrapperToKinematic ), TXT( "Trying to switch to dynamic and kinematic at the same time" ) );
		CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
		if( wrapper )
		{
			wrapper->SwitchToKinematic( m_delayedSwitchWrapperToKinematic );
			m_delayedSwitchWrapperToKinematic = false;
			m_delayedSwitchWrapperToDynamic = false;
		}
	}

    TBaseClass::OnTickPrePhysics(timeDelta);
 
    CEntity* entity = GetEntity();
    if( entity == nullptr )
        return;

#ifdef CORRECT_BOAT_MASSES
    if( m_firstTickMassChange && GetEntity()->IsInGame() )
    {
        for( ComponentIterator<CRigidMeshComponent> it(GetEntity()); it; ++it )
        {
            CRigidMeshComponent* comp = *it;
            
            CPhysicsWrapperInterface* wrapp = comp->GetPhysicsRigidBodyWrapper();
            if( wrapp == nullptr )
                return;

            const Float currentMass = wrapp->GetMass();
            LOG_ENGINE( TXT("_4fitter_ name %s mass %f"), comp->GetName().AsChar(), wrapp->GetMass() );

            if( comp != this && currentMass > 2.0f )
                wrapp->SetMass( 2.0f );
        }

        m_firstTickMassChange = false;
        
        if( !m_isInCutsceneMode )
            GetWorld()->GetTickManager()->RemoveFromGroup( this, TICK_PrePhysics );
    }
#endif
}


//////////////////////////////////////////////////////////////////////////
// Helper static method, to calculate current frame bone transform.
static void CalcBoatBoneModelSpace( Matrix& outBoneModelSpace, const CAnimatedComponent* animComp, Int32 boneIndex )
{
	outBoneModelSpace = animComp->GetBoneMatrixLocalSpace( boneIndex );
	Int32 parent_index = animComp->GetParentBoneIndex( boneIndex );

	while ( parent_index != -1 )
	{
		Matrix parentBoneTransform = animComp->GetBoneMatrixLocalSpace( parent_index );
		outBoneModelSpace = parentBoneTransform * outBoneModelSpace; // I wanted  here to pre mul parent mtx.
		parent_index = animComp->GetParentBoneIndex( parent_index );
	}
}

//////////////////////////////////////////////////////////////
void CBoatBodyComponent::SetupTransformInCutscene()
{
	const CAnimatedComponent* rootAnimatedComponent = GetEntity()->GetRootAnimatedComponent();
	RED_FATAL_ASSERT( rootAnimatedComponent, "CBoatBodyComponent's entity does not have animated component!" );

	Matrix currBoneTransform;
	CalcBoatBoneModelSpace( currBoneTransform, rootAnimatedComponent, m_cutsceneBoneIndex );

	m_cutscenePrevBoneTransform = currBoneTransform;

	// apply to this component:
	SetPosition( currBoneTransform.GetTranslation() );
	SetRotation( currBoneTransform.ToEulerAngles() );
}

//////////////////////////////////////////////////////////////////////////

void CBoatBodyComponent::funcTriggerCutsceneStart( CScriptStackFrame& stack, void* result  )
{
    FINISH_PARAMETERS;

    m_isInCutsceneMode = true;

    // Switch to kinematic
    CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
    RED_ASSERT( wrapper, TXT( "No physics wrapper for boat in cutscene start." ) );
	m_delayedSwitchWrapperToDynamic = false;
	if( wrapper )
	{
	    wrapper->SwitchToKinematic( true );
		m_delayedSwitchWrapperToKinematic = false;
	}
	else
	{
		m_delayedSwitchWrapperToKinematic = true;
	}

    // Transform entity to bone pos
    CEntity* ent = GetEntity();
    RED_FATAL_ASSERT( ent, "No entity of boat in cutscene start." );

    CAnimatedComponent* rootAnimatedComponent = ent->GetRootAnimatedComponent();
    RED_FATAL_ASSERT( rootAnimatedComponent, "No root animated component in cutscene start." );
    
    // Add to tick if was not added ( eg in cutscene preview mode )
    if( !( GetTickMask() & TICK_PrePhysics ) )
    {
        GetWorld()->GetTickManager()->AddToGroup( this, TICK_PrePhysics );
        m_cutsceneAddedTick = true;
    }

    //ASSERT( m_cutsceneBoneIndex > skeleton->GetBonesNum(), TXT("Wrong animation bone index set!! Fix it in entity editor!!") );
    ASSERT( m_cutsceneBoneIndex > 0, TXT("Animation bone index was not set!! Fix it in entity editor!!") );

    // Cutscene main bone initial model space transform
    m_cutscenePrevBoneTransform = rootAnimatedComponent->GetBoneMatrixModelSpace( m_cutsceneBoneIndex );
    m_cutsceneInitialBoneTransform = m_cutscenePrevBoneTransform;
}

//////////////////////////////////////////////////////////////////////////

void CBoatBodyComponent::funcTriggerCutsceneEnd( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;

    m_isInCutsceneMode = false;

    // Switch back to dynamic
    CPhysicsWrapperInterface* wrapper = GetPhysicsRigidBodyWrapper();
    RED_ASSERT( wrapper, TXT( "No physics rigid body wrapper in trigger cutscene end" ) );
	m_delayedSwitchWrapperToKinematic = false;
	if( wrapper )
	{
	    wrapper->SwitchToKinematic( false );
		m_delayedSwitchWrapperToDynamic = false;
	}
	else
	{
		m_delayedSwitchWrapperToDynamic = true;
	}

    CEntity* ent = GetEntity();
    RED_FATAL_ASSERT( ent, "No entity in trigger cutscene end" );

    CAnimatedComponent* rootAnimatedComponent = ent->GetRootAnimatedComponent();
    RED_FATAL_ASSERT( rootAnimatedComponent, "No root animated component in trigger cutscene end" );

    // Remove from tick
    if( m_cutsceneAddedTick )
    {
        GetWorld()->GetTickManager()->RemoveFromGroup( this, TICK_PrePhysics );
        m_cutsceneAddedTick = false;
    }

    // Find reverse transform
    Matrix reverseTransfrom = m_cutsceneInitialBoneTransform.Inverted() * m_cutscenePrevBoneTransform;
    reverseTransfrom = reverseTransfrom.Inverted();

    if( reverseTransfrom == Matrix::IDENTITY )
        return;

    for ( CComponent* comp : ent->GetComponents() )
    {
        // Move all components to root bone
        if( comp != rootAnimatedComponent && comp->GetParentAttachments().Empty() )
        {
            Matrix localTransform;
            comp->CalcLocalTransformMatrix( localTransform );
            localTransform = localTransform * reverseTransfrom;

            comp->SetPosition( localTransform.GetTranslation() );
            comp->SetRotation( localTransform.ToEulerAngles() );
        }
    }

}
//////////////////////////////////////////////////////////////////////////