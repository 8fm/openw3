/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "lookAtController.h"
#include "lookAtParam.h"

#include "../../common/engine/springDampers.h"
#include "../engine/renderFrame.h"

IMPLEMENT_RTTI_ENUM( EReactionLookAtType );
IMPLEMENT_RTTI_ENUM( ELookAtMode );

//////////////////////////////////////////////////////////////////////////

CLookAtTargetDynamic::CLookAtTargetDynamic( const CNode* target ) 
	: m_target( target )
{

}

Vector CLookAtTargetDynamic::GetTarget() const
{
	if ( const CNode* node = m_target.Get() )
	{
		if ( const CEntity* ent = Cast< const CEntity >( node ) )
		{
			if ( const CAnimatedComponent* root = ent->GetRootAnimatedComponent() )
			{
				return root->GetThisFrameTempLocalToWorld().GetTranslation();
			}
		}

		return node->GetWorldPosition();
	}

	return Vector::ZERO_3D_POINT;
}

Bool CLookAtTargetDynamic::IsValid() const
{
	return m_target.Get() != NULL;
}

void CLookAtTargetDynamic::GetTargetName( String& name ) const
{
	if ( IsValid() )
	{
		name = String::Printf( TXT(" Dynamic [%s]"), m_target.Get()->GetName().AsChar() );
	}
	else
	{
		name = TXT("<Invalid>");
	}
}

ELookAtTargetType CLookAtTargetDynamic::GetType() const
{
	return LTT_Dynamic;
}

CLookAtTargetStatic::CLookAtTargetStatic( const Vector& target  )
	: m_target( target )
{

}

Vector CLookAtTargetStatic::GetTarget() const
{
	return m_target;
}

Bool CLookAtTargetStatic::IsValid() const
{
	return true;
}

void CLookAtTargetStatic::GetTargetName( String& name ) const
{
	if ( IsValid() )
	{
		name = String::Printf( TXT(" Static [%.1f %.1f %.1f]"), m_target.X, m_target.Y, m_target.Z );
	}
	else
	{
		name = TXT("<Invalid>");
	}
}

ELookAtTargetType CLookAtTargetStatic::GetType() const
{
	return LTT_Static;
}

CLookAtTargetBone::CLookAtTargetBone( const CAnimatedComponent* boneOwner, Int32 boneIndex )
	: m_targetOwner( boneOwner )
	, m_boneIndex( boneIndex )
{
	ASSERT( m_boneIndex != -1 );
}

Vector CLookAtTargetBone::GetTarget() const
{
	const CAnimatedComponent* ac = m_targetOwner.Get();
	
	Vector boneWS;
	if ( ac && ac->GetThisFrameTempBoneTranslationWorldSpace( m_boneIndex, boneWS ) )
	{
		//additional offset makes sure that actors are looking at the eyes instead of base of the head
		return boneWS + Vector(0.f, 0.f, 0.09f );
	}

	return Vector::ZERO_3D_POINT;
}

Bool CLookAtTargetBone::IsValid() const
{
	return m_targetOwner.Get() != NULL;
}

void CLookAtTargetBone::GetTargetName( String& name ) const
{
	if ( IsValid() )
	{
		name = String::Printf( TXT(" Bone [%s %d]"), m_targetOwner.Get()->GetName().AsChar(), m_boneIndex );
	}
	else
	{
		name = TXT("<Invalid>");
	}
}

ELookAtTargetType CLookAtTargetBone::GetType() const
{
	return LTT_Bone;
}

//////////////////////////////////////////////////////////////////////////

SLookAtStaticInfo::SLookAtStaticInfo( ELookAtSupplierPriority prio )
	: SLookAtInfo( prio, LTT_Static )
{

}

Bool SLookAtStaticInfo::IsValid() const
{
	return true;
}

SLookAtDynamicInfo::SLookAtDynamicInfo( ELookAtSupplierPriority prio )
	: SLookAtInfo( prio, LTT_Dynamic )
{
}

Bool SLookAtDynamicInfo::IsValid() const
{
	return m_target != NULL;
}

