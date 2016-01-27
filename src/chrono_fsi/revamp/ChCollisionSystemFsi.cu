// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Author: Arman Pazouki
// =============================================================================
//
// Base class for fsi system.//
// =============================================================================

#include <stdexcept>
#include "ChCollisionSystemFsi.h"
using namespace fsi;

/**
 * @brief calcHashD
 * @details
 * 		 1. Get particle index. Determine by the block and thread we are in.
 * 		 2. From x,y,z position determine which bin it is in.
 * 		 3. Calculate hash from bin index.
 * 		 4. Store hash and particle index associated with it.
 *
 * @param gridMarkerHashD
 * @param gridMarkerIndexD
 * @param posRad
 * @param numAllMarkers
 */
__global__ void calcHashD(uint* gridMarkerHashD,   // output
		uint* gridMarkerIndexD,  // output
		Real3* posRad,          // input: positions
		uint numAllMarkers, volatile bool *isErrorD) {

	/* Calculate the index of where the particle is stored in posRad. */
	uint index = __umul24(blockIdx.x, blockDim.x) + threadIdx.x;
	if (index >= numAllMarkers)
		return;

	Real3 p = posRad[index];

	if (!(isfinite(p.x) && isfinite(p.y) && isfinite(p.z))) {
		printf("Error! particle position is NAN: thrown from SDKCollisionSystem.cu, calcHashD !\n");
		*isErrorD = true;
		return;
	}

	/* Check particle is inside the domain. */
	Real3 boxCorner = paramsD.worldOrigin;
	if (p.x < boxCorner.x || p.y < boxCorner.y || p.z < boxCorner.z) {
		printf("Out of Min Boundary, point %f %f %f, boundary min: %f %f %f. Thrown from SDKCollisionSystem.cu, calcHashD !\n",
				p.x, p.y, p.z,boxCorner.x, boxCorner.y, boxCorner.z);
		*isErrorD = true;
		return;
	}
	boxCorner = paramsD.worldOrigin + paramsD.boxDims;
	if (p.x > boxCorner.x || p.y > boxCorner.y || p.z > boxCorner.z) {
		printf(
				"Out of max Boundary, point %f %f %f, boundary max: %f %f %f. Thrown from SDKCollisionSystem.cu, calcHashD !\n",
				p.x, p.y, p.z, boxCorner.x, boxCorner.y, boxCorner.z);
		*isErrorD = true;
		return;
	}

	/* Get x,y,z bin index in grid */
	int3 gridPos = calcGridPos(p);
	/* Calculate a hash from the bin index */
	uint hash = calcGridHash(gridPos);

	/* Store grid hash */
	gridMarkerHashD[index] = hash;
	/* Store particle index associated to the hash we stored in gridMarkerHashD */
	gridMarkerIndexD[index] = index;
}

/**
 * @brief reorderDataAndFindCellStartD
 * @details See SDKCollisionSystem.cuh for more info
 */
__global__ void reorderDataAndFindCellStartD(uint* cellStartD, // output: cell start index
		uint* cellEndD,        // output: cell end index
		Real3* sortedPosRadD,  // output: sorted positions
		Real3* sortedVelMasD,  // output: sorted velocities
		Real4* sortedRhoPreMuD, uint* gridMarkerHashD, // input: sorted grid hashes
		uint* gridMarkerIndexD,      // input: sorted particle indices
		uint* mapOriginalToSorted, // mapOriginalToSorted[originalIndex] = originalIndex
		Real3* posRadD,           // input: sorted position array
		Real3* velMasD,           // input: sorted velocity array
		Real4* rhoPresMuD, uint numAllMarkers) {
	extern __shared__ uint sharedHash[];  // blockSize + 1 elements
	/* Get the particle index the current thread is supposed to be looking at. */
	uint index = __umul24(blockIdx.x, blockDim.x) + threadIdx.x;
	uint hash;
	/* handle case when no. of particles not multiple of block size */
	if (index < numAllMarkers) {
		hash = gridMarkerHashD[index];
		/* Load hash data into shared memory so that we can look at neighboring particle's hash
		 * value without loading two hash values per thread
		 */
		sharedHash[threadIdx.x + 1] = hash;

		if (index > 0 && threadIdx.x == 0) {
			/* first thread in block must load neighbor particle hash */
			sharedHash[0] = gridMarkerHashD[index - 1];
		}
	}

	__syncthreads();

	if (index < numAllMarkers) {
		/* If this particle has a different cell index to the previous particle then it must be
		 * the first particle in the cell, so store the index of this particle in the cell. As it
		 * isn't the first particle, it must also be the cell end of the previous particle's cell
		 */
		if (index == 0 || hash != sharedHash[threadIdx.x]) {
			cellStartD[hash] = index;
			if (index > 0)
				cellEndD[sharedHash[threadIdx.x]] = index;
		}

		if (index == numAllMarkers - 1) {
			cellEndD[hash] = index + 1;
		}

		/* Now use the sorted index to reorder the pos and vel data */
		uint originalIndex = gridMarkerIndexD[index];  // map sorted to original
		mapOriginalToSorted[index] = index;	// will be sorted outside. Alternatively, you could have mapOriginalToSorted[originalIndex] = index; without need to sort. But that is not thread safe
		Real3 posRad = FETCH(posRadD, originalIndex); // macro does either global read or texture fetch
		Real3 velMas = FETCH(velMasD, originalIndex); // see particles_kernel.cuh
		Real4 rhoPreMu = FETCH(rhoPresMuD, originalIndex);

		if (!(isfinite(posRad.x) && isfinite(posRad.y)
				&& isfinite(posRad.z))) {
			printf("Error! particle position is NAN: thrown from SDKCollisionSystem.cu, reorderDataAndFindCellStartD !\n");
		}
		if (!(isfinite(velMas.x) && isfinite(velMas.y)
				&& isfinite(velMas.z))) {
			printf("Error! particle velocity is NAN: thrown from SDKCollisionSystem.cu, reorderDataAndFindCellStartD !\n");
		}
		if (!(isfinite(rhoPreMu.x) && isfinite(rhoPreMu.y)
				&& isfinite(rhoPreMu.z) && isfinite(rhoPreMu.w))) {
			printf("Error! particle rhoPreMu is NAN: thrown from SDKCollisionSystem.cu, reorderDataAndFindCellStartD !\n");
		}
		sortedPosRadD[index] = posRad;
		sortedVelMasD[index] = velMas;
		sortedRhoPreMuD[index] = rhoPreMu;
	}
}

