#include "filters.h"

typedef std::vector<double> Array;
typedef std::vector<Array> Matrix;

Matrix getGaussian(const int size, const double sigma) {
  //std::cout << "getGaussian" << std::endl;
  assert(size % 2);  // Must be odd number.
  const int radius = (size - 1) / 2;
  Matrix kernel(size, Array(size));
  double sum = 0.0;
  int i, j;

  for (i = -radius; i <= radius; i++) {
    for (j = -radius; j <= radius; j++) {
      kernel[i + radius][j + radius] =
        exp(-(i*i + j*j) / (2*sigma*sigma)) / (2*M_PI * sigma*sigma);
      sum += kernel[i + radius][j + radius];
    }
  }

  //std::cout << std::endl;
  for (i = -radius; i <= radius; i++) {
    for (j = -radius; j <= radius; j++) {
      kernel[i + radius][j + radius] /= sum;
      //std::cout << std::setw(15)<< kernel[i + radius][j + radius];
    }
    //std::cout << std::endl;
  }

  return kernel;
}

void blur(struct buffer<uint8_t>& inputBuffer,
          const int width,
          const int height,
          const int gausKernelSize,
          const double gausSigma) {
  static struct buffer<uint8_t> tmpBuffer = {0};
  tmpBuffer.resize(inputBuffer.length);

  const int radius = (gausKernelSize - 1) / 2;
  static Matrix k = getGaussian(gausKernelSize, gausSigma);
  static int gausKernelSize_ = gausKernelSize;
  static double gausSigma_ = gausSigma;

  if(gausKernelSize_ != gausKernelSize || gausSigma_ != gausSigma) {
    gausKernelSize_ = gausKernelSize;
    gausSigma_ = gausSigma;
    k = getGaussian(gausKernelSize, gausSigma);
  }

  for(int y = radius; y < height - radius; y++) {
    for(int x = radius *3; x < (width - radius) * 3; x += 3) {
      for(int c = 0; c < 3; c++) {
        //assert(y * width * 3 + x + c >= 0);
        //assert(y * width * 3 + x + c < width * height * 3);
        ((uint8_t*)tmpBuffer.start)[y * width * 3 + x + c] = 0;

        for(int ky = -radius; ky <= radius; ky++) {
          for(int kx = -radius; kx <= radius; kx++) {
            const int address =
              (y + ky) * width * 3 + x + kx * 3 + c;
            //assert(address >= 0);
            //assert(address < width * height * 3);

            ((uint8_t*)tmpBuffer.start)[y * width * 3 + x + c] +=
              k[kx + radius][ky + radius] * ((uint8_t*)inputBuffer.start)[address];
          }
        }
        //assert(tempBuffer.start[y * width * 3 + x + c] >= 0);
        //assert(tempBuffer.start[y * width * 3 + x + c] <= 0xFF);
      }
    }
  }

  memcpy(inputBuffer.start, tmpBuffer.start, inputBuffer.length);
  //std::swap(tmpBuffer.start, inputBuffer.start);
}

