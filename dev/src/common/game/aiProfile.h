/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/entityTemplateParams.h"

#include "lookAtTypes.h"
#include "factsDBEditorQuery.h"
#include "2daProperties.h"
#include "edWizardSavedAnswers.h"

///////////////////////////////////////////////////////////////////////
enum EVisibilityTest
{
	VT_None,
	VT_LineOfSight,
	VT_RangeAndLineOfSight,
};

BEGIN_ENUM_RTTI( EVisibilityTest );
	ENUM_OPTION( VT_None );
	ENUM_OPTION( VT_LineOfSight );
	ENUM_OPTION( VT_RangeAndLineOfSight );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////
struct SAIReactionRange
{
	DECLARE_RTTI_STRUCT( SAIReactionRange );

	SAIReactionRange()
		: m_enabled( false )
		, m_rangeMax( 3.0f )
		, m_rangeAngle( 360.0f )
		, m_rangeBottom( -0.2f )
		, m_rangeTop( 2.5f )
		, m_yaw( 0.0f )
	{
	}

	Bool	m_enabled;
	Float	m_rangeMax;
	Float	m_rangeAngle;
	Float	m_rangeBottom;
	Float	m_rangeTop;
	Float	m_yaw;

	Bool PerformTest( CNode* reactingNPC, const Vector& interestPointPosition ) const;
};

