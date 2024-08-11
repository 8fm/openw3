#pragma once

#include "../../common/engine/meshVertex.h"
#include "../../common/engine/bitmapTexture.h"


struct vec_t { float x,y,z,pad; };
struct aabb_t { 
	vec_t	min;
	vec_t	max;
};

struct ray_t {
	vec_t	pos;
	vec_t	inv_dir;
};
struct ray_segment_t {
	float	t_near,t_far;
};

// turn those verbose intrinsics into something readable.
#define loadps(mem)		_mm_load_ps((const float * const)(mem))
#define storess(ss,mem)		_mm_store_ss((float * const)(mem),(ss))
#define minss			_mm_min_ss
#define maxss			_mm_max_ss
#define minps			_mm_min_ps
#define maxps			_mm_max_ps
#define mulps			_mm_mul_ps
#define subps			_mm_sub_ps
#define rotatelps(ps)		_mm_shuffle_ps((ps),(ps), 0x39)	// a,b,c,d -> b,c,d,a
#define muxhps(low,high)	_mm_movehl_ps((low),(high))	// low{a,b,c,d}|high{e,f,g,h} = {c,d,g,h}


static const float flt_plus_inf = -logf(0);	// let's keep C and C++ compilers happy.
static const float _MM_ALIGN16
	ps_cst_plus_inf[4]	= {  flt_plus_inf,  flt_plus_inf,  flt_plus_inf,  flt_plus_inf },
	ps_cst_minus_inf[4]	= { -flt_plus_inf, -flt_plus_inf, -flt_plus_inf, -flt_plus_inf };


static __forceinline bool ray_box_intersect(const aabb_t &box, const ray_t &ray, ray_segment_t &rs) {
	// you may already have those values hanging around somewhere
	const __m128
		plus_inf	= loadps(ps_cst_plus_inf),
		minus_inf	= loadps(ps_cst_minus_inf);

	// use whatever's apropriate to load.
	const __m128
		box_min	= loadps(&box.min),
		box_max	= loadps(&box.max),
		pos	= loadps(&ray.pos),
		inv_dir	= loadps(&ray.inv_dir);

	// use a div if inverted directions aren't available
	const __m128 l1 = mulps(subps(box_min, pos), inv_dir);
	const __m128 l2 = mulps(subps(box_max, pos), inv_dir);

	// the order we use for those min/max is vital to filter out
	// NaNs that happens when an inv_dir is +/- inf and
	// (box_min - pos) is 0. inf * 0 = NaN
	const __m128 filtered_l1a = minps(l1, plus_inf);
	const __m128 filtered_l2a = minps(l2, plus_inf);

	const __m128 filtered_l1b = maxps(l1, minus_inf);
	const __m128 filtered_l2b = maxps(l2, minus_inf);

	// now that we're back on our feet, test those slabs.
	__m128 lmax = maxps(filtered_l1a, filtered_l2a);
	__m128 lmin = minps(filtered_l1b, filtered_l2b);

	// unfold back. try to hide the latency of the shufps & co.
	const __m128 lmax0 = rotatelps(lmax);
	const __m128 lmin0 = rotatelps(lmin);
	lmax = minss(lmax, lmax0);
	lmin = maxss(lmin, lmin0);

	const __m128 lmax1 = muxhps(lmax,lmax);
	const __m128 lmin1 = muxhps(lmin,lmin);
	lmax = minss(lmax, lmax1);
	lmin = maxss(lmin, lmin1);

	const bool ret = (_mm_comige_ss(lmax, _mm_setzero_ps()) & _mm_comige_ss(lmax,lmin)) == 1;

	storess(lmin, &rs.t_near);
	storess(lmax, &rs.t_far);

	return  ret;
}


class RRay {
public:
	Vector origin;
	Vector direction;
	Vector inv_direction;
	int sign[3];

