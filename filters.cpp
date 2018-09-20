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

void blur(std::vector<unsigned char>& inputBuffer,
           const int width,
           const int height,
           const int gausKernelSize,
           const double gausSigma) {
  unsigned char tempBuffer[width * height * 3] = {};

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

        for(int ky = -radius; ky <= radius; ky++) {
          for(int kx = -radius; kx <= radius; kx++) {
            const int address =
              (y + ky) * width * 3 + x + kx * 3 + c;
            //assert(address >= 0);
            //assert(address < width * height * 3);

            //tempBuffer[y * width * 3 + x + c] += inputBuffer[address] / 9;

            tempBuffer[y * width * 3 + x + c] +=
              k[kx + radius][ky + radius] * inputBuffer[address];
          }
        }
        //assert(tempBuffer[y * width * 3 + x + c] >= 0);
        //assert(tempBuffer[y * width * 3 + x + c] <= 0xFF);
      }
    }
  }

  // TODO Profile these options for the fastest.
  //std::copy_n(tempBuffer, width * height * 3, inputBuffer.data());
  //memcpy(inputBuffer, tempBuffer, width * height * 3);
  for(int i = 0; i < width * height * 3; i++) {
    inputBuffer[i] = tempBuffer[i];
  }
}

void getFeatures(std::vector<unsigned char>& inputBuffer,
                 std::vector<unsigned char>& featureBuffer,
                 const unsigned int width,
                 const unsigned int height,
                 int threshold,
                 int border) {

  featureBuffer.clear(); 
  if(featureBuffer.size() < inputBuffer.size()) {
    featureBuffer.resize(inputBuffer.size());
  }
  
  for(unsigned int x = border; x < width -border; x++) {
    for(unsigned int y = border; y < height -border; y++) {
      int dxR = (int)inputBuffer[3*(x + y * width) +0] -
                     inputBuffer[3*(x + y * width -border) +0];
      int dxG = (int)inputBuffer[3*(x + y * width) +1] -
                     inputBuffer[3*(x + y * width -border) +1];
      int dxB = (int)inputBuffer[3*(x + y * width) +2] -
                     inputBuffer[3*(x + y * width -border) +2];
      int dyR = (int)inputBuffer[3*(x + y * width) +0] -
                     inputBuffer[3*(x + (y -border) * width) +0];
      int dyG = (int)inputBuffer[3*(x + y * width) +1] -
                     inputBuffer[3*(x + (y -border) * width) +1];
      int dyB = (int)inputBuffer[3*(x + y * width) +2] -
                     inputBuffer[3*(x + (y -border) * width) +2];

      if(abs(dxR) + abs(dxG) + abs(dxB) + abs(dyR) + abs(dyG) + abs(dyB) >
          threshold * 2) {
        featureBuffer[x + y * width]++;
      } else if(abs(dxR) > threshold || abs(dxG) > threshold || abs(dxB) >
          threshold || abs(dyR) > threshold ||
          abs(dyG) > threshold || abs(dyB) > threshold) {
        featureBuffer[x + y * width]++;
      }
    }
  }
}

void filterThin(std::vector<unsigned char>& featureBuffer, 
                const unsigned int width,
                const unsigned int height) {
  // http://fourier.eng.hmc.edu/e161/lectures/morphology/node2.html
  unsigned char tempBuffer[width * height] = {};
  int border = 1;
  int count = 1;
  int pass = 0;
  while(count) {
    count = 0;
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
            if((n > 1) && (n < 7) && (s < 2) &&
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
            //tempBuffer[x + y * width] = (n == 1) || (n >= 7) || (s >= 2);
          } else {
            if((n > 1) && (n < 7) && (s < 2) &&
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

void merge(std::vector<unsigned char>& inputBuffer,
           std::vector<unsigned char>& featureBuffer,
           const unsigned int width,
           const unsigned int height) {
  for(unsigned int x = 0; x < width; x++) {
    for(unsigned int y = 0; y < height; y++) {
      if(featureBuffer[x + y * width]) {
        inputBuffer[(x + y * width) * 3 + 0] = 255;
        inputBuffer[(x + y * width) * 3 + 1] = 255;
        inputBuffer[(x + y * width) * 3 + 2] = 255;
      }
    }
  }
}

void filterSmallFeatures(std::vector<unsigned char>& featureBuffer,
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
