/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/factory.h"


//////////////////////////////////////////////////////////////////
class CWizardBaseNode : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CWizardBaseNode, CObject );

public:
	CWizardBaseNode() {}
	virtual Uint32 GetNumChildren() { return 0; }
	virtual CWizardBaseNode* GetChild( Uint32 index ) { return NULL; }
};
BEGIN_ABSTRACT_CLASS_RTTI( CWizardBaseNode );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////
// CWizardQuestionNode
class CWizardQuestionNode : public CWizardBaseNode
{
	DECLARE_ENGINE_CLASS( CWizardQuestionNode, CWizardBaseNode, 0 );

protected:
	CName	m_uniqueName;
	String	m_layoutTemplate;
	String	m_text;
	Bool	m_optional;
	Bool	m_endNode;
public:
	CWizardQuestionNode()
		: m_uniqueName()
		, m_layoutTemplate( TXT( "pageTemplate" ) )
		, m_text( String::EMPTY )
		, m_optional( false )
		, m_endNode( false )
	{
	}

	String GetTemplateName() const { return m_layoutTemplate; }
	Bool IsOptional() const { return m_optional; }
	Bool IsEndNode() const { return m_endNode; }
	String GetText() const { return m_text; }
	CName GetUniqueName()const { return m_uniqueName; }
};
BEGIN_CLASS_RTTI( CWizardQuestionNode );
	PARENT_CLASS( CWizardBaseNode );
	PROPERTY_EDIT( m_uniqueName, TXT("Unique name enabling values to be saved, will appear in the resume") );
	PROPERTY_EDIT( m_layoutTemplate, TXT("Layout template for this node and its data.") );
	PROPERTY_EDIT( m_text, TXT("Entry description text.") );
	PROPERTY_EDIT( m_optional, TXT("Flag denoting if this entry can be skipped.") );
	PROPERTY_EDIT( m_endNode, TXT("Flag denoting if this entry ends the wizard.") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////
class CWizardDefinition : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CWizardDefinition, CResource, "wizdef" , "Wizard Definition" );

protected:
	TDynArray< CWizardQuestionNode* >		m_nodes;

public:
	CWizardDefinition();
	TDynArray< CWizardQuestionNode* > GetNodes() const { return m_nodes; }

};

BEGIN_CLASS_RTTI( CWizardDefinition );
	PARENT_CLASS( CResource );
	PROPERTY_INLINED( m_nodes, TXT("Wizard node collection.") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////
class CWizardDefinitionFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CWizardDefinitionFactory, IFactory, 0 );

public:
	CWizardDefinitionFactory()
	{
		m_resourceClass = ClassID< CWizardDefinition >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CWizardDefinitionFactory );
PARENT_CLASS( IFactory );
END_CLASS_RTTI();