void getFeatures(struct buffer<uint8_t>& inputBuffer,
                 std::vector<uint8_t>& featureBuffer,
                 const unsigned int width,
                 const unsigned int height,
                 int thresholdColour,
                 int thresholdBrightness,
                 int border) {

  featureBuffer.clear(); 
  if(featureBuffer.size() < inputBuffer.length / 3) {
    featureBuffer.resize(inputBuffer.length / 3);
  }
  
  for(unsigned int x = border; x < width -border; x++) {
    for(unsigned int y = border; y < height -border; y++) {
      int dxR = ((uint8_t*)inputBuffer.start)[3*(x + y * width) +0] -
                     ((uint8_t*)inputBuffer.start)[3*(x + y * width -border) +0];
      int dxG = ((uint8_t*)inputBuffer.start)[3*(x + y * width) +1] -
                     ((uint8_t*)inputBuffer.start)[3*(x + y * width -border) +1];
      int dxB = ((uint8_t*)inputBuffer.start)[3*(x + y * width) +2] -
                     ((uint8_t*)inputBuffer.start)[3*(x + y * width -border) +2];
      int dyR = ((uint8_t*)inputBuffer.start)[3*(x + y * width) +0] -
                     ((uint8_t*)inputBuffer.start)[3*(x + (y -border) * width) +0];
      int dyG = ((uint8_t*)inputBuffer.start)[3*(x + y * width) +1] -
                     ((uint8_t*)inputBuffer.start)[3*(x + (y -border) * width) +1];
      int dyB = ((uint8_t*)inputBuffer.start)[3*(x + y * width) +2] -
                     ((uint8_t*)inputBuffer.start)[3*(x + (y -border) * width) +2];

      if(abs(dxR - dxG) + abs(dxG - dxB) + abs(dxB - dxR) +
          abs(dyR - dyG) + abs(dyG - dyB) + abs(dyB - dyR) > thresholdColour) {
        if(abs(dxR) + abs(dxG) + abs(dxB) + abs(dyR) + abs(dyG) + abs(dyB) >
            thresholdBrightness) {
          featureBuffer[x + y * width] = 1;
          /*featureBuffer[x -1 + (y -1) * width] = 1;
          featureBuffer[x +0 + (y -1) * width] = 1;
          featureBuffer[x +1 + (y -1) * width] = 1;
          featureBuffer[x -1 + y * width] = 1;
          featureBuffer[x +1 + y * width] = 1;
          featureBuffer[x -1 + (y +1) * width] = 1;
          featureBuffer[x +0 + (y +1) * width] = 1;
          featureBuffer[x +1 + (y +1) * width] = 1;*/
        }
      }
    }
  }
}

void filterThin(std::vector<uint8_t>& featureBuffer, 
                const unsigned int width,
                const unsigned int height,
                const int trim,
                int maxIterations) {
  // http://fourier.eng.hmc.edu/e161/lectures/morphology/node2.html
  uint8_t tempBuffer[width * height] = {};
  int border = 1;
  int count = 1;
  int pass = 0;
  while(count && maxIterations) {
    count = 0;
    maxIterations--;
    pass++;
    for(unsigned int x = border; x < width - border; x++) {
      for(unsigned int y = border; y < height - border; y++) {
        if(featureBuffer[x + y * width]) {
          assert(featureBuffer[x + y * width] == 1);
          int n = 
            featureBuffer[(x - 1) + (y - 1) * width] +
            featureBuffer[(x + 0) + (y - 1) * width] +
            featureBuffer[(x + 1) + (y - 1) * width] +
            featureBuffer[(x - 1) + (y + 0) * width] +
            featureBuffer[(x + 1) + (y + 0) * width] +
            featureBuffer[(x - 1) + (y + 1) * width] +
            featureBuffer[(x + 0) + (y + 1) * width] +
            featureBuffer[(x + 1) + (y + 1) * width];
          int s = 0;
          s += (featureBuffer[(x - 1) + (y - 1) * width] == 0) &&
                featureBuffer[(x + 0) + (y - 1) * width];
          s += (featureBuffer[(x + 0) + (y - 1) * width] == 0) &&
                featureBuffer[(x + 1) + (y - 1) * width];
          s += (featureBuffer[(x + 1) + (y - 1) * width] == 0) &&
                featureBuffer[(x + 1) + (y + 0) * width];
          s += (featureBuffer[(x + 1) + (y + 0) * width] == 0) &&
                featureBuffer[(x + 1) + (y + 1) * width];
          s += (featureBuffer[(x + 1) + (y + 1) * width] == 0) &&
                featureBuffer[(x + 0) + (y + 1) * width];
          s += (featureBuffer[(x + 0) + (y + 1) * width] == 0) &&
                featureBuffer[(x - 1) + (y + 1) * width];
          s += (featureBuffer[(x - 1) + (y + 1) * width] == 0) &&
                featureBuffer[(x - 1) + (y + 0) * width];
          s += (featureBuffer[(x - 1) + (y + 0) * width] == 0) &&
                featureBuffer[(x - 1) + (y - 1) * width];

          if(pass %2){
            if((n >= trim) && (n < 7) && (s < 2) &&
                (featureBuffer[(x + 0) + (y - 1) * width] *
                 featureBuffer[(x + 1) + (y + 0) * width] *
                 featureBuffer[(x + 0) + (y + 1) * width] == 0) &&
                (featureBuffer[(x + 1) + (y + 0) * width] *
                 featureBuffer[(x + 0) + (y + 1) * width] *
                 featureBuffer[(x - 1) + (y + 0) * width] == 0)) {
              tempBuffer[x + y * width] = 0;
            } else {
              tempBuffer[x + y * width] = 1;
            }
          } else {
            if((n >= trim) && (n < 7) && (s < 2) &&
                (featureBuffer[(x + 0) + (y - 1) * width] *
                 featureBuffer[(x + 1) + (y + 0) * width] *
                 featureBuffer[(x - 1) + (y + 0) * width] == 0) &&
                (featureBuffer[(x + 0) + (y - 1) * width] *
                 featureBuffer[(x + 0) + (y + 1) * width] *
                 featureBuffer[(x - 1) + (y + 0) * width] == 0)) {
              tempBuffer[x + y * width] = 0;
            } else {
              tempBuffer[x + y * width] = 1;
            }
          }

        }
      }
    }
    for(unsigned int i = 0; i < width * height; i++) {
      if(featureBuffer[i] != tempBuffer[i]) {
        count++;
      }
      featureBuffer[i] = tempBuffer[i];
    }
  }
}

