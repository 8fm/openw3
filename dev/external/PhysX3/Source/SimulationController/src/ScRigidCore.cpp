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


#include "ScBodyCore.h"
#include "ScStaticCore.h"
#include "ScRigidSim.h"
#include "ScShapeSim.h"
#include "ScScene.h"


using namespace physx;

Sc::RigidCore::RigidCore(const PxActorType::Enum type) 
: ActorCore(type, PxActorFlag::eVISUALIZATION, PX_DEFAULT_CLIENT, 0, 0)
{
}


Sc::RigidCore::~RigidCore()
{
}

void Sc::RigidCore::addShapeToScene(ShapeCore& shapeCore)
{
	Sc::RigidSim* sim = getSim();
	PX_ASSERT(sim);
	if(!sim)
		return;
	sim->getScene().addShape(*sim, shapeCore);
}

void Sc::RigidCore::removeShapeFromScene(ShapeCore& shapeCore, bool wakeOnLostTouch)
{
	Sc::RigidSim* sim = getSim();
	if(!sim)
		return;
	Sc::ShapeSim& s = sim->getSimForShape(shapeCore);
	sim->getScene().removeShape(s, wakeOnLostTouch);
}

void Sc::RigidCore::onShapeChange(Sc::ShapeCore& shape, ShapeChangeNotifyFlags notifyFlags, PxShapeFlags oldShapeFlags)
{
	// DS: We pass flags to avoid searching multiple times or exposing RigidSim outside SC, and this form is
	// more convenient for the Scb::Shape::syncState method. If we start hitting this a lot we should do it
	// a different way, but shape modification after insertion is rare. 

	Sc::RigidSim* sim = getSim();
	if(!sim)
		return;
	Sc::ShapeSim& s = sim->getSimForShape(shape);

	if(notifyFlags & ShapeChangeNotifyFlag::eGEOMETRY)
		s.onGeometryChange();
	if(notifyFlags & ShapeChangeNotifyFlag::eMATERIAL)
		s.onMaterialChange();
	if(notifyFlags & ShapeChangeNotifyFlag::eRESET_FILTERING)
		s.onResetFiltering();
	if(notifyFlags & ShapeChangeNotifyFlag::eSHAPE2BODY)
		s.onTransformChange();
	if(notifyFlags & ShapeChangeNotifyFlag::eFILTERDATA)
		s.onFilterDataChange();
	if(notifyFlags & ShapeChangeNotifyFlag::eFLAGS)
		s.onFlagChange(oldShapeFlags);
	if(notifyFlags & ShapeChangeNotifyFlag::eRESTOFFSET)
		s.onRestOffsetChange();
}




Sc::RigidSim* Sc::RigidCore::getSim() const
{
	return static_cast<RigidSim*>(Sc::ActorCore::getSim());
}




// The alternative to this switch is to have a virtual interface just for this (which would nullify
// the space advantage of getting rid of back pointers) or exposing the Np implementation to Sc.
PxActor* NpGetPxActorSC(Sc::StaticCore&);
PxActor* NpGetPxActorBC(Sc::BodyCore&);
PxActor* NpGetPxActorForArticulationLink(Sc::BodyCore&);


PxActor* Sc::RigidCore::getPxActor() const
{
	Sc::RigidCore* r = const_cast<Sc::RigidCore*>(this);
	switch(getActorCoreType())
	{
	case PxActorType::eRIGID_STATIC:
		return NpGetPxActorSC(*static_cast<Sc::StaticCore*>(r));
	case PxActorType::eRIGID_DYNAMIC:
		return NpGetPxActorBC(*static_cast<Sc::BodyCore*>(r));
	case PxActorType::eARTICULATION_LINK:
		return NpGetPxActorForArticulationLink(*static_cast<Sc::BodyCore*>(r));
	default:
		PX_ASSERT(0);
		return 0;
	}
}
