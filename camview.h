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

#include <QMainWindow>
#include <QWidget>

#include <vector>
#include "camera.h"
#include "ui_camview.h"

#include <QComboBox>

//---------------------------------------------------------
//   CamDevice
//---------------------------------------------------------

class CamDevice {
      QString _shortName;
      QString _name;
      QString _device;

   public:
      CamDevice() {}
      CamDevice(const QString& a, const QString& b, const QString& c) : _shortName(a), _name(b), _device(c) {}
      const QString& shortName() const { return _shortName; }
      const QString& name() const      { return _name; }
      const QString& device() const    { return _device; }
      };

//---------------------------------------------------------
//   CamView
//---------------------------------------------------------

class CamView : public QMainWindow, public Ui::CamView {
      Q_OBJECT

      QComboBox* devs;
      std::vector<CamDevice> devices;

      void readDevices();

   private slots:
      void changeDevice(int);

   public:
      CamView(QWidget* parent = 0);
      };

Q_DECLARE_METATYPE(CamDevice*)

