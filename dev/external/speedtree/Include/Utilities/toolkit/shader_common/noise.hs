/* Author: Morten S. Mikkelsen
 * Freely available for any type of use.
 */

#ifndef __NOISE_H__
#define __NOISE_H__

DataBuffer<int> uPermData : register( t0 );
DataBuffer<float3> grads_array : register( t1 );


/*********************************************************************************************
************************************* Utility functions **************************************
*********************************************************************************************/


float3 fade(float3 t)
{
	return t*t*t*(t*(t*6-float3(15, 15, 15))+float3(10, 10, 10));	 // new curve
 // return t * t * (3 - 2 * t); // old curve
}

float3 dfade(float3 t)
{	
	return 30*t*t*(t*(t-float3(2, 2, 2))+float3(1, 1, 1));  // new curve
  //return 6*t*(1-t); // old curve
}


int perm(int x)
{
	return uPermData[x&255];
}

float3 get_grad(int x)
{
	return float3(grads_array[x&15].xyz);
}

float grad(int x, float3 p)
{
	float3 vGrad = get_grad(x);
  	return dot(vGrad, p);
}


// derivatives based on a bicubic formulation
// The samples h_ij are defined such that j is column and i is row. Thus j corresponds
// to the x direction and i corresponds to the y direction.
			
//							 [-0.5	1.5		-1.5	0.5	][h00	 h01	 h02	 h03][-0.5     1   -0.5		0]   [x3]
//							 [1		-2.5	2		-0.5][h10	 h11	 h12	 h13][1.5 	-2.5	 0		1]   [x2]
// B(x,y) = [y3  y2  y  1] * [-0.5	0		0.5		0	][h20	 h21	 h22	 h23][-1.5	   2	0.5		0] * [ x]
//							 [0		1		0		0	][h30	 h31	 h32	 h33][0.5 	-0.5	 0		0]   [ 1]
//
//																						   [h00		 h01	 h02	 h03]	[-0.5*(x3+x) + x2		]
//																						   [h10		 h11	 h12	 h13]	[1.5*x3 - 2.5*x2 + 1	]
//		  = [-0.5*(y3+y)+y2		1.5*y3-2.5*y2+1		-1.5*y3+2*y2+0.5*y		0.5*(y3-y2)] * [h20		 h21	 h22	 h23] * [-1.5*x3 + 2*x2 + 0.5*x ]
//																						   [h30		 h31	 h32	 h33]	[0.5*(x3-x2)			]
float2 CalcBicubicDerivatives(Texture2D tex_in, SamplerState sample_in, float2 texST)
{
	uint2 vDim;
	tex_in.GetDimensions(vDim.x, vDim.y);

	float2 fTexLoc = vDim*texST-float2(0.5,0.5);
	int2 iTexLoc = (int2) floor(fTexLoc);
	float2 t = saturate(fTexLoc - iTexLoc);		// sat just to be pedantic

	const float4 vSamplesUL = tex_in.Gather( sample_in, (iTexLoc+int2(-1,-1) + float2(0.5,0.5))/vDim );
	const float4 vSamplesUR = tex_in.Gather( sample_in, (iTexLoc+int2(1,-1) + float2(0.5,0.5))/vDim );
	const float4 vSamplesLL = tex_in.Gather( sample_in, (iTexLoc+int2(-1,1) + float2(0.5,0.5))/vDim );
	const float4 vSamplesLR = tex_in.Gather( sample_in, (iTexLoc+int2(1,1) + float2(0.5,0.5))/vDim );
	
	// scanlines in the vertical direction (ie. float4(UL.wz, UR.wz) is a scanline)
	float4x4 H = {  vSamplesUL.w, vSamplesUL.x, vSamplesLL.w, vSamplesLL.x,
					vSamplesUL.z, vSamplesUL.y, vSamplesLL.z, vSamplesLL.y,
					vSamplesUR.w, vSamplesUR.x, vSamplesLR.w, vSamplesLR.x,
					vSamplesUR.z, vSamplesUR.y, vSamplesLR.z, vSamplesLR.y };

	float x = t.x, y = t.y;
	float x2 = x * x, x3 = x2 * x;
	float y2 = y * y, y3 = y2 * y;

	float4 X = float4(-0.5*(x3+x)+x2,		1.5*x3-2.5*x2+1,		-1.5*x3+2*x2+0.5*x,		0.5*(x3-x2));
	float4 Y = float4(-0.5*(y3+y)+y2,		1.5*y3-2.5*y2+1,		-1.5*y3+2*y2+0.5*y,		0.5*(y3-y2));
	float4 dX = float4(-1.5*x2+2*x-0.5,		4.5*x2-5*x,				-4.5*x2+4*x+0.5,		1.5*x2-x);
	float4 dY = float4(-1.5*y2+2*y-0.5,		4.5*y2-5*y,				-4.5*y2+4*y+0.5,		1.5*y2-y);
			
	float2 dHdST = vDim * float2( dot(Y, mul(dX, H)), dot(dY, mul(X, H)) );


	// convert to screen space
	float2 TexDx = ddx(texST);
	float2 TexDy = ddy(texST);

	float dBx = dHdST.x * TexDx.x + dHdST.y * TexDx.y;
	float dBy = dHdST.x * TexDy.x + dHdST.y * TexDy.y;
	
	return float2(dBx, dBy);
}



