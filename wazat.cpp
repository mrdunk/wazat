#include <iostream>
#include <cstdlib>
#include <cstring>
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
#include <curses.h>
#include <stdlib.h>
#include <algorithm>

/* http://jwhsmith.net/2014/12/capturing-a-webcam-stream-using-v4l2/ 
 *
 * sudo apt install libjpeg-dev libsdl1.2-dev libsdl-image1.2-dev
 *
 * g++ wazat.cpp  -std=c++11 -Wall -lSDL -lSDL_image -ljpeg
 * */


enum IoMethod {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

int xioctl(int fh, int request, void *arg) {
  int r;

  do {
    r = ioctl(fh, request, arg);
  } while (-1 == r && EINTR == errno);

  return r;
}

static void errno_exit(const char *s) {
  std::cout << "Error: " << s << " : " << errno << " : " <<  strerror(errno) << "\n";
  exit(EXIT_FAILURE);
}

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
	Camera(const char* deviceName_, enum IoMethod io_,
         void** cameraBuffer_, unsigned int* bufferLength_){
    deviceName = deviceName_;
    io = io_;
    cameraBuffer = cameraBuffer_;
    bufferLength = bufferLength_;

    // Open devce.
    if((fd = open(deviceName_, O_RDWR)) < 0){
      std::cout << "Could not open " << deviceName << "\n";
      exit(EXIT_FAILURE);
    }
    std::cout << "Opened " << deviceName << "\n";

    checkCapabilities();
    setFormat();
    setBuffers();
    prepareBuffer();
	}

