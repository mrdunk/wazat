#include "filters.h"

typedef std::vector<double> Array;
typedef std::vector<Array> Matrix;

Matrix getGaussian(int height, int width, double sigma) {
  std::cout << "getGaussian" << std::endl;
  Matrix kernel(height, Array(width));
  double sum = 0.0;
  int i, j;

  for (i=0; i < height; i++) {
    for (j=0; j < width; j++) {
      kernel[i][j] = exp(-(i*i + j*j) / (2*sigma*sigma)) / (2*M_PI * sigma*sigma);
      sum += kernel[i][j];
    }
  }

  std::cout << std::endl;
  for (i=0; i < height; i++) {
    for (j=0; j < width; j++) {
      kernel[i][j] /= sum;
      std::cout << std::setw(15)<< kernel[i][j];
    }
    std::cout << std::endl;
  }

  return kernel;
}

void gaussianKernel(std::vector<double>& kernel, int size) {
  const int radius = (size + 1) / 2;
  kernel.resize(size * size);
  Matrix k = getGaussian(radius, radius, (double)1);


  std::cout << std::endl;
  for (int i = 0; i < radius; ++i) {
    for (int j = 0; j < radius; ++j) { 
      kernel[(-i + radius -1) * size + -j + radius -1] = k[i][j];
      kernel[(-i + radius -1) * size + j + radius -1] = k[i][j];
      kernel[(i + radius -1) * size + -j + radius -1] = k[i][j];
      kernel[(i + radius -1) * size + j + radius -1] = k[i][j];
      std::cout << std::setw(15) << k[i][j];
    }
    std::cout << std::endl;
  }

  std::cout << std::endl;
  double sum = 0.0;
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) { 
      sum += kernel[i * size + j];
      std::cout << std::setw(15)<< kernel[i * size + j];
    }
    std::cout << std::endl;
  }

  std::cout << std::endl;
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) { 
      kernel[i * size + j] /= sum;
      std::cout << std::setw(15)<< kernel[i * size + j];
    }
    std::cout << std::endl;
  }

  /* assert(size == 3);
  const float k[3][3] = {
    {1.0/16, 1.0/8, 1.0/16},
    {1.0/8, 1.0/4, 1.0/8},
    {1.0/16, 1.0/8, 1.0/16}
  };
  for(int ky = 0; ky < size; ky++){
    for(int kx = 0; kx < size; kx++){
      kernel[kx * ky] = k[kx][ky];
    }
  }*/
}

void blur2(std::vector<unsigned char>& inputBuffer,
          const int width,
          const int height) {
  unsigned char tempBuffer[width * height * 3] = {};

  /*const int kernelSize = 3; // Must be odd.
  assert(kernelSize % 2);
  const int kernelRadius = (kernelSize - 1) / 2;

  static std::vector<double> kernel;
  if(! kernel.size()) {
    gaussianKernel(kernel, kernelSize);
  }*/

  int radius = 2;
  static Matrix k = getGaussian(radius, radius, (double)1);

  /*for(int y = kernelRadius; y < height - kernelRadius; y++) {
    for(int x = kernelRadius *3; x < (width - kernelRadius) * 3; x += 3) {
      for(int c = 0; c < 3; c++) {
        assert(y * width * 3 + x + c >= 0);
        assert(y * width * 3 + x + c < width * height * 3);
 
        for(int ky = 0; ky < kernelSize; ky++){
          for(int kx = 0; kx < kernelSize; kx++){
            const int address =
              (y + ky - kernelRadius) * width * 3 + x + (kx - kernelRadius) *3 + c;
            assert(address >= 0);
            assert(address < width * height * 3);
            tempBuffer[y * width * 3 + x + c] +=
              kernel[kx * ky] * inputBuffer[address];
          }
        }

      }
    }
  } */
  for(int y = radius; y < height - radius; y++) {
    for(int x = radius *3; x < (width - radius) * 3; x += 3) {
      for(int c = 0; c < 3; c++) {
        assert(y * width * 3 + x + c >= 0);
        assert(y * width * 3 + x + c < width * height * 3);
 
        for(int ky = 0; ky < radius; ky++) {
          for(int kx = 0; kx < radius; kx++) {
            const int address =
              (y + ky) * width * 3 + x + kx *3 + c;
            assert(address >= 0);
            assert(address < width * height * 3);

            //tempBuffer[y * width * 3 + x + c] = inputBuffer[address];

            tempBuffer[y * width * 3 + x + c] +=
              k[kx][ky] * inputBuffer[address];
          }
        }
        assert(tempBuffer[y * width * 3 + x + c] >= 0);
        assert(tempBuffer[y * width * 3 + x + c] <= 0xFF);
      }
    }
  }
  for(int i = 0; i < width * height * 3; i++) {
    inputBuffer[i] = tempBuffer[i];
  }
}

void blur(std::vector<unsigned char>& inputBuffer,
          const int width,
          const int height) {
  unsigned char tempBuffer[width * height * 3] = {};

  const int kernelSize = 3; // Must be odd.
  assert(kernelSize % 2);
  const int kernelRadius = (kernelSize - 1) / 2;

  static std::vector<double> kernel;
  if(! kernel.size()) {
    gaussianKernel(kernel, kernelSize);
  }

  for(int y = kernelRadius; y < height - kernelRadius; y++) {
    for(int x = kernelRadius *3; x < (width - kernelRadius) * 3; x += 3) {
      for(int c = 0; c < 3; c++) {
        assert(y * width * 3 + x + c >= 0);
        assert(y * width * 3 + x + c < width * height * 3);
 
        for(int ky = 0; ky < kernelSize; ky++){
          for(int kx = 0; kx < kernelSize; kx++){
            const int address =
              (y + ky - kernelRadius) * width * 3 + x + (kx - kernelRadius) *3 + c;
            assert(address >= 0);
            assert(address < width * height * 3);
            tempBuffer[y * width * 3 + x + c] +=
              kernel[kx * ky] * inputBuffer[address];
          }
        }

      }
    }
  }
  for(int i = 0; i < width * height * 3; i++) {
    inputBuffer[i] = tempBuffer[i];
  }
}

void getFeatures(std::vector<unsigned char>& inputBuffer,
                 std::vector<unsigned char>& featureBuffer,
                 const unsigned int width,
                 const unsigned int height) {
  int threshold = 50;
  int border = 5;

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
  int border = 1;
  std::vector<unsigned char> tmpBuffer;

  // Thin lines in x direction.
  unsigned int start = 0;
  for(unsigned int y = border; y < height - border; y++) {
    start = 0;
    for(unsigned int x = border; x < width - border; x++) {
      if(featureBuffer[x + y * width]) {
        if(!start) {
          start = x;
        }
      } else {
        if(start) {
          unsigned int mid = (start + x) / 2;
          for(unsigned int xx = start; xx <= x; xx++) {
            featureBuffer[xx + y * width] = 0;
          }
          featureBuffer[mid + y * width] = 1;
          start = 0;
        }
      }
    }
  }

  // Thin lines in y direction.
  for(unsigned int x = border; x < width - border; x++) {
    start = 0;
    for(unsigned int y = border; y < height - border; y++) {
      if(featureBuffer[x + y * width]) {
        if(!start) {
          start = y;
        }
      } else {
        if(start) {
          unsigned int mid = (start + y) / 2;
          for(unsigned int yy = start; yy <= y; yy++) {
            featureBuffer[x + yy * width] = 1;
          }
          featureBuffer[x + mid * width] = 1;
          start = 0;
        }
      }
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