/*********************************************************************************************
**************************************** API functions ***************************************
*********************************************************************************************/

// this function is used to determine filter-width in functions such as fractalsum
float GetPixelSize(float3 vP)
{
	return sqrt(length( cross(ddx(vP), ddy(vP)) ));
}

float mysnoise(float3 p)
{
	float3 vP = floor(p);
	p -= vP;
	int3 P = (int3) vP;
	P &= int3(255,255,255);
  
	float3 f = fade(p);

	// hash coordinates for 6 of the 8 cube corners
	int A = perm(P.x) + P.y;
	int AA = perm(A) + P.z;
	int AB = perm(A + 1) + P.z;
	int B =  perm(P.x + 1) + P.y;
	int BA = perm(B) + P.z;
	int BB = perm(B + 1) + P.z;


	// use f.xyz to blend between all 8 corners
	return lerp(
				lerp(lerp(grad(perm(AA), p),
							grad(perm(BA), p + float3(-1, 0, 0)), f.x),
						lerp(grad(perm(AB), p + float3(0, -1, 0)),
							grad(perm(BB), p + float3(-1, -1, 0)), f.x), f.y),
				lerp(lerp(grad(perm(AA + 1), p + float3(0, 0, -1)),
							grad(perm(BA + 1), p + float3(-1, 0, -1)), f.x),
						lerp(grad(perm(AB + 1), p + float3(0, -1, -1)),
							grad(perm(BB + 1), p + float3(-1, -1, -1)), f.x), f.y),
				f.z);
}