  ~Camera(){
    // Deactivate streaming
    enum v4l2_buf_type type;
    switch (io) {
      case IO_METHOD_READ:
        /* Nothing to do. */
        break;
      case IO_METHOD_MMAP:
      case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type)){
          errno_exit("VIDIOC_STREAMOFF");
        }
        break;
    }

    std::cout << "Closing " << deviceName << "\n";
    close(fd);
  }

  int grabFrame(){
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = {0};
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    int r = select(fd+1, &fds, NULL, NULL, &tv);
    if(0 == r) {
      std::cout << "Waiting for Frame" << std::endl;
      return 0;
    }

    // The buffer's waiting in the outgoing queue.
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &bufferinfo)) {
      switch (errno) {
        case EAGAIN:
          return 0;
        case EIO:
          /* Could ignore EIO, see spec. */
          /* fall through */
        default:
          errno_exit("VIDIOC_DQBUF");
      }
    }

    *bufferLength = bufferinfo.length;

    // Put the buffer in the incoming queue
    if (-1 == xioctl(fd, VIDIOC_QBUF, &bufferinfo)){
      errno_exit("VIDIOC_QBUF");
    }

    return 1;
  }

 private:
  void checkCapabilities(){
    v4l2_capability capabilities;
    
    // Check device v4l2 compatible.
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &capabilities)) {
      if (EINVAL == errno) {
        std::cout << "Error: " << deviceName << " is not a V4L2 device.\n";
        exit(EXIT_FAILURE);
      } else {
        errno_exit("VIDIOC_QUERYCAP");
      }
    }

    if(!(capabilities.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
      std::cout << "Error: The device does not handle single-planar video capture.\n";
      exit(EXIT_FAILURE);
    }

    switch (io) {
      case IO_METHOD_READ:
        std::cout << "Using IO_METHOD_READ\n";
        if (!(capabilities.capabilities & V4L2_CAP_READWRITE)) {
          std::cout << "Error: " << deviceName << " does not support read i/o\n";
          exit(EXIT_FAILURE);
        }
        break;

      case IO_METHOD_MMAP:
        std::cout << "Using IO_METHOD_MMAP\n";
        if (!(capabilities.capabilities & V4L2_CAP_STREAMING)) {
          std::cout << "Error: " << deviceName << " does not support streaming i/o\n";
          exit(EXIT_FAILURE);
        }
        break;
      case IO_METHOD_USERPTR:
        std::cout << "Using IO_METHOD_USERPTR\n";
        if (!(capabilities.capabilities & V4L2_CAP_STREAMING)) {
          std::cout << "Error: " << deviceName << " does not support streaming i/o\n";
          exit(EXIT_FAILURE);
        }
        break;
    }
  }

  void setFormat(){
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = 800;
    format.fmt.pix.height = 600;

    if(xioctl(fd, VIDIOC_S_FMT, &format) < 0){
      errno_exit("VIDIOC_S_FMT");
    }

    /* Buggy driver paranoia. */
    unsigned int min = format.fmt.pix.width * 2;
    if (format.fmt.pix.bytesperline < min){
      format.fmt.pix.bytesperline = min;
    }
    min = format.fmt.pix.bytesperline * format.fmt.pix.height;
    if (format.fmt.pix.sizeimage < min){
      format.fmt.pix.sizeimage = min;
    }

    captureWidth = format.fmt.pix.width;
    captureHeight = format.fmt.pix.height;
    std::cout << "Image width set to " << captureWidth << " by device " << deviceName << ".\n";
    std::cout << "Image height set to " << captureHeight << " by device " << deviceName << ".\n";
  }

  void setBuffers(){
    switch (io) {
      case IO_METHOD_READ:
        //init_read(format.fmt.pix.sizeimage);
        assert(false && "TODO");
        break;

      case IO_METHOD_MMAP:
        initMmap();
        break;

      case IO_METHOD_USERPTR:
        //init_userp(format.fmt.pix.sizeimage);
        assert(false && "TODO");
        break;
    }


    // "$ v4l2-ctl -l" lets us see what our camera is capable of (and set to).
    setControl(V4L2_CID_BRIGHTNESS, 0);
    setControl(V4L2_CID_CONTRAST, 0);
    setControl(V4L2_CID_SATURATION, 64);
    setControl(V4L2_CID_GAIN, 1);
    setControl(V4L2_CID_AUTO_WHITE_BALANCE, 1);
    setControl(V4L2_CID_BACKLIGHT_COMPENSATION, 3);
  }

  void setControl(int id, int value){
    struct v4l2_queryctrl queryctrl;
    queryctrl.id = id;
    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
      if (errno != EINVAL)
        exit(EXIT_FAILURE);
      else
      {
        std::cerr << "ERROR :: Unable to set property (NOT SUPPORTED)\n";
        exit(EXIT_FAILURE);
      }
    }
    else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
    {
      std::cout << "ERROR :: Unable to set property (DISABLED).\n";
      exit(EXIT_FAILURE);
    }
    else
    {
      struct v4l2_control control;
      control.id = queryctrl.id;
      control.value = value;

      if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control))
        exit(EXIT_FAILURE);
      std::cout << "Successfully set property." << std::endl;

    }
  }

  void initMmap(){
    std::cout << "Using IO_METHOD_MMAP\n";

    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 1;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &bufrequest)) {
      if (EINVAL == errno) {
        std::cout << "Error: " << deviceName << " does not support memory mapping\n";
        exit(EXIT_FAILURE);
      } else {
        errno_exit("VIDIOC_REQBUFS");
      }
    }

    struct v4l2_buffer bufferinfo = {0};
    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    if(ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo) < 0){
      errno_exit("VIDIOC_QUERYBUF");
    }

    bufferStart = mmap(
        NULL,
        bufferinfo.length,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        bufferinfo.m.offset
        );
    *cameraBuffer = bufferStart;

    if(bufferStart == MAP_FAILED){
      errno_exit("mmap");
    }

    memset(bufferStart, 0, bufferinfo.length);
  }

  void prepareBuffer(){
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0; /* Queueing buffer index 0. */

    // Put the buffer in the incoming queue.
    // For some hardware this needs done before activating streaming.
    if (-1 == xioctl(fd, VIDIOC_QBUF, &bufferinfo)){
      errno_exit("VIDIOC_QBUF");
    }

    // Activate streaming
    int type = bufferinfo.type;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)){
      errno_exit("VIDIOC_STREAMON");
    }
  }
};


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
  DisplaySdl(void* inputBuffer_, unsigned int* inputBufferLength_) {
    firstFrameRead = 0;
    setBuffer(inputBuffer_, inputBufferLength_);
  }

  ~DisplaySdl() {
    displayCleanup();
  }

  void setBuffer(void* inputBuffer_, unsigned int* inputBufferLength_) {
    inputBuffer = inputBuffer_;
    inputBufferLength = inputBufferLength_;
  }


  int update(){
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, (unsigned char*)inputBuffer, *inputBufferLength);

    jpeg_read_header(&cinfo, TRUE);
    // std::cout << cinfo.image_width << "," << cinfo.image_height << "," << cinfo.num_components << "\n";
    if(! firstFrameRead) {
      firstFrameRead = 1;
      displayInit(cinfo.image_width, cinfo.image_height);
    }

		// Create a stream based on our buffer.
		bufferStream = SDL_RWFromMem((unsigned char*)inputBuffer, *inputBufferLength);

		// Create a surface using the data coming out of the above stream.
		frame = IMG_Load_RW(bufferStream, 0);

		// Blit the surface and flip the screen.
		SDL_BlitSurface(frame, NULL, screen, &position);
		SDL_Flip(screen);

    while(SDL_PollEvent(&events)){
      if(events.type == SDL_QUIT) {
        std::cout << "SDL_QUIT" << std::endl;
        return 0;
      }
    }
    return 1;
  }

 private:
  void displayInit(int width, int height){
		// Initialise everything.
		SDL_Init(SDL_INIT_VIDEO);
		IMG_Init(IMG_INIT_JPG);

		// Get the screen's surface.
		screen = SDL_SetVideoMode( width, height, 32, SDL_HWSURFACE);

		position = {.x = 0, .y = 0};
  }

  void displayCleanup(){
		// Free everything, and unload SDL & Co.
		SDL_FreeSurface(frame);
    if(firstFrameRead) {
      SDL_RWclose(bufferStream);
    }
		IMG_Quit();
		SDL_Quit();
  }
};

