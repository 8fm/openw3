#pragma once


#include "pathlibLinksBuffer.h"
#include "pathlibNode.h"


namespace PathLib
{
////////////////////////////////////////////////////////////////////////////
// CPathLibGraph
// Base pathlib graph chunk structure. Notice that all node accessing
// functions are non-virtual to enable search engine to use templated
// fast-access to graph iteration and structure.
// At the same time loading/unloading management is abstract interface.
////////////////////////////////////////////////////////////////////////////
class CPathLibGraph : public Red::System::NonCopyable
{
protected:
	CLinksBuffer				m_links;
	Uint16						m_initialVersion;
	Uint16						m_version;											// used to compare graphs and area versions - to check if graphs are up-to-date

	virtual void				PreNodeArrayOverflow();
	virtual void				OnNodeArrayOverflow();

	static void					SetNodeIndex( CPathNode& node, CPathNode::Index idx ) { node.m_id.m_index = idx; }

	void						MarkNodeForDeletion( CPathNode& node );
	LinkBufferIndex				AddLink( CPathNode& node, const CPathLink& link );

public:
	CPathLibGraph();
	virtual ~CPathLibGraph();

	virtual CPathLibWorld*		VGetPathLibWorld() const;
	virtual AreaId				VGetAreaId() const;
	virtual CPathNode*			VGetExternalPathNode( CPathNode::Index idx, AreaId areaId );
	virtual CPathNode*			VGetPathNode( CPathNode::Id id ) = 0;
	virtual LinkBufferIndex		VGetExtraLinksSpace() const;

	virtual void				Unload() = 0;

	Uint32						GetVersion() const							{ return m_version; }
	Bool						IsInitialVersion() const					{ return m_version == m_initialVersion; }
	void						ResetVersion()								{ m_initialVersion = m_version; }
	void						SetVersion( Uint32 version )				{ m_version = Uint16(version); }
	void						MarkVersionDirty()							{ ++m_version; }
	const CLinksBuffer&			GetLinksBuffer() const						{ return m_links; }
	CLinksBuffer&				GetLinksBuffer()							{ return m_links; }
};


};		// namespace PathLib