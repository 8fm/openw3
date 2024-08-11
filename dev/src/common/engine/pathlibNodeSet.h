#pragma once

#include "pathlibNavNode.h"


namespace PathLib
{

class CComponentRuntimeProcessingContext;
class CNavModyfication;
class INodeSetPack;
class CNodeSetProcessingContext;


class CNavgraphNodeSet : public CNavNodesGraphBase
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_SmallObjects, MC_PathLib, __alignof( CNavgraphNodeSet ) );

	typedef CNavNodesGraphBase Super;
public:
	typedef CPathNode::NodesetIdx			Id;
	static const Id INVALID_ID = CPathNode::INVALID_INDEX;
protected:

	CNavGraph*								m_navgraph;
	INodeSetPack*							m_owner;
	Box										m_bbox;
	Id										m_id;
	Bool									m_isAttached;
	
	void					Unlink();
	void					PreNodeArrayOverflow() override;
	void					OnNodeArrayOverflow() override;

	Bool					ConnectNodeSet();
public:
	CNavgraphNodeSet( CNavGraph* navgraph = NULL, Id id = INVALID_ID )
		: Super()
		, m_navgraph( navgraph )
		, m_owner( nullptr )
		, m_bbox( Box::RESET_STATE )
		, m_id( id )
		, m_isAttached( false )												{}
	~CNavgraphNodeSet();

	CNavNode::NodesetIdx	GetId() const									{ return m_id; }
	Bool					IsAttached() const								{ return m_isAttached; }
	Bool					IsInitialized() const							{ return !m_nodes.Empty(); }
	const Box&				GetBBox() const									{ return m_bbox; }

	INodeSetPack*			GetOwner() const								{ return m_owner; }
	void					SetOwner( INodeSetPack* nodeSetPack )			{ m_owner = nodeSetPack; }

	//void					OnRegister( CNavGraph* navgraph, Id idx );
	//void					OnUnregister( CNavGraph* navgraph );
	void					Clear();

	// modify nodes prior to node availability processing
	void					PreAttach( CNavGraph* navgraph, CNodeSetProcessingContext& context );			
	void					PreDetach( CNavGraph* navgraph, CNodeSetProcessingContext& context );
	// physicallly attach/detach nodeset
	void					Attach( CNavGraph* navgraph, CNodeSetProcessingContext& context );
	void					Detach( CNavGraph* navgraph, CNodeSetProcessingContext& context );

	using Super::GetNode;
	CNavNode*				GetNode( CPathNode::Id id );

	CPathNode*				VGetPathNode( CPathNode::Id id ) override;
	Bool					VAreNodesLinkedById() const override;
	AreaId					VGetAreaId() const override;
	CPathLibWorld*			VGetPathLibWorld() const override;
	CNavNode::NodesetIdx	VGetNodesetIndex() const override;
	LinkBufferIndex			VGetExtraLinksSpace() const override;

	// creation
	void					ConnectNodes( CNavNode& nodeSetNode, CNavNode& destinationNode, NodeFlags linkFlags, NavLinkCost linkCost );
	void					WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool					ReadFromBuffer( CSimpleBufferReader& reader );

	Bool					TryConnecting( CNavgraphNodeSet* nodeSet );

	CNavNode&				AddNode( const Vector3& position,NodeFlags flags = NF_DEFAULT );
};

class INodeSetPack
{
protected:
	CNavgraphNodeSet*		m_nodeSets[ MAX_ACTOR_CATEGORIES ];
	CNavgraphNodeSet::Id	m_nodeSetsIndexes[ MAX_ACTOR_CATEGORIES ];

	// TODO: RequestDetach
public:
	INodeSetPack();
	virtual ~INodeSetPack();

	// basic initialization stuff
	CNavgraphNodeSet*		RegisterNodeSet( CNavGraph* navgraph );
	CNavgraphNodeSet*		GetNodeSet( CNavGraph* navgraph ) const;
	void					ClearNodeSet( CNavGraph* navgraph );

	void					Attach( CAreaDescription* area, CComponentRuntimeProcessingContext& context );
	void					Detach( CAreaDescription* area, CComponentRuntimeProcessingContext& context );

	void					WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool					ReadFromBuffer( CSimpleBufferReader& reader );

	void					OnPostLoad( CAreaDescription* area );
	void					Clear( CAreaDescription* area );

	virtual Bool			IsUpdatingNodeCollisionOnAttach();
	virtual void			GenerationOnPreNodeSetConnection( CNavGraph* navgraph );
	virtual void			GenerationOnPostNodeSetConnection( CNavGraph* navgraph );

	void					NodeSetDeleted( Uint32 category );

	virtual CNavModyfication*	AsNavModyfication();
};

};		// namespace PathLib