SLookAtBoneInfo::SLookAtBoneInfo( ELookAtSupplierPriority prio )
	: SLookAtInfo( prio, LTT_Bone ), m_targetOwner( nullptr ), m_boneIndex( -1 )
{

}

Bool SLookAtBoneInfo::IsValid() const
{
	return m_targetOwner != NULL && m_boneIndex != -1;
}

//////////////////////////////////////////////////////////////////////////

CLookAtSupplier::CLookAtSupplier( const SLookAtInfo& lookAtInfo, Int32 id ) 
	: m_priority( lookAtInfo.GetPriority() ) 
	, m_timer( 0.f )
	, m_target( NULL )
	, m_id ( id )
	, m_timeFromStart( 0.f )
{
	m_level = lookAtInfo.GetLevel();
	m_duration = lookAtInfo.GetDuration();
	m_speed = lookAtInfo.GetSpeed();
	m_speedOverride = lookAtInfo.GetSpeedOverride();
	m_instantStart = lookAtInfo.IsInstant();
	m_autoLimitDeact = lookAtInfo.IsAutoLimitDeact();
	m_range = lookAtInfo.GetRange();
	m_desc = lookAtInfo.GetDesc();
	m_delay = lookAtInfo.GetDelay();
	m_headRotationRatio = lookAtInfo.GetHeadRotationRatio();
	m_eyesLookAtConvergenceWeight = lookAtInfo.GetEyesLookAtConvergenceWeight();
	m_eyesLookAtIsAdditive = lookAtInfo.IsEyesLookAtAdditive();
	m_eyesLookAtDampScale = lookAtInfo.GetEyesLookAtDampScale();
	m_timeFromStart = lookAtInfo.GetTimeFromStart();

	CreateTarget( lookAtInfo );

	ASSERT( m_target );
}

void CLookAtSupplier::CreateTarget( const SLookAtInfo& lookAtInfo )
{
	switch ( lookAtInfo.GetTargetType() )
	{
	case LTT_Static:
		{
			const SLookAtStaticInfo& info = static_cast< const SLookAtStaticInfo& >( lookAtInfo );
			m_target = new CLookAtTargetStatic( info.m_target );
			return;
		}
	case LTT_Dynamic:
		{
			const SLookAtDynamicInfo& info = static_cast< const SLookAtDynamicInfo& >( lookAtInfo );
			m_target = new CLookAtTargetDynamic( info.m_target );
			return;
		}
	case LTT_Bone:
		{
			const SLookAtBoneInfo& info = static_cast< const SLookAtBoneInfo& >( lookAtInfo );
			m_target = new CLookAtTargetBone( info.m_targetOwner, info.m_boneIndex );
			return;
		}
	default:
		ASSERT( 0 );
	}
}

CLookAtSupplier::~CLookAtSupplier()
{
	if ( m_target )
	{
		delete m_target;
		m_target = NULL;
	}
}

ELookAtSupplierPriority CLookAtSupplier::GetPriority() const 
{ 
	return m_priority; 
}

namespace
{
	const Char* GetEnumName( ELookAtSupplierPriority prio )
	{
		switch ( prio )
		{
		case LPP_Debug:
			return TXT("Debug");
		case LPP_Dialog:
			return TXT("Dialog");
		case LPP_Script:
			return TXT("Script");
		case LPP_Reaction:
			return TXT("Reaction");
		default:
			return TXT("<unnamed>");
		}
	}
}

void CLookAtSupplier::GetFriendlyName( String& name ) const
{
	if ( m_target )
	{
		m_target->GetTargetName( name );
		name += String::Printf( TXT(" - %s"), GetEnumName( GetPriority() ) );
	}
}

ELookAtLevel CLookAtSupplier::GetLevel() const
{
	return m_level;
}

Float CLookAtSupplier::GetSpeed() const
{
	return m_speed;
}

Float CLookAtSupplier::GetSpeedOverride() const
{
	return m_speedOverride;
}

Bool CLookAtSupplier::IsAutoLimitDeact() const
{
	return m_autoLimitDeact;
}

Float CLookAtSupplier::GetRange() const
{
	return m_range;
}

Bool CLookAtSupplier::IsInstant() const
{
	return m_instantStart;
}