void merge(struct buffer<uint8_t>& finalBuffer,
           std::vector<uint8_t>& featureBuffer,
           const unsigned int width,
           const unsigned int height) {
  assert(width * height <= featureBuffer.size());
  for(unsigned int x = 0; x < width; x++) {
    for(unsigned int y = 0; y < height; y++) {
      if(featureBuffer[x + y * width]) {
        ((uint8_t*)finalBuffer.start)[(x + y * width) * 3 + 0] = 255;
        ((uint8_t*)finalBuffer.start)[(x + y * width) * 3 + 1] = 255;
        ((uint8_t*)finalBuffer.start)[(x + y * width) * 3 + 2] = 255;
      }
    }
  }
}

void filterSmallFeatures(std::vector<uint8_t>& featureBuffer,
            const unsigned int width,
            const unsigned int height) {
  unsigned int border = 10;

  unsigned int minBorder = 1;
  if(border > 1) {

    // TODO: Work out why the first loop of border behaves differently when
    // border > 1.
    minBorder = 2;
  }

  for(unsigned int x = border; x < width - border; x++) {
    for(unsigned int y = border; y < height - border; y++) {

      if(featureBuffer[x + y * width]) {
        int foundNeighbour = 0;
        for(unsigned int b = minBorder; b <= border; b++) {
          foundNeighbour = 0;
          unsigned int xxL = x - b;
          unsigned int xxR = x + b;
          unsigned int yyT = y - b;
          unsigned int yyB = y + b;
          for(unsigned int xx = x - b; xx <= x + b; xx++) {
            assert(xx + yyT * width != x + y * width);
            assert(xx + yyB * width != x + y * width);
            if(featureBuffer[xx + yyT * width] ||
                featureBuffer[xx + yyB * width]) {
              foundNeighbour = 1;
              break;
            }
          }
          for(unsigned int yy = y - b; yy <= y + b && !foundNeighbour; yy++) {
            if(featureBuffer[xxL + yy * width] ||
                featureBuffer[xxR + yy * width]) {
              foundNeighbour = 1;
              break;
            }
          }
          if(!foundNeighbour) {
            featureBuffer[x + y * width] = 0;
            break;
          }
        }
      }

    }
  }
}