	RRay(const Vector &o, const Vector &d) {
		origin = o;
		direction = d;
		inv_direction = Vector(1/d.X, 1/d.Y, 1/d.Z);

		sign[0] = (inv_direction.X < 0);
		sign[1] = (inv_direction.Y < 0);
		sign[2] = (inv_direction.Z < 0);
	}
};


class BBox {
public:

	Vector bounds[2];

	BBox(const Vector &min, const Vector &max) {
		bounds[0] = min;
		bounds[1] = max;
	}

	Bool BBox::Intersect(const RRay &r, Vector & pointMax, Float& dist ) const {

		Float tmin, tmax, tymin, tymax, tzmin, tzmax;
		tmin = (bounds[r.sign[0]].X - r.origin.X) * r.inv_direction.X;
		tmax = (bounds[1-r.sign[0]].X - r.origin.X) * r.inv_direction.X;
		tymin = (bounds[r.sign[1]].Y - r.origin.Y) * r.inv_direction.Y;
		tymax = (bounds[1-r.sign[1]].Y - r.origin.Y) * r.inv_direction.Y;

		if ( (tmin > tymax) || (tymin > tmax) )
			return false;
		if (tymin > tmin)
			tmin = tymin;
		if (tymax < tmax)
			tmax = tymax;

		tzmin = (bounds[r.sign[2]].Z - r.origin.Z) * r.inv_direction.Z;
		tzmax = (bounds[1-r.sign[2]].Z - r.origin.Z) * r.inv_direction.Z;
		if ( (tmin > tzmax) || (tzmin > tmax) )
			return false;

		if (tzmin > tmin)
			tmin = tzmin;

		if (tzmax < tmax)
			tmax = tzmax;

		Vector dMul = r.direction;
		dMul.Mul3( tmax );

		pointMax = r.origin + dMul;
		dist = tmax;

		return true;
	}
};


struct CVoxelNode
{
	// XYZ
	CVoxelNode* childNodes[8];
	Vector normal;
	Uint32 color;
	volatile Uint32 full;
	//volatile Uint32 normalType;

	CVoxelNode()
	{
		full = 0;
		color = 0;

		for ( Uint32 i = 0; i < 8; ++i )
		{
			childNodes[i] = NULL;
		}
	}

	~CVoxelNode()
	{
		for ( Uint32 i = 0; i < 8; ++i )
		{
			if ( childNodes[i] )
			{
				delete childNodes[i];
			}
		}
	}
};

class CVoxelOctree
{
public:

	CVoxelNode* m_root;
	Uint32		m_levels;
	Float		m_minX;
	Float		m_minY;
	Float		m_minZ;

	Float		m_maxX;
	Float		m_maxY;
	Float		m_maxZ;
	
	Float		m_minNodeSizeX;
	Float		m_minNodeSizeY;
	Float		m_minNodeSizeZ;

	Float		m_epsMag;

	Uint32 m_levelsStats[20];
	
	CVoxelOctree( Uint32 nodeDensity, const Vector& min, const Vector& max )
		: m_minX( min.X )
		, m_minY( min.Y )
		, m_minZ( min.Z )
		, m_maxX( max.X )
		, m_maxY( max.Y )
		, m_maxZ( max.Z )
	{
		m_minNodeSizeX = (m_maxX - m_minX) /(Float)nodeDensity;
		m_minNodeSizeY = (m_maxY - m_minY) /(Float)nodeDensity;
		m_minNodeSizeZ = (m_maxZ - m_minZ) /(Float)nodeDensity;

		m_minX -= m_minNodeSizeX * 3.0f;
		m_minY -= m_minNodeSizeY * 3.0f;
		m_minZ -= m_minNodeSizeZ * 3.0f;

		m_maxX += m_minNodeSizeX * 3.0f;
		m_maxY += m_minNodeSizeY * 3.0f;
		m_maxZ += m_minNodeSizeZ * 3.0f;

		m_minNodeSizeX = (m_maxX - m_minX) /(Float)nodeDensity;
		m_minNodeSizeY = (m_maxY - m_minY) /(Float)nodeDensity;
		m_minNodeSizeZ = (m_maxZ - m_minZ) /(Float)nodeDensity;

		m_epsMag = Vector(m_minNodeSizeX,m_minNodeSizeY,m_minNodeSizeZ).Mag3();


		m_levels = MLog2( nodeDensity );
		m_root = new CVoxelNode();

		for ( Uint32 i = 0; i < 20; ++i )
		{
			m_levelsStats[i] = 0;
		}
	}

