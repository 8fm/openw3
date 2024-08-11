#pragma once

#include "selfUpdatingComponent.h"

class CHeadManagerComponent : public CSelfUpdatingComponent, public INamesListOwner
{
	DECLARE_ENGINE_CLASS( CHeadManagerComponent, CSelfUpdatingComponent, 0 );

	GameTime m_timePeriod;
	GameTime m_lastChangeGameTime;
	Bool m_hasTattoo;
	Bool m_hasDemonMark;
	Int32 m_initHeadIndex;
	Uint32 m_curIndex;
	TDynArray< CName > m_heads;
	TDynArray< CName > m_headsWithTattoo;
	TDynArray< CName > m_headsDemon;
	TDynArray< CName > m_headsDemonWithTattoo;
	SItemUniqueId m_curHeadId;
	Bool m_blockGrowing;

public:
	CHeadManagerComponent();

	virtual Bool CheckShouldSave() const override { return true; }
	virtual void OnSaveGameplayState( IGameSaver* saver ) override;
	virtual void OnLoadGameplayState( IGameLoader* loader ) override;

	void SetTattoo( Bool hasTattoo );
	void SetDemonMark( Bool hasDemonMark );

	virtual void GetNamesList( TDynArray< CName >& names ) const;

	virtual void OnAttachFinished( CWorld* world ) override;
	virtual void OnAttachFinishedEditor( CWorld* world ) override;

	void SetBeardStage( Uint32 stage );
	void Shave();
	void SetCustomHead( CName head );
	void RemoveCustomHead();

	virtual bool UsesAutoUpdateTransform() override { return false; }

protected:
	void InitHead();
	void InitTattooAndDemonMark();
	virtual void CustomTick( Float deltaTime ) override;
	void UpdateHead();
	void ChangeHead( CName head );
	void ChangeHeadsSet();
	TDynArray< CName >& GetCurHeadArray();

protected:
	void funcSetTattoo( CScriptStackFrame& stack, void* result );
	void funcSetDemonMark( CScriptStackFrame& stack, void* result );
	void funcShave( CScriptStackFrame& stack, void* result );
	void funcSetBeardStage( CScriptStackFrame& stack, void* result );
	void funcSetCustomHead( CScriptStackFrame& stack, void* result );
	void funcRemoveCustomHead( CScriptStackFrame& stack, void* result );
	void funcBlockGrowing( CScriptStackFrame& stack, void* result );
	void funcMimicTest( CScriptStackFrame& stack, void* result );	
	void funcGetCurHeadName( CScriptStackFrame& stack, void* result );	
};


BEGIN_CLASS_RTTI( CHeadManagerComponent )
	PARENT_CLASS( CSelfUpdatingComponent )
	PROPERTY_CUSTOM_EDIT( m_timePeriod, TXT("Game time, how often beard should grow."), TXT( "GameTimePropertyEditor" ) );
	PROPERTY_EDIT( m_initHeadIndex, TXT("Init head index.") );
	PROPERTY( m_lastChangeGameTime );
	PROPERTY( m_hasTattoo );
	PROPERTY( m_hasDemonMark );
	PROPERTY( m_curIndex );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_heads, TXT("Name of head item in order of beard lenght"), TXT("SuggestedListSelection") );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_headsWithTattoo, TXT("Name of head item with tattoo in order of beard lenght"), TXT("SuggestedListSelection") );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_headsDemon, TXT("Name of head item with demon mark in order of beard lenght"), TXT("SuggestedListSelection") );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_headsDemonWithTattoo, TXT("Name of head item with demon mark and tattoo in order of beard lenght"), TXT("SuggestedListSelection") );
	PROPERTY( m_curHeadId );
	PROPERTY_EDIT( m_blockGrowing, TXT("If true beard won't grow") );
	

	NATIVE_FUNCTION( "SetTattoo", funcSetTattoo );
	NATIVE_FUNCTION( "SetDemonMark", funcSetDemonMark );
	NATIVE_FUNCTION( "Shave", funcShave );
	NATIVE_FUNCTION( "SetBeardStage", funcSetBeardStage );
	NATIVE_FUNCTION( "SetCustomHead", funcSetCustomHead );
	NATIVE_FUNCTION( "RemoveCustomHead", funcRemoveCustomHead);
	NATIVE_FUNCTION( "BlockGrowing", funcBlockGrowing );
	NATIVE_FUNCTION( "MimicTest", funcMimicTest );
	NATIVE_FUNCTION( "GetCurHeadName", funcGetCurHeadName );
	
END_CLASS_RTTI()