Int32	CLookAtSupplier::GetID() const
{
	return m_id;
}

const String& CLookAtSupplier::GetDesc() const
{
	return m_desc;
}

Float CLookAtSupplier::GetDelay() const
{
	return m_delay;
}

Float CLookAtSupplier::GetHeadRotationRatio() const
{
	return m_headRotationRatio;
}

Float CLookAtSupplier::GetEyesLookAtConvergenceWeight() const
{
	return m_eyesLookAtConvergenceWeight;
}

Bool CLookAtSupplier::IsEyesLookAtAdditive() const
{
	return m_eyesLookAtIsAdditive;
}

Float CLookAtSupplier::GetEyesLookAtDampScale() const 
{ 
	return m_eyesLookAtDampScale; 
}

Float CLookAtSupplier::GetTimeFromStart() const
{
	return m_timeFromStart;
}

Vector CLookAtSupplier::GetTarget() const
{
	if ( m_target )
	{
		return m_target->GetTarget();
	}
	else
	{
		ASSERT( m_target );

		return Vector::ZERO_3D_POINT;
	}
}

Bool CLookAtSupplier::Wait( Float dt )
{
	m_delay -= dt;
	return m_delay > 0;
}

Bool CLookAtSupplier::Update( Float dt )
{
	ASSERT( m_target );

	if ( !m_target || ( m_target && !m_target->IsValid() ) )
	{
		return false;
	}

	if ( m_duration > 0.f )
	{
		m_timer += dt;

		if ( m_timer > m_duration )
		{
			return false;
		}
	}

	return true;
}

Bool CLookAtSupplier::IsEqual( const SLookAtInfo& lookAtInfo  ) const
{
	if ( !m_target )
	{
		ASSERT( m_target );
		return false;
	}

	return	m_priority == lookAtInfo.GetPriority() &&
			m_target->GetType() == lookAtInfo.GetTargetType();
}

//////////////////////////////////////////////////////////////////////////

const Float CLookAtController::m_headRotationRatio_damp = 10.f;

CLookAtController::CLookAtController()
	: m_bestSupplier( -1 )
	, m_level( LL_Body )
	, m_nextId( 0 )
	, m_deactSpeed( 0.f )
	, m_modeFlags( 0 )
	, m_headRotationRatio_current(1.f)
	, m_headRotationRatio_velo(0.f)
{
}

CLookAtController::~CLookAtController()
{
	DestroySuppliers();
}

void CLookAtController::SetMode( ELookAtMode mode )
{
	m_modeFlags |= mode;

	CheckLookAtsAvailability();
}

void CLookAtController::ResetMode( ELookAtMode mode )
{
	m_modeFlags &= ~mode;
}

Bool CLookAtController::HasFlag( ELookAtMode mode ) const 
{ 
	return ( m_modeFlags & mode ) != 0; 
}

void CLookAtController::FindAndCacheStaticLookAtParam( const CEntityTemplate* templ )
{
	if ( templ )
	{
		m_staticParam = templ->FindParameter< CLookAtStaticParam >();
	}
}

Bool CLookAtController::Update( Float dt )
{
	PC_SCOPE_PIX( LookAtController );

	for ( Int32 i=m_suppliers.SizeInt()-1; i>=0; --i )
	{
		CLookAtSupplier* s = m_suppliers[ i ];
		if ( !s->Update( dt ) )
		{
			m_suppliers.Erase( m_suppliers.Begin() + i );
			DestroySupplier( s );
		}
	}

	for ( Int32 i=m_waitingSuppliers.SizeInt()-1; i>=0; --i )
	{
		CLookAtSupplier* ws = m_waitingSuppliers[ i ];
		if ( !ws->Wait( dt ) )
		{
			m_waitingSuppliers.Erase( m_waitingSuppliers.Begin() + i );
			AddSupplier( ws );
		}
	}

	TFloatCriticalDamp damper( m_headRotationRatio_damp );
	Float dest = GetBestSupplier() ? GetBestSupplier()->GetHeadRotationRatio() : 1.f ;
	damper.Update( m_headRotationRatio_current , m_headRotationRatio_velo , dest , dt );

	return !m_suppliers.Empty() || !m_waitingSuppliers.Empty();
}

