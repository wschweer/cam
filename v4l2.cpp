//=============================================================================
//  Cam
//  Webcam client
//
//  Copyright (C) 2016 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "v4l2.h"

//---------------------------------------------------------
//   V4l2
//---------------------------------------------------------

V4l2::V4l2()
      {
      }

V4l2::~V4l2()
      {
      close();
      }

//---------------------------------------------------------
//   open
//---------------------------------------------------------

bool V4l2::open(const QString& p)
      {
      if (fd != -1)
            return false;
      path = p;
      char* videodevice = path.toLocal8Bit().data();
      fd = ::open(videodevice, O_RDWR);
      return fd != -1;
      }

//---------------------------------------------------------
//   close
//---------------------------------------------------------

bool V4l2::close()
      {
      if (fd == -1)
            return false;
      int rv = ::close(fd);
      fd = -1;
      return rv != -1;
      }

//---------------------------------------------------------
//   canVideoCapture
//---------------------------------------------------------

bool V4l2::canVideoCapture()
      {
      struct v4l2_capability cap;
      memset(&cap, 0, sizeof(struct v4l2_capability));
      int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
      if (ret < 0) {
            fprintf(stderr, "Camera: <%s>: unable to query device.\n", qPrintable(path));
            return false;
            }
      return cap.capabilities & V4L2_CAP_VIDEO_CAPTURE;
      }

//---------------------------------------------------------
//   canStreaming
//---------------------------------------------------------

bool V4l2::canStreaming()
      {
      struct v4l2_capability cap;
      memset(&cap, 0, sizeof(struct v4l2_capability));
      int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
      if (ret < 0) {
            fprintf(stderr, "Camera: <%s>: unable to query device.\n", qPrintable(path));
            return false;
            }
      return cap.capabilities & V4L2_CAP_STREAMING;
      }

//---------------------------------------------------------
//   setMjpegFormat
//---------------------------------------------------------

bool V4l2::setMjpegFormat(int w, int h)
      {
      struct v4l2_format fmt;
      memset(&fmt, 0, sizeof(struct v4l2_format));
      fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      fmt.fmt.pix.width       = w;
      fmt.fmt.pix.height      = h;
      fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
      fmt.fmt.pix.field       = V4L2_FIELD_ANY;
      int ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
      if (ret < 0) {
            fprintf(stderr, "Camera <%s> does not support format MJPEG: %s.\n", qPrintable(path), strerror(errno));
            return false;
            }
      if ((int(fmt.fmt.pix.width) != w) || (int(fmt.fmt.pix.height) != h)) {
            fprintf(stderr, " format %d x %d unavailable, get %d x %d \n",
               w, h, fmt.fmt.pix.width, fmt.fmt.pix.height);
            }
      return true;
      }

//---------------------------------------------------------
//   setFramerate
//---------------------------------------------------------

bool V4l2::setFramerate(int fps)
      {
      struct v4l2_streamparm setfps;
      memset(&setfps, 0, sizeof(setfps));
      setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      setfps.parm.capture.timeperframe.numerator = 1;
      setfps.parm.capture.timeperframe.denominator = fps;
      int ret = ioctl(fd, VIDIOC_S_PARM, &setfps);
      return ret >= 0;
      }

//---------------------------------------------------------
//   isControl
//    return >= 0 ok otherwhise -1
//---------------------------------------------------------

int V4l2::isControl(int control, struct v4l2_queryctrl* queryctrl)
      {
      int err = 0;
      queryctrl->id = control;
      if ((err = ioctl(fd, VIDIOC_QUERYCTRL, queryctrl)) < 0)
            printf("ioctl querycontrol error %d \n", errno);
      else if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED)
            printf("control %s disabled \n", (char*) queryctrl->name);
      else if (queryctrl->flags & V4L2_CTRL_TYPE_BOOLEAN)
            return 1;
      else if (queryctrl->type & V4L2_CTRL_TYPE_INTEGER)
            return 0;
      else
            printf("contol %s unsupported  \n", (char*) queryctrl->name);
      return -1;
      }

//---------------------------------------------------------
//   getControl
//---------------------------------------------------------

int V4l2::getControl(int control)
      {
      struct v4l2_queryctrl queryctrl;
      struct v4l2_control control_s;
      int err;
      if (isControl(control, &queryctrl) < 0)
            return -1;
      control_s.id = control;
      if ((err = ioctl(fd, VIDIOC_G_CTRL, &control_s)) < 0) {
            printf("ioctl get control error\n");
            return -1;
            }
      return control_s.value;
      }

//---------------------------------------------------------
//   setControl
//---------------------------------------------------------

