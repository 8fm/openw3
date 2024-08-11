#pragma once

#include "boidNodeData.h"

class CAtomicBoidNode;
class CLoopBoidNode;
class CAnimBoidNode;
class CBaseBoidNode;

CBaseBoidNode *const		BaseAnimParseXML( const SCustomNode & baseAnimNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies );

class CBaseBoidNode : public Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_Boids );
public:
	CBaseBoidNode( Uint32 type );
	virtual ~CBaseBoidNode();

	virtual const CAtomicBoidNode *const	GetNextAtomicAnimNode()const;

	virtual Bool					ParseXMLAttribute( const SCustomNodeAttribute & animAtt, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies );
	virtual Bool					ParseXML( const SCustomNode & loopBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies );

	const Float &					GetProba()const{return m_proba;}
	
protected:
	Uint32						m_type;		// RTTI type used for casting
	const CBaseBoidNode *		m_next;
	Float						m_proba;

public:
	enum EType
	{
		TYPE_BASE_BOID_NODE			= FLAG( 0 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_BASE_BOID_NODE, 
	};

	template < class TClass >
	const TClass* As()const
	{
		if ( (m_type & TClass::E_TYPE) == TClass::E_TYPE )
		{
			return static_cast<const TClass*>(this);
		}
		return NULL;
	}

	template < class TClass >
	TClass* As()
	{
		if ( (m_type & TClass::E_TYPE) == TClass::E_TYPE )
		{
			return static_cast<TClass*>(this);
		}
		return NULL;
	}
};






