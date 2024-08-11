#pragma once
#include "../../common/game/behTreeNodeAtomicAction.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorMoveToDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeAtomicSailorMoveToInstance;
class CBehTreeNodeAtomicSailorMoveToDefinition : public CBehTreeNodeAtomicActionDefinition
{
    DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicSailorMoveToDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicSailorMoveToInstance, SailorMoveTo );
protected:
	CBehTreeValCName							m_boatTag;
	CBehTreeValCName							m_entityTag;
public:
    CBehTreeNodeAtomicSailorMoveToDefinition() 
        : m_entityTag( )
		, m_boatTag( )	{}
	
    IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;

};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicSailorMoveToDefinition );
    PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
	PROPERTY_EDIT( m_boatTag, TXT("Tag of the boat component to use") );
	PROPERTY_EDIT( m_entityTag, TXT("Tag of the entity to go to") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorMoveToInstance
class CBehTreeNodeAtomicSailorMoveToInstance : public CBehTreeNodeAtomicActionInstance
{
    typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	CName										m_entityTag;
	CName										m_boatTag;
	THandle< CBoatComponent >					m_boatComponent;
	Bool										m_destinationReached;

public:
    typedef CBehTreeNodeAtomicSailorMoveToDefinition Definition;

    CBehTreeNodeAtomicSailorMoveToInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr );
	void OnDestruction() override;

	Bool OnListenedEvent( CBehTreeEvent& e )override;
	Bool Activate() override;
    void Update() override;
};