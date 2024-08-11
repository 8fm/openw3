/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


///////////////////////////////////////////////////////
// CEDSavedAnswer
class CEDSavedAnswer
{
	DECLARE_RTTI_SIMPLE_CLASS( CEDSavedAnswer );
public:
	CName	m_questionName;
	String	m_answer;
	// This flag is used to prune the list after the ai wizard has been run
	Bool	m_valid; 

	CEDSavedAnswer();
	CEDSavedAnswer( CName questionName, const String & answer, Bool valid = false );
};
BEGIN_CLASS_RTTI( CEDSavedAnswer )
	PROPERTY( m_questionName );
	PROPERTY( m_answer );
END_CLASS_RTTI()


///////////////////////////////////////////////////////
// CEdWizardSavedAnswers
class CEdWizardSavedAnswers
{
	DECLARE_RTTI_SIMPLE_CLASS( CEdWizardSavedAnswers );
public:
	TDynArray< CEDSavedAnswer > m_list;

	CEdWizardSavedAnswers();
	void AddAnswer( CName questionName, const String& answer );
	Bool FindAnswer( CName questionName, String &answer );

	void PostCommit();
	void Prune();

	const TDynArray< CEDSavedAnswer > &GetList()const{ return m_list; }
};
BEGIN_CLASS_RTTI( CEdWizardSavedAnswers )
	PROPERTY( m_list )
END_CLASS_RTTI()