	~CVoxelOctree()
	{
		delete m_root;
	}

	Bool TestPoint( const Vector& pos, Box& lowestNode, Uint32& color, Vector& normal ) const
	{
		if ( pos.X < m_minX || pos.Y < m_minY || pos.Z < m_minZ || 
			pos.X > m_maxX || pos.Y > m_maxY || pos.Z > m_maxZ )
		{
			return false; 
		}


		Vector currentMin = Vector( m_minX, m_minY, m_minZ );
		Vector currentMax = Vector( m_maxX, m_maxY, m_maxZ );

		Vector currentXL = currentMax - currentMin;
		
		CVoxelNode* currentNode = m_root;
		Uint32 currentLevel = 0;

		// 
		while ( true )
		{
			if ( !currentNode )
			{
				lowestNode = Box( currentMin, currentMin + currentXL );
				return false;
			}

			if ( currentLevel == m_levels || currentNode->full )
			{
				color = currentNode->color;
				normal = currentNode->normal;

				return true;
			}

			currentXL *= 0.5f;

			Vector currentMinInc = currentMin + currentXL;

			Bool bitOne = ( pos.X > currentMinInc.X);
			Bool bitTwo = ( pos.Y > currentMinInc.Y);
			Bool bitThree = ( pos.Z > currentMinInc.Z);

			currentNode = currentNode->childNodes[ bitOne * 4 + bitTwo * 2 + bitThree ];

			++currentLevel;
			currentMin.X += currentXL.X * bitOne;
			currentMin.Y += currentXL.Y * bitTwo;
			currentMin.Z += currentXL.Z * bitThree;
		}

	}

	Bool RayCast( const Vector& origin, const Vector& direction, Float minDist, Float maxDist, Vector& finalPos, Uint32& color, Vector& normal )
	{
		Vector deltaVector = direction;
		deltaVector *= m_epsMag;

		Vector rayPosition = origin;

		//RRay r( origin, direction );
		ray_t _MM_ALIGN16 ray = { origin.X, origin.Y, origin.Z, 0, 1.0f/direction.X, 1.0f/direction.Y, 1.0f/direction.Z, 0 };

		Float travelledDist = 0.0f;

		while ( travelledDist < maxDist )
		{
			if ( rayPosition.X < m_minX || rayPosition.Y < m_minY || rayPosition.Z < m_minZ || 
				rayPosition.X > m_maxX || rayPosition.Y > m_maxY || rayPosition.Z > m_maxZ )
			{
				finalPos = rayPosition;
				return false;
			}

			rayPosition += deltaVector;


			Box box;
			box.Min = Vector::ZEROS;
			box.Max = Vector::ZEROS;

			Uint32 col = 0;

			Vector norm;

			if ( TestPoint( rayPosition, box, col, norm ) && travelledDist > minDist )
			{
				color = col;
				normal = norm;
				finalPos = rayPosition;

				return true;
			}
			else 
			{
				Vector pointMax = Vector::ZEROS;
				Float dist = 0.0f;

				aabb_t _MM_ALIGN16 aabb = {box.Min.X,box.Min.Y,box.Min.Z,0,box.Max.X,box.Max.Y,box.Max.Z,0};
				ray_segment_t _MM_ALIGN16 ray_segment;

				//BBox b( box.Min, box.Max );

				if ( ray_box_intersect( aabb, ray, ray_segment ) )
				{
					travelledDist = ray_segment.t_far;
					rayPosition = origin + direction * travelledDist;
				}
			}
		}

		return false;
	}

