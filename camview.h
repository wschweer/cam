//============================================================================
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

#include <QMainWindow>
#include <QWidget>

#include <vector>
#include "camera.h"
#include "ui_camview.h"

#include <QComboBox>
#include <QSize>

//---------------------------------------------------------
//   CamView
//---------------------------------------------------------

class CamView : public QMainWindow, public Ui::CamView {
      Q_OBJECT

      QComboBox* devs;
      QComboBox* sizes;
      QComboBox* fps;

      std::vector<CamDevice> devices;
      CamDeviceSetting setting;    // current setting

      void readDevices();

      void changeCam(const CamDeviceSetting&);
      void setCam(const CamDeviceSetting&);
      void updateSetting();

   private slots:
      void changeDevice(int);
      void changeSize(int);
      void changeFps(int);

   public:
      CamView(QWidget* parent = 0);
      };

Q_DECLARE_METATYPE(CamDevice*)

