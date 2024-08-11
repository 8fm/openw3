/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeAtomicAction.h"

#include "behTreeFlightData.h"


class IBehTreeNodeAtomicFlightInstance;

class IBehTreeNodeAtomicFlightDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeAtomicFlightDefinition, CBehTreeNodeAtomicActionDefinition, IBehTreeNodeAtomicFlightInstance, Flight );
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeAtomicFlightDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )
END_CLASS_RTTI()

class IBehTreeNodeAtomicFlightInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	CBehTreeFlightDataPtr			m_flightData;
public:
	typedef IBehTreeNodeAtomicFlightDefinition Definition;

	IBehTreeNodeAtomicFlightInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool			Activate() override;
	void			Deactivate() override;

	void			Update() override;

	void			OnGenerateDebugFragments( CRenderFrame* frame ) override;

	virtual CAreaComponent*	GetAreaEncompassingMovement();					// the interface should be different, but we don't care for now as the E3 stuff is approaching

};