
#include "build.h"
#include "comboString.h"
#include "comboPlayer.h"

IMPLEMENT_ENGINE_CLASS( CComboString );
IMPLEMENT_ENGINE_CLASS( SComboAnimationData );

namespace
{
	static void AddAttack( TDynArray< SComboAnimationData >& list, const SComboAnimationData& data )
	{
		for ( auto it = list.Begin(), end = list.End(); it != end; ++it )
		{
			if ( data.IsSame( *it ) )
			{
				RED_HALT( "Combat String definition PROBLEM! ATTACK %s is duplicated in given string.", data.m_animationName.AsChar() );
				return;
			}
		}
		list.PushBack( data );
	}
};

CComboString::CComboString()
	: m_leftSide( true )
{
	// we have 3 distances, so we push 3 arrays
	m_distAttacks.PushBack( TDynArray< SComboAnimationData >() );
	m_distAttacks.PushBack( TDynArray< SComboAnimationData >() );
	m_distAttacks.PushBack( TDynArray< SComboAnimationData >() );
	
	// we have 3 distances, so we push 3 arrays
	m_dirAttacks.PushBack( TDynArray< TDynArray< SComboAnimationData > >() );
	m_dirAttacks.PushBack( TDynArray< TDynArray< SComboAnimationData > >() );
	m_dirAttacks.PushBack( TDynArray< TDynArray< SComboAnimationData > >() );

	for ( Uint32 i = 0; i < m_dirAttacks.Size(); ++i )
	{
		// we have 4 directions so we push 4 arrays
		m_dirAttacks[i].PushBack( TDynArray< SComboAnimationData >() );
		m_dirAttacks[i].PushBack( TDynArray< SComboAnimationData >() );
		m_dirAttacks[i].PushBack( TDynArray< SComboAnimationData >() );
		m_dirAttacks[i].PushBack( TDynArray< SComboAnimationData >() );
	}
}

void CComboString::AddAttack( const SComboAnimationData& data, EAttackDistance dist )
{
	ASSERT( data.IsValid() );

	if ( dist == ADIST_None )
	{
		::AddAttack( m_attacks, data );
	}
	else
	{
		ASSERT( dist < ADIST_Last );
		::AddAttack( m_distAttacks[ dist ], data );
	}
}

Bool CComboString::HasDirAttacks() const
{
	return m_dirAttacks[0][0].Size() > 0 || m_dirAttacks[0][1].Size() || m_dirAttacks[0][2].Size() || m_dirAttacks[0][3].Size()
		|| m_dirAttacks[1][0].Size() > 0 || m_dirAttacks[1][1].Size() || m_dirAttacks[1][2].Size() || m_dirAttacks[1][3].Size()
		|| m_dirAttacks[2][0].Size() > 0 || m_dirAttacks[2][1].Size() || m_dirAttacks[2][2].Size() || m_dirAttacks[2][3].Size();
}

void CComboString::AddDirAttack( const SComboAnimationData& data, EAttackDirection dir, EAttackDistance dist )
{
	ASSERT( data.IsValid() );
	
	COMPILE_ASSERT( AD_Last == 4 );
	COMPILE_ASSERT( ADIST_Last == 3 );

	::AddAttack( m_dirAttacks[ dist ][ dir ], data );
}

const SComboAnimationData& CComboString::GetAttack( SComboPlayerState& state, const TDynArray< SComboAnimationData >& attacks ) const
{
	ASSERT( attacks.Size() > 0 ); // we need at least 2 animations, so they will not repeat in consecutive attacks
	if ( attacks.Size() == 1 ) return attacks[ 0 ];
	do
	{
		Uint32 n = attacks.Size();
		Uint32 val = GEngine->GetRandomNumberGenerator().Get< Uint32 >( n );
		Uint32 i = val;
		do
		{
			if ( !state.m_animationsUsed.Exist( attacks[i].m_animationName ) && state.m_animationState.m_animation != attacks[i].m_animationName )
			{
				state.m_animationsUsed.Insert( attacks[i].m_animationName );
				return attacks[i];
			}
			i = ( i + 1 ) % n;
		}
		while ( i != val );

		for ( TDynArray< SComboAnimationData >::const_iterator it = attacks.Begin(); it != attacks.End(); ++it )
		{
			state.m_animationsUsed.Erase( it->m_animationName );
		}

		state.m_animationsUsed.Insert( state.m_animationState.m_animation );
	} while ( true );
}

const SComboAnimationData CComboString::FilterAttacks( SComboPlayerState& state, const TDynArray< SComboAnimationData >& attacks ) const
{
	return GetAttack( state, attacks );
}

