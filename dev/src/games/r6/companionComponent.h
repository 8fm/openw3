#pragma once
#include "../../common/engine/component.h"



class CCompanionComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CCompanionComponent, CComponent, 0 );	

public:
	static const Int32 MAX_COMPANON_SIZE = 4;

protected:
	TStaticArray< CName, MAX_COMPANON_SIZE > m_members;

public:

	CCompanionComponent();

	void SetMember( CName name, Int32 slot );
	CName GetMember( Int32 slot ) const;
	Bool HasMember( CName name ) const;
	Int32 GetMemberNum() const;

	void funcSetMember( CScriptStackFrame& stack, void* result );
	void funcGetMember( CScriptStackFrame& stack, void* result );
	void funcHasMember( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCompanionComponent );
	PARENT_CLASS( CComponent );	
	INTERNAL_PROPERTY_RO( m_members, TXT( "Trait data" ) );
	NATIVE_FUNCTION( "I_SetMember"						, funcSetMember );
	NATIVE_FUNCTION( "I_GetMember"						, funcGetMember );
	NATIVE_FUNCTION( "I_HasMember"						, funcHasMember );
END_CLASS_RTTI();