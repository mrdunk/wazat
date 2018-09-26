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

enum IoMethod {
        IO_METHOD_READ,
        IO_METHOD_MMAP_SINGLE,
        IO_METHOD_MMAP_DOUBLE,
};

struct buffer {
  void   *start;
  size_t length;
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
  struct buffer buffers[2];
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
  void** buffer;
  size_t* bufferLength;

 public:
  unsigned int width;
  unsigned int height;

  File(const char* filename_, void** buffer_, size_t* bufferLength_);
  ~File();
  int grabFrame();
 private:
  void getImageProperties();
};


/* Save a JPEG formatted buffer to disk. */
void saveJpeg(void* inputBuffer, size_t inputBufferLength);

/* Take a JPEG formatted buffer and convert it to an array of pixel data. */
void parseJpeg(void* inputBuffer,
               size_t& inputBufferLength,
               std::vector<unsigned char>& outputBuffer,
               unsigned int& width,
               unsigned int& height);

void parseImage(void* inputBuffer,
                size_t& inputBufferLength,
                std::vector<unsigned char>& outputBuffer);

