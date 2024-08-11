
#pragma once

#include "../../common/engine/behaviorGraphComboNode.h"

struct SComboPlayerState;

struct SComboAnimationData
{
	DECLARE_RTTI_STRUCT( SComboAnimationData )

	CName	m_animationName;
	Float	m_blendIn;
	Float	m_blendOut;

	//...

	SComboAnimationData() : m_animationName( CName::NONE ), m_blendIn( 0.2f ), m_blendOut( 0.2f ) {}

	RED_INLINE Bool IsSame( const SComboAnimationData& data ) const { return m_animationName == data.m_animationName; }
	RED_INLINE Bool IsValid() const { return m_animationName != CName::NONE; }
};

BEGIN_CLASS_RTTI( SComboAnimationData );
	PROPERTY( m_animationName );
	PROPERTY( m_blendIn );
	PROPERTY( m_blendOut );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CComboString : public CObject
{
	DECLARE_ENGINE_CLASS( CComboString, CObject, 0 );

	TDynArray< SComboAnimationData >							m_attacks;
	TDynArray< TDynArray< SComboAnimationData > >				m_distAttacks;

	TDynArray< TDynArray< TDynArray< SComboAnimationData > > >	m_dirAttacks;

	Bool														m_leftSide;

public:
	CComboString();

	void AddAttack( const SComboAnimationData& data, EAttackDistance dist = ADIST_None );
	void AddDirAttack( const SComboAnimationData& data, EAttackDirection dir, EAttackDistance dist );

	Bool RemoveAttack( const CName& animationName );
	void RemoveAllAttacks();
	void RemoveAllDirAttacks();

	Bool HasDirAttacks() const;
	const SComboAnimationData GetDirAttack( SComboPlayerState& state, EAttackDirection dir, EAttackDistance dist ) const;

	const SComboAnimationData GetAttack( SComboPlayerState& state, EAttackDistance dist ) const;

	void SetSide( Bool left );
	RED_INLINE Bool IsLeftSide() const { return m_leftSide; }

private:
	Bool RemoveAttackFrom( const CName& animationName, TDynArray< SComboAnimationData >& attacks );
	const SComboAnimationData& GetAttack( SComboPlayerState& state, const TDynArray< SComboAnimationData >& attacks ) const;
	const SComboAnimationData FilterAttacks( SComboPlayerState& state, const TDynArray< SComboAnimationData >& attacks ) const;

	void funcAddAttack( CScriptStackFrame& stack, void* result );
	void funcAddDirAttack( CScriptStackFrame& stack, void* result );
	void funcAddDirAttacks( CScriptStackFrame& stack, void* result );

	void funcRemoveAttack( CScriptStackFrame& stack, void* result );
	void funcRemoveAllAttacks( CScriptStackFrame& stack, void* result );
	void funcRemoveAllDirAttacks( CScriptStackFrame& stack, void* result );

};

BEGIN_CLASS_RTTI( CComboString );
	PARENT_CLASS( CObject );
	PROPERTY( m_attacks );
	PROPERTY( m_distAttacks );
	PROPERTY( m_dirAttacks );
	PROPERTY( m_leftSide );
	NATIVE_FUNCTION( "AddAttack", funcAddAttack );
	NATIVE_FUNCTION( "AddDirAttack", funcAddDirAttack );
	NATIVE_FUNCTION( "AddDirAttacks", funcAddDirAttacks );
	NATIVE_FUNCTION( "RemoveAttack", funcRemoveAttack );
	NATIVE_FUNCTION( "RemoveAllAttacks", funcRemoveAllAttacks );
	NATIVE_FUNCTION( "RemoveAllDirAttacks", funcRemoveAllDirAttacks );
END_CLASS_RTTI();