void filterHough(std::vector<uint8_t>& inputBuffer,
                 struct buffer<uint16_t>& outputBuffer,
                 const unsigned int width,
                 const unsigned int height) {
  static const int16_t maxLineLen = sqrt(width * width + height * height);

  outputBuffer.resize(2 * maxLineLen * 360);
  outputBuffer.resize(2 * maxLineLen * 360);
  outputBuffer.clear();

  for(unsigned int x = 10; x < width -10; x++) {
    for(unsigned int y = 10; y < height -10; y++) {
      if(inputBuffer[x + y * width]) {
        for(int16_t a = -180; a < 180; a++) {
          int r = x * cos(M_PI * a / 180) + y * sin(M_PI * a / 180);
          int rOffset = r + maxLineLen;
          int16_t aOffset = a + 180;
          if(r < -maxLineLen || r >= maxLineLen) {
            std::cout << r << std::endl;
            assert(0);
          }
          uint16_t* value = &(outputBuffer.start[rOffset * 360 + aOffset]);
          if(*value < 0xffff -1) {
            (*value)++;
          }
        }
      }
    }
  }

  /*for(int rOffset = 0; rOffset < 2 * maxLineLen; rOffset++) {
    uint16_t* lastValue = nullptr;
    for(int16_t aOffset = 1; aOffset < 360; aOffset++) {
      uint16_t* value = &(tmpBuffer2.start[rOffset * 360 + aOffset]);
      if(lastValue && *lastValue > 170 && *lastValue > *value) {
        outputBuffer.start[rOffset * 360 + aOffset -1] = 1;
      } else {
        outputBuffer.start[rOffset * 360 + aOffset -1] = 0;
      }
      lastValue = value;
    }
  }*/
}

void dilateHough(struct buffer<uint16_t>& houghBuffer,
                  const unsigned int width,
                  const unsigned int height) {
  static const int16_t maxLineLen = sqrt(width * width + height * height);

  static struct buffer<uint16_t> tmpBuffer = {0};
  tmpBuffer.resize(2 * maxLineLen * 360);
  tmpBuffer.clear();

  for(size_t aOffset = 1; aOffset < 360 -1; aOffset++) {
    for(int rOffset = 1; rOffset < 2 * maxLineLen -1; rOffset++) {
      uint16_t* value = &(tmpBuffer.start[rOffset * 360 + aOffset]);
      *value = houghBuffer.start[rOffset * 360 + aOffset];

      if(houghBuffer.start[(rOffset -1) * 360 + aOffset -1] >= *value) {
        *value = houghBuffer.start[(rOffset -1) * 360 + aOffset -1];
      }
      if(houghBuffer.start[(rOffset +0) * 360 + aOffset -1] >= *value) {
        *value = houghBuffer.start[(rOffset +0) * 360 + aOffset -1];
      }
      if(houghBuffer.start[(rOffset +1) * 360 + aOffset -1] >= *value) {
        *value = houghBuffer.start[(rOffset +1) * 360 + aOffset -1];
      }
      if(houghBuffer.start[(rOffset -1) * 360 + aOffset +0] >= *value) {
        *value = houghBuffer.start[(rOffset -1) * 360 + aOffset +0];
      }
      if(houghBuffer.start[(rOffset +1) * 360 + aOffset +0] >= *value) {
        *value = houghBuffer.start[(rOffset +1) * 360 + aOffset +0];
      }
      if(houghBuffer.start[(rOffset -1) * 360 + aOffset +1] >= *value) {
        *value = houghBuffer.start[(rOffset -1) * 360 + aOffset +1];
      }
      if(houghBuffer.start[(rOffset +0) * 360 + aOffset +1] >= *value) {
        *value = houghBuffer.start[(rOffset +0) * 360 + aOffset +1];
      }
      if(houghBuffer.start[(rOffset +1) * 360 + aOffset +1] >= *value) {
        *value = houghBuffer.start[(rOffset +1) * 360 + aOffset +1];
      }
    }
  }
  memcpy(houghBuffer.start, tmpBuffer.start, houghBuffer.length * sizeof(uint16_t));
}

