
#include "comboString.h"

#pragma once

class CComboAspect : public CObject
{
	DECLARE_ENGINE_CLASS( CComboAspect, CObject, 0 );

	CName								m_name;
	TDynArray< CComboString* >			m_strings;
	THashMap< CName, TDynArray< CName >	>	m_links;
	THashMap< CName, TDynArray< CName > >	m_hits;

public:
	CComboString* CreateComboString( Bool leftSide );
	CComboString* GetRandomComboString( Bool leftSide ) const;
	void AddLinks( const CName& animationName, const TDynArray< CName >& connections );
	void AddLink( const CName& animationName, const CName& linkedAnimationName );
	void AddHit( const CName& animationName, const CName& hitAnimationName );

	RED_INLINE void SetAspectName( const CName& name ) { m_name = name; }
	RED_INLINE const CName& GetAspectName() const { return m_name; }

	const CName& GetHitAnimation( const CName& animationName ) const;
	const CName& GetLinkedAnimation( const CName& animationName ) const;

private:
	void funcCreateComboString( CScriptStackFrame& stack, void* result );

	void funcAddLinks( CScriptStackFrame& stack, void* result );
	void funcAddLink( CScriptStackFrame& stack, void* result );
	void funcAddHit( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CComboAspect );
	PARENT_CLASS( CObject );
	PROPERTY( m_name );
	PROPERTY( m_strings );
	NATIVE_FUNCTION( "CreateComboString", funcCreateComboString );
	NATIVE_FUNCTION( "AddLinks", funcAddLinks );
	NATIVE_FUNCTION( "AddLink", funcAddLink );
	NATIVE_FUNCTION( "AddHit", funcAddHit );
END_CLASS_RTTI();
