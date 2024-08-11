/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//#include "wizardDefinition.h"

class CEdWizardTemplate;

// Nodes

// Data

//////////////////////////////////////////////////////////////////
class CWizardOptionData : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CWizardOptionData, CObject );
	
public:
	

	CWizardOptionData()	{}

	virtual String GetValueName() const { return String::EMPTY; }
	virtual void CommitData( CObject* arg ) {}
};
BEGIN_CLASS_RTTI( CWizardOptionData );
	PARENT_CLASS( CObject );
	
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////
class CAiPresetWizardData : public CWizardOptionData
{
	DECLARE_ENGINE_CLASS( CAiPresetWizardData, CWizardOptionData, 0 );

protected:
	THandle< CEntityTemplate > m_entityTemplate;

public:
	CAiPresetWizardData()
		: m_entityTemplate( nullptr ) {}
	String GetValueName() const override { return m_entityTemplate.IsValid() ? m_entityTemplate->GetFile()->GetFileName() : String::EMPTY; }
	void CommitData( CObject* arg ) override;
};

BEGIN_CLASS_RTTI( CAiPresetWizardData );
	PARENT_CLASS( CWizardOptionData );
	PROPERTY_EDIT( m_entityTemplate, TXT("Entity template to inherit AI from.") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////
class CCustomParamWizardData : public CWizardOptionData
{
	DECLARE_ENGINE_CLASS( CCustomParamWizardData, CWizardOptionData, 0 );

protected:
	String								m_value;
	THandle< ICustomValAIParameters >	m_customValAIParameters;
public:
	CCustomParamWizardData()
		: m_value()
	{}
	String GetValueName() const override { return String::EMPTY; }
	void SetValue( const String & value ){ m_value = value; }
	void CommitData( CObject* arg ) override;
};

BEGIN_CLASS_RTTI( CCustomParamWizardData );
	PARENT_CLASS( CWizardOptionData );
	PROPERTY( m_value );
	PROPERTY_INLINED( m_customValAIParameters, TXT("The AiParameter with the values that needs to be redefined") );
END_CLASS_RTTI();