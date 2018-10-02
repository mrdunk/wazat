#include "inputs.h"

static void xioctl(int fh, int request, void *arg)
{
  int r;

  do {
    r = v4l2_ioctl(fh, request, arg);
  } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

  if (r == -1) {
    fprintf(stderr, "error %d, %s\\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void errno_exit(const char *s) {
  std::cout << "Error: " << s << " : " << errno << " : " <<  strerror(errno) << "\n";
  exit(EXIT_FAILURE);
}

Camera::Camera(const char* deviceName_,
               enum IoMethod io_,
               void** cameraBuffer_,
               size_t* bufferLength_){
  deviceName = deviceName_;
  io = io_;
  cameraBuffer = cameraBuffer_;
  bufferLength = bufferLength_;

  // Open devce.
  fd = v4l2_open(deviceName, O_RDWR | O_NONBLOCK, 0);
  if (fd < 0) {
    perror("Cannot open device");
    exit(EXIT_FAILURE);
  }
  std::cout << "Opened " << deviceName << "\n";

  checkCapabilities();
  setFormat();
  setBuffers();
  prepareBuffer();
  getImageProperties();
}

Camera::~Camera(){
  // Deactivate streaming
  enum v4l2_buf_type type;
  switch (io) {
    case IO_METHOD_READ:
      /* Nothing to do. */
      break;
    case IO_METHOD_MMAP_SINGLE:
    case IO_METHOD_MMAP_DOUBLE:
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      xioctl(fd, VIDIOC_STREAMOFF, &type);
      for (unsigned int i = 0; i < numBuffers; ++i)
        v4l2_munmap(buffers[i].start, buffers[i].length);
      break;
  }

  std::cout << "Closing " << deviceName << "\n";
  v4l2_close(fd);
}

int Camera::grabFrame(){
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
  xioctl(fd, VIDIOC_DQBUF, &bufferinfo);

  *cameraBuffer = buffers[bufferinfo.index].start;
  *bufferLength = bufferinfo.length;
  //*bufferLength = bufferinfo.bytesused;

  // Put the buffer in the incoming queue
  xioctl(fd, VIDIOC_QBUF, &bufferinfo);
  return 1;
}

void Camera::checkCapabilities(){
  v4l2_capability capabilities;

  // Check device v4l2 compatible.
  xioctl(fd, VIDIOC_QUERYCAP, &capabilities);

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
    case IO_METHOD_MMAP_SINGLE:
    case IO_METHOD_MMAP_DOUBLE:
      std::cout << "Using IO_METHOD_MMAP\n";
      if (!(capabilities.capabilities & V4L2_CAP_STREAMING)) {
        std::cout << "Error: " << deviceName << " does not support streaming i/o\n";
        exit(EXIT_FAILURE);
      }
      break;
  }
}

void Camera::getImageProperties(){
  grabFrame();  // Will populate bufferLength.

  /*struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, (unsigned char*)(*cameraBuffer), *bufferLength);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_calc_output_dimensions(&cinfo);

  width = cinfo.image_width;
  height = cinfo.image_height;

  jpeg_destroy_decompress(&cinfo);*/
  width = captureWidth;
  height = captureHeight;
  std::cout << "getImageProperties\t" << width << "," << height << std::endl;
}

void Camera::setFormat(){
  memset(&format, 0, sizeof(struct v4l2_format));
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.width = 1200;
  format.fmt.pix.height = 600;
  //format.fmt.pix.width = 1280;
  //format.fmt.pix.height = 720;
  //format.fmt.pix.width = 1280 / 2;
  //format.fmt.pix.height = 720 / 2;
  format.fmt.pix.pixelformat =  V4L2_PIX_FMT_RGB24;

  xioctl(fd, VIDIOC_S_FMT, &format);
  if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
    std::cout << "Image not in recognised format. Can't proceed." << std::endl;
    std::cout << (char)(format.fmt.pix.pixelformat) << " " <<
                 (char)(format.fmt.pix.pixelformat >> 8) << " " <<
                 (char)(format.fmt.pix.pixelformat >> 16) << " " <<
                 (char)(format.fmt.pix.pixelformat >> 24) << std::endl;
    exit(EXIT_FAILURE);
  }

  captureWidth = format.fmt.pix.width;
  captureHeight = format.fmt.pix.height;
  std::cout << "Image width set to " << captureWidth << " by device " << deviceName << ".\n";
  std::cout << "Image height set to " << captureHeight << " by device " << deviceName << ".\n";
}

void Camera::setBuffers(){
  switch (io) {
    case IO_METHOD_READ:
      //init_read(format.fmt.pix.sizeimage);
      assert(false && "TODO");
      break;

    case IO_METHOD_MMAP_SINGLE:
      initMmap(1);
      break;

    case IO_METHOD_MMAP_DOUBLE:
      initMmap(2);
      break;
  }


  // "$ v4l2-ctl -l" lets us see what our camera is capable of (and set to).
  /*setControl(V4L2_CID_BRIGHTNESS, 0);
  setControl(V4L2_CID_CONTRAST, 0);
  setControl(V4L2_CID_SATURATION, 64);
  setControl(V4L2_CID_GAIN, 1);
  setControl(V4L2_CID_AUTO_WHITE_BALANCE, 1);
  setControl(V4L2_CID_BACKLIGHT_COMPENSATION, 3);*/
}

