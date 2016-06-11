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

#include <QDir>
#include "camview.h"

//---------------------------------------------------------
//   CamView
//---------------------------------------------------------

CamView::CamView(QWidget* parent)
   : QMainWindow(parent)
      {
      setupUi(this);
      cam->init("/dev/video0", 800, 600, 30);
      cam->start();

      readDevices();
      devs = new QComboBox(this);
      for (auto& i : devices)
            devs->addItem(i.name(), QVariant::fromValue<CamDevice*>(&i));

      toolBar->addWidget(devs);
      connect(devs, SIGNAL(activated(int)), SLOT(changeDevice(int)));
      }

//---------------------------------------------------------
//   readDevices
//---------------------------------------------------------

void CamView::readDevices()
      {
      devices.clear();

      QDir d("/sys/class/video4linux/");
      for (auto i : d.entryList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
            if (!i.startsWith("video"))
                  continue;
            CamDevice cd(i, i, "/dev/"+i);
            devices.push_back(cd);
            }
      }

//---------------------------------------------------------
//   changeDevice
//---------------------------------------------------------

void CamView::changeDevice(int idx)
      {
      CamDevice* cd = devs->itemData(idx).value<CamDevice*>();
      cam->setDevice(cd->device());
      }


