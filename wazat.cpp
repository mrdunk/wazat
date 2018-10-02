#include "inputs.h"
#include "outputs.h"
#include "filters.h"
#include "config.h"
#include "types.h"

/* http://jwhsmith.net/2014/12/capturing-a-webcam-stream-using-v4l2/ 
 *
 * sudo apt install libjpeg-dev libsdl1.2-dev libsdl-image1.2-dev libv4l-dev
 *
 * g++ -std=c++11 -g -Wall inputs.cpp outputs.cpp filters.cpp config.cpp wazat.cpp -lSDL -lSDL_image -ljpeg -lmenu -lcurses -lv4l2 -O3
 * */

#define CAMERA

int main()
{
  int keyPress = 0;
  int run = 1;
  struct buffer<uint8_t> inputBuffer = {0};
  struct buffer<uint8_t> outputJpegBuffer = {0};
  std::vector<uint8_t> featureBuffer;
  //std::map<struct polarCoord, uint8_t> houghBuffer;
  struct buffer<uint16_t> houghBuffer = {0};

  #ifdef CAMERA
	const char* deviceName = "/dev/video0";
  Camera inputDevice(deviceName,
                     IO_METHOD_MMAP_SINGLE,
                     (void**)(&(inputBuffer.start)),
                     &(inputBuffer.length));
  #else
  // const char* filename = "testData/im1small.jpg";
  // const char* filename = "testData/CHECKERBOARD.jpg";
  // const char* filename = "testData/310FlbGm2qL.jpg";
  const char* filename = "testData/CountingTriangles.jpg";
  File inputDevice(filename, inputBuffer);
  #endif
  
  DisplaySdl displayProcessed(outputJpegBuffer);
  DisplayAsci displayParsed(inputBuffer,
                            inputDevice.width,
                            inputDevice.height);

  timeout(0);  // Non blocking keyboard read.
  while(run){
    if(!inputDevice.grabFrame()){
      break;
    }
    //saveJpeg(inputBuffer, inputBufferLength);
    //run &= displayRaw.update();
    
    displayParsed.update(keyPress);

    if(config.blurGaussian.enabled){    
      blur(inputBuffer,
           inputDevice.width,
           inputDevice.height,
           config.blurGaussian.values[0].value,
           config.blurGaussian.values[1].value);
    }
    getFeatures(inputBuffer,
                featureBuffer,
                inputDevice.width,
                inputDevice.height,
                config.getFeatures.values[0].value,
                config.getFeatures.values[1].value);
    if(config.filterThin.enabled){
      filterThin(featureBuffer, inputDevice.width, inputDevice.height);
    }
    if(config.filterSmallFeatures.enabled){
      filterSmallFeatures(featureBuffer, inputDevice.width, inputDevice.height);
    }
    filterHough(featureBuffer, houghBuffer, inputDevice.width, inputDevice.height);
    memset(inputBuffer.start, 0, inputBuffer.length);
    merge(inputBuffer,
          featureBuffer,
          inputDevice.width,
          inputDevice.height);
    mergeHough(inputBuffer,
          houghBuffer,
          inputDevice.width,
          inputDevice.height);

    makeJpeg(inputBuffer,
             outputJpegBuffer,
             inputDevice.width,
             inputDevice.height);

    keyPress = getch();
    run &= displayProcessed.update(keyPress);
    if(keyPress == 'q' || keyPress == 'Q') {
      run = 0;
    }
  }

  #ifndef CAMERA
  inputBuffer.destroy();
  #endif
  outputJpegBuffer.destroy();
  houghBuffer.destroy();
  featureBuffer.clear();
}