BEGIN_CLASS_RTTI( SAIReactionRange );
	PROPERTY_EDIT( m_enabled, TXT("Is enabled") );
	PROPERTY_EDIT( m_rangeMax, TXT("Range max distance") );
	PROPERTY_EDIT( m_rangeAngle, TXT("Range angle") );
	PROPERTY_EDIT( m_rangeBottom, TXT("Range bottom") );
	PROPERTY_EDIT( m_rangeTop, TXT("Range top") );
	PROPERTY_EDIT( m_yaw, TXT("Yaw") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////
struct SAIReactionFactTest
{
	DECLARE_RTTI_STRUCT( SAIReactionFactTest );

	SAIReactionFactTest()
		: m_enabled( false )
		, m_value( 0 )

	{
	}

	Bool				m_enabled;
	String				m_factId;			//!< The id/name of desired fact
	Int32				m_value;			//!< Value to compare with
	ECompareFunc		m_compareFunc;		//!< How to compare values
	EQueryFact			m_queryFact;		//!< Which query method to use	

	Bool PerformTest() const;
};

BEGIN_CLASS_RTTI( SAIReactionFactTest );
	PROPERTY_EDIT( m_enabled, TXT("Is enabled") );
	PROPERTY_EDIT( m_factId, TXT("The id/name of desired fact.") )
	PROPERTY_EDIT( m_queryFact, TXT("Select query method to use.") )
	PROPERTY_EDIT( m_value, TXT("Value to compare with.") )
	PROPERTY_EDIT( m_compareFunc, TXT("How to compare values.") )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////
class IReactionAction;
class IReactionCondition;
class CAIReaction : public CObject
{
	DECLARE_ENGINE_CLASS( CAIReaction, CObject, 0 );

	CName										m_fieldName;
	Float										m_cooldownTime;
	EVisibilityTest								m_visibilityTest;
	SAIReactionRange							m_range;
	SAIReactionFactTest							m_factTest;
	IReactionCondition*							m_condition;
	IReactionAction*							m_action;

	CAIReaction() 
		: m_cooldownTime( 0.0f )
		, m_visibilityTest( VT_None )
		, m_condition( NULL )
		, m_action( NULL )
	{}
};
BEGIN_CLASS_RTTI( CAIReaction );
	PARENT_CLASS( CObject );
	PROPERTY_CUSTOM_EDIT( m_fieldName, TXT( "Field events from which we're interested in" ), TXT( "ReactionFieldEditor" ) );
	PROPERTY_EDIT( m_cooldownTime, TXT( "Reaction cooldown time" ) )
	PROPERTY_EDIT( m_visibilityTest, TXT( "Visibility test mode" ) )
	PROPERTY_EDIT( m_range, TXT( "Range test params" ) )
	PROPERTY_EDIT( m_factTest, TXT( "Fact test params" ) )
	PROPERTY_INLINED( m_condition, TXT( "Condition required by reaction" ) )
	PROPERTY_INLINED( m_action, TXT( "Action we want the Actor to perform" ) )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CAISenseParams : public CObject
{
	DECLARE_ENGINE_CLASS( CAISenseParams, CObject, 0 );
public:
	Bool	m_enabled;
	Float	m_rangeMin;
	Float	m_rangeMax;
	Float	m_rangeAngle;
	Float	m_height;
	Bool	m_testLOS;
	Bool	m_detectOnlyHostiles;

	CAISenseParams()
		: m_enabled( true )
		, m_rangeMin( 0.0f )
		, m_rangeMax( 10.0f )
		, m_rangeAngle( 100.0f )
		, m_height( 2.0f )
		, m_testLOS( true )
		, m_detectOnlyHostiles( true )
	{}
};

BEGIN_CLASS_RTTI( CAISenseParams );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_enabled, TXT("Enabled") );
	PROPERTY_EDIT_RANGE( m_rangeMin, TXT("Minimum range"), 0.0f, NumericLimits< Float >::Max() );
	PROPERTY_EDIT_RANGE( m_rangeMax, TXT("Maximum range"), 0.0f, NumericLimits< Float >::Max() );	
	PROPERTY_EDIT_RANGE( m_rangeAngle, TXT("Angle range"), 0.0f, 360.0f );
	PROPERTY_EDIT_RANGE( m_height, TXT("Height"), 0.0f, NumericLimits< Float >::Max() );
	PROPERTY_EDIT( m_testLOS, TXT("Test Line Of sight") );
	PROPERTY_EDIT( m_detectOnlyHostiles, TXT("Detect only hostile actors") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
enum EAISenseType
{
	AIST_Invalid,
	AIST_Vision,
	AIST_Absolute,
};

///////////////////////////////////////////////////////////////////////////////
enum EAIMinigameDifficulty
{
	AIMD_Easy,
	AIMD_Normal,
	AIMD_Hard,
};

BEGIN_ENUM_RTTI( EAIMinigameDifficulty );
	ENUM_OPTION( AIMD_Easy );
	ENUM_OPTION( AIMD_Normal );
	ENUM_OPTION( AIMD_Hard );
END_ENUM_RTTI();

template<> RED_INLINE String ToString( const EAIMinigameDifficulty& value )
{
	switch ( value )
	{
	case AIMD_Easy:
		return TXT("Easy");
	case AIMD_Normal:
		return TXT("Normal");
	case AIMD_Hard:
		return TXT("Hard");
	default:
		ASSERT( !TXT("ToString(): Unknown EAIMinigameDifficulty value.") );
		return TXT("Unknown");
	}
}

template<> RED_INLINE Bool FromString( const String& text, EAIMinigameDifficulty& value )
{
	if ( text == TXT("Easy") )
	{
		value = AIMD_Easy;
		return true;
	}
	else if ( text == TXT("Normal") )
	{
		value = AIMD_Normal;
		return true;
	}
	else if ( text == TXT("Hard") )
	{
		value = AIMD_Hard;
		return true;
	}
	else
	{
		ASSERT( !TXT("FromString(): Unknown string EAIMinigameDifficulty value."));
		value = AIMD_Normal;
		return false;
	}
}

class CAIMinigameParamsWristWrestling : public CObject
{
	DECLARE_ENGINE_CLASS( CAIMinigameParamsWristWrestling, CObject, 0 );

public:
	Int32						m_hotSpotMinWidth;
	Int32						m_hotSpotMaxWidth;
	EAIMinigameDifficulty	m_gameDifficulty;

	CAIMinigameParamsWristWrestling() : m_hotSpotMinWidth( 6 ), m_hotSpotMaxWidth( 20 ), m_gameDifficulty( AIMD_Normal ) {}
};

BEGIN_CLASS_RTTI( CAIMinigameParamsWristWrestling )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT_RANGE( m_hotSpotMinWidth, TXT("Hot Spot minimum width"), 1, 100 );
	PROPERTY_EDIT_RANGE( m_hotSpotMaxWidth, TXT("Hot Spot maximum width"), 1, 100 );
	PROPERTY_EDIT( m_gameDifficulty,  TXT("Game Difficulty") );
END_CLASS_RTTI();

struct SAIMinigameParams
{
	DECLARE_RTTI_STRUCT( SAIMinigameParams )
public:
	CAIMinigameParamsWristWrestling *m_wristWrestling;

public:
	SAIMinigameParams() : m_wristWrestling( NULL ) {}

	CAIMinigameParamsWristWrestling* GetWristWrestlingParams() { return m_wristWrestling; }
};

BEGIN_CLASS_RTTI( SAIMinigameParams );
PROPERTY_INLINED( m_wristWrestling, TXT("Wrist Wrestling game difficulty params") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
class CAIProfile : public CEntityTemplateParam
#ifndef NO_EDITOR
	, public CAttitude2dPropertyOwner
#endif
{
	DECLARE_ENGINE_CLASS( CAIProfile, CEntityTemplateParam, 0 );

public:
	typedef TDynArray< THandle< CEntityTemplate > > TAIPresets;
	typedef TDynArray< THandle< CAIReaction > > TAIReactions;

	TAIReactions				m_reactions;
	
	CAISenseParams*				m_senseVisionParams;	//!< Vision sense params
	CAISenseParams*				m_senseAbsoluteParams;	//!< Absolute sense params
	CName						m_attitudeGroup;		//!< Attitude group name
	SAIMinigameParams			m_minigameParams;
#ifdef AI_WIZARD_BACKWARD_COMPAT
	THandle< CAIBaseTree >		m_aiResource;			//!< AI Tree
	TAIPresets					m_aiPresets;			//!< Templates whose AI is injected in this profile
#endif
#ifndef NO_EDITOR
	THandle< CResource >		m_aiWizardRes;			//!< Custom ai preset wizard definition resource
#endif

public:
	CAIProfile();

	CAIReaction* FindReaction( const CName& fieldName ) const;

	void AddReaction( CAIReaction* reaction );

	void RemoveReaction( CAIReaction* reaction );

	void MoveReaction( CAIReaction* reaction, Bool up );

	RED_INLINE const TAIReactions& GetReactions() const { return m_reactions; }

	//! Get sense params
	CAISenseParams* GetSenseParams( EAISenseType senseType ) const;

	//! Set sense params
	void SetSenseParams( EAISenseType senseType, CAISenseParams* senseParams );

	//! Retrieve attitude group
	const CName& GetAttitudeGroup() const { return m_attitudeGroup; }

	//! Set attitude group
	void SetAttitudeGroup( CName& group ) { m_attitudeGroup = group; }

	//! Minigame difficulty parameters
	SAIMinigameParams* GetMinigameParams() { return &m_minigameParams; }

#ifdef AI_WIZARD_BACKWARD_COMPAT
	//! Get AI Tree object
	RED_INLINE CAIBaseTree* GetAiResource() const { return m_aiResource.Get(); }
	RED_INLINE const TAIPresets& GetAiPresets()const{ return m_aiPresets; }
	void ClearOldDataFromIncludes();
#endif
};
BEGIN_CLASS_RTTI( CAIProfile );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_INLINED( m_reactions, TXT( "Reactions of this NPC" ) );
	PROPERTY_INLINED( m_senseVisionParams, TXT("Vision sense params") );
	PROPERTY_INLINED( m_senseAbsoluteParams, TXT("Absolute sense params") );
	PROPERTY_CUSTOM_EDIT( m_attitudeGroup, TXT( "Attitude group" ), TXT( "2daValueSelection" ) );
	PROPERTY( m_minigameParams );
#ifdef AI_WIZARD_BACKWARD_COMPAT
	PROPERTY_INLINED( m_aiResource, TXT("AI Tree") );
	PROPERTY( m_aiPresets );
#endif
#ifndef NO_EDITOR
	PROPERTY_EDIT( m_aiWizardRes, TXT("Custom ai preset wizard resource") );
#endif
END_CLASS_RTTI();


///////////////////////////////////////////////////////
// CAITemplateParam
// Parent parameter for all ai parameters
// This class is used along with CollectGameParam< CAITemplateParam > to retreive all ai wizard related templates
class CAITemplateParam  : public CGameplayEntityParam
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CAITemplateParam, CGameplayEntityParam );
public:
	
public:
	CAITemplateParam( const String & name = TXT("none") )
		: CGameplayEntityParam( name, true )	{}
};
BEGIN_ABSTRACT_CLASS_RTTI( CAITemplateParam );
	PARENT_CLASS( CGameplayEntityParam );
