#ifndef WAZAT_INPUT_H
#define WAZAT_INPUT_H

#include <iostream>
#include <fstream>
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
#include <linux/videodev2.h>
#include <libv4l2.h>

#include "types.h"

enum IoMethod {
        IO_METHOD_READ,
        IO_METHOD_MMAP_SINGLE,
        IO_METHOD_MMAP_DOUBLE,
};

void errno_exit(const char *s);

class Camera {
  int fd;
  const char* deviceName;
  enum IoMethod io;
  int captureWidth = 0;
  int captureHeight = 0;
  unsigned int numBuffers;
  void** cameraBuffer;
  size_t* bufferLength;
  struct buffer<uint8_t> buffers[2];
	struct v4l2_format format;
  struct v4l2_buffer bufferinfo;
  int imageformat;

 public:
  unsigned int width;
  unsigned int height;

	Camera(const char* deviceName_,
         enum IoMethod io_,
         void** cameraBuffer_,
         size_t* bufferLength_);

  ~Camera();

  int grabFrame();

 private:
  void checkCapabilities();

  void getImageProperties();

  void setFormat();

  void setBuffers();
  void setControl(int id, int value);

  void initMmap(unsigned int numBuffers_);

  void prepareBuffer();
};


class File {
  const char* filename;
  struct buffer<uint8_t> internalBuffer;
  struct buffer<uint8_t>* externalBuffer;

 public:
  unsigned int width;
  unsigned int height;

  File(const char* filename_, struct buffer<uint8_t>& buffer_);
  ~File();
  int grabFrame();
 private:
  void getImageProperties();
  void parseJpeg(struct buffer<uint8_t>* inputBuffer,
                 struct buffer<uint8_t>* outputBuffer,
                 unsigned int& width,
                 unsigned int& height);
};


/* Save a JPEG formatted buffer to disk. */
void saveJpeg(void* inputBuffer, size_t inputBufferLength);

#endif  // WAZAT_INPUT_H
