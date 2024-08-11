#pragma once

#include "lockableEntity.h"

//////////////////////////////////////////////////////////////////////////////////////////////
class W3Container : public W3LockableEntity
{
	DECLARE_ENGINE_CLASS( W3Container, W3LockableEntity, 0 );

protected:

	Bool m_shouldBeFullySaved;

public:
	W3Container();

	//! Should save?
	virtual Bool CheckShouldSave() const override;

	void SetShouldBeFullySaved( Bool fullySaved );

	void UpdateShouldBeFullySaved();

protected:

	virtual Bool HasToBeFullySaved() const { return m_shouldBeFullySaved; }
	void funcSetIsQuestContainer( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( W3Container );
	PARENT_CLASS( W3LockableEntity );
	PROPERTY_SAVED( m_shouldBeFullySaved );

	NATIVE_FUNCTION( "SetIsQuestContainer", funcSetIsQuestContainer );	
END_CLASS_RTTI();