/* Save a JPEG formatted buffer to disk. */
void saveJpeg(void* inputBuffer, unsigned int inputBufferLength){
  int jpgfile;
  if((jpgfile = open("/tmp/myimage.jpeg", O_WRONLY | O_CREAT, 0660)) < 0){
    errno_exit("open");
  }

  write(jpgfile, inputBuffer, inputBufferLength);
  close(jpgfile);
}

unsigned int parseJpeg(void* inputBuffer, unsigned int* inputBufferLength,
                       unsigned char** outputBuffer, unsigned int& width, unsigned int& height){
  static unsigned int outputBufferLength = 0;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_mem_src(&cinfo, (unsigned char*)inputBuffer, *inputBufferLength);

  jpeg_read_header(&cinfo, TRUE);
  unsigned int row_stride = cinfo.image_width * cinfo.num_components;

  jpeg_start_decompress(&cinfo);

  width = cinfo.image_width;
  height = cinfo.image_height;
  unsigned int tmpOutputBufferLength =
    cinfo.image_width * cinfo.image_height * cinfo.num_components;

  if(*outputBuffer == NULL || tmpOutputBufferLength > outputBufferLength){
    outputBufferLength = tmpOutputBufferLength;
    *outputBuffer = (unsigned char*)realloc(*outputBuffer, outputBufferLength);
    if(!*outputBuffer) {
      errno_exit("realloc");
    }
  }

  unsigned char* ptr = *outputBuffer; 
  while (cinfo.output_scanline < cinfo.image_height){
    jpeg_read_scanlines(&cinfo, &ptr, 1);
    ptr += row_stride;
  }
  width = cinfo.image_width;
  height = cinfo.image_height;

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  return tmpOutputBufferLength;
}

