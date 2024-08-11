#pragma once
#include "../../common/game/behTreeNodeAtomicAction.h"
////////////////////////////////////////////////////////////////////////
// CFollowPathUtils 
////////////////////////////////////////////////////////////////////////
class CFollowPathUtils
{
public:
	static Bool ComputeTargetAndHeading( const Vector &actorPos, CName pathName, Bool startFromBeginning, Bool upThePath, THandle< CPathComponent > & pathComponentHandle, Vector & outStartingPoint, Float & outHeading );
};

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorMoveToPathDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeAtomicSailorMoveToPathInstance;
class CBehTreeNodeAtomicSailorMoveToPathDefinition : public CBehTreeNodeAtomicActionDefinition
{
    DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicSailorMoveToPathDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicSailorMoveToPathInstance, SailorMoveToPath );
protected:
	CBehTreeValCName							m_pathTag;
	CBehTreeValBool								m_upThePath;
	CBehTreeValBool								m_startFromBeginning;
	CBehTreeValCName							m_boatTag;
public:
    CBehTreeNodeAtomicSailorMoveToPathDefinition() 
        : m_pathTag( )
		, m_upThePath( true )
		, m_startFromBeginning( false )	
		, m_boatTag( )	{}
	
    IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;

};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicSailorMoveToPathDefinition );
    PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
	PROPERTY_EDIT( m_boatTag, TXT("Tag of the boat to use component") );
	PROPERTY_EDIT( m_pathTag, TXT("Tag of path component") );
	PROPERTY_EDIT( m_upThePath, TXT("Unmark this to make boat move from path end to beginning") )
	PROPERTY_EDIT( m_startFromBeginning, TXT("Mark if movemenet should begin always from beginning of path") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorMoveToPathInstance
class CBehTreeNodeAtomicSailorMoveToPathInstance : public CBehTreeNodeAtomicActionInstance
{
    typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	CName										m_pathTag;
	THandle< CPathComponent >					m_pathComponent;
	Bool										m_upThePath;
	Bool										m_startFromBeginning;
	CName										m_boatTag;
	THandle< CBoatComponent >					m_boatComponent;
	Bool										m_pathReached;

public:
    typedef CBehTreeNodeAtomicSailorMoveToPathDefinition Definition;

    CBehTreeNodeAtomicSailorMoveToPathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr );
	void OnDestruction() override;

	Bool OnListenedEvent( CBehTreeEvent& e )override;
	Bool Activate() override;
    void Update() override;
private:
	Bool UpdateTarget();
};


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorFollowPathDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeAtomicSailorFollowPathInstance;
class CBehTreeNodeAtomicSailorFollowPathDefinition : public CBehTreeNodeAtomicActionDefinition
{
    DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicSailorFollowPathDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicSailorFollowPathInstance, SailorFollowPath );
protected:
	CBehTreeValCName							m_pathTag;
	CBehTreeValBool								m_upThePath;
	CBehTreeValBool								m_startFromBeginning;
	CBehTreeValCName							m_boatTag;
public:
    CBehTreeNodeAtomicSailorFollowPathDefinition() 
        : m_pathTag( )
		, m_upThePath( true )
		, m_startFromBeginning( false )	
		, m_boatTag( )	{}

    IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicSailorFollowPathDefinition );
    PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
	PROPERTY_EDIT( m_boatTag, TXT("Tag of the boat to use component") );
	PROPERTY_EDIT( m_pathTag, TXT("Tag of path component") );
	PROPERTY_EDIT( m_upThePath, TXT("Unmark this to make boat move from path end to beginning") )
	PROPERTY_EDIT( m_startFromBeginning, TXT("Mark if movemenet should begin always from beginning of path") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicSailorFollowPathInstance
class CBehTreeNodeAtomicSailorFollowPathInstance : public CBehTreeNodeAtomicActionInstance
{
    typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	CName										m_pathTag;
	THandle< CPathComponent >					m_pathComponent;
	Bool										m_upThePath;
	Bool										m_startFromBeginning;
	CName										m_boatTag;
	THandle< CBoatComponent >					m_boatComponent;
	Bool										m_pathReached;

public:
    typedef CBehTreeNodeAtomicSailorFollowPathDefinition Definition;

    CBehTreeNodeAtomicSailorFollowPathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr );
	void OnDestruction() override;

	Bool OnListenedEvent( CBehTreeEvent& e )override;
	Bool Activate() override;
    void Update() override;
};