// this function returns mysnoise() in .x and the gradient of this in .yzw
float4 mydsnoise(float3 p)
{
	float3 vP = floor(p);
	p -= vP;
	int3 P = (int3) vP;
	P &= int3(255,255,255);

	float3 F = fade(p);
	float3 dF = dfade(p);
	

	// hash coordinates for 6 of the 8 cube corners
	int A = perm(P.x) + P.y;
	int AA = perm(A) + P.z;
	int AB = perm(A + 1) + P.z;
	int B =  perm(P.x + 1) + P.y;
	int BA = perm(B) + P.z;
	int BB = perm(B + 1) + P.z;
  

	// use f.xyz to blend between all 8 corners
	float4 vA = float4(get_grad(perm(AA)), 0);
	float4 vB = float4(get_grad(perm(BA)), 0);
	float4 vC = float4(get_grad(perm(AB)), 0);
	float4 vD = float4(get_grad(perm(BB)), 0);
	float4 vE = float4(get_grad(perm(AA+1)), 0);
	float4 vF = float4(get_grad(perm(BA+1)), 0);
	float4 vG = float4(get_grad(perm(AB+1)), 0);
	float4 vH = float4(get_grad(perm(BB+1)), 0);
  
	vA.w = dot(vA.xyz, p);
	vB.w = dot(vB.xyz, p + float3(-1, 0, 0));
	vC.w = dot(vC.xyz, p + float3(0, -1, 0));
	vD.w = dot(vD.xyz, p + float3(-1, -1, 0));
	vE.w = dot(vE.xyz, p + float3(0, 0, -1));
	vF.w = dot(vF.xyz, p + float3(-1, 0, -1));
	vG.w = dot(vG.xyz, p + float3(0, -1, -1));
	vH.w = dot(vH.xyz, p + float3(-1, -1, -1));
	
	const float u = F.x;
	const float v = F.y;
	const float w = F.z;

	/*const float noise = 
  		lerp(	
  				lerp(	lerp(a,b,u), lerp(c,d,u), 	v),
    			lerp(	lerp(e,f,u), lerp(g,h,u),	v),
				w);*/
		
	
	/*const float4 vK0 = vA;
	const float4 vK1 = vB-vA;
	const float4 vK2 = vC-vA;
	const float4 vK3 = vE-vA;
	const float4 vK4 = vA-vB-vC+vD;
	
	const float4 vK5 = vA-vC-vE+vG;
	const float4 vK6 = vA-vB-vE+vF;
	const float4 vK7 = -vA+vB+vC-vD+vE-vF-vG+vH;
	
	
	float4 dNoise = vK0 + u*vK1 + v*vK2 + w*vK3 + u*v*vK4 + v*w*vK5 + u*w*vK6 + u*v*w*vK7;
	
	dNoise.x += dF.x*(vK1.w + vK4.w*v + vK6.w*w + vK7.w*v*w);
	dNoise.y += dF.y*(vK2.w + vK5.w*w + vK4.w*u + vK7.w*w*u);
	dNoise.z += dF.z*(vK3.w + vK6.w*u + vK5.w*v + vK7.w*u*v);*/
	
	
	
	float4 vK0 = vA;
	float4 vK1 = vB-vA;
	float4 vK2 = vC-vA;
	float4 vK3 = vE-vA;
	float4 vK4 = vD-vK1-vC;
	float4 vK5 = vG-vK2-vE;
	float4 vK6 = vF-vK1-vE;
	float4 vK7 = vE-vF-vG+vH-vK4;
	
	vK0.xyz += dF*float3(vK1.w, vK2.w, vK3.w);
	vK1.yz += dF.yz * float2(vK4.w, vK6.w);
	vK2.xz += dF.xz * float2(vK4.w, vK5.w);
	vK3.xy += dF.xy * float2(vK6.w, vK5.w);
	vK4.z += dF.z*vK7.w;
	vK5.x += dF.x*vK7.w;
	vK6.y += dF.y*vK7.w;

	float4 dNoise = vK0 + u*vK1 + v*(vK2 + u*vK4) + w*(vK3 + u*vK6 + v*(vK5 + u*vK7));
	
	
	// (height, gradient)
	return float4(dNoise.w, dNoise.xyz);
}

float mynoise(float3 p)
{
	return 0.5*mysnoise(p)+0.5;
}

// this function returns mynoise() in .x and the gradient of this in .yzw
float4 mydnoise(float3 p)
{
	return 0.5*mydsnoise(p)+float4(0.5,0,0,0);
}

float VerifyPixSize(float fPixSize)
{
	// return fPixSize;
	return max(fPixSize, 1.0/(1<<24));
}

float filteredsnoise(float3 p, float width)
{
	return mysnoise(p) * (1 - smoothstep(0.25,0.75,width));
}

float4 filtereddsnoise(float3 p, float width)
{
	return mydsnoise(p) * (1 - smoothstep(0.25,0.75,width));
}

float filterednoise(float3 p, float width)
{
	return mynoise(p) * (1 - smoothstep(0.25,0.75,width));
}

float4 filtereddnoise(float3 p, float width)
{
	return mydnoise(p) * (1 - smoothstep(0.25,0.75,width));
}

