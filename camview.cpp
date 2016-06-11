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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

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
      devs = new QComboBox(toolBar);
      for (auto& i : devices)
            devs->addItem(i.name(), QVariant::fromValue<CamDevice*>(&i));
      QAction* a = toolBar->addWidget(devs);
      device = &devices.front();

      sizes = new QComboBox(toolBar);
      for (auto& i : device->sizes())
            sizes->addItem(QString("%1 x %2").arg(i.width()).arg(i.height()), i);
      toolBar->insertWidget(a, sizes);

      connect(devs, SIGNAL(activated(int)), SLOT(changeDevice(int)));
      connect(sizes, SIGNAL(activated(int)), SLOT(changeSize(int)));
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
            QString name = i;
            QDir dd(d.filePath(i));
            QFile f(dd.filePath("name"));
            if (f.open(QIODevice::ReadOnly)) {
                  QByteArray ba = f.readAll();
                  f.close();
                  name = ba.simplified();
                  }
            // QDir ddd(dd.filePath("device"));

            CamDevice cd(i, name, "/dev/"+i);

            char* videodevice = cd.device().toLocal8Bit().data();
            int fd = open(videodevice, O_RDWR);
            if (fd == -1) {
                  fprintf(stderr, "CamView: cannot open <%s>: %s\n", videodevice, strerror(errno));
                  return;
                  }
            struct v4l2_frmsizeenum s;
            memset(&s, 0, sizeof(s));
            s.pixel_format = V4L2_PIX_FMT_MJPEG;

            for (int idx = 0;;++idx) {
                  s.index = idx;
                  int ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &s);
                  if (ret == -1) {
                        if (errno != EINVAL) {
                              fprintf(stderr, "CamView: <%s>: %d ioctl failed %s\n",
                                 qPrintable(cd.device()),
                                 errno, strerror(errno));
                              }
                        break;
                        }
                  if (s.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                        cd.sizes().push_back(QSize(s.discrete.width, s.discrete.height));
                  else if (s.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
                        }
                  else if (s.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                        }
                  }
            ::close(fd);
            devices.push_back(cd);
            }
      }

//---------------------------------------------------------
//   changeDevice
//---------------------------------------------------------

void CamView::changeDevice(int idx)
      {
      device = devs->itemData(idx).value<CamDevice*>();
      sizes->clear();
      for (auto& i : device->sizes())
            sizes->addItem(QString("%1 x %2").arg(i.width()).arg(i.height()), i);
      sizes->setCurrentIndex(sizes->count()-1);
      cam->setDevice(device->device(), device->sizes().back());   // set to highest resolution
      }

//---------------------------------------------------------
//   changeSize
//---------------------------------------------------------

void CamView::changeSize(int idx)
      {
      QSize s = sizes->itemData(idx).toSize();
      cam->setSize(s);
      }


