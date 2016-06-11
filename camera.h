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

#ifndef __UVCCAM_H__
#define __UVCCAM_H__

#include <QWidget>
#include <linux/videodev2.h>
#include <thread>

#define NB_BUFFER 4
// #define DHT_SIZE 432

class QString;

//---------------------------------------------------------
//   Camera
//---------------------------------------------------------

class Camera : public QWidget {
      Q_OBJECT
      QString device;
      int fd           { -1 };
      bool isstreaming { false };
      QImage image;
      qreal mag        { 1.0 };

      std::thread grabLoop;
      void loop();

      virtual void resizeEvent(QResizeEvent*) override;
      virtual void wheelEvent(QWheelEvent*) override;
      virtual void paintEvent(QPaintEvent*) override;

   public:
      struct v4l2_capability cap;
      struct v4l2_format fmt;
      struct v4l2_buffer buf;
      struct v4l2_requestbuffers rb;
      void* mem[NB_BUFFER];
      int grabmethod;
      unsigned cwidth;
      unsigned cheight;
      int fps;
      int formatIn;
      int framesizeIn;

   private:
      int isv4l2Control(int control, struct v4l2_queryctrl* queryctrl);
      int close_v4l2();

   public:
      Camera(QWidget* parent = 0) : QWidget(parent) {}
      ~Camera();
      int init(const QString&, int width, int height, int fps);
      int grab();
      int v4l2GetControl(int control);
      int v4l2SetControl(int control, int value);
      int v4l2UpControl(int control);
      int v4l2DownControl(int control);
      int v4l2ToggleControl(int control);
      int v4l2ResetControl(int control);
      int v4l2SetLightFrequencyFilter(int flt);
      int start();
      int stop();
      void setDevice(const QString& path, const QSize&);
      void setSize(const QSize&);
      };

#endif

