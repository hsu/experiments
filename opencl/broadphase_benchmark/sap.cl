/*
Copyright (c) 2012 Advanced Micro Devices, Inc.  

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
//Originally written by Erwin Coumans


typedef struct 
{
	union
	{
		float4	m_min;
		float   m_minElems[4];
		int			m_minIndices[4];
	};
	union
	{
		float4	m_max;
		float   m_maxElems[4];
		int			m_maxIndices[4];
	};
} btAabbCL;


/// conservative test for overlap between two aabbs
bool TestAabbAgainstAabb2(const btAabbCL* aabb1, __local const btAabbCL* aabb2);
bool TestAabbAgainstAabb2(const btAabbCL* aabb1, __local const btAabbCL* aabb2)
{
	bool overlap = true;
	overlap = (aabb1->m_min.x > aabb2->m_max.x || aabb1->m_max.x < aabb2->m_min.x) ? false : overlap;
	overlap = (aabb1->m_min.z > aabb2->m_max.z || aabb1->m_max.z < aabb2->m_min.z) ? false : overlap;
	overlap = (aabb1->m_min.y > aabb2->m_max.y || aabb1->m_max.y < aabb2->m_min.y) ? false : overlap;
	return overlap;
}
bool TestAabbAgainstAabb2GlobalGlobal(__global const btAabbCL* aabb1, __global const btAabbCL* aabb2);
bool TestAabbAgainstAabb2GlobalGlobal(__global const btAabbCL* aabb1, __global const btAabbCL* aabb2)
{
	bool overlap = true;
	overlap = (aabb1->m_min.x > aabb2->m_max.x || aabb1->m_max.x < aabb2->m_min.x) ? false : overlap;
	overlap = (aabb1->m_min.z > aabb2->m_max.z || aabb1->m_max.z < aabb2->m_min.z) ? false : overlap;
	overlap = (aabb1->m_min.y > aabb2->m_max.y || aabb1->m_max.y < aabb2->m_min.y) ? false : overlap;
	return overlap;
}

bool TestAabbAgainstAabb2Global(const btAabbCL* aabb1, __global const btAabbCL* aabb2);
bool TestAabbAgainstAabb2Global(const btAabbCL* aabb1, __global const btAabbCL* aabb2)
{
	bool overlap = true;
	overlap = (aabb1->m_min.x > aabb2->m_max.x || aabb1->m_max.x < aabb2->m_min.x) ? false : overlap;
	overlap = (aabb1->m_min.z > aabb2->m_max.z || aabb1->m_max.z < aabb2->m_min.z) ? false : overlap;
	overlap = (aabb1->m_min.y > aabb2->m_max.y || aabb1->m_max.y < aabb2->m_min.y) ? false : overlap;
	return overlap;
}


__kernel void   computePairsKernelOriginal( __global const btAabbCL* aabbs, volatile __global int2* pairsOut,volatile  __global int* pairCount, int numObjects, int axis, int maxPairs)
{
	int i = get_global_id(0);
	if (i>=numObjects)
		return;
	for (int j=i+1;j<numObjects;j++)
	{
  	if(aabbs[i].m_maxElems[axis] < (aabbs[j].m_minElems[axis])) 
		{
			break;
		}
		if (TestAabbAgainstAabb2GlobalGlobal(&aabbs[i],&aabbs[j]))
		{
			int2 myPair;
			myPair.x = aabbs[i].m_minIndices[3];
			myPair.y = aabbs[j].m_minIndices[3];
			int curPair = atomic_inc (pairCount);
			if (curPair<maxPairs)
			{
					pairsOut[curPair] = myPair; //flush to main memory
			}
		}
	}
}


__kernel void   computePairsKernelBarrier( __global const btAabbCL* aabbs, volatile __global int2* pairsOut,volatile  __global int* pairCount, int numObjects, int axis, int maxPairs)
{
	int i = get_global_id(0);
	int localId = get_local_id(0);

	__local int numActiveWgItems[1];
	__local int breakRequest[1];

	if (localId==0)
	{
		numActiveWgItems[0] = 0;
		breakRequest[0] = 0;
	}
	barrier(CLK_LOCAL_MEM_FENCE);
	atomic_inc(numActiveWgItems);
	barrier(CLK_LOCAL_MEM_FENCE);
	int localBreak = 0;

	int j=i+1;
	do
	{
		barrier(CLK_LOCAL_MEM_FENCE);
	
		if (j<numObjects)
		{
	  	if(aabbs[i].m_maxElems[axis] < (aabbs[j].m_minElems[axis])) 
			{
				if (!localBreak)
				{
					atomic_inc(breakRequest);
					localBreak = 1;
				}
			}
		}
		
		barrier(CLK_LOCAL_MEM_FENCE);
		
		if (j>=numObjects && !localBreak)
		{
			atomic_inc(breakRequest);
			localBreak = 1;
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		
		if (!localBreak)
		{
			if (TestAabbAgainstAabb2GlobalGlobal(&aabbs[i],&aabbs[j]))
			{
				int2 myPair;
				myPair.x = aabbs[i].m_minIndices[3];
				myPair.y = aabbs[j].m_minIndices[3];
				int curPair = atomic_inc (pairCount);
				if (curPair<maxPairs)
				{
						pairsOut[curPair] = myPair; //flush to main memory
				}
			}
		}
		j++;

	} while (breakRequest[0]<numActiveWgItems[0]);
}


__kernel void   computePairsKernelLocalSharedMemory( __global const btAabbCL* aabbs, volatile __global int2* pairsOut,volatile  __global int* pairCount, int numObjects, int axis, int maxPairs)
{
	int i = get_global_id(0);
	int localId = get_local_id(0);

	__local int numActiveWgItems[1];
	__local int breakRequest[1];
	__local btAabbCL localAabbs[128];// = aabbs[i];
	
	btAabbCL myAabb;
	
	myAabb = (i<numObjects)? aabbs[i]:aabbs[0];
	float testValue = 	myAabb.m_maxElems[axis];
	
	if (localId==0)
	{
		numActiveWgItems[0] = 0;
		breakRequest[0] = 0;
	}
	int localCount=0;
	int block=0;
	localAabbs[localId] = (i+block)<numObjects? aabbs[i+block] : aabbs[0];
	localAabbs[localId+64] = (i+block+64)<numObjects? aabbs[i+block+64]: aabbs[0];
	
	barrier(CLK_LOCAL_MEM_FENCE);
	atomic_inc(numActiveWgItems);
	barrier(CLK_LOCAL_MEM_FENCE);
	int localBreak = 0;
	
	int j=i+1;
	do
	{
		barrier(CLK_LOCAL_MEM_FENCE);
	
		if (j<numObjects)
		{
	  	if(testValue < (localAabbs[localCount+localId+1].m_minElems[axis])) 
			{
				if (!localBreak)
				{
					atomic_inc(breakRequest);
					localBreak = 1;
				}
			}
		}
		
		barrier(CLK_LOCAL_MEM_FENCE);
		
		if (j>=numObjects && !localBreak)
		{
			atomic_inc(breakRequest);
			localBreak = 1;
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		
		if (!localBreak)
		{
			if (TestAabbAgainstAabb2(&myAabb,&localAabbs[localCount+localId+1]))
			{
				int2 myPair;
				myPair.x = myAabb.m_minIndices[3];
				myPair.y = localAabbs[localCount+localId+1].m_minIndices[3];
				int curPair = atomic_inc (pairCount);
				if (curPair<maxPairs)
				{
						pairsOut[curPair] = myPair; //flush to main memory
				}
			}
		}
		
		barrier(CLK_LOCAL_MEM_FENCE);

		localCount++;
		if (localCount==64)
		{
			localCount = 0;
			block+=64;			
			localAabbs[localId] = ((i+block)<numObjects) ? aabbs[i+block] : aabbs[0];
			localAabbs[localId+64] = ((i+64+block)<numObjects) ? aabbs[i+block+64] : aabbs[0];
		}
		j++;
		
	} while (breakRequest[0]<numActiveWgItems[0]);
	
}




//http://stereopsis.com/radix.html
unsigned int FloatFlip(float fl);
unsigned int FloatFlip(float fl)
{
	unsigned int f = *(unsigned int*)&fl;
	unsigned int mask = -(int)(f >> 31) | 0x80000000;
	return f ^ mask;
}
float IFloatFlip(unsigned int f);
float IFloatFlip(unsigned int f)
{
	unsigned int mask = ((f >> 31) - 1) | 0x80000000;
	unsigned int fl = f ^ mask;
	return *(float*)&fl;
}

__kernel void   flipFloatKernel( __global const btAabbCL* aabbs, volatile __global int2* sortData, int numObjects, int axis)
{
	int i = get_global_id(0);
	if (i>=numObjects)
		return;
		
		sortData[i].x = FloatFlip(aabbs[i].m_minElems[axis]);
		sortData[i].y = i;
		
}


__kernel void   scatterKernel( __global const btAabbCL* aabbs, volatile __global const int2* sortData, __global btAabbCL* sortedAabbs, int numObjects)
{
	int i = get_global_id(0);
	if (i>=numObjects)
		return;

		sortedAabbs[i] = aabbs[sortData[i].y];
}
