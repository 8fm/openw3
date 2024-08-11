/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/game/behTreeNode.h"
#include "../../common/game/aiStorage.h"
#include "playerLocomotionController.h"

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
namespace InputHelpers
{
	static const Float INPUT_EPSILON( FLT_EPSILON );

	// convert input in controller space to world space normalized vector
	Vector2 RawInputToWorldSpace( const CCameraDirector* const camDirector, Vector2 rawInput );

	// reduce vector length by some amount
	void VecReduce3( Vector& vec, Float amount );

	// reduce vector X and Y part by some amount
	void VecReduce2( Vector& vec, Float amount );

	// add one vector to another with some limit on length
	void VecAddLimited3( Vector& vec, const Vector& add, Float maxLen );
	void VecAddLimited2( Vector& vec, const Vector& add, Float maxLen );
};

//------------------------------------------------------------------------------------------------------------------
// SPlayerMovementData is meant to be stored in AIStorage as a shared struct for all the player movement states
//------------------------------------------------------------------------------------------------------------------
struct SPlayerMovementData
{
	DECLARE_RTTI_STRUCT( SPlayerMovementData )

	SPlayerMovementData();
	~SPlayerMovementData();

	Vector	m_velocity;	 
	Float	m_requestedFacingDirection;
	Float	m_requestedMovementDirection;

	CPlayerLocomotionController* m_playerLocomotionController;

	// pass the variables from this struct to behavior graph
	void SetBehaviorVariables( CBehaviorGraphStack* stack ) const;

	// initializer class for AIStorage
	class CInitializer : public CAIStorageItem::CInitializer
	{
	public:
		CName GetItemName() const override { return CNAME( PlayerMovementData ); }
		IRTTIType* GetItemType() const override { return SPlayerMovementData::GetStaticClass(); }
	};
};

BEGIN_CLASS_RTTI( SPlayerMovementData )
	PROPERTY( m_velocity )
END_CLASS_RTTI()

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class IBehTreeNodePlayerStateInstance;

class IBehTreeNodePlayerStateDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodePlayerStateDefinition, IBehTreeNodeDefinition, IBehTreeNodePlayerStateInstance, PlayerState )
	DECLARE_AS_R6_ONLY
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodePlayerStateDefinition )
	PARENT_CLASS( IBehTreeNodeDefinition )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class IBehTreeNodePlayerStateInstance : public IBehTreeNodeInstance
{
	typedef IBehTreeNodeInstance Super;

protected:
	CName										m_stateName;		// name of this state
	THandle< CMovingPhysicalAgentComponent >	m_component;		// a component we're working on
	TAIStoragePtr< SPlayerMovementData >		m_movementDataPtr;	// pointer to the shared movement data

	RED_INLINE SPlayerMovementData& GetMovementData() { return *m_movementDataPtr; }

public:
	typedef IBehTreeNodePlayerStateDefinition Definition;

	IBehTreeNodePlayerStateInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	virtual ~IBehTreeNodePlayerStateInstance();

	virtual Bool Activate() override;

	/// @return PlayerLocomotionController
	CPlayerLocomotionController* GetPLC();
};





RED_INLINE CPlayerLocomotionController* IBehTreeNodePlayerStateInstance::GetPLC()
{
	R6_ASSERT( m_movementDataPtr );
	R6_ASSERT( m_movementDataPtr->m_playerLocomotionController );
	return m_movementDataPtr->m_playerLocomotionController;
}