const SComboAnimationData CComboString::GetDirAttack( SComboPlayerState& state, EAttackDirection dir, EAttackDistance dist ) const
{
	if ( dist != ADIST_None && m_dirAttacks[ dist ][ dir ].Size() ) return FilterAttacks( state, m_dirAttacks[ dist ][ dir ] );
	// fallback 1 - try to find attack in a proper direction but for other distance
	for ( Uint32 i = 0; i < m_dirAttacks.Size(); ++i )
	{
		if ( m_dirAttacks[ i ][ dir ].Size() ) return FilterAttacks( state, m_dirAttacks[ i ][ dir ] );
	}
	// fallback 1 - try to find attack in a proper distance but for other direction
	if ( dist != ADIST_None )
	{
		for ( Uint32 i = 0; i < m_dirAttacks[ dist ].Size(); ++i )
		{
			if ( m_dirAttacks[ dist ][ i ].Size() ) return FilterAttacks( state, m_dirAttacks[ dist ][ i ] );
		}
	}
	// fallback 3 - try to find any attack
	for ( Uint32 i = 0; i < m_dirAttacks.Size(); ++i )
	{
		for ( Uint32 j = 0; j < m_dirAttacks[ i ].Size(); ++j )
		{
			if ( m_dirAttacks[ i ][ j ].Size() ) return FilterAttacks( state, m_dirAttacks[ i ][ j ] );
		}
	}
	return m_dirAttacks[0][0][0];
}

const SComboAnimationData CComboString::GetAttack( SComboPlayerState& state, EAttackDistance dist ) const
{
	if ( dist != ADIST_None && m_distAttacks[ dist ].Size() ) return FilterAttacks( state, m_distAttacks[ dist ] );
	if ( m_attacks.Size() ) return FilterAttacks( state, m_attacks );
	if ( m_distAttacks[ 0 ].Size() ) return FilterAttacks( state, m_distAttacks[ 0 ] );
	if ( m_distAttacks[ 1 ].Size() ) return FilterAttacks( state, m_distAttacks[ 1 ] );
	return FilterAttacks( state, m_distAttacks[ 2 ] );
}

Bool CComboString::RemoveAttack( const CName& animationName )
{
	for ( TDynArray< TDynArray< SComboAnimationData > >::iterator iDistAttacks = m_distAttacks.Begin(); iDistAttacks != m_distAttacks.End(); ++ iDistAttacks )
	{
		if ( RemoveAttackFrom( animationName, *iDistAttacks ) )
		{
			return true;
		}
	}
	return RemoveAttackFrom( animationName, m_attacks );
}

Bool CComboString::RemoveAttackFrom( const CName& animationName, TDynArray< SComboAnimationData >& attacks )
{
	const Uint32 size = attacks.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const SComboAnimationData& data = attacks[ i ];

		if ( data.m_animationName == animationName )
		{
			attacks.RemoveAt( i );
			return true;
		}
	}
	return false;
}

void CComboString::RemoveAllAttacks()
{
	m_distAttacks[0].Clear();
	m_distAttacks[1].Clear();
	m_distAttacks[2].Clear();
	m_attacks.Clear();
}

void CComboString::RemoveAllDirAttacks()
{

}

void CComboString::SetSide( Bool left )
{
	m_leftSide = left;
}

//////////////////////////////////////////////////////////////////////////

void CComboString::funcAddAttack( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER_OPT( EAttackDistance, distance, ADIST_None );
	FINISH_PARAMETERS;

	SComboAnimationData data;
	data.m_animationName = animationName;

	AddAttack( data, distance );
}

void CComboString::funcAddDirAttack( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER( EAttackDirection, direction, AD_Front );
	GET_PARAMETER( EAttackDistance, distance, ADIST_Medium );
	FINISH_PARAMETERS;

	SComboAnimationData data;
	data.m_animationName = animationName;

	AddDirAttack( data, direction, distance );
}

void CComboString::funcAddDirAttacks( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationNameFront, CName::NONE );
	GET_PARAMETER( CName, animationNameBack, CName::NONE );
	GET_PARAMETER( CName, animationNameLeft, CName::NONE );
	GET_PARAMETER( CName, animationNameRight, CName::NONE );
	GET_PARAMETER( EAttackDistance, distance, ADIST_Medium );
	FINISH_PARAMETERS;

	SComboAnimationData dataF;
	dataF.m_animationName = animationNameFront;

	SComboAnimationData dataB;
	dataB.m_animationName = animationNameBack;

	SComboAnimationData dataL;
	dataL.m_animationName = animationNameLeft;

	SComboAnimationData dataR;
	dataR.m_animationName = animationNameRight;

	AddDirAttack( dataF, AD_Front, distance );
	AddDirAttack( dataB, AD_Back, distance );
	AddDirAttack( dataL, AD_Left, distance );
	AddDirAttack( dataR, AD_Right, distance );
}

void CComboString::funcRemoveAttack( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
}

void CComboString::funcRemoveAllAttacks( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
}

void CComboString::funcRemoveAllDirAttacks( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
}

