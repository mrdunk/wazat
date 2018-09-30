#ifndef WAZAT_FILTERS_H
#define WAZAT_FILTERS_H

#include <iostream>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <iomanip>
#include <algorithm>
#include <string.h>
#include <map>

#include "types.h"

/* Blur an image to smooth the detail. */
void blur(struct buffer& inputBuffer,
          const int width,
          const int height,
          const int gausKernelSize,
          const double gausSigma);

void getFeatures(struct buffer& inputBuffer,
                 std::vector<uint8_t>& featureBuffer,
                 const unsigned int width,
                 const unsigned int height,
                 int threshold,
                 int border);

void filterThin(std::vector<uint8_t>& featureBuffer, 
                const unsigned int width,
                const unsigned int height);

void merge(struct buffer& finalBuffer,
           std::vector<uint8_t>& featureBuffer,
           const unsigned int width,
           const unsigned int height);

void filterSmallFeatures(std::vector<uint8_t>& featureBuffer,
            const unsigned int width,
            const unsigned int height);

void filterHough(std::vector<uint8_t>& inputBuffer,
                 std::map<struct polarCoord, uint8_t>&  outputBuffer,
                 const unsigned int width,
                 const unsigned int height);

void mergeHough(struct buffer& finalBuffer,
                std::map<struct polarCoord, uint8_t>& houghBuffer,
                const unsigned int width,
                const unsigned int height);

#endif  // WAZAT_FILTERS_H
