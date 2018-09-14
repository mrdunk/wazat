#include "inputs.h"

int xioctl(int fh, int request, void *arg) {
  int r;

  do {
    r = ioctl(fh, request, arg);
  } while (-1 == r && EINTR == errno);

  return r;
}

void errno_exit(const char *s) {
  std::cout << "Error: " << s << " : " << errno << " : " <<  strerror(errno) << "\n";
  exit(EXIT_FAILURE);
}

Camera::Camera(const char* deviceName_,
               enum IoMethod io_,
               void** cameraBuffer_,
               unsigned int* bufferLength_){
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
  getImageProperties();
}

Camera::~Camera(){
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

void Camera::checkCapabilities(){
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

void Camera::getImageProperties(){
  grabFrame();

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, (unsigned char*)(*cameraBuffer), *bufferLength);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_calc_output_dimensions(&cinfo);

  width = cinfo.image_width;
  height = cinfo.image_height;

  jpeg_destroy_decompress(&cinfo);
  std::cout << "getImageProperties\t" << width << "," << height << std::endl;
}

void Camera::setFormat(){
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

void Camera::setBuffers(){
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

void Camera::setControl(int id, int value){
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

void Camera::initMmap(){
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

void Camera::prepareBuffer(){
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



void parseJpeg(void* inputBuffer,
               unsigned int& inputBufferLength,
               std::vector<unsigned char>& outputBuffer,
               unsigned int& width,
               unsigned int& height) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_mem_src(&cinfo, (unsigned char*)inputBuffer, inputBufferLength);

  jpeg_read_header(&cinfo, TRUE);
  unsigned int row_stride = cinfo.image_width * cinfo.num_components;

  jpeg_start_decompress(&cinfo);

  width = cinfo.image_width;
  height = cinfo.image_height;

  unsigned int desiredOutputBufferLen =
    cinfo.image_width * cinfo.image_height * cinfo.num_components;
  if(outputBuffer.size() < desiredOutputBufferLen) {
    outputBuffer.resize(desiredOutputBufferLen);
  }

  unsigned char* ptr = &outputBuffer.front(); 
  while (cinfo.output_scanline < cinfo.image_height){
    jpeg_read_scanlines(&cinfo, &ptr, 1);
    ptr += row_stride;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

}


