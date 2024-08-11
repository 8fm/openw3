//##protect##"disclaimer"
/**********************************************************************

Filename    :   .h
Content     :   
Created     :   Apr, 2013
Authors     :   Sergey Sikorskiy

Copyright   :   Copyright 2013 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**********************************************************************/
//##protect##"disclaimer"

#ifndef INC_AS3_Obj_Display_IGraphicsPath_H
#define INC_AS3_Obj_Display_IGraphicsPath_H

#include "../AS3_Obj_Object.h"
//##protect##"includes"
//##protect##"includes"


namespace Scaleform { namespace GFx { namespace AS3 
{
// Forward declaration.
namespace fl_display
{
    extern const TypeInfo IGraphicsPathTI;
    extern const ClassInfo IGraphicsPathCI;
} // namespace fl_display

namespace ClassTraits { namespace fl_display
{
    class IGraphicsPath;
}}

namespace InstanceTraits { namespace fl_display
{
    class IGraphicsPath;
}}

namespace Classes { namespace fl_display
{
    class IGraphicsPath;
}}

//##protect##"forward_declaration"
//##protect##"forward_declaration"
    
namespace ClassTraits { namespace fl_display
{
    class IGraphicsPath : public fl::Object
    {
#ifdef GFX_AS3_VERBOSE
    private:
        virtual const char* GetAS3ObjectType() const { return "ClassTraits::IGraphicsPath"; }
#endif
    public:
        typedef Class ClassType;
        typedef InstanceTraits::Interface InstanceTraitsType;
        typedef InstanceTraitsType::InstanceType InstanceType;

    public:
        IGraphicsPath(VM& vm, const ClassInfo& ci);
        static Pickable<Traits> MakeClassTraits(VM& vm);
//##protect##"ClassTraits$methods"
//##protect##"ClassTraits$methods"

    };
}}
//##protect##"methods"
//##protect##"methods"

}}} // namespace Scaleform { namespace GFx { namespace AS3

#endif

