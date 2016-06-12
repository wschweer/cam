//=============================================================================
//  Cam
//  Webcam client
//
//  This file is based on uvc_streamer from TomStöveken
//    Copyright (C) 2007 Tom Stöveken
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

#include <QPushButton>
#include <QPainter>
#include <QPoint>
#include <QWheelEvent>
#include <QImageReader>
#include <QBuffer>
#include <QDir>

#include "camera.h"

//---------------------------------------------------------
//   ~Camera
//---------------------------------------------------------

Camera::~Camera()
      {
      if (isstreaming)
            stop();
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void Camera::paintEvent(QPaintEvent*)
      {
      QPainter p(this);

      p.save();
      p.scale(mag, mag);
      int w = width();
      int h = height();

      if (image.width()) {
            qreal x = 0.5 * (w / mag - image.width());
            qreal y = 0.5 * (h / mag - image.height());
            p.drawImage(QPointF(x, y), image);
            }
      else
            p.fillRect(0, 0, width(), height(), QColor(255, 0, 0, 255));
      p.restore();
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Camera::resizeEvent(QResizeEvent* e)
      {
      QWidget::resizeEvent(e);
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void Camera::wheelEvent(QWheelEvent* e)
      {
      if (e->modifiers() != Qt::ControlModifier)
            return;

      int step  = e->delta() / 120;
      if (step > 0) {
            for (int i = 0; i < step; ++i) {
                  if (mag > 10.0)
                        break;
                  mag *= 1.1;
                  }
            }
      else {
            for (int i = 0; i < -step; ++i) {
                  if (mag < 0.1)
                        break;
                  mag *= .9;
                  }
            }
      update();
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

int Camera::init(const CamDeviceSetting& s)
      {
      setting = s;
      char* videodevice = s.device->device.toLocal8Bit().data();

      if ((fd = open(videodevice, O_RDWR)) == -1) {
            fprintf(stderr, "Camera: cannot open <%s>: %s\n", videodevice, strerror(errno));
            return -1;
            }

      struct v4l2_capability cap;
      memset(&cap, 0, sizeof(struct v4l2_capability));
      int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
      if (ret < 0) {
            fprintf(stderr, "Camera: <%s>: unable to query device.\n", videodevice);
            return -1;
            }
      if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
            fprintf(stderr, "Camera <%s> does not support video capture.\n", videodevice);
            return -1;
            }
      if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf(stderr, "Camera <%s> does not support streaming i/o.\n", videodevice);
            return -1;
            }
      /*
       * set format in
       */
      struct v4l2_format fmt;
      memset(&fmt, 0, sizeof(struct v4l2_format));
      fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      fmt.fmt.pix.width       = setting.size.width();
      fmt.fmt.pix.height      = setting.size.height();
      fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
      fmt.fmt.pix.field       = V4L2_FIELD_ANY;
      ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
      if (ret < 0) {
            fprintf(stderr, "Camera <%s> does not support format MJPEG: %s.\n", videodevice, strerror(errno));
            return -1;
            }
      if ((int(fmt.fmt.pix.width) != setting.size.width()) || (int(fmt.fmt.pix.height) != setting.size.height())) {
            fprintf(stderr, " format %d x %d unavailable, get %d x %d \n",
               setting.size.width(), setting.size.height(), fmt.fmt.pix.width, fmt.fmt.pix.height);
            setting.size.setWidth(fmt.fmt.pix.width);
            setting.size.setHeight(fmt.fmt.pix.height);
            }

      /*
       * set framerate
       */
      struct v4l2_streamparm setfps;
      memset(&setfps, 0, sizeof(setfps));
      setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      setfps.parm.capture.timeperframe.numerator = 1;
      setfps.parm.capture.timeperframe.denominator = setting.fps;
      ret = ioctl(fd, VIDIOC_S_PARM, &setfps);
      if (ret < 0) {
            fprintf(stderr, "Camera <%s>: Unable to set frame rate: %s.\n", videodevice, strerror(errno));
            return -1;
            }

      /*
       * request buffers
       */
      struct v4l2_requestbuffers rb;
      memset(&rb, 0, sizeof(struct v4l2_requestbuffers));
      rb.count  = NB_BUFFER;
      rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      rb.memory = V4L2_MEMORY_MMAP;

      ret = ioctl(fd, VIDIOC_REQBUFS, &rb);
      if (ret < 0) {
            fprintf(stderr, "Unable to allocate buffers: %s\n", strerror(errno));
            return -1;
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
                  return -1;
                  }
            mem[i] = mmap(0, buf.length, PROT_READ, MAP_SHARED, fd, buf.m.offset);
            if (mem[i] == MAP_FAILED) {
                  fprintf(stderr, "Unable to map buffer: %s\n", strerror(errno));
                  return -1;
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
                  return -1;
                  }
            }
//      framesizeIn = (cwidth * cheight << 1);
      return fd;
      }

//---------------------------------------------------------
//   loop
//---------------------------------------------------------

void Camera::loop()
      {
      isstreaming = true;
      int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      int ret;

      ret = ioctl(fd, VIDIOC_STREAMON, &type);
      if (ret < 0) {
            printf("Unable to start capture: %d.\n", errno);
            return;
            }
      while (isstreaming) {
            int n = grab();
            if (n > 0)
                  update();
            else
                  sleep(1);
            }

      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
      if (ret < 0)
            printf("Unable to stop capture: %d.\n", errno);
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

int Camera::start()
      {
      grabLoop = std::thread(&Camera::loop, this);
      return 0;
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

int Camera::stop()
      {
      isstreaming = false;
      grabLoop.join();
      return 0;
      }

#define HEADERFRAME1 0xaf

//---------------------------------------------------------
//   grab
//    try reading a picture into tmpbuffer
//    return image size or -1 on error
//---------------------------------------------------------

int Camera::grab()
      {
      struct v4l2_buffer buf;
      memset(&buf, 0, sizeof(struct v4l2_buffer));
      buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;

      int ret = ioctl(fd, VIDIOC_DQBUF, &buf);
      if (ret < 0) {
            printf("Unable to dequeue buffer: %s\n", strerror(errno));
            printf("fd = %d\n", fd);
            return -1;
            }

      int size = buf.bytesused;
      if (size <= HEADERFRAME1) {
            printf("Ignoring empty buffer ...\n");
            return 0;
            }
      QByteArray data((const char*)mem[buf.index], size);
      image = QImage::fromData(data, "mjpeg");

      ret = ioctl(fd, VIDIOC_QBUF, &buf);
      if (ret >= 0)
            return size;

      printf("Unable to requeue buffer (%d).\n", errno);
      return -1;
      }

//---------------------------------------------------------
//   isControl
//    return >= 0 ok otherwhise -1
//---------------------------------------------------------

int Camera::isControl(int control, struct v4l2_queryctrl* queryctrl)
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

int Camera::getControl(int control)
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

int Camera::setControl(int control, int value)
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

int Camera::upControl(int control)
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

int Camera::downControl(int control)
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

int Camera::toggleControl(int control)
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

bool Camera::resetControl(int control)
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
//   change
//---------------------------------------------------------

void Camera::change(const CamDeviceSetting& s)
      {
      if (isstreaming)
            stop();
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
                  return;
                  }
            ret = munmap(mem[i], buf.length);
            if (ret) {
                  fprintf(stderr, "Unable to unmap buffer: %s\n", strerror(errno));
                  return;
                  }
            }
      if (::close(fd))
            printf("close failed: %s\n", strerror(errno));

      init(s);
      start();
      }

