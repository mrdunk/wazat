#include "inputs.h"
#include "outputs.h"
#include "filters.h"
#include "config.h"

/* http://jwhsmith.net/2014/12/capturing-a-webcam-stream-using-v4l2/ 
 *
 * sudo apt install libjpeg-dev libsdl1.2-dev libsdl-image1.2-dev
 *
 * g++ -std=c++11 -g -Wall inputs.cpp outputs.cpp filters.cpp wazat.cpp -lSDL -lSDL_image -ljpeg -lcurses
 * */


int main()
{
  int keyPress;
  int run = 1;
	const char* deviceName = "/dev/video0";
  void* cameraBuffer = NULL;
  unsigned int cameraBufferLength = 0;
  std::vector<unsigned char> parsedBuffer;
  std::vector<unsigned char> featureBuffer;
  unsigned int width = 0;
  unsigned int  height = 0;

  Camera inputDevice(deviceName,
                     IO_METHOD_MMAP,
                     &cameraBuffer,
                     &cameraBufferLength);
  
  unsigned int outputJpegBufferEstimatedLength =
    inputDevice.width * inputDevice.height * 3;
  void* outputJpegBuffer = new unsigned char[outputJpegBufferEstimatedLength];

  //DisplaySdl displayRaw(cameraBuffer, &cameraBufferLength);
  DisplaySdl displayProcessed(outputJpegBuffer, &outputJpegBufferEstimatedLength);
  DisplayAsci displayParsed(parsedBuffer, inputDevice.width, inputDevice.height);

  timeout(0);  // Non blocking keyboard read.
  while(run){
    if(!inputDevice.grabFrame()){
      break;
    }
    //saveJpeg(cameraBuffer, cameraBufferLength);
    //run &= displayRaw.update();
    parseJpeg(cameraBuffer, cameraBufferLength, parsedBuffer, width, height);
    displayParsed.update(keyPress);

    if(config.blurGaussian.enabled){    
      blur(parsedBuffer,
           width,
           height,
           config.blurGaussian.values[0].value,
           config.blurGaussian.values[1].value);
    }
    getFeatures(parsedBuffer,
                featureBuffer,
                width,
                height,
                config.getFeatures.values[0].value,
                config.getFeatures.values[1].value);
    if(config.filterThin.enabled){
      filterThin(featureBuffer, width, height);
    }
    if(config.filterSmallFeatures.enabled){
      filterSmallFeatures(featureBuffer, width, height);
    }
    merge(parsedBuffer, featureBuffer, width, height);

    makeJpeg(parsedBuffer, 
             &outputJpegBuffer,
             outputJpegBufferEstimatedLength,
             width, height);

    keyPress = getch();
    run &= displayProcessed.update(keyPress);
    if(keyPress == 'q' || keyPress == 'Q') {
      run = 0;
    }
  }

  delete[] (unsigned char*)outputJpegBuffer;
}
