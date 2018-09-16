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

  while(run){
    if(!inputDevice.grabFrame()){
      break;
    }
    //saveJpeg(cameraBuffer, cameraBufferLength);
    //run &= displayRaw.update();
    parseJpeg(cameraBuffer, cameraBufferLength, parsedBuffer, width, height);
    displayParsed.update(keyPress);

    if(config.blurGaussian.value){    
      blur(parsedBuffer, width, height);
    }
    getFeatures(parsedBuffer, featureBuffer, width, height);
    if(config.filterThin.value){
      filterThin(featureBuffer, width, height);
    }
    if(config.filterSmallFeatures.value){
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
