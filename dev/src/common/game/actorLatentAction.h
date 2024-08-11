/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../game/aiParameters.h"

class CBehTreeInstance;
class CBehTreeSpawnContext;
class IBehTreeNodeInstance;

///////////////////////////////////////////////////////////////////////////////
// Base for latent actions performed on an actor
class IActorLatentAction : public IAIParameters
{
	typedef IAIParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IActorLatentAction )
public:
	virtual IBehTreeNodeInstance* ComputeAction( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	THandle< IAIActionTree > ConvertToActionTree( const THandle< IScriptable >& parentObj );
};
BEGIN_CLASS_RTTI( IActorLatentAction )
	PARENT_CLASS( IAIParameters );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
class CCustomBehTreeActorLatentAction : public IActorLatentAction
{
	typedef IActorLatentAction TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CCustomBehTreeActorLatentAction );
public:
	CCustomBehTreeActorLatentAction();
	IBehTreeNodeInstance* ComputeAction( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) override;
protected:
	IBehTreeNodeDefinition*			m_behTree;
};
BEGIN_CLASS_RTTI( CCustomBehTreeActorLatentAction )
	PARENT_CLASS( IActorLatentAction );
	PROPERTY_CUSTOM_EDIT( m_behTree, TXT("AI action definition"), TXT("TreeEditorButton") );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
class IPresetActorLatentAction : public IActorLatentAction
{
	typedef IActorLatentAction TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IPresetActorLatentAction )
public:
	IPresetActorLatentAction();
	IBehTreeNodeInstance* ComputeAction( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) override;
protected:
	THandle< CBehTree >					m_res;
	CBehTreeNodeTemplateDefinition*		m_def;
	String								m_resName;
private:

	// todo natywne funkcje do ustawiania parametrów
};
BEGIN_CLASS_RTTI( IPresetActorLatentAction )
	PARENT_CLASS( IActorLatentAction );
	PROPERTY_NOSERIALIZE( m_res );
	PROPERTY_NOSERIALIZE( m_def );
	PROPERTY( m_resName );
END_CLASS_RTTI()