int V4l2::setControl(int control, int value)
      {
      struct v4l2_control control_s;
      struct v4l2_queryctrl queryctrl;
      int min, max;
      int err;
      if (isControl(control, &queryctrl) < 0)
            return -1;
      min = queryctrl.minimum;
      max = queryctrl.maximum;
      if ((value >= min) && (value <= max)) {
            control_s.id = control;
            control_s.value = value;
            if ((err = ioctl(fd, VIDIOC_S_CTRL, &control_s)) < 0) {
                  printf("ioctl set control error\n");
                  return -1;
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   upControl
//---------------------------------------------------------

int V4l2::upControl(int control)
      {
      struct v4l2_control control_s;
      struct v4l2_queryctrl queryctrl;
      int max, current, step;
      int err;

      if (isControl(control, &queryctrl) < 0)
            return -1;
      max  = queryctrl.maximum;
      step = queryctrl.step;
      current = getControl(control);
      current += step;
      if (current <= max) {
            control_s.id = control;
            control_s.value = current;
            if ((err = ioctl(fd, VIDIOC_S_CTRL, &control_s)) < 0) {
                  printf("ioctl set control error\n");
                  return -1;
                  }
            printf ("Control name:%s set to value:%d\n", queryctrl.name, control_s.value);
            }
      else {
            printf ("Control name:%s already has max value:%d \n", queryctrl.name, max);
            }
      return control_s.value;
      }

//---------------------------------------------------------
//   downControl
//---------------------------------------------------------

int V4l2::downControl(int control)
      {
      struct v4l2_control control_s;
      struct v4l2_queryctrl queryctrl;
      int min, current, step;
      int err;
      if (isControl(control, &queryctrl) < 0)
            return -1;
      min = queryctrl.minimum;
      step = queryctrl.step;
      current = getControl(control);
      current -= step;
      if (current >= min) {
            control_s.id = control;
            control_s.value = current;
            if ((err = ioctl(fd, VIDIOC_S_CTRL, &control_s)) < 0) {
                  printf("ioctl set control error\n");
                  return -1;
                  }
            printf ("Control name:%s set to value:%d\n", queryctrl.name, control_s.value);
            }
      else
            printf ("Control name:%s already has min value:%d \n", queryctrl.name, min);
      return control_s.value;
      }

//---------------------------------------------------------
//   toggleControl
//---------------------------------------------------------

int V4l2::toggleControl(int control)
      {
      struct v4l2_control control_s;
      struct v4l2_queryctrl queryctrl;
      int current;
      int err;
      if (isControl(control, &queryctrl) != 1)
            return -1;
      current = getControl(control);
      control_s.id = control;
      control_s.value = !current;
      if ((err = ioctl(fd, VIDIOC_S_CTRL, &control_s)) < 0) {
            printf("ioctl toggle control error\n");
            return -1;
            }
      return control_s.value;
      }

//---------------------------------------------------------
//   resetControl
//---------------------------------------------------------

bool V4l2::resetControl(int control)
      {
      struct v4l2_queryctrl queryctrl;
      if (isControl(control, &queryctrl) < 0)
            return false;
      int val_def = queryctrl.default_value;
      struct v4l2_control control_s;
      control_s.id = control;
      control_s.value = val_def;
      int err = ioctl(fd, VIDIOC_S_CTRL, &control_s);
      if (err < 0) {
            printf("ioctl reset control error\n");
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   grab
//    try reading a picture into tmpbuffer
//    return image size or -1 on error
//---------------------------------------------------------

#define HEADERFRAME1 0xaf

QImage V4l2::grab()
      {
      struct v4l2_buffer buf;
      memset(&buf, 0, sizeof(struct v4l2_buffer));
      buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;

      int ret = ioctl(fd, VIDIOC_DQBUF, &buf);
      if (ret < 0) {
            printf("Unable to dequeue buffer: %s\n", strerror(errno));
            return QImage();
            }

      int size = buf.bytesused;
      if (size <= HEADERFRAME1) {
            printf("Ignoring empty buffer ...\n");
            return QImage();
            }
      QByteArray data((const char*)mem[buf.index], size);
      QImage image = QImage::fromData(data, "mjpeg");

      ret = ioctl(fd, VIDIOC_QBUF, &buf);
      if (ret < 0)
            printf("Unable to requeue buffer (%d).\n", errno);
      return image;
      }

//---------------------------------------------------------
//   initBuffers
//---------------------------------------------------------

bool V4l2::initBuffers()
      {
      /*
       * request buffers
       */
      struct v4l2_requestbuffers rb;
      memset(&rb, 0, sizeof(struct v4l2_requestbuffers));
      rb.count  = NB_BUFFER;
      rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      rb.memory = V4L2_MEMORY_MMAP;

      int ret = ioctl(fd, VIDIOC_REQBUFS, &rb);
      if (ret < 0) {
            fprintf(stderr, "Unable to allocate buffers: %s\n", strerror(errno));
            return false;
            }
      /*
       * map the buffers
       */
      for (int i = 0; i < NB_BUFFER; i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(struct v4l2_buffer));
            buf.index  = i;
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            ret        = ioctl(fd, VIDIOC_QUERYBUF, &buf);
            if (ret < 0) {
                  fprintf(stderr, "Unable to query buffer: %s\n", strerror(errno));
                  return false;
                  }
            mem[i] = mmap(0, buf.length, PROT_READ, MAP_SHARED, fd, buf.m.offset);
            if (mem[i] == MAP_FAILED) {
                  fprintf(stderr, "Unable to map buffer: %s\n", strerror(errno));
                  return false;
                  }
            }
      /*
       * Queue the buffers.
       */
      for (int i = 0; i < NB_BUFFER; ++i) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(struct v4l2_buffer));
            buf.index  = i;
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            ret        = ioctl(fd, VIDIOC_QBUF, &buf);
            if (ret < 0) {
                  fprintf(stderr, "Unable to queue buffer: %s\n", strerror(errno));
                  return false;
                  }
            }
      return true;
      }

//---------------------------------------------------------
//   freeBuffers
//---------------------------------------------------------

bool V4l2::freeBuffers()
      {
      /*
       * unmap the buffers
       */
      for (int i = 0; i < NB_BUFFER; i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(struct v4l2_buffer));
            buf.index  = i;
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            int ret    = ioctl(fd, VIDIOC_QUERYBUF, &buf);
            if (ret < 0) {
                  fprintf(stderr, "Unable to query buffer: %s\n", strerror(errno));
                  return false;
                  }
            ret = munmap(mem[i], buf.length);
            if (ret) {
                  fprintf(stderr, "Unable to unmap buffer: %s\n", strerror(errno));
                  return false;
                  }
            }
      return true;
      }