void blur(unsigned char* inputBuffer, int width, int height) {
  unsigned char tempBuffer[width * height * 3] = {};
  float gaus[3][3] = {
    //{0.01, 0.08, 0.01},
    //{0.08, 0.64, 0.08},
    //{0.01, 0.08, 0.01}
    {1.0/16, 1.0/8, 1.0/16},
    {1.0/8, 1.0/4, 1.0/8},
    {1.0/16, 1.0/8, 1.0/16}
  };

  for(int y = 0; y < height; y++) {
    for(int x = 0; x < width * 3; x += 3) {
      for(int c = 0; c < 3; c++) {
        tempBuffer[y * width * 3 + x + c] = std::max((float)0, std::min((float)0xff, 
          gaus[0][0] * inputBuffer[std::max(0, (y - 1)) *      width * 3 +      std::max(0, (x - 3)) +     c] +
          gaus[1][0] * inputBuffer[ y *                        width * 3 +      std::max(0, (x - 3)) +     c] +
          gaus[2][0] * inputBuffer[std::min(height, (y + 1)) * width * 3 +      std::max(0, (x - 3)) +     c] +
          gaus[0][1] * inputBuffer[std::max(0, (y - 1)) *      width * 3 +      x +                        c] +
          gaus[1][1] * inputBuffer[ y *                        width * 3 +      x +                        c] +
          gaus[2][1] * inputBuffer[std::min(height, (y + 1)) * width * 3 +      x +                        c] +
          gaus[0][2] * inputBuffer[std::max(0, (y - 1)) *      width * 3 +      std::min(width * 3, (x + 3)) + c] +
          gaus[1][2] * inputBuffer[ y *                        width * 3 +      std::min(width * 3, (x + 3)) + c] +
          gaus[2][2] * inputBuffer[std::min(height, (y + 1)) * width * 3 +      std::min(width * 3, (x + 3)) + c] ));
      }
    }
  }
  for(int i = 0; i < width * height * 3; i++) {
    inputBuffer[i] = tempBuffer[i];
  }
}

void filter(unsigned char* inputBuffer, unsigned int width, unsigned int height){
  int threshold = 20;
  int border = 1;
  unsigned char* calculated = new unsigned char[width * height]();
  unsigned char* calculated2 = new unsigned char[width * height]();

  for(unsigned int x = border; x < width -border; x++) {
    for(unsigned int y = border; y < height -border; y++) {
      int dxR = (int)inputBuffer[3*(x + y * width) +0] - inputBuffer[3*(x + y * width -border) +0];
      int dxG = (int)inputBuffer[3*(x + y * width) +1] - inputBuffer[3*(x + y * width -border) +1];
      int dxB = (int)inputBuffer[3*(x + y * width) +2] - inputBuffer[3*(x + y * width -border) +2];
      int dyR = (int)inputBuffer[3*(x + y * width) +0] - inputBuffer[3*(x + (y -border) * width) +0];
      int dyG = (int)inputBuffer[3*(x + y * width) +1] - inputBuffer[3*(x + (y -border) * width) +1];
      int dyB = (int)inputBuffer[3*(x + y * width) +2] - inputBuffer[3*(x + (y -border) * width) +2];

      if(abs(dxR) + abs(dxG) + abs(dxB) + abs(dyR) + abs(dyG) + abs(dyB) > threshold * 2){
        calculated[x + y * width]++;
      } else if(abs(dxR) > threshold || abs(dxG) > threshold || abs(dxB) > threshold || abs(dyR) > threshold || abs(dyG) > threshold || abs(dyB) > threshold){
        calculated[x + y * width]++;
      }
    }
  }


  // Thin lines in x direction.
  unsigned int start = 0;
  for(unsigned int y = border; y < height -border; y++) {
    start = 0;
    for(unsigned int x = border; x < width -border; x++) {
      if(calculated[x + y * width]) {
        if(!start) {
          start = x;
        }
      } else {
        if(start) {
          unsigned int mid = (start + x) / 2;
          calculated2[mid + y * width] = 1;
          start = 0;
        }
      }
    }
  }

  // Thin lines in y direction.
  for(unsigned int x = border; x < width -border; x++) {
    start = 0;
    for(unsigned int y = border; y < height -border; y++) {
      if(calculated[x + y * width]) {
        if(!start) {
          start = y;
        }
      } else {
        if(start) {
          unsigned int mid = (start + y) / 2;
          calculated2[x + mid * width] = 1;
          start = 0;
        }
      }
    }
  }

  for(unsigned int x = border; x < width -border; x++) {
    for(unsigned int y = border; y < height -border; y++) {

      if(calculated2[x + (y - border) * width] && calculated2[x + y * width] && calculated2[x + (y + border) * width]) {
        inputBuffer[3*(x + y * width)] = 255;
        inputBuffer[3*(x + y * width) +1] = 255;
        inputBuffer[3*(x + y * width) +2] = 255;
      } else if(calculated2[x - border + y * width] && calculated2[x + y * width] && calculated2[x + border + y * width]) {
        inputBuffer[3*(x + y * width)] = 255;
        inputBuffer[3*(x + y * width) +1] = 255;
        inputBuffer[3*(x + y * width) +2] = 255;
      /*} else if(calculated[x + (y - border) * width] && calculated[x + y * width] && calculated[x + (y + border) * width]) {
        inputBuffer[3*(x + y * width)] = 0;
        inputBuffer[3*(x + y * width) +1] = 0;
        inputBuffer[3*(x + y * width) +2] = 255;
      } else if(calculated[x - border + y * width] && calculated[x + y * width] && calculated[x + border + y * width]) {
        inputBuffer[3*(x + y * width)] = 0;
        inputBuffer[3*(x + y * width) +1] = 0;
        inputBuffer[3*(x + y * width) +2] = 255;*/
      } else {
        inputBuffer[3*(x + y * width)] /= 4;
        inputBuffer[3*(x + y * width) +1] /= 4;
        inputBuffer[3*(x + y * width) +2] /= 4;
      }
    }
  }

  delete[] calculated;
  delete[] calculated2;
}


