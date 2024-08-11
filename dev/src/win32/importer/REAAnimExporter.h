/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/skeletalAnimation.h"

class CREAAnimExporter : public ISkeletalAnimationExporter
{
	DECLARE_RTTI_SIMPLE_CLASS( CREAAnimExporter );

public:
	CREAAnimExporter();
	virtual Bool DoExport( CSkeletalAnimation* animation, const AnimExporterParams& options );

private:
	void ExportSingleCurve( const String& file, Int32 curveIndex, TDynArray<Float> & data );
};

BEGIN_CLASS_RTTI( CREAAnimExporter )
	PARENT_CLASS( ISkeletalAnimationExporter )
END_CLASS_RTTI()

