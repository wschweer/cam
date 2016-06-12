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

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <thread>
#include <vector>

#include <QWidget>
#include <QString>
#include <QSize>

class V4l2;

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
      QString buttonDevice;
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

      V4l2* cam        { 0 };
      bool isstreaming { false };
      QImage image;
      qreal mag        { 1.0 };
      bool snapshot    { false };

      QString _picturePath   { ""    };
      QString _picturePrefix { "pic" };
      int pictureNumber     { 1     };

      CamDeviceSetting setting;
      std::thread grabLoop;
      std::thread buttonLoop;

      virtual void resizeEvent(QResizeEvent*) override;
      virtual void wheelEvent(QWheelEvent*) override;
      virtual void paintEvent(QPaintEvent*) override;

      void loop();
      void watchButton();

   public slots:
      void takeSnapshot();
      void setPicturePath(const QString& s);
      void setPicturePrefix(const QString& s);

   signals:
      void cameraButtonPressed();
      void click(const QString&, int);

   public:
      Camera(QWidget* parent = 0);
      ~Camera();
      int start();
      int stop();
      int init(const CamDeviceSetting&);
      void change(const CamDeviceSetting&);
      const QString& picturePath() const { return _picturePath; }
      const QString& picturePrefix() const { return _picturePrefix; }
      };

#endif