	void InsertPoint( Float posX, Float posY, Float posZ, const Vector& normal, Uint32 color )
	{
		if ( posX < m_minX || posY < m_minY || posZ < m_minZ || 
			posX > m_maxX || posY > m_maxY || posZ > m_maxZ )
		{
			return; 
		}

		Float currentMinX = m_minX;
		Float currentMinY = m_minY;
		Float currentMinZ = m_minZ;

		Float currentMaxX = m_maxX;
		Float currentMaxY = m_maxY;
		Float currentMaxZ = m_maxZ;

		Float currentXL = currentMaxX - currentMinX;
		Float currentYL = currentMaxY - currentMinY;
		Float currentZL = currentMaxZ - currentMinZ;

		CVoxelNode* currentNode = m_root;
		CVoxelNode* parrent = NULL;
		Uint32 currentLevel = 0;

		// 
		while ( true )
		{
			if ( currentLevel == m_levels || currentNode->full )
			{
				if ( currentLevel == m_levels )
				{
					currentNode->full = true;
					currentNode->normal = normal;
					currentNode->color = color;
// 					if ( ::InterlockedCompareExchange( &(currentNode->normalType), positive ? 0 : 1, 3 ) != 3 )
// 					{
// 						if ( currentNode->normalType != (positive ? 0 : 1) )
// 						{
// 							currentNode->normalType = 2;
// 						}
// 					}
				}

				if ( parrent )
				{
					Bool full = true; //currentNode->normalType == (positive ? 0 : 1);

					//Uint32 normalType = positive ? 0 : 1;

					for ( Uint32 i = 0; i < 8; ++i )
					{
						full &= parrent->childNodes[i] && parrent->childNodes[i]->full;
						
						if ( !full )
						{
							break;
						}

// 						if ( normalType != parrent->childNodes[i]->normalType )
// 						{
// 							normalType = 2;
// 
// 							// wait until watertight floodfill
// 							full = false;
// 						}
					}

					if ( full )
					{
						if ( ::InterlockedCompareExchange( &(parrent->full), 1, 0 ) == 0 )
						{
							Vector normalAveraged = Vector::ZEROS;
							
							Uint8 bAveraged = 0;
							Uint8 rAveraged = 0;
							Uint8 gAveraged = 0;

							for ( Uint32 i = 0; i < 8; ++i )
							{
								normalAveraged += parrent->childNodes[i]->normal;
								bAveraged += (parrent->childNodes[i]->color & 0x000000FF) >> 3;
								rAveraged += (parrent->childNodes[i]->color & 0x00FF0000)>> 19;
								gAveraged += (parrent->childNodes[i]->color & 0x0000FF00 ) >> 11;

								delete parrent->childNodes[i];
								parrent->childNodes[i] = NULL;
							}

							parrent->normal = normalAveraged.Normalized3();
							parrent->color = bAveraged + (gAveraged << 8) + (rAveraged << 16);
						}
					}
				}

				return;
			}

			currentXL /= 2.0f;
			currentYL /= 2.0f;
			currentZL /= 2.0f;

			Bool bitOne = ( posX > currentMinX+currentXL);
			Bool bitTwo = ( posY > currentMinY+currentYL);
			Bool bitThree = ( posZ > currentMinZ+currentZL);

			Uint32 index = bitOne * 4 + bitTwo * 2 + bitThree;

			if ( !currentNode->childNodes[ index ] )
			{
				CVoxelNode* node = new CVoxelNode();
				if ( ::InterlockedCompareExchangePointer( reinterpret_cast<PVOID*>(&(currentNode->childNodes[ index ])), node, NULL ) != NULL )
				{
					delete node;
				}
			}

			parrent = currentNode;
			currentNode = currentNode->childNodes[ index ];
			++currentLevel;
			currentMinX += currentXL * bitOne;
			currentMinY += currentYL * bitTwo;
			currentMinZ += currentZL * bitThree;
		}

	}