void Camera::setControl(int id, int value){
  struct v4l2_queryctrl queryctrl;
  queryctrl.id = id;
  if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
    if (errno != EINVAL)
      exit(EXIT_FAILURE);
    else {
      std::cerr << "WARNING :: Unable to set property (NOT SUPPORTED)\n";
    }
  } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
    std::cout << "ERROR :: Unable to set property (DISABLED).\n";
    exit(EXIT_FAILURE);
  } else {
    struct v4l2_control control;
    control.id = queryctrl.id;
    control.value = value;

    if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control)) {
      exit(EXIT_FAILURE);
    }
    std::cout << "Successfully set property." << std::endl;
  }
}

void Camera::initMmap(unsigned int numBuffers_) {
  std::cout << "Using IO_METHOD_MMAP_SINGLE\n";

  numBuffers = numBuffers_;

  struct v4l2_requestbuffers bufrequest = {0};
  bufrequest.count = numBuffers;
  bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  bufrequest.memory = V4L2_MEMORY_MMAP;

  xioctl(fd, VIDIOC_REQBUFS, &bufrequest);

  struct v4l2_buffer bufferinfo = {0};
  for (numBuffers = 0; numBuffers < bufrequest.count; ++numBuffers) {
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = numBuffers;

    xioctl(fd, VIDIOC_QUERYBUF, &bufferinfo);

    buffers[numBuffers].length = bufferinfo.length;
    buffers[numBuffers].start = (uint8_t*)v4l2_mmap(NULL, bufferinfo.length,
        PROT_READ | PROT_WRITE, MAP_SHARED,
        fd, bufferinfo.m.offset);

    if (MAP_FAILED == buffers[numBuffers].start) {
      perror("mmap");
      exit(EXIT_FAILURE);
    }
  }
}

void Camera::prepareBuffer(){
  for (unsigned int i = 0; i < numBuffers; ++i) {
    memset(&bufferinfo, 0, sizeof(bufferinfo));
    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = i;
    xioctl(fd, VIDIOC_QBUF, &bufferinfo);
  }

  // Activate streaming
  int type = bufferinfo.type;
  xioctl(fd, VIDIOC_STREAMON, &type);
}


File::File(const char* filename_,
           struct buffer<uint8_t>& buffer_) :
              filename(filename_),
              externalBuffer(&buffer_) {
  internalBuffer.start = nullptr;
  internalBuffer.length = 0;
  getImageProperties();
}

File::~File() {
  if(internalBuffer.length > 0) {
    internalBuffer.length = 0;
    delete[] (uint8_t*)(internalBuffer.start);
  }
}

int File::grabFrame() {
  std::ifstream file(filename, std::ios::in|std::ios::binary|std::ios::ate);
  if(file.is_open()) {
    std::streampos size = file.tellg();
    internalBuffer.resize(size);

    file.seekg (0, std::ios::beg);
    file.read (((char*)internalBuffer.start), size);
    file.close();

    parseJpeg(&internalBuffer, externalBuffer, width, height);
    return externalBuffer->length;
  }
  std::cout << "Could not read file: " << filename << std::endl;
  return -1;
}

void File::getImageProperties(){
  size_t size = grabFrame();

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, (unsigned char*)(internalBuffer.start), internalBuffer.length);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_calc_output_dimensions(&cinfo);

  width = cinfo.image_width;
  height = cinfo.image_height;

  jpeg_destroy_decompress(&cinfo);
  std::cout << "getImageProperties\t" << width << "," << height << std::endl;
  assert(size >= width * height * 3);
}

void File::parseJpeg(struct buffer<uint8_t>* inputBuffer,
                     struct buffer<uint8_t>* outputBuffer,
                     unsigned int& width,
                     unsigned int& height) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_mem_src(&cinfo, (uint8_t*)inputBuffer->start, inputBuffer->length);

  jpeg_read_header(&cinfo, TRUE);
  unsigned int row_stride = cinfo.image_width * cinfo.num_components;

  jpeg_start_decompress(&cinfo);

  width = cinfo.image_width;
  height = cinfo.image_height;

  unsigned int desiredOutputBufferLen =
    cinfo.image_width * cinfo.image_height * cinfo.num_components;
  outputBuffer->resize(desiredOutputBufferLen);

  uint8_t* ptr = ((uint8_t*)outputBuffer->start);
  while (cinfo.output_scanline < cinfo.image_height){
    jpeg_read_scanlines(&cinfo, &ptr, 1);
    ptr += row_stride;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
}
