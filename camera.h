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

#include <thread>
#include <vector>

#include <QWidget>
#include <QString>
#include <QSize>

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

      void* mem[NB_BUFFER];
      CamDeviceSetting setting;
      std::thread grabLoop;

      virtual void resizeEvent(QResizeEvent*) override;
      virtual void wheelEvent(QWheelEvent*) override;
      virtual void paintEvent(QPaintEvent*) override;

      void loop();
      int grab();
      int isControl(int control, struct v4l2_queryctrl* queryctrl);
      int close_v4l2();

   public:
      Camera(QWidget* parent = 0) : QWidget(parent) {}
      ~Camera();
      int getControl(int control);
      int setControl(int control, int value);
      int upControl(int control);
      int downControl(int control);
      int toggleControl(int control);
      bool resetControl(int control);
      int start();
      int stop();
      int init(const CamDeviceSetting&);
      void change(const CamDeviceSetting&);
      };

#endif

