/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "behTreeNodeCondition.h"
#include "behTreeWorkData.h"
#include "actionPointDataDef.h"

class CBehTreeNodeSelectActionPointDecoratorInstance;
class CActionPointSelectorInstance;

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectActionPointDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectActionPointDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeSelectActionPointDecoratorInstance, SelectActionPoint );
protected:
	CBehTreeValFloat			m_delayOnFailure;
	CBehTreeValFloat			m_delayOnSuccess;
	CBehTreeValFloat			m_delayOnInterruption;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeSelectActionPointDecoratorDefinition()
		: m_delayOnFailure( 5.f )
		, m_delayOnSuccess( -1.f )
		, m_delayOnInterruption( -1.f )									{}
};


BEGIN_CLASS_RTTI( CBehTreeNodeSelectActionPointDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_delayOnFailure, TXT( "Delay next work loop on failure" ) )
	PROPERTY_EDIT( m_delayOnSuccess, TXT( "Delay next work loop on success" ) )
	PROPERTY_EDIT( m_delayOnInterruption, TXT( "Delay next work loop on interruption" ) )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectActionPointDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	static const Float	AP_RESERVATION_TIME; //seconds

	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeNodeSelectActionPointDecoratorDefinition Definition;

	CBehTreeNodeSelectActionPointDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	~CBehTreeNodeSelectActionPointDecoratorInstance();

	void OnDestruction() override;

	// base node interface
	Bool Activate() override;
	void Deactivate() override;
	Bool Interrupt() override;

	Bool IsAvailable() override;
	Int32 Evaluate() override;

	Bool OnEvent( CBehTreeEvent& e ) override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;
	void Complete( eTaskOutcome outcome ) override;

#ifdef DEBUG_ACTION_POINT_RESERVATION
	void OnGenerateDebugFragments( CRenderFrame* frame ) override;
#endif

protected:
	Bool SelectNewActionPoint( const SActionPointId& lastActionPoint, SActionPointId& newActionPoint, CName& workCategory, Float forceRadius = -1.f ) const;
	void SetActiveActionPoint( const SActionPointId& actionPoint, const CName& category );

	void UnlockActionPoint();
	Bool DoSelection();
	void ReserveSelectedAP();
	Bool CheckCondition();


protected:
	CBehTreeWorkDataPtr				m_workData;
	TagList							m_npcActionPointTags;
	CActionPointSelectorInstance*	m_apSelector;
	SActionPointId					m_chosenAP;
	CName							m_chosenCategory;
	Float							m_selectTimout;
	Float							m_activationTimeout;								// on failure we automatically delay work activation
	Float							m_delayOnFailure;
	Float							m_delayOnSuccess;
	Float							m_delayOnInterruption;
	Bool							m_doReserveSelectedAP;							// NOTICE: its quite unclear, but community system is registering AP usage on its own, so in case of using community ap selector we can't handle AP reservation by ourselves
};


