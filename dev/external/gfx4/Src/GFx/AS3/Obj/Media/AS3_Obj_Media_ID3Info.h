//##protect##"disclaimer"
/**************************************************************************

Filename    :   AS3_Obj_Media_ID3Info.h
Content     :   
Created     :   Jan, 2010
Authors     :   Sergey Sikorskiy

Copyright   :   Copyright 2011 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**************************************************************************/
//##protect##"disclaimer"

#ifndef INC_AS3_Obj_Media_ID3Info_H
#define INC_AS3_Obj_Media_ID3Info_H

#include "../AS3_Obj_Object.h"
//##protect##"includes"
//##protect##"includes"


namespace Scaleform { namespace GFx { namespace AS3 
{
// Forward declaration.
namespace fl_media
{
    extern const TypeInfo ID3InfoTI;
    extern const ClassInfo ID3InfoCI;
} // namespace fl_media

namespace ClassTraits { namespace fl_media
{
    class ID3Info;
}}

namespace InstanceTraits { namespace fl_media
{
    class ID3Info;
}}

namespace Classes { namespace fl_media
{
    class ID3Info;
}}

//##protect##"forward_declaration"
//##protect##"forward_declaration"

namespace Instances { namespace fl_media
{
    class ID3Info : public Instances::fl::Object
    {
#ifndef SF_OS_WII
        template <class TR> friend Pickable<typename TR::InstanceType> InstanceTraits::MakeInstance(TR& tr);
#endif
        friend class InstanceTraits::fl_media::ID3Info;
        
    public:
        typedef ID3Info SelfType;
        typedef Classes::fl_media::ID3Info ClassType;
        typedef InstanceTraits::fl_media::ID3Info TraitsType;
        static const TypeInfo& GetTypeInfo() { return AS3::fl_media::ID3InfoTI; }

#ifdef GFX_AS3_VERBOSE
    private:
        virtual const char* GetAS3ObjectType() const { return "Instances::fl_media::ID3Info"; }
#endif
#ifdef SF_OS_WII
    public:
#else
    protected:
#endif
        ID3Info(InstanceTraits::Traits& t);

//##protect##"instance$methods"
//##protect##"instance$methods"

    public:
        // AS3 API members.
        ASString album;
        ASString artist;
        ASString comment;
        ASString genre;
        ASString songName;
        ASString track;
        ASString year;

//##protect##"instance$data"
//##protect##"instance$data"

    };
}} // namespace Instances

namespace InstanceTraits { namespace fl_media
{
    class ID3Info : public fl::Object
    {
#ifdef GFX_AS3_VERBOSE
    private:
        virtual const char* GetAS3ObjectType() const { return "InstanceTraits::ID3Info"; }
#endif
    public:
        typedef Instances::fl_media::ID3Info InstanceType;

    public:
        ID3Info(VM& vm, const ClassInfo& ci);

    public: 
        static Pickable<Instances::fl_media::ID3Info> MakeInstance(ID3Info& t)
        {
            return Pickable<Instances::fl_media::ID3Info>(new(t.Alloc()) Instances::fl_media::ID3Info(t));
        }
        SPtr<Instances::fl_media::ID3Info> MakeInstanceS(ID3Info& t)
        {
            return MakeInstance(t);
        }

        virtual void MakeObject(Value& result, Traits& t);

        enum { MemberInfoNum = 7 };
        static const MemberInfo mi[MemberInfoNum];
//##protect##"instance_traits$methods"
//##protect##"instance_traits$methods"

//##protect##"instance_traits$data"
//##protect##"instance_traits$data"

    };
    
}}
    
namespace ClassTraits { namespace fl_media
{
    class ID3Info : public fl::Object
    {
#ifdef GFX_AS3_VERBOSE
    private:
        virtual const char* GetAS3ObjectType() const { return "ClassTraits::ID3Info"; }
#endif
    public:
        typedef Class ClassType;
        typedef InstanceTraits::fl_media::ID3Info InstanceTraitsType;
        typedef InstanceTraitsType::InstanceType InstanceType;

    public:
        ID3Info(VM& vm, const ClassInfo& ci);
        static Pickable<Traits> MakeClassTraits(VM& vm);
//##protect##"ClassTraits$methods"
//##protect##"ClassTraits$methods"

    };
}}
//##protect##"methods"
//##protect##"methods"

}}} // namespace Scaleform { namespace GFx { namespace AS3

#endif

