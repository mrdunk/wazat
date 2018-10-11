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
void blur(struct buffer<uint8_t>& inputBuffer,
          const int width,
          const int height,
          const int gausKernelSize,
          const double gausSigma);

void getFeatures(struct buffer<uint8_t>& inputBuffer,
                 std::vector<uint8_t>& featureBuffer,
                 const unsigned int width,
                 const unsigned int height,
                 int thresholdColour,
                 int thresholdBrightness,
                 int border);

void filterThin(std::vector<uint8_t>& featureBuffer, 
                const unsigned int width,
                const unsigned int height,
                const int trim,
                int maxIterations);

void merge(struct buffer<uint8_t>& finalBuffer,
           std::vector<uint8_t>& featureBuffer,
           const unsigned int width,
           const unsigned int height);

void filterSmallFeatures(std::vector<uint8_t>& featureBuffer,
            const unsigned int width,
            const unsigned int height);

void filterHough(std::vector<uint8_t>& inputBuffer,
                 struct buffer<uint16_t>& outputBuffer,
                 const unsigned int width,
                 const unsigned int height);

void dilateHough(struct buffer<uint16_t>& houghBuffer,
                  const unsigned int width,
                  const unsigned int height);

size_t erodeHough(struct buffer<uint16_t>& houghBuffer,
               const unsigned int width,
               const unsigned int height,
               const double threshold);

void mergeHough(struct buffer<uint8_t>& finalBuffer,
                struct buffer<uint16_t>& outputBuffer,
                const unsigned int width,
                const unsigned int height,
                const double threshold);

#endif  // WAZAT_FILTERS_H