//--------------------------------------------------------------------------------------------------------------------------------

ChCollisionSystemFsi::ChCollisionSystemFsi(
	SphMarkerDataD * otherSortedSphMarkersD;
	ProximityDataD * otherMarkersProximityD, 
	SimParams* otherParamsH, 
	NumberOfObjects* otherNumObjects)
: sortedSphMarkersD(otherSortedSphMarkersD), markersProximityD(otherMarkersProximityD), paramsH(otherParamsH), numObjects(otherNumObjects) {
	sphMarkersD = NULL;
	this->setParameters(otherParamsH, otherNumObjects);
}
//--------------------------------------------------------------------------------------------------------------------------------

void ChCollisionSystemFsi::calcHash() {
	if (!(markersProximityD->gridMarkerHashD.size() == paramsH.numAllMarkers &&
		markersProximityD->gridMarkerIndexD.size() == paramsH.numAllMarkers)) {
		throw std::runtime_error ("Error! size error, calcHash!\n");
	}

	bool *isErrorH, *isErrorD;
	isErrorH = (bool *)malloc(sizeof(bool));
	cudaMalloc((void**) &isErrorD, sizeof(bool));
	*isErrorH = false;
	cudaMemcpy(isErrorD, isErrorH, sizeof(bool), cudaMemcpyHostToDevice);
	//------------------------------------------------------------------------
	/* Is there a need to optimize the number of threads used at once? */
	uint numThreads, numBlocks;
	computeGridSize(paramsH.numAllMarkers, 256, numBlocks, numThreads);
	/* Execute Kernel */
	calcHashD<<<numBlocks, numThreads>>>(U1CAST(markersProximityD->gridMarkerHashD),
			U1CAST(markersProximityD->gridMarkerIndexD), mR3CAST(sphMarkersD->posRadD),
			paramsH.numAllMarkers, isErrorD);

	/* Check for errors in kernel execution */
	cudaThreadSynchronize();
	cudaCheckError();
	//------------------------------------------------------------------------
	cudaMemcpy(isErrorH, isErrorD, sizeof(bool), cudaMemcpyDeviceToHost);
	if (*isErrorH == true) {
		throw std::runtime_error ("Error! program crashed in  calcHashD!\n");
	}
	cudaFree(isErrorD);
	free(isErrorH);
}

void ChCollisionSystemFsi::reorderDataAndFindCellStart() {

	if (!(markersProximityD->cellStartD.size() == paramsH.numCells &&
		markersProximityD->cellEndD.size() == paramsH.numCells)) {
		throw std::runtime_error ("Error! size error, reorderDataAndFindCellStart!\n");
	}

	thrust::fill(markersProximityD->cellStartD.begin(), markersProximityD->cellStartD.end(), 0);
	thrust::fill(markersProximityD->cellEndD.begin(), markersProximityD->cellEndD.end(), 0);

	uint numThreads, numBlocks;
	computeGridSize(paramsH.numAllMarkers, 256, numBlocks, numThreads); //?$ 256 is blockSize

	uint smemSize = sizeof(uint) * (numThreads + 1);
	reorderDataAndFindCellStartD<<<numBlocks, numThreads, smemSize>>>(
			U1CAST(markersProximityD->cellStartD), U1CAST(markersProximityD->cellEndD), mR3CAST(sortedSphMarkersD->posRadD),
			mR3CAST(sortedSphMarkersD->velMasD), mR4CAST(sortedSphMarkersD->rhoPreMuD),
			U1CAST(markersProximityD->gridMarkerHashD), U1CAST(markersProximityD->gridMarkerIndexD),
			U1CAST(markersProximityD->mapOriginalToSorted), mR3CAST(sphMarkersD->posRadD), mR3CAST(sphMarkersD->velMasD),
			mR4CAST(sphMarkersD->rhoPresMuD), paramsH.numAllMarkers);
	cudaThreadSynchronize();
	cudaCheckError();

	// unroll sorted index to have the location of original particles in the sorted arrays
	thrust::device_vector<uint> dummyIndex = gridMarkerIndexD;
	thrust::sort_by_key(dummyIndex.begin(), dummyIndex.end(),
			markersProximityD->mapOriginalToSorted.begin());
	dummyIndex.clear();
}

void ChCollisionSystemFsi::ArrangeData(SphMarkerDataD * otherSphMarkersD) {
	sphMarkersD = otherSphMarkerD;
	calcHash();
	thrust::sort_by_key(markersProximityD->gridMarkerHashD.begin(), markersProximityD->gridMarkerHashD.end(),
			markersProximityD->gridMarkerIndexD.begin());
	reorderDataAndFindCellStart();
}