void CLookAtController::SetLevel( ELookAtLevel level )
{
	m_level = level;
}

void CLookAtController::SetLookatFilterData( ELookAtLevel level, CName key )
{
	for( Uint32 i = 0; i < m_filterData.Size(); ++i )
	{
		if( m_filterData[i].key == key )
		{
			m_filterData[i] =  SDialogLookatFilterData( key, level );
			return;
		}
	}
	new (m_filterData) SDialogLookatFilterData( key, level );
};

void CLookAtController::RemoveLookatFilterData( CName key )
{
	for( Uint32 i = 0; i < m_filterData.Size(); ++i )
	{
		if( m_filterData[i].key == key )
		{
			m_filterData.Erase( m_filterData.Begin() + i );
			return;
		}
	}
};


void CLookAtController::ActivateLookatFilter( CName key, Bool value )
{
	for( Uint32 i = 0; i < m_filterData.Size(); ++i )
	{
		if( m_filterData[i].key == key )
		{
			m_filterData[i].isActive = value;
		}
	}
}

ELookAtLevel CLookAtController::GetLevel() const
{
	CLookAtSupplier* s = GetBestSupplier();
	ELookAtLevel level = m_level;
	if ( s )
	{
		level = Max( s->GetLevel(), level );
	}

	for( Uint32 i = 0; i < m_filterData.Size(); ++i )
	{
		if( m_filterData[i].isActive )
		{
			level = Max( m_filterData[i].level, level );
		}
	}

	return level;
}

void CLookAtController::GenerateDebugFragments( CRenderFrame* frame, const Matrix& localToWorld ) const
{
	CLookAtSupplier* s = GetBestSupplier();
	if ( s )
	{
		Vector pos= Vector( 0.f, 0.f, 2.2f ) + localToWorld.GetTranslation();

		String text;
		s->GetFriendlyName( text );
		
		for ( Uint32 i=0; i<m_suppliers.Size(); ++i )
		{
			String str;
			m_suppliers[i]->GetFriendlyName( str );
			text += String::Printf( TXT("\n%d.%s"), i, str.AsChar() );
		}

		frame->AddDebugText( pos, text, true );
	}
}

void CLookAtController::GetDesc( CActorLookAtDesc& desc ) const
{
	desc.Setup( m_suppliers, m_bestSupplier, m_modeFlags );
}

Bool CLookAtController::AddLookAt( const SLookAtInfo& lookAtInfo )
{
	if ( !lookAtInfo.IsValid() )
	{
		AI_LOG( TXT("[Look at controller] ERROR - look at info is invalid") );
		return false;
	}

	if ( CanAddSupplier( lookAtInfo ) )
	{
		CLookAtSupplier* supplier = CreateSupplier( lookAtInfo );
		if ( supplier )
		{
			AddSupplier( supplier );
			return true;
		}
		else
		{
			ASSERT( supplier );
		}
	}

	return false;
}

Bool CLookAtController::IsLookAtTypeAvailable( ELookAtSupplierPriority prio ) const
{
	if ( HasFlag( LM_Cutscene ) || HasFlag( LM_GameplayLock ) || HasFlag( LM_MiniGame ) )
	{
		return false;
	}
	else if ( HasFlag( LM_Dialog ) )
	{
		return prio == LPP_Dialog;
	}
	else
	{
		return true;
	}
}

void CLookAtController::CheckLookAtsAvailability()
{
	for ( Int32 i=(Int32)m_suppliers.Size()-1; i>=0; --i )
	{
		CLookAtSupplier* s = m_suppliers[ i ];
		if ( !IsLookAtTypeAvailable( s->GetPriority() ) )
		{
			m_suppliers.Erase( m_suppliers.Begin() + i );
			DestroySupplier( s );
		}
	}
}

Bool CLookAtController::CanAddSupplier( const SLookAtInfo& lookAtInfo ) const
{
	if ( !IsLookAtTypeAvailable( lookAtInfo.GetPriority() ) )
	{
		return false;
	}

	if ( lookAtInfo.GetPriority() == LPP_Dialog || lookAtInfo.GetPriority() == LPP_Reaction )
	{
		return true;
	}

	for ( Uint32 i=0; i<m_suppliers.Size(); ++i )
	{
		CLookAtSupplier* s = m_suppliers[ i ];

		if ( s->IsEqual( lookAtInfo ) )
		{
			return false;
		}
	}

	return true;
}