size_t erodeHough(struct buffer<uint16_t>& houghBuffer,
               const unsigned int width,
               const unsigned int height,
               const double threshold) {
  static const int16_t maxLineLen = sqrt(width * width + height * height);

  static struct buffer<uint16_t> tmpBuffer = {0};
  tmpBuffer.resize(2 * maxLineLen * 360);
  tmpBuffer.clear();

  size_t count = 0;

  for(size_t aOffset = 1; aOffset < 360 -1; aOffset++) {
    for(int rOffset = 1; rOffset < 2 * maxLineLen -1; rOffset++) {
      uint16_t center = houghBuffer.start[rOffset * 360 + aOffset];
      uint16_t tl = houghBuffer.start[(rOffset -1) * 360 + aOffset -1];
      uint16_t tc = houghBuffer.start[(rOffset +0) * 360 + aOffset -1];
      uint16_t tr = houghBuffer.start[(rOffset +1) * 360 + aOffset -1];
      uint16_t lc = houghBuffer.start[(rOffset -1) * 360 + aOffset +0];
      uint16_t rc = houghBuffer.start[(rOffset +1) * 360 + aOffset +0];
      uint16_t bl = houghBuffer.start[(rOffset -1) * 360 + aOffset +1];
      uint16_t bc = houghBuffer.start[(rOffset +0) * 360 + aOffset +1];
      uint16_t br = houghBuffer.start[(rOffset +1) * 360 + aOffset +1];
      bool tlb = tl >= center;
      bool tcb = tc >= center;
      bool trb = tr >= center;
      bool lcb = lc >= center;
      bool rcb = rc >= center;
      bool blb = bl >= center;
      bool bcb = bc >= center;
      bool brb = br >= center;

      if(center < threshold ||
          tl > center || tc > center || tr > center || lc > center ||
          rc > center || bl > center || bc > center || br > center) {
        tmpBuffer.start[rOffset * 360 + aOffset] = 0;
      } else {
        uint8_t transitionCount = 0;
        uint8_t setCount = tlb + tcb + trb + lcb + rcb + blb + bcb + brb;
        if(tlb != tcb) { transitionCount++; }
        if(tcb != trb) { transitionCount++; }
        if(trb != rcb) { transitionCount++; }
        if(rcb != brb) { transitionCount++; }
        if(brb != bcb) { transitionCount++; }
        if(bcb != blb) { transitionCount++; }
        if(blb != lcb) { transitionCount++; }
        if(lcb != tlb) { transitionCount++; }
        assert(transitionCount % 2 == 0);
        if(transitionCount == 2){
          if(setCount > 1) {
            tmpBuffer.start[rOffset * 360 + aOffset] = 0;
            count++;
          } else if(rcb || blb || bcb || brb) {
            // Lower end of line.
            tmpBuffer.start[rOffset * 360 + aOffset] = 0;
            count++;
          } else {
            // Upper end of line.
            tmpBuffer.start[rOffset * 360 + aOffset] = center;
          }
        } else {
          tmpBuffer.start[rOffset * 360 + aOffset] = center;
        }
      }
    }
  }
  memcpy(houghBuffer.start, tmpBuffer.start, houghBuffer.length * sizeof(uint16_t));
  return count;
}

void mergeHough(struct buffer<uint8_t>& finalBuffer,
                struct buffer<uint16_t>& houghBuffer,
                const unsigned int width,
                const unsigned int height,
                const double threshold) {
  const int16_t maxLineLen = sqrt(width * width + height * height);

  if(houghBuffer.length == 0) {
    return;
  }

  for(size_t aOffset = 0; aOffset < 360; aOffset++) {
    for(int rOffset = 0; rOffset < 2 * maxLineLen; rOffset++) {
      if(houghBuffer.start[rOffset * 360 + aOffset] > threshold) {
        int16_t a = aOffset - 180;
        int r = rOffset - maxLineLen;
        int xStart = cos(M_PI * a / 180) * r;
        int yStart = sin(M_PI * a / 180) * r;
        for(int16_t l = -maxLineLen; l < maxLineLen; l++) {
          uint16_t x = xStart + l * cos(M_PI * (a - 90) / 180);
          uint16_t y = yStart + l * sin(M_PI * (a - 90) / 180);
          if(x < width && y < height) {
            finalBuffer.start[(x + y * width) * 3 + 0] = 0;
            finalBuffer.start[(x + y * width) * 3 + 1] = 255;
            finalBuffer.start[(x + y * width) * 3 + 2] = 255;
          }
        }
      }
    }
  }
  houghBuffer.clear();
}


