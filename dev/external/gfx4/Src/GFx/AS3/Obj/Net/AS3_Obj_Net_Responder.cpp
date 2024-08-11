//##protect##"disclaimer"
/**************************************************************************

Filename    :   AS3_Obj_Net_Responder.cpp
Content     :   
Created     :   Jan, 2010
Authors     :   Sergey Sikorskiy

Copyright   :   Copyright 2011 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**************************************************************************/
//##protect##"disclaimer"

#include "AS3_Obj_Net_Responder.h"
#include "../../AS3_VM.h"
#include "../../AS3_Marshalling.h"
//##protect##"includes"
//##protect##"includes"


namespace Scaleform { namespace GFx { namespace AS3 
{

//##protect##"methods"
//##protect##"methods"


namespace ClassTraits { namespace fl_net
{

    Responder::Responder(VM& vm, const ClassInfo& ci)
    : fl::Object(vm, ci)
    {
//##protect##"ClassTraits::Responder::Responder()"
//##protect##"ClassTraits::Responder::Responder()"

    }

    Pickable<Traits> Responder::MakeClassTraits(VM& vm)
    {
        MemoryHeap* mh = vm.GetMemoryHeap();
        Pickable<Traits> ctr(SF_HEAP_NEW_ID(mh, StatMV_VM_CTraits_Mem) Responder(vm, AS3::fl_net::ResponderCI));

        Pickable<InstanceTraits::Traits> itr(SF_HEAP_NEW_ID(mh, StatMV_VM_ITraits_Mem) InstanceTraitsType(vm, AS3::fl_net::ResponderCI));
        ctr->SetInstanceTraits(itr);

        // There is no problem with Pickable not assigned to anything here. Class constructor takes care of this.
        Pickable<Class> cl(SF_HEAP_NEW_ID(mh, StatMV_VM_Class_Mem) ClassType(*ctr));

        return ctr;
    }
//##protect##"ClassTraits$methods"
//##protect##"ClassTraits$methods"

}} // namespace ClassTraits

namespace fl_net
{
    const TypeInfo ResponderTI = {
        TypeInfo::CompileTime | TypeInfo::NotImplemented,
        sizeof(ClassTraits::fl_net::Responder::InstanceType),
        0,
        0,
        0,
        0,
        "Responder", "flash.net", &fl::ObjectTI,
        TypeInfo::None
    };

    const ClassInfo ResponderCI = {
        &ResponderTI,
        ClassTraits::fl_net::Responder::MakeClassTraits,
        NULL,
        NULL,
        NULL,
        NULL,
    };
}; // namespace fl_net


}}} // namespace Scaleform { namespace GFx { namespace AS3