	void TrimNode( CVoxelNode*& node, Uint32 level )
	{
		if ( !node || level == m_levels )
		{
			return;
		}

		for ( Uint32 i = 0; i < 8; ++i )
		{
			TrimNode( node->childNodes[i], level + 1 );
		}

		Bool full = true;
		
		for ( Uint32 i = 0; i < 8; ++i )
		{
			full &= node->childNodes[i] && node->childNodes[i]->full;
			if ( !full )
				break;
		}

		if ( full )
		{
			CVoxelNode* newNode = new CVoxelNode();
			newNode->full = true;
			node = newNode;
		}
	}

	void PrintTreeStats( CVoxelNode* node, Uint32 level )
	{
		if ( node )
		{
			m_levelsStats[level]++;
			
			for ( Uint32 i = 0; i < 8; ++i )
			{
				PrintTreeStats( node->childNodes[i], level + 1 );
			}
		}
	}

	void TrimTree()
	{
		TrimNode( m_root, 0 );
	}

	void ListVectors( CVoxelNode* node, TDynArray<Vector>& origins, TDynArray<Vector>& normal, TDynArray<Uint32>& color, Uint32 level, Float x1, Float y1, Float z1, Float x2, Float y2, Float z2 )
	{
		if ( level == 0 || node->full )
		{
			origins.PushBack( Vector(x1+x2,y1+y2,z1+z2) * 0.5f );
			normal.PushBack( node->normal );
			color.PushBack( node->color );

			return;
		}

		Float halfX = (x2-x1)/2.0f;
		Float halfY = (y2-y1)/2.0f;
		Float halfZ = (z2-z1)/2.0f;
		
		for ( Uint32 i = 0; i < 8; ++i )
		{
			if ( node->childNodes[i] )
			{

				Bool bitOne = (i & 4)!=0;
				Bool bitTwo = (i & 2)!=0;
				Bool bitThree = (i & 1)!=0;

				Float x11 = x1 + halfX * bitOne;
				Float y11 = y1 + halfY * bitTwo;
				Float z11 = z1 + halfZ * bitThree;

				Float x22 = x2 - halfX * !bitOne;
				Float y22 = y2 - halfY * !bitTwo;
				Float z22 = z2 - halfZ * !bitThree;

				ListVectors( node->childNodes[i], origins, normal, color, level - 1, x11, y11, z11, x22, y22, z22 );
			}
		}
	}


	void InsertTriangleColor( const SMeshVertex& v1, const SMeshVertex& v2, const SMeshVertex& v3, Uint32 color )
	{
		// Barycentric shit
		Vector p1 = Vector(v1.m_position[0],v1.m_position[1],v1.m_position[2]);
		Vector p2 = Vector(v2.m_position[0],v2.m_position[1],v2.m_position[2]);
		Vector p3 = Vector(v3.m_position[0],v3.m_position[1],v3.m_position[2]);

		Vector triangleNormal = Vector::Cross(p3-p1,p2-p1);
		triangleNormal.Normalize3();
		Float maxDiffX= Abs(p2.X-p1.X);
		Float maxDiffY= Abs(p2.Y-p1.Y);
		Float maxDiffZ= Abs(p2.Z-p1.Z);

		Float maxStepsX = Max<Float>( maxDiffX / m_minNodeSizeX, 1.0f );
		Float maxStepsY = Max<Float>( maxDiffY / m_minNodeSizeY, 1.0f );
		Float maxStepsZ = Max<Float>( maxDiffZ / m_minNodeSizeZ, 1.0f );

		Float maxStep1 = Vector( maxStepsX, maxStepsY, maxStepsZ ).Mag3();

		Float step1 = 1.0f / Max( maxStep1 * 10.f, 3.0f );

		Float c1 = 0.0f;
		while ( c1 < 1.0f )
		{
			Vector v1 = Vector::Interpolate( p1, p2, c1 );

			Float maxDiffX1= Abs(v1.X-p3.X);
			Float maxDiffY1= Abs(v1.Y-p3.Y);
			Float maxDiffZ1= Abs(v1.Z-p3.Z);

			Float maxStepsX1 = maxDiffX1 / m_minNodeSizeX;
			Float maxStepsY1 = maxDiffY1 / m_minNodeSizeY;
			Float maxStepsZ1 = maxDiffZ1 / m_minNodeSizeZ;

			Float maxStep2 = Vector( maxStepsX1, maxStepsY1, maxStepsZ1 ).Mag3();

			Float step2 = 1.0f / Max( maxStep2 * 10.f, 3.0f );

			Float c2 = 0.0f;
			while ( c2 < 1.0f )
			{
				Vector v2 = Vector::Interpolate( v1, p3, c2 );

				InsertPoint( v2.X, v2.Y, v2.Z, triangleNormal, color );
				c2 += step2;
			}

			c1 += step1;
		}
	}

