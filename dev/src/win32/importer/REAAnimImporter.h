/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/skeletalAnimation.h"

class CREAAnimImporter : public ISkeletalAnimationImporter
{
	DECLARE_RTTI_SIMPLE_CLASS( CREAAnimImporter );

public:
	CREAAnimImporter();
	virtual CSkeletalAnimation*		DoImport( const AnimImporterParams& options );
	virtual Bool					PrepareForImport( const String& filePath, AnimImporterParams& options );

	static Bool ImportSingleCurve( const String& file, Int32 curveIndex, TDynArray<Float> & data );
};

BEGIN_CLASS_RTTI( CREAAnimImporter )
	PARENT_CLASS( ISkeletalAnimationImporter )
END_CLASS_RTTI()