class DisplayAsci {
  unsigned char* inputBuffer;
  unsigned int inputBufferLength;
  unsigned int width;
  unsigned int height;

 public:
  DisplayAsci(unsigned char* inputBuffer_, unsigned int inputBufferLength_, unsigned int width_, unsigned int height_) {
    setBuffer(inputBuffer_, inputBufferLength_, width_, height_);
    cursesInit();
  }

  void setBuffer(unsigned char* inputBuffer_, unsigned int inputBufferLength_, unsigned int width_, unsigned int height_) {
    inputBuffer = inputBuffer_;
    inputBufferLength = inputBufferLength_;
    width = width_;
    height = height_;
  }

  ~DisplayAsci() {
    cursesCleanup();
  }

  int update(){
    unsigned int row_stride = inputBufferLength / height;

    clear();
    for(unsigned int line = 0; line < height; line += 15){
      for(unsigned int col = 0; col < row_stride; col += 30){
        int r = inputBuffer[line * row_stride + col];
        int g = inputBuffer[line * row_stride + col +1];
        int b = inputBuffer[line * row_stride + col +2];

        int y = (r + g) / 1.9;
        int m = (r + b) / 1.9;
        int c = (g + b) / 1.9;
        int w = (r + g + b) / 2.8;

        if(r > g && r > b && r > y && r > m && r > c && r > w){
          // Red
          attron(COLOR_PAIR(1));
          if(r > 128){
            addstr("R");
          } else if(r > 75){
            addstr("r");
          } else if(r > 37){
            addstr(".");
          } else {
            addstr(" ");
          }
          attroff(COLOR_PAIR(1));
        } else if(g > r && g > b && g > y && g > m && g > c && g > w){
          // Green
          attron(COLOR_PAIR(2));
          if(g > 128){
            addstr("G");
          } else if(g > 75){
            addstr("g");
          } else if(g > 37){
            addstr(".");
          } else {
            addstr(" ");
          }
          attroff(COLOR_PAIR(2));
        } else if(b > r && b > g && b > y && b > m && b > c && b > w){
          // Blue
          attron(COLOR_PAIR(3));
          if(b > 128){
            addstr("B");
          } else if(b > 75){
            addstr("b");
          } else if(b > 37){
            addstr(".");
          } else {
            addstr(" ");
          }
          attroff(COLOR_PAIR(3));
        } else if(y > r && y > g && y > b && y > m && y > c && y > w){
          // Yellow
          attron(COLOR_PAIR(4));
          if(y > 128){
            addstr("Y");
          } else if(y > 75){
            addstr("y");
          } else if(y > 37){
            addstr(".");
          } else {
            addstr(" ");
          }
          attroff(COLOR_PAIR(4));
        } else if(m > r && m > g && m > b && m > y && m > c && m > w){
          // Magenta
          attron(COLOR_PAIR(5));
          if(m > 128){
            addstr("M");
          } else if(m > 75){
            addstr("m");
          } else if(m > 37){
            addstr(".");
          } else {
            addstr(" ");
          }
          attroff(COLOR_PAIR(5));
        } else if(c > r && c > g && c > b && c > y && c > m && c > w){
          // Cyan
          attron(COLOR_PAIR(6));
          if(c > 128){
            addstr("C");
          } else if(c > 75){
            addstr("c");
          } else if(c > 37){
            addstr(".");
          } else {
            addstr(" ");
          }
          attroff(COLOR_PAIR(6));
        } else if(w > r && w > g && w > b && w > y && w > m && w > c){
          //White
          if(w > 128){
            addstr("W");
          } else if(w > 75){
            addstr("w");
          } else if(w > 37){
            addstr(".");
          } else {
            addstr(" ");
          }
        } else {
          addstr(" ");
        }
      }
      addstr("\n");
    }
    refresh();

    return 1;
  }

