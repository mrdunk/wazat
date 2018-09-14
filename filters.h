#include <iostream>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <iomanip>

/* Blur an image to smooth the detail. */
void blur(std::vector<unsigned char>& inputBuffer,
          const int width,
          const int height);

void getFeatures(std::vector<unsigned char>& inputBuffer,
                 std::vector<unsigned char>& featureBuffer,
                 const unsigned int width,
                 const unsigned int height);

void filterThin(std::vector<unsigned char>& featureBuffer, 
                const unsigned int width,
                const unsigned int height);

void merge(std::vector<unsigned char>& inputBuffer,
           std::vector<unsigned char>& featureBuffer,
           const unsigned int width,
           const unsigned int height);

void filterSmallFeatures(std::vector<unsigned char>& featureBuffer,
            const unsigned int width,
            const unsigned int height);

