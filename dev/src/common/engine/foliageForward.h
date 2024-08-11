/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef _ENGINE_FOLIAGE_FORWARD_H_
#define _ENGINE_FOLIAGE_FORWARD_H_

#include "../core/sharedPtr.h"

class IFoliageResourceLoader;
class CFoliageResourceLoader;
class CFoliageResourceHandler;

class CFoliageCell;
class CFoliageGrid;
class CFoliageCellIterator;

class CSoftHandleProxy;

typedef Red::TSharedPtr< CFoliageCell > CellHandle;
typedef TDynArray< CellHandle > CellHandleContainer;

typedef Red::TSharedPtr< CSoftHandleProxy > FoliageResourceHandle;

#endif
