/**********************************************************************

Filename    :   GTX_ImageFile.h
Content     :   
Created     :   Jun 2011
Authors     :   Bart Muzzin

Copyright   :   Copyright 2011 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

***********************************************************************/

#ifndef INC_SF_GTX_IMAGEFILE_H
#define INC_SF_GTX_IMAGEFILE_H
#pragma once

#include "Render_ImageFile.h"

namespace Scaleform { namespace Render { namespace GTX {

//--------------------------------------------------------------------
// ***** Image File Handlers
//
// DDS::FileHandler provides file format detection for DDS and its data loading.
class FileReader : public ImageFileReader_Mixin<FileReader>
{
public:
    virtual ImageFileFormat GetFormat() const { return ImageFile_GTX; }

    virtual bool    MatchFormat(File* file, UByte* header, UPInt headerSize) const;
    virtual ImageSource* ReadImageSource(File* file,
                            const ImageCreateArgs& args = ImageCreateArgs()) const;

    // Instance singleton; to be used for accessing this functionality.
    static FileReader Instance;
};

}}}; // namespace Scaleform::Render::DDS


#endif // INC_SF_GTX_IMAGEFILE_H