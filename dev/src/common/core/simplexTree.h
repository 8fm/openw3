
#pragma once

#include "classBuilder.h"

struct SSimplexTreeStruct
{
	DECLARE_RTTI_STRUCT( SSimplexTreeStruct );
	
	Int32 m_parent;
	Int32 m_positiveStruct;
	Int32 m_negativeStruct;
	Int32 m_positiveID;
	Int32 m_negativeID;
	Float m_normalX;
	Float m_normalY;
	Float m_offset;
	Int32 m_reserved;

	SSimplexTreeStruct();
	void Set( Float px, Float py, Float dx, Float dy );
	Float Gradient( Float x, Float y ) const { return m_normalX*x + m_normalY*y + m_offset; }
};

BEGIN_CLASS_RTTI( SSimplexTreeStruct );
	PROPERTY_RO( m_parent,			TXT("Parent Index") );
	PROPERTY_RO( m_positiveStruct,	TXT("Positive Node Index") );
	PROPERTY_RO( m_negativeStruct,	TXT("Negative Node Index") );
	PROPERTY_RO( m_positiveID,		TXT("Positive Node ID") );
	PROPERTY_RO( m_negativeID,		TXT("Negative Node ID") );
	PROPERTY_RO( m_normalX,			TXT("Normal X") );
	PROPERTY_RO( m_normalY,			TXT("Normal Y") );
	PROPERTY_RO( m_offset,			TXT("Offset") );
	//PROPERTY_RO( m_reserved,		TXT("Reserved") );
END_CLASS_RTTI();


class CSimplexTreeNode
{
public:
	CSimplexTreeNode();
	virtual ~CSimplexTreeNode();
	virtual Int32 CreatePositiveStruct( Int32 par, Int32 id, Float px, Float py, Float dx, Float dy );
	virtual Int32 CreateNegativeStruct( Int32 par, Int32 id, Float px, Float py, Float dx, Float dy );
	virtual void RemoveNode( Int32 ind );
	virtual void RemoveNodes666( );
	virtual Int32 FindNodeAtPoint( Float x, Float y );
	virtual Int32 FindIDAtPoint( Float x, Float y );
	TDynArray<SSimplexTreeStruct> & GetNodes() { return m_nodes; }
protected:
	TDynArray<SSimplexTreeStruct> m_nodes;
};