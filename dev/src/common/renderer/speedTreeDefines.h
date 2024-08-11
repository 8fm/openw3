/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#define STATIC_FOREST_CELL_SIZE					128.0
#define DYNAMIC_FOREST_CELL_SIZE				256.0
#define GRASS_CELL_SIZE_MIN						8.0f
#define GRASS_CELL_SIZE_MAX						32.0f
#define GRASS_MAX_VISIBLE_CELL_RESERVATION		100		// How many cells to allocate for each layer in initial culling phase. We can make this quite big as the cell memory footprint is small
#define TREE_MAX_VISIBLE_CELL_RESERVATION		200		// How many cells to allocate for visible trees
#define TREE_MAX_VISIBLE_INSTANCE_RESERVATION	0		// How many tree instances to reserve for each cell
#define MAX_GRASS_INSTANCES_ON_CELL_EDGE		40
#define GRASS_MAX_INSTANCE_COUNT				MAX_GRASS_INSTANCES_ON_CELL_EDGE * MAX_GRASS_INSTANCES_ON_CELL_EDGE	// Max. instances in a single grass cell. Mainly used during autopopulation to limit cell budgets
#define GENERATED_GRASS_EXTENTS_MULTIPLIER		2		// How much we extend the auto-populated grass extents by (multiplied by cell size)
