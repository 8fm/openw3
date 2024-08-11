/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"
#include "idHud.h"

enum EIDChoiceType
{
	IDCT_Default	= 0	,
	IDCT_Quest			,
	IDCT_Object			,
	IDCT_QuestObject	,
};
BEGIN_ENUM_RTTI( EIDChoiceType );
	ENUM_OPTION( IDCT_Default );
	ENUM_OPTION( IDCT_Quest );
	ENUM_OPTION( IDCT_Object );
	ENUM_OPTION( IDCT_QuestObject );
END_ENUM_RTTI();

//------------------------------------------------------------------------------------------------------------------
// Dialog option - a single choice
//------------------------------------------------------------------------------------------------------------------
struct SIDOption
{
	DECLARE_RTTI_STRUCT( SIDOption );

	EHudChoicePosition				m_hudPosition;
	LocalizedString					m_text;
	IDConditionList*				m_conditions; 
	EIDChoiceType					m_type;
	EntityHandle					m_relatedObject;

	#ifndef NO_EDITOR
		String						m_comment;
	#endif

	RED_INLINE Bool operator==( const SIDOption& otherOption ) const { return this == &otherOption; }

	Bool CanOptionBeShown( CIDTopicInstance* topicInstance ) const;
};

BEGIN_NODEFAULT_CLASS_RTTI( SIDOption )
	PROPERTY_EDIT( m_hudPosition, TXT("") )
	PROPERTY_CUSTOM_EDIT( m_text, TXT("Choice content"), TXT("LocalizedStringEditor") );
	PROPERTY_INLINED( m_conditions, TXT("All have to be met for this option to be used") )
	PROPERTY_EDIT( m_type, TXT("Choice type") )
	PROPERTY_EDIT( m_relatedObject, TXT("") )
	#ifndef NO_EDITOR
		PROPERTY_EDIT_NOT_COOKED( m_comment, TXT("") )
	#endif
END_CLASS_RTTI()

#ifndef NO_EDITOR
struct SIDOptionStub
{
	String				m_text;
	String				m_comment;
	EHudChoicePosition	m_position;
	Uint32				m_index;

	SIDOptionStub() : m_position( CHOICE_Max ) {}
	RED_INLINE Bool MatchesOption( const SIDOption* option ) const { return m_position == option->m_hudPosition && m_comment == option->m_comment && m_text == option->m_text.GetString(); }
};
#endif

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockChoice : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockChoice, CIDGraphBlock, 0 );

protected: 
	TDynArray< SIDOption >		m_options;
	Bool						m_clearLeftOption;
	Bool						m_clearRightOption;
	Bool						m_clearUpOption;
	Bool						m_clearDownOption;
	
	TInstanceVar< Uint32 >		i_optionsActive;
	TInstanceVar< Int8 >		i_currentOutput; 

public:
	CIDGraphBlockChoice();

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Choice ); }
	virtual String GetCaption() const;

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! Called in the editor when property had been changed
	virtual void OnPropertyPostChange( IProperty* prop );

	// ILocalizableObject interface
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */;

	//! Choice block is always active (all non-regular blocks are)
	virtual void SetActive( CIDTopicInstance* topicInstance, Bool activate ) const {} 
	virtual Bool IsActivated( const CIDTopicInstance* topicInstance ) const { return true; } 

	//! Graph evaluation
	virtual const CIDGraphBlock* ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate = true ) const;
	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
	virtual const CIDGraphBlock* ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate = true ) const;
	virtual Bool IsRegular() const { return false; }

	//! OnEncoutered() is called when the signal reaches this block
	void OnEncoutered( CIDTopicInstance* topicInstance ) const;

	//! InstanceBuffer stuff
	virtual void OnInitInstance( InstanceBuffer& data ) const;
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	
	const CIDGraphSocket* FindOptionOutput( const SIDOption* option ) const;

	RED_INLINE const SIDOption& GetOption( Uint32 optionIndex ) const { return m_options[ optionIndex ]; }
	RED_INLINE Uint32 GetNumOptions() const { return m_options.Size(); }
	const SIDOption* FindOption( EHudChoicePosition position ) const;
	Int32 FindOptionIndex( EHudChoicePosition position ) const;

	Bool IsAnyOptionActive( CIDTopicInstance* topicInstance ) const;

	#ifndef NO_EDITOR
		SIDOption* FindOption( EHudChoicePosition position );
		RED_INLINE SIDOption* AddOption( EHudChoicePosition pos, Uint32 idx ) { m_options.Insert( idx, SIDOption() ); SIDOption* opt = &( m_options[ idx ] ); opt->m_hudPosition = pos; return opt; }
		RED_INLINE void RemoveOption( SIDOption& opt ) { m_options.Remove( opt ); }
	#endif 

	void ValidateOptions();

protected:
	String GetOutputNameFor( EHudChoicePosition pos ) const;
	Bool IsOptionActive( CIDTopicInstance* topicInstance, Uint32 optionIndex ) const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockChoice )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_INLINED( m_options, TXT("Dialog choices") )
	PROPERTY_EDIT( m_clearLeftOption, TXT("") )
	PROPERTY_EDIT( m_clearRightOption, TXT("") )
	PROPERTY_EDIT( m_clearUpOption, TXT("") )
	PROPERTY_EDIT( m_clearDownOption, TXT("") )
END_CLASS_RTTI()