	void InsertTriangleColor( const SMeshVertex& v1, const SMeshVertex& v2, const SMeshVertex& v3, CBitmapTexture::MipMap& textureMip )
	{
		// Barycentric shit
		Vector p1 = Vector(v1.m_position[0],v1.m_position[1],v1.m_position[2]);
		Vector p2 = Vector(v2.m_position[0],v2.m_position[1],v2.m_position[2]);
		Vector p3 = Vector(v3.m_position[0],v3.m_position[1],v3.m_position[2]);

		Vector uv1 = Vector(v1.m_uv0[0],v1.m_uv0[1],0);
		Vector uv2 = Vector(v2.m_uv0[0],v2.m_uv0[1],0);
		Vector uv3 = Vector(v3.m_uv0[0],v3.m_uv0[1],0);

		Vector triangleNormal = Vector::Cross(p3-p1,p2-p1);
		triangleNormal.Normalize3();
		Float maxDiffX= Abs(p2.X-p1.X);
		Float maxDiffY= Abs(p2.Y-p1.Y);
		Float maxDiffZ= Abs(p2.Z-p1.Z);

		Float maxStepsX = Max<Float>( maxDiffX / m_minNodeSizeX, 1.0f );
		Float maxStepsY = Max<Float>( maxDiffY / m_minNodeSizeY, 1.0f );
		Float maxStepsZ = Max<Float>( maxDiffZ / m_minNodeSizeZ, 1.0f );

		Float maxStep1 = Vector( maxStepsX, maxStepsY, maxStepsZ ).Mag3();

		Float step1 = 1.0f / Max( maxStep1 * 10.f, 3.0f );

		Float c1 = 0.0f;
		while ( c1 < 1.0f )
		{
			Vector v1 = Vector::Interpolate( p1, p2, c1 );
			
			Vector uv11 = Vector::Interpolate( uv1, uv2, c1 );

			Float maxDiffX1= Abs(v1.X-p3.X);
			Float maxDiffY1= Abs(v1.Y-p3.Y);
			Float maxDiffZ1= Abs(v1.Z-p3.Z);

			Float maxStepsX1 = maxDiffX1 / m_minNodeSizeX;
			Float maxStepsY1 = maxDiffY1 / m_minNodeSizeY;
			Float maxStepsZ1 = maxDiffZ1 / m_minNodeSizeZ;

			Float maxStep2 = Vector( maxStepsX1, maxStepsY1, maxStepsZ1 ).Mag3();

			Float step2 = 1.0f / Max( maxStep2 * 10.f, 3.0f );

			Float c2 = 0.0f;
			while ( c2 < 1.0f )
			{
				Vector v2 = Vector::Interpolate( v1, p3, c2 );

				Vector uv22 = Vector::Interpolate( uv11, uv3, c2 );

				Uint32 posX = (Uint32)((uv22.X-floorf(uv22.X)) * textureMip.m_width);
				Uint32 posY = (Uint32)((uv22.Y-floorf(uv22.Y)) * textureMip.m_height);

				InsertPoint( v2.X, v2.Y, v2.Z, triangleNormal, ((Uint32*)textureMip.m_data.GetData())[ posX + posY*textureMip.m_height ] );
				c2 += step2;
			}

			c1 += step1;
		}
	}

};
