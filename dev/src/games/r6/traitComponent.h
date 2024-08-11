#pragma once
#include "traitData.h"

enum ETraitState
{
	TS_locked = 0,
	TS_unlocked,
	TS_active
};

BEGIN_ENUM_RTTI( ETraitState );
ENUM_OPTION( TS_locked );
ENUM_OPTION( TS_unlocked );
ENUM_OPTION( TS_active );
END_ENUM_RTTI();

struct TraitInfo
{
	ETraitState m_state;
	TDynArray<bool> m_requirements;
};

class CTraitComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CTraitComponent, CComponent, 0 );

private:
	THandle< CTraitData > m_traitData;

	THashMap< CName, TraitInfo > m_traitStates;
	THashMap< CName, Int32 > m_skillLevels;

	struct RequirementChecker
	{
		virtual bool Check( IRequirement* req ) const = 0;
	};

	struct SkillRequirementChecker : public RequirementChecker
	{
		CName m_skill;
		Int32 m_level;
		bool Check( IRequirement* req ) const;
	};

	struct TraitRequirementChecker : public RequirementChecker
	{
		CName m_trait;
		bool Check( IRequirement* req ) const;
	};

public:
	virtual void OnPostInstanced(); 

	virtual void OnLoadGameplayState( IGameLoader* loader );

	virtual void OnSaveGameplayState( IGameSaver* saver );
	virtual Bool CheckShouldSave() const { return true; }

	virtual Bool TryUsingResource( CResource * resource );

	RED_INLINE const THandle< CTraitData >& GetTraitData() { return m_traitData; }
	bool BuySkill( CName skill );
	bool IncreaseSkillLevel( CName skill, Int32 level );
	bool BuyTrait( CName trait );
	RED_INLINE const THashMap< CName, TraitInfo >& GetTraitStates() const { return m_traitStates; }
	RED_INLINE const THashMap< CName, Int32 >& GetSkillNames() const { return m_skillLevels; }

	STraitTableEntry* GetTrait( CName name ) { return m_traitData->GetTrait( name ); }
	SSkillTableEntry* GetSkill( CName name ) { return m_traitData->GetSkill( name ); }
	ETraitState GetTraitState( CName name ) const;
	Int32 GetSkillLevel( CName name ) const;

	void funcGetTraitData( CScriptStackFrame& stack, void* result );
	void funcBuySkill( CScriptStackFrame& stack, void* result );
	void funcIncreaseSkillLevel( CScriptStackFrame& stack, void* result );
	void funcBuyTrait( CScriptStackFrame& stack, void* result );
	void funcGetSkillTable( CScriptStackFrame& stack, void* result );
	void funcGetTraitState( CScriptStackFrame& stack, void* result );
	void funcGetSkillLevel( CScriptStackFrame& stack, void* result );
	void funcReset( CScriptStackFrame& stack, void* result );
	
protected:
	void InitRuntimeData();
	void CheckTraitRequirement( CName traitName );
	void CheckSkillRequirement( CName skillName, Int32 level );
	void CheckRequirement( CTraitComponent::RequirementChecker& checker );

};

BEGIN_CLASS_RTTI( CTraitComponent )
	PARENT_CLASS( CComponent )
	PROPERTY_EDIT( m_traitData, TXT( "Trait data" ) );
	NATIVE_FUNCTION( "I_GetTraitData"					, funcGetTraitData );
	NATIVE_FUNCTION( "I_BuySkill"						, funcBuySkill );
	NATIVE_FUNCTION( "I_IncreaseSkillLevel"				, funcIncreaseSkillLevel );
	NATIVE_FUNCTION( "I_BuyTrait"						, funcBuyTrait );
	NATIVE_FUNCTION( "I_GetTraitState"					, funcGetTraitState );
	NATIVE_FUNCTION( "I_GetSkillLevel"					, funcGetSkillLevel );
	NATIVE_FUNCTION( "I_Reset"							, funcReset );
	
END_CLASS_RTTI()
