// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

        
#ifndef PXS_CONTACTMANAGER_H
#define PXS_CONTACTMANAGER_H

#include "PxvConfig.h"
#include "PxvManager.h"
#include "PxcSolverConstraintDesc.h"
#include "PxcFrictionPatch.h"
#include "PxcNpWorkUnit.h"
#include "PxcNpContactPrep.h"
#include "PxcSolverContactPF.h"
#include "PxcSolverContact.h"
#include "PxcSolverConstraintTypes.h"

namespace physx
{

class PxsThreadContext;
class PxsContext;
class PxsRigidBody;
struct PxsCCDBody;
class PxsMaterialManager;
struct PxsCCDShape;

enum PxsPairVisColor 
{

	eVIS_COLOR_SWEPTINTEGRATE_OFF = 0x000000,
	eVIS_COLOR_SWEPTINTEGRATE_SLOW = 0x404040,
	eVIS_COLOR_SWEPTINTEGRATE_CLEAR = 0x007f00,
	eVIS_COLOR_SWEPTINTEGRATE_IMPACT = 0x1680ff,
	eVIS_COLOR_SWEPTINTEGRATE_FAIL = 0x0000ff,

};


class PxsContactManager
{
public:
											PxsContactManager(PxsContext* context, PxU32 index);
											~PxsContactManager();

						void				init(const PxvManagerDescRigidRigid& desc, const PxsMaterialManager* materialManager);

	//PX_FORCE_INLINE		void				setDynamicFriction(PxReal v)								{ mNpUnit.dynamicFriction = v;				}
	//PX_FORCE_INLINE		PxReal				getDynamicFriction()								const	{ return mNpUnit.dynamicFriction;			}

	//PX_FORCE_INLINE		void				setStaticFriction(PxReal v)									{ mNpUnit.staticFriction = v;				}
	//PX_FORCE_INLINE		PxReal				getStaticFriction()									const	{ return mNpUnit.staticFriction;			}

	//PX_FORCE_INLINE		void				setRestitution(PxReal v)									{ mNpUnit.restitution = v;					}
	//PX_FORCE_INLINE		PxReal				getRestitution()									const	{ return mNpUnit.restitution;				}

	PX_FORCE_INLINE		void				setDisableStrongFriction(PxU32 d)							{ (!d)	? mNpUnit.flags &= ~PxcNpWorkUnitFlag::eDISABLE_STRONG_FRICTION 
																												: mNpUnit.flags |= PxcNpWorkUnitFlag::eDISABLE_STRONG_FRICTION; }

	PX_FORCE_INLINE		PxReal				getRestDistance()									const	{ return mNpUnit.restDistance;				}
	PX_FORCE_INLINE		void				setRestDistance(PxReal v)									{ mNpUnit.restDistance = v;					}

	//PX_FORCE_INLINE		PxReal				getCorrelationDistance()							const	{ return mNpUnit.correlationDistance;		}
	//PX_FORCE_INLINE		void				setCorrelationDistance(PxReal v)							{ mNpUnit.correlationDistance = v;			}

						void				destroy();

	PX_FORCE_INLINE		PxReal				getDominance0()										const	{ return mNpUnit.dominance0;				}
	PX_FORCE_INLINE		void				setDominance0(PxReal v)										{ mNpUnit.dominance0 = v;					}

	PX_FORCE_INLINE		PxReal				getDominance1()										const	{ return mNpUnit.dominance1;				}
	PX_FORCE_INLINE		void				setDominance1(PxReal v)										{ mNpUnit.dominance1 = v;					}

	PX_FORCE_INLINE		bool				getTouchStatus()									const	{ return mNpUnit.touch;						}
	PX_FORCE_INLINE		PxU32				getIndex()											const	{ return mNpUnit.index;						}
	
	PX_FORCE_INLINE		bool				getHasSolverConstraint()							const	{ return (mNpUnit.hasSolverConstraints && (mNpUnit.contactCount != 0));	}

	// flags stuff - needs to be refactored

	PX_FORCE_INLINE		Ps::IntBool			isChangeable()										const	{ return mFlags & PXS_CM_CHANGEABLE;		}
	PX_FORCE_INLINE		Ps::IntBool			getCCD()								const	{ return mFlags & PXS_CM_CCD_LINEAR; }
	PX_FORCE_INLINE		Ps::IntBool			getHadCCDContact()									const	{ return mFlags & PXS_CM_CCD_CONTACT; }
	PX_FORCE_INLINE		void				setHadCCDContact()											{ mFlags |= PXS_CM_CCD_CONTACT; }
						void				setCCD(bool enable);

