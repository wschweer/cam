//=============================================================================
//  UVCCam
//  Camera Modul
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

#ifndef __UVCCAM_H__
#define __UVCCAM_H__

#include <QWidget>
#include <linux/videodev2.h>
#include <thread>

#define NB_BUFFER 4
#define DHT_SIZE 432

//---------------------------------------------------------
//   Camera
//---------------------------------------------------------

class Camera : public QWidget {
      Q_OBJECT
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
      char* videodevice           { 0 };
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
      int init(const char* device, int width, int height, int fps);
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
      };

#endif

