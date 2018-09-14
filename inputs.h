#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <algorithm>


enum IoMethod {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};


void errno_exit(const char *s);

class Camera {
  int fd;
  const char* deviceName;
  enum IoMethod io;
  int captureWidth = 0;
  int captureHeight = 0;
  void* bufferStart;
  void** cameraBuffer;
  unsigned int* bufferLength;
	struct v4l2_format format;
  struct v4l2_buffer bufferinfo;

 public:
  unsigned int width;
  unsigned int height;

	Camera(const char* deviceName_,
         enum IoMethod io_,
         void** cameraBuffer_,
         unsigned int* bufferLength_);

  ~Camera();

  int grabFrame();

 private:
  void checkCapabilities();

  void getImageProperties();

  void setFormat();

  void setBuffers();
  void setControl(int id, int value);

  void initMmap();

  void prepareBuffer();
};



/* Save a JPEG formatted buffer to disk. */
void saveJpeg(void* inputBuffer, unsigned int inputBufferLength);

/* Take a JPEG formatted buffer and convert it to an array of pixel data. */
void parseJpeg(void* inputBuffer,
               unsigned int& inputBufferLength,
               std::vector<unsigned char>& outputBuffer,
               unsigned int& width,
               unsigned int& height);

