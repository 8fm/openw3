
#pragma once

//64b: changed from Int32
typedef uintptr_t kdTreePtr;

// Splitting indices
enum EKdTreeSplit
{
	TST_LO = 0, 
	TST_HI = 1,
	TST_Size = 2
};

// Defines
#define KD_POINT_VAL(i,d)		(pa[idxbase[idx+(i)]][(d)])

#define KD_TREE_POW(v)			((v)*(v))
#define KD_TREE_ROOT(x)			sqrt(x)
#define KD_TREE_SUM(x,y)		((x) + (y))
#define KD_TREE_DIFF(x,y)		((y) - (x))
