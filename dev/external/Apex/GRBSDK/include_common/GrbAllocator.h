#ifndef GRB_ALLOCATOR_H
#define GRB_ALLOCATOR_H

class GrbAllocator
{
public:

	virtual void * malloc(size_t size) = 0;
	virtual void free(void * mem) = 0;
};

#endif