END_CLASS_RTTI();


///////////////////////////////////////////////////////
// CAIBaseTreeTemplateParam
// Parameter holding the Ai base tree for the entity
class CAIBaseTreeTemplateParam  : public CAITemplateParam
{
	DECLARE_ENGINE_CLASS( CAIBaseTreeTemplateParam, CAITemplateParam, 0 );
public:
	THandle< CAIBaseTree >				m_aiBaseTree;			//!< AI Tree
public:
	CAIBaseTreeTemplateParam();
};
BEGIN_CLASS_RTTI( CAIBaseTreeTemplateParam );
	PARENT_CLASS( CAITemplateParam );
	PROPERTY_INLINED( m_aiBaseTree, TXT("AI Base Tree defined by Ai wizard or set by hand") );
END_CLASS_RTTI();


///////////////////////////////////////////////////////
// CAIPresetsTemplateParam
// All ai presets set by the ai wizard
class CAIPresetsTemplateParam : public CTemplateListParam
{
	DECLARE_ENGINE_CLASS( CAIPresetsTemplateParam, CTemplateListParam, 0 );
protected:
	// List of custom vars redefinition parameters
	TDynArray< THandle< ICustomValAIParameters > >	m_customValParameters;

public:
	const TDynArray< THandle< ICustomValAIParameters > > & GetCustomValParameters() const { return m_customValParameters; }
	void AddCustomValParameters( ICustomValAIParameters *param );
	CAIPresetsTemplateParam();

#ifndef NO_RESOURCE_COOKING
	virtual Bool IsCooked() override;
#endif

// Editor only properties
#ifndef NO_EDITOR
protected:
	CEdWizardSavedAnswers					m_aiWizardAnswers;
public:

