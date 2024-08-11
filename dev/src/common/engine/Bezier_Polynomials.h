
#pragma once

namespace QuadraticBezierNamespace
{
	inline float Sign(float x){if(x>=0.0){return 1.0f;}else{return -1.0f;}}
	class Polynomial
	{
	public:
		Polynomial(int Degree);
		~Polynomial();
		inline float Value(float x)
		{
			float Sum=0.f; 
			for(int i=0;i<Count();i++)
			{
				Sum+=pow(x,m_degree-i)*m_constants[i];
			} 
			return Sum;
		}
		inline int Count(){return m_degree+1;}
		void FindRoots(float* roots, int & numroots);
		Polynomial* Derivative();
		float* m_constants;
		int m_degree;
	private:
		void FindRoots0(float* roots, int & numroots);
		void FindRoots1(float* roots, int & numroots);
		void FindRoots2(float* roots, int & numroots);
		void FindRoots3(float* roots, int & numroots);
		void FindRoots4(float* roots, int & numroots);
		void GetRoots4(float x, float m, float n, float A, float B, float C, float pierw, float t1, float t2, float* roots, int & numroots );
	};
}