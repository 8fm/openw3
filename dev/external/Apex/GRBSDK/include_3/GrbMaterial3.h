#ifndef GRBMATERIAL_H
#define GRBMATERIAL_H

#include "PxAssert.h"
#include "PxVec3.h"
#include "PxMaterial.h"

namespace physx
{

//-----------------------------------------------------------------------------
#define MATERIAL_API_UNDEF( x )	PX_ASSERT( 0 && "PxMaterial method not implemented in GRB: "##x )
//-----------------------------------------------------------------------------
class GrbMaterial3 : public PxMaterial //, public Ps::UserAllocated, public Cm::RefCountable
{
public:

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR >= 3)
	GrbMaterial3() : PxMaterial(PxConcreteType::eMATERIAL, PxBaseFlag::eIS_RELEASABLE | PxBaseFlag::eOWNS_MEMORY) {}
#endif

// PX_SERIALIZATION
	virtual	PxU32					getObjectSize()	const	{ return sizeof(*this);		}
//~PX_SERIALIZATION

	virtual		void				release() = 0;
	virtual		PxU32				getReferenceCount() const {MATERIAL_API_UNDEF("getReferenceCount"); return 0; }

	virtual		void				setDynamicFriction(PxReal) = 0;
	virtual		PxReal				getDynamicFriction() const = 0;
	virtual		void				setStaticFriction(PxReal) = 0;
	virtual		PxReal				getStaticFriction() const = 0;
	virtual		void				setRestitution(PxReal) = 0;
	virtual		PxReal				getRestitution() const = 0;
	virtual		void				setFlag(PxMaterialFlag::Enum flag, bool value) = 0;
	virtual		void				setFlags(PxMaterialFlags inFlags) = 0;
	virtual		PxMaterialFlags		getFlags() const = 0;
	virtual		void				setFrictionCombineMode(PxCombineMode::Enum) = 0;
	virtual		PxCombineMode::Enum	getFrictionCombineMode() const = 0;
	virtual		void				setRestitutionCombineMode(PxCombineMode::Enum) = 0;
	virtual		PxCombineMode::Enum	getRestitutionCombineMode() const = 0;
};
//-----------------------------------------------------------------------------

};

#endif