	CEdWizardSavedAnswers *const	GetAIWizardAswers(){ return &m_aiWizardAnswers; }
#else
	CEdWizardSavedAnswers *const	GetAIWizardAswers(){ return nullptr; }
#endif 
};
BEGIN_CLASS_RTTI( CAIPresetsTemplateParam );
	PARENT_CLASS( CTemplateListParam );
	PROPERTY_RO( m_customValParameters, TXT("Custom defined AI parameters") );
#ifndef NO_EDITOR
	PROPERTY_NOT_COOKED( m_aiWizardAnswers )
#endif
END_CLASS_RTTI();

#ifndef NO_EDITOR

///////////////////////////////////////////////////////
// CAIWizardTemplateParam
// Parameter defining custom Ai wizard 
class CAIWizardTemplateParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CAIWizardTemplateParam, CGameplayEntityParam, 0 );
public:
	THandle< CResource >			m_aiWizardResource;

public:
	CAIWizardTemplateParam();

#ifndef NO_RESOURCE_COOKING
	virtual Bool IsCooked() override;
#endif
};
BEGIN_CLASS_RTTI( CAIWizardTemplateParam );
	PARENT_CLASS( CGameplayEntityParam );
	PROPERTY_EDIT_NOT_COOKED( m_aiWizardResource, TXT("AI Base Tree defined by Ai wizard or set by hand") );
END_CLASS_RTTI();

#endif			// #ifndef NO_EDITOR