float filteredabs(float x, float dx)
{
	float x0 = x-0.5*dx;
	float x1 = x+0.5*dx;
	return (sign(x1)*x1*x1 - sign(x0)*x0*x0) / (2*dx);
}


// FRACTAL SUM
#define NEW_METHOD

float fractalsum(float3 P, float fPixSize_in=1)
{
	float fracsum = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);
	
#ifdef NEW_METHOD
	float freq = 1;
	float fw = fPixSize;	// should clamp this
	while(fw < 1.0)
	{
		fracsum += (1/freq) * filteredsnoise(P*freq, fw);
		freq*=2; fw *= 2;
	}
#else
   	const float twice = 2*fPixSize;
	float scale = 1;
   	for(scale = 1; scale > twice; scale /= 2) 
    		fracsum += scale * mysnoise(P/scale);

   /* Gradual fade out of highest-frequency component near limit */
   if (scale > fPixSize)
   {
		float weight = (scale / fPixSize) - 1;
		weight = clamp(weight, 0, 1);
		fracsum += weight * scale * mysnoise(P/scale);
   }
#endif

	return fracsum;
	
}

// this function returns fractalsum() in .x and the gradient of this in .yzw
float4 dfractalsum(float3 P, float fPixSize_in=1)
{
	float4 fracsum = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);
	
#ifdef NEW_METHOD
	float freq = 1;
	float fw = fPixSize;	// should clamp this
	while(fw < 1.0)
	{
		fracsum += float4((1/freq),1,1,1) * filtereddsnoise(P*freq, fw);
		freq*=2; fw *= 2;
	}
#else
	const float twice = 2*fPixSize;
	float scale = 1;
   	for(scale = 1; scale > twice; scale /= 2) 
    		fracsum += float4(scale,1,1,1) * mydsnoise(P/scale);

   /* Gradual fade out of highest-frequency component near limit */
   if (scale > fPixSize)
   {
		float weight = (scale / fPixSize) - 1;
		weight = clamp(weight, 0, 1);
		fracsum += weight * float4(scale,1,1,1) * mydsnoise(P/scale);
   }
#endif

	return fracsum;
}

float turbulence(float3 P, float fPixSize_in=1)
{
	float turb = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);
	
#ifdef NEW_METHOD
	float freq = 1;
	float fw = fPixSize;	// should clamp this
	while(fw < 1.0)
	{
		turb += (1/freq) * filteredabs( filteredsnoise(P*freq, fw), 2*fw);
		freq*=2; fw *= 2;
	}
#else
   	const float twice = 2*fPixSize;
	float scale = 1;
   	for(scale = 1; scale > twice; scale /= 2) 
    		turb += scale * abs(mysnoise(P/scale));

   /* Gradual fade out of highest-frequency component near limit */
   if (scale > fPixSize)
   {
		float weight = (scale / fPixSize) - 1;
		weight = clamp(weight, 0, 1);
		turb += weight * scale * abs(mysnoise(P/scale));
   }
#endif

	return turb;
}

// MAX versions. As an optimization only the last (heighest) frequencies are accumulated
// this is sufficient for derivative calculations used for normal perturbation.
float fractalsummax(float3 P, float fPixSize_in=1, int iMaxOctaves=8)
{
   	float fracsum = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);
	
	//1/(2^i) = t
	//fI = -log_2(t)	// ceil is break
	//i = ceil(fI)
	//scale = 1/(2^i) = pow(2,-i)
	//scale = pow(2,-ceil(-log_2(t))
	//	  = pow(2,floor(log_2(t))

	float fK = -floor(log2(fPixSize));
	int iPermOctaves = (int) fK;
	if(iPermOctaves>0)
	{
		float freq = pow(2.0,fK-1);
		const float weight = saturate((1 / (freq*fPixSize)) - 1);
#ifdef NEW_METHOD
		float fw = fPixSize*freq;	// should clamp this
		const int iIterations = min(iPermOctaves, iMaxOctaves-1);
		for(int k=0; k<iIterations; k++)
		{
			fracsum += (1/freq) * filteredsnoise(P*freq, fw);
			freq/=2; fw /= 2;
		}
		if(iPermOctaves>=iMaxOctaves)
		{
			fracsum += ((1-weight) / freq) * filteredsnoise(P*freq, fw);
		}
	
#else
		const int iIterations = min(iPermOctaves-1, iMaxOctaves-2);
		fracsum += (weight / freq) * mysnoise(P*freq);
		for(int k=0; k<iIterations; k++)
		{
			freq /= 2;
			fracsum += mysnoise(P*freq) / freq;
		}
		
		if(iPermOctaves>=iMaxOctaves)
		{
			freq /= 2;
			fracsum += ((1-weight) / freq) * mysnoise(P*freq);
		}
		//assert(scale>=1);
#endif
	}
	return fracsum;
}

