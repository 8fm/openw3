#ifndef GRB_ACTOR_3
#define GRB_ACTOR_3

#include "PxActor.h"

namespace physx
{

//-----------------------------------------------------------------------------
#define ACTOR_API_UNDEF( x )	PX_ASSERT( 0 && "PxActor method not implemented in GRB: "##x )
//-----------------------------------------------------------------------------
class GrbActor3
{
public:
	void									release()						{ ACTOR_API_UNDEF("release"); }

	virtual		PxActorType::Enum			getType()						{ ACTOR_API_UNDEF("getType"); return (PxActorType::Enum)0; }
	PxAggregate*							getAggregate() const			{ ACTOR_API_UNDEF("getAggregate"); return 0;}

	virtual									~GrbActor3()					{}
};

};

//-----------------------------------------------------------------------------
#endif
