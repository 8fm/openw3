/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_RESOURCE_IMPORT

//--------------------------------------------------------

namespace MergedWorldGeometryBuilder
{
	/// Build grid data
	extern const Bool Build( CLayer* mergedLayer, CDirectory* contentDir, const Uint32 gridSize, const TDynArray< IMergedWorldGeometryData* >& mergers, const IMergedWorldGeometrySupplier* worldDataSupplier, const Vector& worldCenter, const Float worldRadius );
};

//--------------------------------------------------------

#endif