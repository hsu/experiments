/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, version 1.0 beta 4      *
*                (c) 2006-2009 MGH, INRIA, USTL, UJF, CNRS                    *
*                                                                             *
* This library is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This library is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this library; if not, write to the Free Software Foundation,     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.          *
*******************************************************************************
*                               SOFA :: Modules                               *
*                                                                             *
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#ifndef CUDAMATH_H
#define CUDAMATH_H

#include <cuda_runtime.h>
#include <cuda.h>

#if defined(__cplusplus) && CUDA_VERSION < 2000
namespace sofa
{
namespace gpu
{
namespace cuda
{
#endif

template<int i>
struct t_log2;

template<> struct t_log2<(1<< 0)> { enum { V =  0 }; };
template<> struct t_log2<(1<< 1)> { enum { V =  1 }; };
template<> struct t_log2<(1<< 2)> { enum { V =  2 }; };
template<> struct t_log2<(1<< 3)> { enum { V =  3 }; };
template<> struct t_log2<(1<< 4)> { enum { V =  4 }; };
template<> struct t_log2<(1<< 5)> { enum { V =  5 }; };
template<> struct t_log2<(1<< 6)> { enum { V =  6 }; };
template<> struct t_log2<(1<< 7)> { enum { V =  7 }; };
template<> struct t_log2<(1<< 8)> { enum { V =  8 }; };
template<> struct t_log2<(1<< 9)> { enum { V =  9 }; };
template<> struct t_log2<(1<<10)> { enum { V = 10 }; };
template<> struct t_log2<(1<<11)> { enum { V = 11 }; };
template<> struct t_log2<(1<<12)> { enum { V = 12 }; };
template<> struct t_log2<(1<<13)> { enum { V = 13 }; };
template<> struct t_log2<(1<<14)> { enum { V = 14 }; };
template<> struct t_log2<(1<<15)> { enum { V = 15 }; };
template<> struct t_log2<(1<<16)> { enum { V = 16 }; };
#define LOG2(bsize) (t_log2<bsize>::V)

#if (__CUDA_ARCH__ < 200)
#define fastmul(x,y)	__mul24((x),(y))
#else
#define fastmul(x,y)	((x)*(y))
#endif


template<class real>
class CudaVec2;

template<class real>
class CudaVec3;

template<class real>
class CudaVec4;

template<>
class CudaVec2<float> : public float2
{
public:
    typedef float Real;
    static __inline__ __device__ __host__ CudaVec2<float> make(Real x, Real y)
    {
	CudaVec2<float> r; r.x = x; r.y = y; return r;
    }
    static __inline__ __device__ __host__ CudaVec2<float> make(float2 v)
    {
	CudaVec2<float> r; r.x = v.x; r.y = v.y; return r;
    }
    static __inline__ __device__ __host__ CudaVec2<float> make(float3 v)
    {
	CudaVec2<float> r; r.x = v.x; r.y = v.y; return r;
    }
};

template<>
class CudaVec3<float> : public float3
{
public:
    typedef float Real;
    static __inline__ __device__ __host__ CudaVec3<float> make(Real x, Real y, Real z=0)
    {
	CudaVec3<float> r; r.x = x; r.y = y;  r.z = z; return r;
    }
    static __inline__ __device__ __host__ CudaVec3<float> make(float2 v, Real z=0)
    {
	CudaVec3<float> r; r.x = v.x; r.y = v.y;  r.z = z; return r;
    }
    static __inline__ __device__ __host__ CudaVec3<float> make(float3 v)
    {
	CudaVec3<float> r; r.x = v.x; r.y = v.y;  r.z = v.z; return r;
    }
    static __inline__ __device__ __host__ CudaVec3<float> make(float4 v)
    {
	CudaVec3<float> r; r.x = v.x; r.y = v.y;  r.z = v.z; return r;
    }
};

template<>
class CudaVec4<float> : public float4
{
public:
    typedef float Real;
    static __inline__ __device__ __host__ CudaVec4<float> make(Real x, Real y, Real z, Real w=0)
    {
	CudaVec4<float> r; r.x = x; r.y = y;  r.z = z; r.w = w; return r;
    }
    static __inline__ __device__ __host__ CudaVec4<float> make(float3 v, Real w=0)
    {
	CudaVec4<float> r; r.x = v.x; r.y = v.y;  r.z = v.z; r.w = w; return r;
    }
    static __inline__ __device__ __host__ CudaVec4<float> make(float4 v)
    {
	CudaVec4<float> r; r.x = v.x; r.y = v.y;  r.z = v.z; r.w = v.w; return r;
    }
};

typedef CudaVec2<float> CudaVec2f;
typedef CudaVec3<float> CudaVec3f;
typedef CudaVec4<float> CudaVec4f;

#ifdef SOFA_GPU_CUDA_DOUBLE

#if CUDA_VERSION<3000

class __align__(8) double3
{
public:
    double x, y, z;
};

class __align__(16) double4
{
public:
    double x, y, z, w;
};

#endif

template<>
class CudaVec2<double> : public double2
{
public:
    typedef double Real;
    static __inline__ __device__ __host__ CudaVec2<double> make(Real x, Real y)
    {
	CudaVec2<double> r; r.x = x; r.y = y; return r;
    }
    static __inline__ __device__ __host__ CudaVec2<double> make(const double2& v)
    {
	CudaVec2<double> r; r.x = v.x; r.y = v.y; return r;
    }
    static __inline__ __device__ __host__ CudaVec2<double> make(const double3& v)
    {
	CudaVec2<double> r; r.x = v.x; r.y = v.y; return r;
    }
};

template<>
class CudaVec3<double> : public double3
{
public:
    typedef double Real;
    static __inline__ __device__ __host__ CudaVec3<double> make(Real x, Real y, Real z=0)
    {
	CudaVec3<double> r; r.x = x; r.y = y;  r.z = z; return r;
    }
    static __inline__ __device__ __host__ CudaVec3<double> make(double2 v, Real z=0)
    {
	CudaVec3<double> r; r.x = v.x; r.y = v.y;  r.z = z; return r;
    }
    static __inline__ __device__ __host__ CudaVec3<double> make(const double3& v)
    {
	CudaVec3<double> r; r.x = v.x; r.y = v.y;  r.z = v.z; return r;
    }
    static __inline__ __device__ __host__ CudaVec3<double> make(const double4& v)
    {
	CudaVec3<double> r; r.x = v.x; r.y = v.y;  r.z = v.z; return r;
    }
};

template<>
class CudaVec4<double> : public double4
{
public:
    typedef double Real;
    static __inline__ __device__ __host__ CudaVec4<double> make(Real x, Real y, Real z, Real w=0)
    {
	CudaVec4<double> r; r.x = x; r.y = y;  r.z = z; r.w = w; return r;
    }
    static __inline__ __device__ __host__ CudaVec4<double> make(const double3& v, Real w=0)
    {
	CudaVec4<double> r; r.x = v.x; r.y = v.y;  r.z = v.z; r.w = w; return r;
    }
    static __inline__ __device__ __host__ CudaVec4<double> make(const double4& v)
    {
	CudaVec4<double> r; r.x = v.x; r.y = v.y;  r.z = v.z; r.w = v.w; return r;
    }
};

typedef CudaVec2<double> CudaVec2d;
typedef CudaVec3<double> CudaVec3d;
typedef CudaVec4<double> CudaVec4d;

#endif // SOFA_GPU_CUDA_DOUBLE

template<class real>
__device__ CudaVec3<real> operator+(CudaVec3<real> a, CudaVec3<real> b)
{
    return CudaVec3<real>::make(a.x+b.x, a.y+b.y, a.z+b.z);
}

template<class real>
__device__ void operator+=(CudaVec3<real>& a, CudaVec3<real> b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
}

template<class real>
__device__ void operator+=(CudaVec3<real>& a, real b)
{
	a.x += b;
	a.y += b;
	a.z += b;
}

template<class real>
__device__ CudaVec3<real> operator-(CudaVec3<real> a, CudaVec3<real> b)
{
	return CudaVec3<real>::make(a.x-b.x, a.y-b.y, a.z-b.z);
}

template<class real>
__device__ void operator-=(CudaVec3<real>& a, CudaVec3<real> b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
}

template<class real>
__device__ void operator-=(CudaVec3<real>& a, real b)
{
	a.x -= b;
	a.y -= b;
	a.z -= b;
}

template<class real>
__device__ CudaVec3<real> operator-(CudaVec3<real>& a)
{
	return CudaVec3<real>::make(-a.x, -a.y, -a.z);
}

template<class real>
__device__ CudaVec3<real> operator*(CudaVec3<real> a, real b)
{
	return CudaVec3<real>::make(a.x*b, a.y*b, a.z*b);
}

template<class real>
__device__ CudaVec3<real> operator/(CudaVec3<real> a, real b)
{
	return CudaVec3<real>::make(a.x/b, a.y/b, a.z/b);
}

template<class real>
__device__ void operator*=(CudaVec3<real>& a, real b)
{
	a.x *= b;
	a.y *= b;
	a.z *= b;
}

template<class real>
__device__ CudaVec3<real> operator*(real a, CudaVec3<real> b)
{
	return CudaVec3<real>::make(a*b.x, a*b.y, a*b.z);
}

template<class real>
__device__ CudaVec3<real> mul(CudaVec3<real> a, CudaVec3<real> b)
{
	return CudaVec3<real>::make(a.x*b.x, a.y*b.y, a.z*b.z);
}

template<class real>
__device__ real dot(CudaVec3<real> a, CudaVec3<real> b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

template<class real>
__device__ CudaVec3<real> cross(CudaVec3<real> a, CudaVec3<real> b)
{
	return CudaVec3<real>::make(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x);
}

template<class real>
__device__ real norm2(CudaVec3<real> a)
{
    return a.x*a.x+a.y*a.y+a.z*a.z;
}

template<class real>
__device__ real norm(CudaVec3<real> a)
{
    return sqrtf(norm2(a));
}

template<class real>
__device__ real invnorm(CudaVec3<real> a)
{
    return rsqrtf(norm2(a));
}

template<class real>
__device__ real norm2(CudaVec4<real> a)
{
	return a.x*a.x+a.y*a.y+a.z*a.z+a.w*a.w;
}

template<class real>
__device__ real invnorm(CudaVec4<real> a)
{
    return rsqrtf(norm2(a));
}

template<class real>
__device__ CudaVec4<real> inv(CudaVec4<real> a)
{
	real norm = norm2(a);
	if (norm > 0)
		return CudaVec4<real>::make(-a.x/norm, -a.y/norm, -a.z/norm, a.w/norm);
	return CudaVec4<real>::make(0.0, 0.0, 0.0, 0.0);
}

template<class real>
__device__ CudaVec4<real> operator*(CudaVec4<real> a, CudaVec4<real> b)
{
	CudaVec4<real> quat;
	quat.w = a.w * b.w - (a.x * b.x + a.y * b.y + a.z * b.z);
	quat.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
	quat.y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
	quat.z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;

	return quat;
}

template<class real>
__device__ CudaVec4<real> operator*(CudaVec4<real> a, real f)
{
	return CudaVec4<real>::make(a.x*f, a.y*f, a.z*f, a.w*f);
}

template<class real>
__device__ CudaVec3<real> toEulerVector(CudaVec4<real> a)
{
	real angle = acos(a.w)*2;
	CudaVec3<real> euler = CudaVec3<real>::make(a.x, a.y, a.z);
	euler = euler*invnorm(euler);
	return euler*angle;
}

template<class real>
__device__ CudaVec4<real> createQuaterFromEuler(CudaVec3<real> v)
{
	CudaVec4<real> quat;
	real a0 = v.x;
	a0 /= (real)2.0;
	real a1 = v.y;
	a1 /= (real)2.0;
	real a2 = v.z;
	a2 /= (real)2.0;
	quat.w = cos(a0)*cos(a1)*cos(a2) + sin(a0)*sin(a1)*sin(a2);
	quat.x = sin(a0)*cos(a1)*cos(a2) - cos(a0)*sin(a1)*sin(a2);
	quat.y = cos(a0)*sin(a1)*cos(a2) + sin(a0)*cos(a1)*sin(a2);
	quat.z = cos(a0)*cos(a1)*sin(a2) - sin(a0)*sin(a1)*cos(a2);
	return quat;
}

template<class real>
class /*__align__(4)*/ matrix3
{
public:
    CudaVec3<real> x,y,z;
/*
    CudaVec3<real> getLx() { return x; }
    CudaVec3<real> getLy() { return y; }
    CudaVec3<real> getLz() { return z; }

    CudaVec3<real> getCx() { return CudaVec3<real>::make(x.x,y.x,z.x); }
    CudaVec3<real> getCy() { return CudaVec3<real>::make(x.y,y.y,z.y); }
    CudaVec3<real> getCz() { return CudaVec3<real>::make(x.z,y.z,z.z); }

    void setLx(CudaVec3<real> v) { x = v; }
    void setLy(CudaVec3<real> v) { y = v; }
    void setLz(CudaVec3<real> v) { z = v; }

    void setCx(CudaVec3<real> v) { x.x = v.x; y.x = v.y; z.x = v.z; }
    void setCy(CudaVec3<real> v) { x.y = v.x; y.y = v.y; z.y = v.z; }
    void setCz(CudaVec3<real> v) { x.z = v.x; y.z = v.y; z.z = v.z; }
*/
    __device__ CudaVec3<real> operator*(CudaVec3<real> v)
    {
        return CudaVec3<real>::make(dot(x,v),dot(y,v),dot(z,v));
    }
    __device__ CudaVec3<real> mulT(CudaVec3<real> v)
    {
        return x*v.x+y*v.y+z*v.z;
    }
    __device__ matrix3<real> operator*(matrix3<real> v)
    {
    	matrix3<real> r;
    	r.x.x = x.x * v.x.x + x.y * v.y.x + x.z * v.z.x;
    	r.x.y = x.x * v.x.y + x.y * v.y.y + x.z * v.z.y;
    	r.x.z = x.x * v.x.z + x.y * v.y.z + x.z * v.z.z;

    	r.y.x = y.x * v.x.x + y.y * v.y.x + y.z * v.z.x;
    	r.y.y = y.x * v.x.y + y.y * v.y.y + y.z * v.z.y;
    	r.y.z = y.x * v.x.z + y.y * v.y.z + y.z * v.z.z;

    	r.z.x = z.x * v.x.x + z.y * v.y.x + z.z * v.z.x;
    	r.z.y = z.x * v.x.y + z.y * v.y.y + z.z * v.z.y;
    	r.z.z = z.x * v.x.z + z.y * v.y.z + z.z * v.z.z;
    	return r;
    }
    __device__ matrix3<real> mulT(matrix3<real> v)
    {
    	matrix3<real> r;
    	r.x.x = x.x * v.x.x + y.x * v.y.x + z.x * v.z.x;
    	r.x.y = x.x * v.x.y + y.x * v.y.y + z.x * v.z.y;
    	r.x.z = x.x * v.x.z + y.x * v.y.z + z.x * v.z.z;

    	r.y.x = x.y * v.x.x + y.y * v.y.x + z.y * v.z.x;
    	r.y.y = x.y * v.x.y + y.y * v.y.y + z.y * v.z.y;
    	r.y.z = x.y * v.x.z + y.y * v.y.z + z.y * v.z.z;

    	r.z.x = x.z * v.x.x + y.z * v.y.x + z.z * v.z.x;
    	r.z.y = x.z * v.x.y + y.z * v.y.y + z.z * v.z.y;
    	r.z.z = x.z * v.x.z + y.z * v.y.z + z.z * v.z.z;
    	return r;
    }
    __device__ real determinant(matrix3<real> v)
    {
      real det;
      det = v.x.x*v.y.y*v.z.z;
      det += v.y.x*v.z.y*v.x.z;
      det += v.z.x*v.x.y*v.y.z;
      det -= v.x.x*v.z.y*v.y.z;
      det -= v.y.x*v.x.y*v.z.z;
      det -= v.z.x*v.y.y*v.x.z;
      return det;
    }
    __device__ matrix3<real> invert(matrix3<real> v)
    {
      real det = determinant(v);
      matrix3<real> r;
      r.x.x = (v.y.y*v.z.z - v.z.y*v.y.z)/det;
      r.y.x = (v.y.z*v.z.x - v.z.z*v.y.x)/det;
      r.z.x = (v.y.x*v.z.y - v.z.x*v.y.y)/det;
      r.x.y = (v.z.y*v.x.z - v.x.y*v.z.z)/det;
      r.y.y = (v.z.z*v.x.x - v.x.z*v.z.x)/det;
      r.z.y = (v.z.x*v.x.y - v.x.x*v.z.y)/det;
      r.x.z = (v.x.y*v.y.z - v.y.y*v.x.z)/det;
      r.y.z = (v.x.z*v.y.x - v.y.z*v.x.x)/det;
      r.z.z = (v.x.x*v.y.y - v.y.x*v.x.y)/det;
      
      return r;
    }
    __device__ real mulX(CudaVec3<real> v)
    {
        return dot(x,v);
    }
    __device__ real mulY(CudaVec3<real> v)
    {
        return dot(y,v);
    }
    __device__ real mulZ(CudaVec3<real> v)
    {
        return dot(z,v);
    }
    __device__ void readAoS(const real* data, int bsize)
    {
        x.x=*data; data+=bsize;
        x.y=*data; data+=bsize;
        x.z=*data; data+=bsize;
        y.x=*data; data+=bsize;
        y.y=*data; data+=bsize;
        y.z=*data; data+=bsize;
        z.x=*data; data+=bsize;
        z.y=*data; data+=bsize;
        z.z=*data; data+=bsize;
    }
    __device__ void readAoS(const real* data)
    {
    	readAoS(data, blockDim.x);
    }
    __device__ void writeAoS(real* data, int bsize)
    {
        *data=x.x; data+=bsize;
        *data=x.y; data+=bsize;
        *data=x.z; data+=bsize;
        *data=y.x; data+=bsize;
        *data=y.y; data+=bsize;
        *data=y.z; data+=bsize;
        *data=z.x; data+=bsize;
        *data=z.y; data+=bsize;
        *data=z.z; data+=bsize;
    }

    __device__ void writeAoS(real* data)
    {
    	writeAoS(data, blockDim.x);
    }

    __device__ void readAoS6(const real* data, int bsize)
    {
        x.x=*data; data+=bsize;
        x.y=*data; data+=bsize;
        x.z=*data; data+=bsize;
        y.x=*data; data+=bsize;
        y.y=*data; data+=bsize;
        y.z=*data; data+=bsize;
        z = cross(x,y);
    }
    __device__ void readAoS6(const real* data)
    {
    	readAoS6(data, blockDim.x);
    }
    __device__ void writeAoS6(real* data, int bsize)
    {
        *data=x.x; data+=bsize;
        *data=x.y; data+=bsize;
        *data=x.z; data+=bsize;
        *data=y.x; data+=bsize;
        *data=y.y; data+=bsize;
        *data=y.z; data+=bsize;
    }

    __device__ void writeAoS6(real* data)
    {
    	writeAoS6(data, blockDim.x);
    }
};

template<class real>
__device__ matrix3<real> toMatrix(CudaVec4<real> q)
{
	matrix3<real> mat;
	mat.x.x = (real)(1.0f - 2.0f * (q.y * q.y + q.z * q.z)); 
	mat.x.y = (real)(2.0f * (q.x * q.y - q.z * q.w));
	mat.x.z = (real)(2.0f * (q.z * q.x + q.y * q.w));        

	mat.y.x = (real)(2.0f * (q.x * q.y + q.z * q.w));        
	mat.y.y = (real)(1.0f - 2.0f * (q.z * q.z + q.x * q.x)); 
	mat.y.z = (real)(2.0f * (q.y * q.z - q.x * q.w));        

	mat.z.x = (real)(2.0f * (q.z * q.x - q.y * q.w));        
	mat.z.y = (real)(2.0f * (q.y * q.z + q.x * q.w));        
	mat.z.z = (real)(1.0f - 2.0f * (q.y * q.y + q.x * q.x));

	return mat;
}


template<class real>
class /*__align__(4)*/ matrix4
{
public:
    CudaVec4<real> x,y,z,w;
    __device__ CudaVec4<real> operator*(CudaVec4<real> v)
    {
        return CudaVec4<real>::make(dot(x,v),dot(y,v),dot(z,v),dot(w,v));
    }
    __device__ CudaVec4<real> operator*(CudaVec3<real> v)
    {
        return CudaVec4<real>::make(x.x*v.x+x.y*v.y+x.z*v.z+x.w,
                                    y.x*v.x+y.y*v.y+y.z*v.z+y.w,
                                    z.x*v.x+z.y*v.y+z.z*v.z+z.w,
                                    w.x*v.x+w.y*v.y+w.z*v.z+w.w);
    }
    __device__ CudaVec4<real> mulT(CudaVec4<real> v)
    {
        return x*v.x+y*v.y+z*v.z+w*v.w;
    }
    __device__ matrix4<real> operator*(matrix4<real> v)
    {
    	matrix3<real> r;
    	r.x.x = x.x * v.x.x + x.y * v.y.x + x.z * v.z.x + x.w * v.w.x;
    	r.x.y = x.x * v.x.y + x.y * v.y.y + x.z * v.z.y + x.w * v.w.y;
    	r.x.z = x.x * v.x.z + x.y * v.y.z + x.z * v.z.z + x.w * v.w.z;
    	r.x.w = x.x * v.x.w + x.y * v.y.w + x.z * v.z.w + x.w * v.w.w;

    	r.y.x = y.x * v.x.x + y.y * v.y.x + y.z * v.z.x + y.w * v.w.x;
    	r.y.y = y.x * v.x.y + y.y * v.y.y + y.z * v.z.y + y.w * v.w.y;
    	r.y.z = y.x * v.x.z + y.y * v.y.z + y.z * v.z.z + y.w * v.w.z;
    	r.y.w = y.x * v.x.w + y.y * v.y.w + y.z * v.z.w + y.w * v.w.w;

    	r.z.x = z.x * v.x.x + z.y * v.y.x + z.z * v.z.x + z.w * v.w.x;
    	r.z.y = z.x * v.x.y + z.y * v.y.y + z.z * v.z.y + z.w * v.w.y;
    	r.z.z = z.x * v.x.z + z.y * v.y.z + z.z * v.z.z + z.w * v.w.z;
    	r.z.w = z.x * v.x.w + z.y * v.y.w + z.z * v.z.w + z.w * v.w.w;

    	r.w.x = w.x * v.x.x + w.y * v.y.x + w.z * v.z.x + w.w * v.w.x;
    	r.w.y = w.x * v.x.y + w.y * v.y.y + w.z * v.z.y + w.w * v.w.y;
    	r.w.z = w.x * v.x.z + w.y * v.y.z + w.z * v.z.z + w.w * v.w.z;
    	r.w.w = w.x * v.x.w + w.y * v.y.w + w.z * v.z.w + w.w * v.w.w;

    	return r;
    }
    __device__ matrix4<real> mulT(matrix4<real> v)
    {
    	matrix3<real> r;
    	r.x.x = x.x * v.x.x + y.x * v.y.x + z.x * v.z.x + w.x * v.w.x;
    	r.x.y = x.x * v.x.y + y.x * v.y.y + z.x * v.z.y + w.x * v.w.y;
    	r.x.z = x.x * v.x.z + y.x * v.y.z + z.x * v.z.z + w.x * v.w.z;
    	r.x.w = x.x * v.x.w + y.x * v.y.w + z.x * v.z.w + w.x * v.w.w;

    	r.y.x = x.y * v.x.x + y.y * v.y.x + z.y * v.z.x + w.y * v.w.x;
    	r.y.y = x.y * v.x.y + y.y * v.y.y + z.y * v.z.y + w.y * v.w.y;
    	r.y.z = x.y * v.x.z + y.y * v.y.z + z.y * v.z.z + w.y * v.w.z;
    	r.y.w = x.y * v.x.w + y.y * v.y.w + z.y * v.z.w + w.y * v.w.w;

    	r.z.x = x.z * v.x.x + y.z * v.y.x + z.z * v.z.x + w.z * v.w.x;
    	r.z.y = x.z * v.x.y + y.z * v.y.y + z.z * v.z.y + w.z * v.w.y;
    	r.z.z = x.z * v.x.z + y.z * v.y.z + z.z * v.z.z + w.z * v.w.z;
    	r.z.w = x.z * v.x.w + y.z * v.y.w + z.z * v.z.w + w.z * v.w.w;

    	r.w.x = x.w * v.x.x + y.w * v.y.x + z.w * v.z.x + w.w * v.w.x;
    	r.w.y = x.w * v.x.y + y.w * v.y.y + z.w * v.z.y + w.w * v.w.y;
    	r.w.z = x.w * v.x.z + y.w * v.y.z + z.w * v.z.z + w.w * v.w.z;
    	r.w.w = x.w * v.x.w + y.w * v.y.w + z.w * v.z.w + w.w * v.w.w;

    	return r;
    }
    __device__ real mulX(CudaVec4<real> v)
    {
        return dot(x,v);
    }
    __device__ real mulY(CudaVec4<real> v)
    {
        return dot(y,v);
    }
    __device__ real mulZ(CudaVec4<real> v)
    {
        return dot(z,v);
    }
    __device__ void readAoS(const real* data, int bsize)
    {
        x.x=*data; data+=bsize;
        x.y=*data; data+=bsize;
        x.z=*data; data+=bsize;
        x.w=*data; data+=bsize;
        y.x=*data; data+=bsize;
        y.y=*data; data+=bsize;
        y.z=*data; data+=bsize;
        y.w=*data; data+=bsize;
        z.x=*data; data+=bsize;
        z.y=*data; data+=bsize;
        z.z=*data; data+=bsize;
        z.w=*data; data+=bsize;
        w.x=*data; data+=bsize;
        w.y=*data; data+=bsize;
        w.z=*data; data+=bsize;
        w.w=*data; data+=bsize;
    }
    __device__ void readAoS(const real* data)
    {
    	readAoS(data, blockDim.x);
    }
    __device__ void writeAoS(real* data, int bsize)
    {
        *data=x.x; data+=bsize;
        *data=x.y; data+=bsize;
        *data=x.z; data+=bsize;
        *data=x.w; data+=bsize;
        *data=y.x; data+=bsize;
        *data=y.y; data+=bsize;
        *data=y.z; data+=bsize;
        *data=y.w; data+=bsize;
        *data=z.x; data+=bsize;
        *data=z.y; data+=bsize;
        *data=z.z; data+=bsize;
        *data=z.w; data+=bsize;
        *data=w.x; data+=bsize;
        *data=w.y; data+=bsize;
        *data=w.z; data+=bsize;
        *data=w.w; data+=bsize;
    }
    __device__ void writeAoS(real* data)
    {
    	writeAoS(data, blockDim.x);
    }
};

#if defined(__cplusplus) && CUDA_VERSION < 2000
} // namespace cuda
} // namespace gpu
} // namespace sofa
#endif

#endif