// this function returns fractalsummax() in .x and the gradient of this in .yzw
float4 dfractalsummax(float3 P, float fPixSize_in=1, int iMaxOctaves=8)
{
   	float4 fracsum = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);

	float fK = -floor(log2(fPixSize));
	int iPermOctaves = (int) fK;
	if(iPermOctaves>0)
	{
		float freq = pow(2.0,fK-1);
		const float weight = saturate((1 / (freq*fPixSize)) - 1);
#ifdef NEW_METHOD
		float fw = fPixSize*freq;	// should clamp this
		const int iIterations = min(iPermOctaves, iMaxOctaves-1);
		for(int k=0; k<iIterations; k++)
		{
			fracsum += float4(1.0/freq,1,1,1) * filtereddsnoise(P*freq, fw);
			freq/=2; fw /= 2;
		}
		if(iPermOctaves>=iMaxOctaves)
		{
			fracsum += (1-weight) * float4(1.0/freq,1,1,1) * filtereddsnoise(P*freq, fw);
		}
	
#else
		const int iIterations = min(iPermOctaves-1, iMaxOctaves-2);
		fracsum += weight * float4(1.0/freq,1,1,1) * mydsnoise(P*freq);
		for(int k=0; k<iIterations; k++)
		{
			freq /= 2;
			fracsum += float4(1.0/freq,1,1,1) * mydsnoise(P*freq);
		}
		
		if(iPermOctaves>=iMaxOctaves)
		{
			freq /= 2;
			fracsum += (1-weight) * float4(1.0/freq,1,1,1) * mydsnoise(P*freq);
		}
		//assert(scale>=1);
#endif
	}

	return fracsum;
}

// MAX versions. As an optimization only the last (heighest) frequencies are accumulated
// this is sufficient for derivative calculations used for normal perturbation.
float turbulencemax(float3 P, float fPixSize_in=1, int iMaxOctaves=8)
{
   	float turb = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);

	float fK = -floor(log2(fPixSize));
	int iPermOctaves = (int) fK;
	if(iPermOctaves>0)
	{
		float freq = pow(2.0,fK-1);	// scale is an integer power of two
		const float weight = saturate((1 / (freq*fPixSize)) - 1);
#ifdef NEW_METHOD
		float fw = fPixSize*freq;	// should clamp this
		const int iIterations = min(iPermOctaves, iMaxOctaves-1);
		for(int k=0; k<iIterations; k++)
		{
			turb += (1/freq) * filteredabs( filteredsnoise(P*freq, fw), 2*fw);
			freq/=2; fw /= 2;
		}
		if(iPermOctaves>=iMaxOctaves)
		{
			turb += ((1-weight) / freq) * filteredabs( filteredsnoise(P*freq, fw), 2*fw);
		}
#else
		const int iIterations = min(iPermOctaves-1, iMaxOctaves-2);
		turb += (weight / freq) * abs(mysnoise(P*freq));
		for(int k=0; k<iIterations; k++)
		{
			freq /= 2;
			turb += abs(mysnoise(P*freq)) / freq;
		}
		
		if(iPermOctaves>=iMaxOctaves)
		{
			freq /= 2;
			turb += ((1-weight) / freq) * abs(mysnoise(P*freq));
		}
		//assert(scale>=1);
#endif
	}

	return turb;
}