void CLookAtController::AddSupplier( CLookAtSupplier* supplier )
{
	if ( supplier->GetDelay() > 0.05f  )
	{
		m_waitingSuppliers.PushBack( supplier );
	}
	else
	{
		ELookAtSupplierPriority newPrio = supplier->GetPriority();

		if ( !IsMultiSupplierAllowed( newPrio ) && HasAnySupplierWithPrio( newPrio ) )
		{
			ASSERT( GetNumberOfSuppliersWithPrio( newPrio ) == 1 );

			DestroySuppliersWithPrio( newPrio );
		}

		DestroyEqualsSuppliers( supplier );

		m_suppliers.PushBack( supplier );

		FindBestSupplier();
	}
}

void CLookAtController::FindBestSupplier()
{
	if ( m_suppliers.Empty() )
	{
		m_bestSupplier = -1;
	}
	else
	{
		m_bestSupplier = 0;

		for ( Uint32 i=1; i<m_suppliers.Size(); ++i )
		{
			// a < b
			if ( m_suppliers[ i ]->GetPriority() < m_suppliers[ m_bestSupplier ]->GetPriority() )
			{
				m_bestSupplier = i;
			}
		}

		m_deactSpeed = 0.f;
	}
}

Bool CLookAtController::IsMultiSupplierAllowed( ELookAtSupplierPriority prio ) const
{
	return prio == LPP_Reaction;
}

CLookAtSupplier* CLookAtController::GetBestSupplier() const
{
	ASSERT( m_bestSupplier <= (Int32)m_suppliers.Size() );
	return m_bestSupplier >= 0 && m_bestSupplier < (Int32)m_suppliers.Size() ? m_suppliers[ m_bestSupplier ] : NULL;
}

void CLookAtController::GetLookAtParams( const CLookAtDynamicParam*& dynamicParam, const CLookAtStaticParam*& staticParam, CLookAtContextParam& contextParam ) const
{
	staticParam = m_staticParam;

	dynamicParam = GetBestSupplier();

	ELookAtLevel level = m_level;
	for( Uint32 i = 0; i < m_filterData.Size(); ++i )
	{
		if( m_filterData[i].isActive )
		{
			level = Max( m_filterData[i].level, level );
		}
	}

	contextParam.m_headRotationRatio = m_headRotationRatio_current;
	contextParam.m_actorLevel = level;
	contextParam.m_speed = m_deactSpeed;
	contextParam.m_reset = HasFlag( LM_Cutscene );
}

void CLookAtController::SetNoLookAts()
{
	DestroySuppliers();
}

void CLookAtController::SetNoDialogsLookAts( Float speed )
{
	m_deactSpeed = speed;
	DestroySuppliersWithPrio( LPP_Dialog );
}

void CLookAtController::SetNoScriptLookAts()
{
	DestroySuppliersWithPrio( LPP_Script );
}

void CLookAtController::RemoveAllNonDialogsLookAts()
{
	DestroySuppliersWithoutPrio( LPP_Dialog );
}

String CLookAtController::GetSupplierName() const
{
	CLookAtSupplier* s = GetBestSupplier();

	if ( s )
	{
		String str;
		s->GetFriendlyName( str );
		return str;
	}

	return TXT("<None>");
}

void CLookAtController::DestroyEqualsSuppliers( CLookAtSupplier* newS )
{
	for ( Int32 i=(Int32)m_suppliers.Size()-1; i>=0; --i )
	{
		CLookAtSupplier* currS = m_suppliers[ i ];
		if ( currS->GetPriority() == newS->GetPriority() && 
			currS->GetTarget() == newS->GetTarget() && 
			currS->GetLevel() == newS->GetLevel() )
		{
			m_suppliers.Erase( m_suppliers.Begin() + i );
			DestroySupplier( currS );
		}
	}
}

