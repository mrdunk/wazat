#include <iostream>
#include <vector>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <jpeglib.h>
#include <curses.h>

void errno_exit(const char *s);

class DisplaySdl {
  void* inputBuffer;
  unsigned int* inputBufferLength;
  SDL_Surface* frame;
  SDL_RWops* bufferStream;
  SDL_Surface* screen;
  SDL_Rect position;
  SDL_Event events;
  int firstFrameRead;

 public:
  DisplaySdl(void* inputBuffer_, unsigned int* inputBufferLength_);

  ~DisplaySdl();

  void setBuffer(void* inputBuffer_, unsigned int* inputBufferLength_);

  int update();

 private:
  void displayInit(int width, int height);

  void displayCleanup();
};

class DisplayAsci {
  std::vector<unsigned char>* inputBuffer;
  unsigned int width;
  unsigned int height;

 public:
  DisplayAsci(std::vector<unsigned char>& inputBuffer_,
              unsigned int width_, unsigned int height_);

  void setBuffer(std::vector<unsigned char>& inputBuffer_,
                 unsigned int width_, 
                 unsigned int height_);

  ~DisplayAsci();

  int update();

 private:
  void cursesInit();

  void cursesCleanup();
};

void makeJpeg(std::vector<unsigned char>& inputBuffer,
              void** outputBuffer, unsigned int& outputBufferLength,
              unsigned int width, unsigned int height);