// do bump mapping using either height or derivative map using the "surface gradient based formulation"
// see the paper: "Bump Mapping Unparametrized Surfaces on the GPU".
// Since the underlying parametrization is irrelevant we may use
// the screen as our domain space.

// normal perturbation method in listing 1
float3 PerturbNormal(float3 surf_pos, float3 surf_norm, float bump)
{
	float3 vSigmaX = ddx( surf_pos );
	float3 vSigmaY = ddy( surf_pos );
	float3 vN = surf_norm;		// normalized
	
	float3 R1 = cross(vSigmaY,vN);
	float3 R2 = cross(vN,vSigmaX);
	
	float fDet = dot(vSigmaX, R1);
	
	float dBx = ddx_fine( bump );
	float dBy = ddy_fine( bump );
	
	float3 vGrad = sign(fDet) * ( dBx * R1 + dBy * R2 );
	return normalize( abs(fDet)*surf_norm - vGrad);
}

// normal perturbation method in listing 3
float3 PerturbNormal2(float3 surf_norm, float3 gradient)
{
	// surf_norm must be normalized
	float3 vSurfGrad = gradient - surf_norm*dot(surf_norm, gradient);
	return normalize(surf_norm - vSurfGrad);
}

// This version allows the user to compute the height derivative in screen-space
// using alternative methods such as the listing 2 method.
float3 PerturbNormalArb(float3 surf_pos, float3 surf_norm, float dBx, float dBy)
{
	float3 vSigmaX = ddx( surf_pos );
	float3 vSigmaY = ddy( surf_pos );
	float3 vN = surf_norm;		// normalized
	
	float3 R1 = cross(vSigmaY,vN);
	float3 R2 = cross(vN,vSigmaX);
	
	float fDet = dot(vSigmaX, R1);
	
	float3 vGrad = sign(fDet) * ( dBx * R1 + dBy * R2 );
	return normalize( abs(fDet)*surf_norm - vGrad);
}

// This version is completely general and allows the user to perturb the normal using any form of domain space.
float3 PerturbNormalArbDomain(float3 vSigmaU, float3 vSigmaV, float3 surf_norm, float dBu, float dBv)
{
	float3 vN = surf_norm;		// normalized
	
	float3 R1 = cross(vSigmaV,vN);
	float3 R2 = cross(vN,vSigmaU);
	
	float fDet = dot(vSigmaU, R1);
	
	float3 vGrad = sign(fDet) * ( dBu * R1 + dBv * R2 );
	return normalize( abs(fDet)*surf_norm - vGrad);
}

// regular single sample crude evaluation of screen space derivatives
float2 CalcDerivativesFast(Texture2D tex_in, SamplerState sample_in, float2 texST)
{
	float height = tex_in.Sample(sample_in, texST).x;
	return float2(ddx_fine(height), ddy_fine(height));
}

// manual forward difference evaluation of screen space derivatives (listing 2)
float2 CalcDerivativesFwd(Texture2D tex_in, SamplerState sample_in, float2 texST)
{
	float2 TexDx = ddx(texST);
	float2 TexDy = ddy(texST);

	float Hll = tex_in.Sample(sample_in, texST).x;
	float dBx = tex_in.Sample(sample_in, texST+TexDx).x - Hll;
	float dBy = tex_in.Sample(sample_in, texST+TexDy).x - Hll;
	
	return float2(dBx, dBy);
}

// manual central difference evaluation of screen space derivatives
float2 CalcDerivativesCen(Texture2D tex_in, SamplerState sample_in, float2 texST)
{
	float2 TexDx = 0.5*ddx(texST);
	float2 TexDy = 0.5*ddy(texST);

	float dBx = tex_in.Sample(sample_in, texST+TexDx).x - tex_in.Sample(sample_in, texST-TexDx).x;
	float dBy = tex_in.Sample(sample_in, texST+TexDy).x - tex_in.Sample(sample_in, texST-TexDy).x;
	
	return float2(dBx, dBy);
}

