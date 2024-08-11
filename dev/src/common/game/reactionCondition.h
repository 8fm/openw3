/**
* Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

///////////////////////////////////////////////////////////////////////////////

class CInterestPointInstance;

class IReactionCondition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IReactionCondition, CObject );

public:
	// Performs the action on the selected NPC
	virtual Bool Perform( const CNode* source, const CNode* target, CInterestPointInstance* instance ) { return true; }

};

BEGIN_ABSTRACT_CLASS_RTTI( IReactionCondition );
PARENT_CLASS( CObject );
END_CLASS_RTTI();

////////////////////////////////////////////////////

class CReactionOrCondition : public IReactionCondition
{
	DECLARE_ENGINE_CLASS( CReactionOrCondition, IReactionCondition, 0 );

private:
	TDynArray<IReactionCondition*> m_conditions;

public:
	virtual ~CReactionOrCondition() {}
	virtual Bool Perform( const CNode* source, const CNode* target, CInterestPointInstance* instance );
};

BEGIN_CLASS_RTTI( CReactionOrCondition );
	PARENT_CLASS( IReactionCondition );
	PROPERTY_INLINED( m_conditions, TXT("Set of conditions one of which (at least) has to be met for the reaction to be evaluated") );
END_CLASS_RTTI();

////////////////////////////////////////////////////

class CReactionAndCondition : public IReactionCondition
{
	DECLARE_ENGINE_CLASS( CReactionAndCondition, IReactionCondition, 0 );

private:
	TDynArray<IReactionCondition*> m_conditions;

public:
	virtual ~CReactionAndCondition() {}
	virtual Bool Perform( const CNode* source, const CNode* target, CInterestPointInstance* instance );
};

BEGIN_CLASS_RTTI( CReactionAndCondition );
	PARENT_CLASS( IReactionCondition );
	PROPERTY_INLINED( m_conditions, TXT("Set of conditions all of which have to be met for the reaction to be evaluated") );
END_CLASS_RTTI();

////////////////////////////////////////////////////

class CReactionScriptedCondition : public IReactionCondition
{
	DECLARE_ENGINE_CLASS( CReactionScriptedCondition, IReactionCondition, 0 );

public:
	CReactionScriptedCondition();
	virtual ~CReactionScriptedCondition() {}

	// Performs the action on the selected actor
	virtual Bool Perform( const CNode* source, const CNode* target, CInterestPointInstance* instance );

};

BEGIN_CLASS_RTTI( CReactionScriptedCondition );
PARENT_CLASS( IReactionCondition );
END_CLASS_RTTI();