 private:
  void cursesInit(){
    initscr();
    (void)echo();
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
  }

  void cursesCleanup(){
    endwin();
  }
};

boolean emptyBuffer(jpeg_compress_struct* cinfo) {
  return TRUE;
}
void init_buffer(jpeg_compress_struct* cinfo) {}
void term_buffer(jpeg_compress_struct* cinfo) {}

void makeJpeg(unsigned char* inputBuffer, unsigned int inputBufferLength,
              void** outputBuffer, unsigned int& outputBufferLength,
              unsigned int width, unsigned int height) {

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;
  struct jpeg_destination_mgr dmgr;

  /* Set up output buffer. */
  dmgr.init_destination    = init_buffer;
  dmgr.empty_output_buffer = emptyBuffer;
  dmgr.term_destination    = term_buffer;
  dmgr.next_output_byte    = (JOCTET*)(*outputBuffer);
  dmgr.free_in_buffer      = outputBufferLength;
   
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  
  cinfo.dest = &dmgr;
  cinfo.image_width      = width;
  cinfo.image_height     = height;
  cinfo.input_components = 3;
  cinfo.in_color_space   = JCS_RGB;

  jpeg_set_defaults(&cinfo);
  /*set the quality [0..100]  */
  jpeg_set_quality (&cinfo, 75, true);
  jpeg_start_compress(&cinfo, true);

  JSAMPROW row_pointer;

  /* main code to write jpeg data */
  while (cinfo.next_scanline < cinfo.image_height) {    
    row_pointer = (JSAMPROW) &inputBuffer[cinfo.next_scanline * width * 3];
    jpeg_write_scanlines(&cinfo, &row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
}


int main()
{
  int run = 1;
  int firstLoop = 1;
	const char* deviceName = "/dev/video0";
  void* cameraBuffer = NULL;
  unsigned int cameraBufferLength = 0;
  unsigned char* parsedBuffer = NULL;
  unsigned int parsedBufferLength = 0;
  unsigned int width = 0;
  unsigned int  height = 0;

  void* outputJpegBuffer = NULL;
  unsigned int outputJpegBufferEstimatedLength = 0;
  unsigned int outputJpegBufferLength = 0;

  Camera inputDevice(deviceName, IO_METHOD_MMAP, &cameraBuffer, &cameraBufferLength);
  //DisplaySdl displayRaw(cameraBuffer, &cameraBufferLength);
  DisplaySdl displayProcessed(outputJpegBuffer, &outputJpegBufferLength);
  DisplayAsci displayParsed(parsedBuffer, parsedBufferLength, width, height);

  while(run){
    if(!inputDevice.grabFrame()){
      break;
    }
    //saveJpeg(cameraBuffer, cameraBufferLength);
    //run &= displayRaw.update();
    parsedBufferLength = parseJpeg(cameraBuffer, &cameraBufferLength, &parsedBuffer, width, height);
    blur(parsedBuffer, width, height);
    filter(parsedBuffer, width, height);
    if(firstLoop) {
      displayParsed.setBuffer(parsedBuffer, parsedBufferLength, width, height);

      outputJpegBufferEstimatedLength = width * height * 3;
      outputJpegBuffer = new unsigned char[outputJpegBufferEstimatedLength];
      displayProcessed.setBuffer(outputJpegBuffer, &outputJpegBufferEstimatedLength);
    } else {
      run &= displayProcessed.update();
    }
    //displayParsed.update();
    makeJpeg(parsedBuffer, parsedBufferLength, &outputJpegBuffer, outputJpegBufferEstimatedLength, width, height);
    firstLoop = 0;
  }

  free(parsedBuffer);
  if(!firstLoop) {
    delete[] (unsigned char*)outputJpegBuffer;
  }

  displayParsed.~DisplayAsci();
}