// this transforms the derivative sampled from a derivative map into a screen-space derivative
// http://mmikkelsen3d.blogspot.com/2011/07/derivative-maps.html
// set bFlipVertDeriv to true when texST has flipped t coordinate
float2 CalcDerivativesFromDerivMap(Texture2D tex_in, SamplerState sample_in, float2 texST, bool bFlipVertDeriv = false)
{
	// texture space derivative remapped to normalized coordinates
	uint2 vDim;
	tex_in.GetDimensions(vDim.x, vDim.y);
	const float2 dBdST = vDim * (2*tex_in.Sample(sample_in, texST).xy-1);

	// chain rule
	const float2 TexDx = ddx(texST);
	const float2 TexDy = ddy(texST);
	const float fS = bFlipVertDeriv ? -1 : 1;		// resolved compile time
	float dBx = dBdST.x * TexDx.x + fS*dBdST.y * TexDx.y;
	float dBy = dBdST.x * TexDy.x + fS*dBdST.y * TexDy.y;
	
	return float2(dBx, dBy);
}



// this solves the faceted look you get during texture magnification when using CalcDerivativesFwd().
// However, it requires significantly more ALU (putting it in anyway for completion).
float2 CalcDerivativesBicubicToFwd(Texture2D tex_in, SamplerState sample_in, float2 texST)
{
	float fMix = saturate(1-tex_in.GetLODUnclamped(sample_in, texST));
	float2 dHdxy_simple = CalcDerivativesFwd(tex_in, sample_in, texST);
	if(fMix==0.0) return dHdxy_simple;
	else
	{
		// the derivative of the bicubic sampling of level 0
		float2 dHdxy_bicubic = CalcBicubicDerivatives(tex_in, sample_in, texST);
		return dHdxy_simple*(1-fMix) + dHdxy_bicubic*fMix;
	}
}

// this solves the faceted look you get during texture magnification when using CalcDerivativesCen().
// However, it requires significantly more ALU (putting it in anyway for completion).
float2 CalcDerivativesBicubicToCen(Texture2D tex_in, SamplerState sample_in, float2 texST)
{
	float fMix = saturate(1-tex_in.GetLODUnclamped(sample_in, texST));
	float2 dHdxy_simple = CalcDerivativesCen(tex_in, sample_in, texST);
	if(fMix==0.0) return dHdxy_simple;
	else
	{
		// the derivative of the bicubic sampling of level 0
		float2 dHdxy_bicubic = CalcBicubicDerivatives(tex_in, sample_in, texST);
		return dHdxy_simple*(1-fMix) + dHdxy_bicubic*fMix;
	}
}


// this produces a more conventional tangent and bitangent orthonormal basis
void BuildBasisTB(out float3 vT, out float3 vB, float3 surf_pos, float3 surf_norm, float2 texST)
{
	float3 vSigmaX = ddx( surf_pos );
	float3 vSigmaY = ddy( surf_pos );
	
	// assume surf_norm is normalized
	vSigmaX = vSigmaX - dot(vSigmaX, surf_norm)*surf_norm;
	vSigmaY = vSigmaY - dot(vSigmaY, surf_norm)*surf_norm;

	const float2 TexDx = ddx(texST);
	const float2 TexDy = ddy(texST);
	float sign_det = (TexDx.x*TexDy.y - TexDx.y*TexDy.x)<0 ? -1 : 1;

	// invC0 represents (dXds, dYds); but we don't divide by determinant (scale by sign instead)
	float2 invC0 = sign_det*float2(TexDy.y, -TexDx.y);

	vT = normalize(vSigmaX*invC0.x + vSigmaY*invC0.y);
	vB = cross(surf_norm, vT);

#if 1
	vB *= sign_det*(dot(vSigmaY, cross(surf_norm, vSigmaX))<0 ? -1 : 1);
#else
	// invC1 represents (dXdt, dYdt); but we don't divide by determinant (scale by sign instead)
	float2 invC1 = sign_det*float2(-TexDy.x, TexDx.x);
	float3 vBtmp = vSigmaX*invC1.x + vSigmaY*invC1.y;
	vB *= (dot(vBtmp, vB)<0 ? -1 : 1);
#endif
}

#endif