	PX_FORCE_INLINE		PxcNpWorkUnit&		getWorkUnit()												{ return mNpUnit; }
	PX_FORCE_INLINE		const PxcNpWorkUnit&		getWorkUnit()	const								{ return mNpUnit; }

						PxU32				getConstraintDesc(PxcSolverConstraintDesc& desc)	const;
						PxU32				getFrictionConstraintDesc(PxcSolverConstraintDesc& desc)	const;

	PX_FORCE_INLINE		void*				getUserData()										const	{ return mUserData;			}
	PX_FORCE_INLINE		void				setUserData(void* data)										{ mUserData = data;			}

	// Setup solver-constraints
						void				resetCachedState();
	PX_FORCE_INLINE		PxU32				getContactCount()	{ return mNpUnit.contactCount; }

	PX_FORCE_INLINE		PxModifiableContact*		
											getContactsForModification() 
	{
		return (PxModifiableContact*)(mNpUnit.compressedContacts + sizeof(PxModifyContactHeader) + sizeof(PxContactPatchBase)); 
	}
						PxU32				getContactPointData(const void*& compressedContactData, PxU32& compressedContactSize, const PxReal*& forces)
											{
												//points = mNpUnit.contactPoints;

												PX_ASSERT((mNpUnit.compressedContacts != NULL && mNpUnit.compressedContactSize > 0) || 
													(mNpUnit.compressedContacts == NULL && mNpUnit.compressedContactSize == 0));

												compressedContactData = mNpUnit.compressedContacts;
												compressedContactSize = mNpUnit.compressedContactSize;
												forces = mNpUnit.contactForces;
												return mNpUnit.contactCount;
											}

						PxsRigidBody*							getRigidBody0() const { return mRigidBody0;}
						PxsRigidBody*							getRigidBody1() const { return mRigidBody1;}

private:
						//KS - moving this up - we want to get at flags
						PxsRigidBody*		mRigidBody0;					//100		//144
						PxsRigidBody*		mRigidBody1;					//104		//152	

						PxU32				mFlags;							//108		//156
						void*				mUserData;						//112		//164

						friend class PxsContext;
	// everything required for narrow phase to run
	PX_ALIGN_PREFIX(16)	PxcNpWorkUnit		mNpUnit PX_ALIGN_SUFFIX(16);	


						

	enum
	{
		PXS_CM_CHANGEABLE				= (1<<0),
		PXS_CM_CCD_LINEAR = (1<<1),
		PXS_CM_CCD_CONTACT = (1 << 2)
	};

	friend class PxsDynamicsContext;
	friend struct PxsCCDPair;
	friend class PxsIslandManager;
	friend class PxsCCDContext;
};

PX_INLINE PxU32 PxsContactManager::getConstraintDesc(PxcSolverConstraintDesc& desc)	const
{
	setConstraintLength(desc, mNpUnit.solverConstraintSize);			
	desc.constraint	= mNpUnit.solverConstraintPointer;
	desc.writeBack	= mNpUnit.contactForces;
	setWritebackLength(desc, mNpUnit.contactForces ? mNpUnit.contactCount * sizeof(PxReal) : 0);	

	return mNpUnit.axisConstraintCount;
}


PX_INLINE PxU32 PxsContactManager::getFrictionConstraintDesc(PxcSolverConstraintDesc& desc)	const
{
	PxcSolverContactCoulombHeader* header = (PxcSolverContactCoulombHeader*)mNpUnit.solverConstraintPointer;
	if(header)
	{
		PxU32 frictionOffset = header->frictionOffset;
		PxU8* PX_RESTRICT constraint =  (PxU8*)header + frictionOffset;
		const PxU32 length = (mNpUnit.solverConstraintSize - frictionOffset);
		setConstraintLength(desc, length);
		desc.constraint	= constraint;

		//desc.writeBack	= mNpUnit.contactForces;
		setWritebackLength(desc, mNpUnit.contactForces ? mNpUnit.contactCount * sizeof(PxReal) : 0);

		PX_ASSERT(*constraint < PXS_SC_CONSTRAINT_TYPE_COUNT);
		return header->type != PXS_SC_TYPE_NOFRICTION_RB_CONTACT ? mNpUnit.axisConstraintCount : 0;
	}
	else
	{
		desc.constraint = NULL;
		setConstraintLength(desc, 0);
		setWritebackLength(desc, 0);
		return 0;
	}
}

}

#endif
