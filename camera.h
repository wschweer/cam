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

#include <linux/videodev2.h>
#include <thread>

#include <QWidget>
#include <QString>
#include <QSize>

class CamDevice;

#define NB_BUFFER 4

//---------------------------------------------------------
//   CamDeviceFormat
//---------------------------------------------------------

struct CamDeviceFormat {
      QSize size;
      std::vector<int> frameRates;
      };

//---------------------------------------------------------
//   CamDevice
//---------------------------------------------------------

struct CamDevice {
      QString shortName;
      QString name;
      QString device;
      std::vector<CamDeviceFormat> formats;
      };

//---------------------------------------------------------
//   CamDeviceSetting
//---------------------------------------------------------

struct CamDeviceSetting {
      CamDevice* device;
      QSize   size;
      int     fps;
      };

//---------------------------------------------------------
//   Camera
//---------------------------------------------------------

class Camera : public QWidget {
      Q_OBJECT
      int fd           { -1 };
      bool isstreaming { false };
      QImage image;
      qreal mag        { 1.0 };

      CamDeviceSetting setting;

      std::thread grabLoop;
      void loop();

      virtual void resizeEvent(QResizeEvent*) override;
      virtual void wheelEvent(QWheelEvent*) override;
      virtual void paintEvent(QPaintEvent*) override;

      int grab();

   public:
      struct v4l2_capability cap;
      struct v4l2_format fmt;
      struct v4l2_buffer buf;
      struct v4l2_requestbuffers rb;
      void* mem[NB_BUFFER];
      int grabmethod;
      int formatIn;
//      int framesizeIn;

   private:
      int isv4l2Control(int control, struct v4l2_queryctrl* queryctrl);
      int close_v4l2();

   public:
      Camera(QWidget* parent = 0) : QWidget(parent) {}
      ~Camera();
      int v4l2GetControl(int control);
      int v4l2SetControl(int control, int value);
      int v4l2UpControl(int control);
      int v4l2DownControl(int control);
      int v4l2ToggleControl(int control);
      int v4l2ResetControl(int control);
      int v4l2SetLightFrequencyFilter(int flt);
      int start();
      int stop();
      int init(const CamDeviceSetting&);
      void change(const CamDeviceSetting&);
      };

#endif

