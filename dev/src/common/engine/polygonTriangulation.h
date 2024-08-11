#include <list>
using namespace std;

typedef Double TPartitionFloat;

struct CPartitionVertex
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, MC_PartitionEngine );

	TPartitionFloat x;
	TPartitionFloat y;	
	Int32 index;

	CPartitionVertex()
		: x(0)
		, y(0)
		, index(-1)
	{}

	CPartitionVertex operator+(const CPartitionVertex& p) const
	{
		CPartitionVertex r;
		r.x = x + p.x;
		r.y = y + p.y;
		r.index = -1;
		return r;
	}

	CPartitionVertex operator-(const CPartitionVertex& p) const
	{
		CPartitionVertex r;
		r.x = x - p.x;
		r.y = y - p.y;
		r.index = -1;
		return r;
	}

	CPartitionVertex operator*(const TPartitionFloat f ) const
	{
		CPartitionVertex r;
		r.x = x*f;
		r.y = y*f;
		r.index = -1;
		return r;
	}

	CPartitionVertex operator/(const TPartitionFloat f ) const
	{
		CPartitionVertex r;
		r.x = x/f;
		r.y = y/f;
		r.index = -1;
		return r;
	}

	Bool operator==(const CPartitionVertex& p) const
	{
		if ((x == p.x)&&(y==p.y))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	Bool operator!=(const CPartitionVertex& p) const
	{
		if ((x == p.x)&&(y==p.y))
		{
			return false;
		}
		else
		{
			return true;
		}
	}
};

class CPartitionPoly
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_PartitionEngine );
protected:
	CPartitionVertex *points;
	Int32 numpoints;
	Bool hole;

public:
	CPartitionPoly();
	~CPartitionPoly();

	CPartitionPoly(const CPartitionPoly &src);
	CPartitionPoly& operator=(const CPartitionPoly &src);

	Int32 GetNumPoints() const
	{
		return numpoints;
	}

	Bool IsHole() const
	{
		return hole;
	}

	void SetHole(Bool hole)
	{
		this->hole = hole;
	}

	CPartitionVertex &GetPoint(Int32 i)
	{
		return points[i];
	}

	const CPartitionVertex &GetPoint(Int32 i) const
	{
		return points[i];
	}

	CPartitionVertex *GetPoints()
	{
		return points;
	}

	const CPartitionVertex *GetPoints() const
	{
		return points;
	}

	CPartitionVertex& operator[](Int32 i)
	{
		return points[i];	
	}

	const CPartitionVertex& operator[](Int32 i) const
	{
		return points[i];	
	}

	void Clear();

	void Init(Int32 numpoints);

	void Triangle(CPartitionVertex &p1, CPartitionVertex &p2, CPartitionVertex &p3);

	void Invert();

	Int32 GetOrientation();

	void SetOrientation(Int32 orientation);
};

class CPartitionEngine
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_PartitionEngine );
protected:
	struct PartitionVertex
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, MC_PartitionEngine );
		Bool isActive;
		Bool isConvex;
		Bool isEar;

		CPartitionVertex p;
		TPartitionFloat angle;
		PartitionVertex *previous;
		PartitionVertex *next;
	};

	struct Diagonal
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, MC_PartitionEngine );
		Int32 index1;
		Int32 index2;
	};

	struct DPState
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, MC_PartitionEngine );
		Bool visible;
		TPartitionFloat weight;
		Int32 bestvertex;
	};

	struct DPState2
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, MC_PartitionEngine );
		Bool visible;
		Int32 weight;
		list<Diagonal> pairs;
	};

	Bool IsConvex(CPartitionVertex& p1, CPartitionVertex& p2, CPartitionVertex& p3);
	Bool IsReflex(CPartitionVertex& p1, CPartitionVertex& p2, CPartitionVertex& p3);
	Bool IsInside(CPartitionVertex& p1, CPartitionVertex& p2, CPartitionVertex& p3, CPartitionVertex &p);
	
	Bool InCone(CPartitionVertex &p1, CPartitionVertex &p2, CPartitionVertex &p3, CPartitionVertex &p);
	Bool InCone(PartitionVertex *v, CPartitionVertex &p);

	Int32 Intersects(CPartitionVertex &p11, CPartitionVertex &p12, CPartitionVertex &p21, CPartitionVertex &p22);

	CPartitionVertex Normalize(const CPartitionVertex &p);
	TPartitionFloat Distance(const CPartitionVertex &p1, const CPartitionVertex &p2);

	void UpdateVertexReflexity(PartitionVertex *v);
	void UpdateVertex(PartitionVertex *v,PartitionVertex *vertices, Int32 numvertices);

	void UpdateState(Int32 a, Int32 b, Int32 w, Int32 i, Int32 j, DPState2 **dpstates);
	void TypeA(Int32 i, Int32 j, Int32 k, PartitionVertex *vertices, DPState2 **dpstates);
	void TypeB(Int32 i, Int32 j, Int32 k, PartitionVertex *vertices, DPState2 **dpstates);

public:
	Int32 Triangulate(CPartitionPoly *poly, list<CPartitionPoly> *triangles);

	Int32 ConvexPartition_Optimal(CPartitionPoly *poly, list<CPartitionPoly> *parts);
	Int32 ConvexPartition_Forced(CPartitionPoly *poly, list<CPartitionPoly> *parts);

};