void CLookAtController::DestroySuppliers()
{
	m_suppliers.ClearPtr();
	m_waitingSuppliers.ClearPtr();

	FindBestSupplier();
}

void CLookAtController::DestroySupplier( CLookAtSupplier* s )
{
	delete s;
	FindBestSupplier();
}

void CLookAtController::DestroySuppliersWithPrio( ELookAtSupplierPriority prio )
{
	for ( Int32 i=(Int32)m_suppliers.Size()-1; i>=0; --i )
	{
		CLookAtSupplier* s = m_suppliers[ i ];
		if ( s->GetPriority() == prio )
		{
			m_suppliers.Erase( m_suppliers.Begin() + i );
			DestroySupplier( s );
		}
	}
}

void CLookAtController::DestroySuppliersWithoutPrio( ELookAtSupplierPriority prio )
{
	for ( Int32 i=(Int32)m_suppliers.Size()-1; i>=0; --i )
	{
		CLookAtSupplier* s = m_suppliers[ i ];
		if ( s->GetPriority() != prio )
		{
			m_suppliers.Erase( m_suppliers.Begin() + i );
			DestroySupplier( s );
		}
	}
}

Bool CLookAtController::HasAnySupplierWithPrio( ELookAtSupplierPriority prio ) const
{
	for ( Uint32 i=0; i<m_suppliers.Size(); ++i )
	{
		if ( m_suppliers[ i ]->GetPriority() == prio )
		{
			return true;
		}
	}

	return false;
}

Uint32 CLookAtController::GetNumberOfSuppliersWithPrio( ELookAtSupplierPriority prio ) const
{
	Uint32 num = 0;

	for ( Uint32 i=0; i<m_suppliers.Size(); ++i )
	{
		if ( m_suppliers[ i ]->GetPriority() == prio )
		{
			++num;
		}
	}

	return num;
}

Bool CLookAtController::HasLookAt() const
{
	return GetBestSupplier() != NULL;
}

Vector CLookAtController::GetTarget() const
{
	CLookAtSupplier* s = GetBestSupplier();
	if ( s )
	{
		//ASSERT( !Vector::Near3( s->GetTarget(), Vector::ZERO_3D_POINT ) );
		return s->GetTarget();
	}
	else
	{
		ASSERT( s );
		return Vector::ZERO_3D_POINT;
	}
}

Vector CLookAtController::GetCompressedData() const
{
	return Vector(	GetDampSpeed(),
					GetFollowSpeed(),
					IsAutoLimitDeact() ? 1.f : 0.f,
					GetRange() );
}

Vector CLookAtController::GetEyesCompressedData() const
{
	Vector vec( 0.f, 1.f, 1.f, 0.f );

	CLookAtSupplier* s = GetBestSupplier();
	if ( s )
	{
		vec.X = s->GetEyesLookAtConvergenceWeight();
		vec.Y = s->IsEyesLookAtAdditive() ? 1.f : 0.f;
		vec.Z = s->GetEyesLookAtDampScale();
		vec.W = s->GetTimeFromStart();
	}

	return vec;
}

Float CLookAtController::GetDampSpeed() const
{
	CLookAtSupplier* s = GetBestSupplier();
	return s ? s->GetSpeed() : 0.f;
}

Float CLookAtController::GetFollowSpeed() const
{
	CLookAtSupplier* s = GetBestSupplier();
	return s ? s->GetSpeed() : 0.f;
}

Bool CLookAtController::IsAutoLimitDeact() const
{
	CLookAtSupplier* s = GetBestSupplier();
	return s ? s->IsAutoLimitDeact() : true;
}

Float CLookAtController::GetRange() const
{
	CLookAtSupplier* s = GetBestSupplier();
	return s ? s->GetRange() : 0.f;
}

Vector CLookAtController::GetBodyPartWeights() const
{
	return Vector::ZERO_3D_POINT;
}

CLookAtSupplier* CLookAtController::CreateSupplier( const SLookAtInfo& lookAtInfo )
{
	m_nextId++;

	if ( m_nextId < 0 )
	{
		m_nextId = 0;
	}

	return new CLookAtSupplier( lookAtInfo, m_nextId );
}
