#pragma once

class CR4JournalPlaceEntity: public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( CR4JournalPlaceEntity, CGameplayEntity, 0 );

protected:
	THandle< CJournalPath > m_placeEntry;

private:
	void funcGetJournalPlaceEntry( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR4JournalPlaceEntity )
	PARENT_CLASS( CGameplayEntity )	
	PROPERTY_CUSTOM_EDIT( m_placeEntry, TXT( "Place entry" ), TXT( "JournalPropertyBrowserPlace" ) )
	NATIVE_FUNCTION( "GetJournalPlaceEntry", funcGetJournalPlaceEntry );
END_CLASS_RTTI()
