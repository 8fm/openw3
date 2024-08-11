#pragma once

#include "wizardImpl.h"
#include "../../common/game/wizardDefinition.h"

//////////////////////////////////////////////////////////////////
// Question with selectable options
class CWizardNode : public CWizardQuestionNode
{
	DECLARE_ENGINE_CLASS( CWizardNode, CWizardQuestionNode, 0 );

protected:
	TDynArray< CWizardOption* > m_options;

public:
	CWizardNode()
		: CWizardQuestionNode()
	{
	}

	Uint32 GetNumChildren() override { return m_options.Size(); }
	CWizardBaseNode* GetChild( Uint32 index ) override;
};

BEGIN_CLASS_RTTI( CWizardNode );
	PARENT_CLASS( CWizardQuestionNode );
	PROPERTY_INLINED( m_options, TXT("Available options for this entry.") );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////
// Question with CName field
class CWizardCNameQuestionNode : public CWizardQuestionNode
{
	DECLARE_ENGINE_CLASS( CWizardCNameQuestionNode, CWizardQuestionNode, 0 );

protected:
	CCustomParamWizardData *	m_value;
public:

	CCustomParamWizardData * GetValue(){ return m_value; }

	CWizardCNameQuestionNode()
		: CWizardQuestionNode()
	{
	}
};

BEGIN_CLASS_RTTI( CWizardCNameQuestionNode );
	PARENT_CLASS( CWizardQuestionNode );
	PROPERTY_INLINED( m_value, TXT("fer") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////
class CWizardOption : public CWizardBaseNode
{
	DECLARE_ENGINE_CLASS( CWizardOption, CWizardBaseNode, 0 );

public:
	String	m_customText;
	CWizardOptionData* m_value;
	TDynArray< CWizardQuestionNode* > m_subNodes;

	CWizardOption()
		: m_customText( String::EMPTY )
		, m_value( NULL )
	{
	}

	String GetValueName() const;
	CWizardOptionData* GetValue() const { return m_value; }
	Uint32 GetNumChildren() override { return m_subNodes.Size(); }
	CWizardBaseNode* GetChild( Uint32 index ) override;
};

BEGIN_CLASS_RTTI( CWizardOption );
	PARENT_CLASS( CWizardBaseNode );
	PROPERTY_EDIT( m_customText, TXT("Custom text to be presented instead of the value as an option.") );
	PROPERTY_INLINED( m_value, TXT("Option data.") );
	PROPERTY_INLINED( m_subNodes, TXT("Sub entries chained to this option.") );
END_CLASS_RTTI();
