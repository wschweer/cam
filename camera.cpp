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
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <poll.h>

#include <QPushButton>
#include <QPainter>
#include <QPoint>
#include <QWheelEvent>
#include <QImageReader>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QSettings>

#include "v4l2.h"
#include "camera.h"

//---------------------------------------------------------
//   ~Camera
//---------------------------------------------------------

Camera::Camera(QWidget* parent)
   : QWidget(parent)
      {
      QSettings settings;
      _picturePath   = settings.value("picPath",   _picturePath).toString();
      _picturePrefix = settings.value("picPrefix", _picturePrefix).toString();

      connect(this, SIGNAL(cameraButtonPressed()), this, SLOT(takeSnapshot()), Qt::QueuedConnection);
      }

Camera::~Camera()
      {
      if (isstreaming)
            stop();
      }

//---------------------------------------------------------
//   takeSnapshot
//---------------------------------------------------------

void Camera::takeSnapshot()
      {
      snapshot = true;
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
            if (snapshot) {
                  for (int i = 0; i < 50000; ++i) {
                        QString picName = QString("%1%2%3.jpeg").arg(_picturePath).arg(_picturePrefix).arg(pictureNumber);
                        if (!QFile::exists(picName)) {
                              image.save(picName, "jpeg" -1);
                              emit click(picName, 1000);
                              break;
                              }
                        pictureNumber++;
                        }
                  snapshot = false;
                  }
            if (_crosshair) {
                  qreal x1 = x + image.width() / 2.0;
                  qreal y1 = y + image.height() / 2.0;
                  p.drawLine(x1, y, x1, y + image.height());
                  p.drawLine(x, y1, x + image.width(), y1);
                  }
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
      cam = new V4l2();
      if (!cam->open(s.device->device)) {
            fprintf(stderr, "Camera: cannot open <%s>: %s\n", qPrintable(s.device->device), strerror(errno));
            return -1;
            }
      setting = s;

      if (!cam->canVideoCapture()) {
            fprintf(stderr, "Camera <%s> does not support video capture.\n", qPrintable(s.device->device));
            return -1;
            }
      if (!cam->canStreaming()) {
            fprintf(stderr, "Camera <%s> does not support streaming i/o.\n", qPrintable(s.device->device));
            return -1;
            }

      if (!cam->setMjpegFormat(setting.size.width(), setting.size.height())) {
            fprintf(stderr, "Camera <%s> does not support format MJPEG: %s.\n", qPrintable(s.device->device), strerror(errno));
            return -1;
            }

      if (!cam->setFramerate(setting.fps)) {
            fprintf(stderr, "Camera <%s>: Unable to set frame rate: %s.\n", qPrintable(s.device->device), strerror(errno));
            return -1;
            }
      if (!cam->initBuffers()) {
            fprintf(stderr, "Unable to initialize buffers: %s\n", strerror(errno));
            return -1;
            }
      return 0;
      }

//---------------------------------------------------------
//   loop
//---------------------------------------------------------

void Camera::loop()
      {
      isstreaming = true;
      int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      int ret;
      ret = ioctl(cam->getFd(), VIDIOC_STREAMON, &type);
      if (ret < 0) {
            printf("Unable to start capture: %d.\n", errno);
            return;
            }
      while (isstreaming) {
            image = cam->grab();
            if (!image.isNull())
                  update();
            else
                  sleep(1);
            }

      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      ret = ioctl(cam->getFd(), VIDIOC_STREAMOFF, &type);
      if (ret < 0)
            printf("Unable to stop capture: %d.\n", errno);
      }

//---------------------------------------------------------
//   watchButton
//---------------------------------------------------------

void Camera::watchButton()
      {
      if (setting.device->buttonDevice.isEmpty())
            return;

      int fd = ::open(setting.device->buttonDevice.toLocal8Bit().data(), O_RDWR);
      if (fd == -1) {
            printf("cannot open button input\n");
            return;
            }
      printf("watch button...\n");
      while (isstreaming) {
            struct pollfd fds = { fd, POLLIN, 0 };
            int r = poll(&fds, 1, 100);
            if (r > 0) {
                  struct input_event event;
                  int n = read(fd, &event, sizeof(event));
                  if (n > 0 && event.type == EV_KEY && event.code == KEY_CAMERA && event.value == 1) {
                        // camera button was pressed
                        emit cameraButtonPressed();
                        }
                  }
            }
      printf("   close button\n");
      ::close(fd);
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

int Camera::start()
      {
      grabLoop   = std::thread(&Camera::loop, this);
      buttonLoop = std::thread(&Camera::watchButton, this);
      return 0;
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

int Camera::stop()
      {
      isstreaming = false;
      grabLoop.join();
      buttonLoop.join();
      return 0;
      }

//---------------------------------------------------------
//   change
//---------------------------------------------------------

void Camera::change(const CamDeviceSetting& s)
      {
      if (isstreaming)
            stop();
      if (!cam->freeBuffers()) {
            fprintf(stderr, "Unable to unmap buffer: %s\n", strerror(errno));
            return;
            }
      delete cam;
      cam = 0;
      init(s);
      start();
      }

//---------------------------------------------------------
//   setPicturePath
//---------------------------------------------------------

void Camera::setPicturePath(const QString& s)
      {
      _picturePath = s;
      QSettings settings;
      settings.setValue("picPath", _picturePath);
      }

//---------------------------------------------------------
//   setPicturePrefix
//---------------------------------------------------------

void Camera::setPicturePrefix(const QString& s)
      {
      _picturePrefix = s;
      QSettings settings;
      settings.setValue("picPrefix", _picturePrefix);
      }

