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
#include <linux/videodev2.h>

#include <QDir>
#include <QSettings>
#include "camview.h"

//---------------------------------------------------------
//   CamView
//---------------------------------------------------------

CamView::CamView(QWidget* parent)
   : QMainWindow(parent)
      {
      setupUi(this);

      readDevices();
      QSettings settings;
      QString dname  = settings.value("device", devices.front().shortName).toString();
      if (devices.empty()) {
            fprintf(stderr, "CamView: no cameras found\n");
            exit(-1);
            }
      bool found = false;
      for (auto& i : devices) {
            if (i.shortName == dname) {
                  setting.device = &i;
                  found = true;
                  break;
                  }
            }
      if (!found)
            setting.device = &devices[0];
      setting.size = settings.value("size", devices.front().formats.front().size).toSize();
      setting.fps  = settings.value("fps", 30).toInt();

      for (auto& i : devices) {
            devs->addItem(i.name, QVariant::fromValue<CamDevice*>(&i));
            if (i.shortName == dname)
                  devs->setCurrentIndex(devs->count()-1);
            }

      connect(devs,          SIGNAL(activated(int)), SLOT(changeDevice(int)));
      connect(sizes,         SIGNAL(activated(int)), SLOT(changeSize(int)));
      connect(fps,           SIGNAL(activated(int)), SLOT(changeFps(int)));
      connect(cam,           SIGNAL(click(const QString&, int)), statusBar(), SLOT(showMessage(const QString&,int)));
      connect(picturePath,   SIGNAL(textEdited(const QString&)), cam, SLOT(setPicturePath(const QString&)));
      connect(picturePrefix, SIGNAL(textEdited(const QString&)), cam, SLOT(setPicturePrefix(const QString&)));
      connect(click,         SIGNAL(clicked()), cam, SLOT(takeSnapshot()));
      setCam(setting);
      picturePath->setText(cam->picturePath());
      picturePrefix->setText(cam->picturePrefix());

      cam->start();
      }

//---------------------------------------------------------
//   setCam
//---------------------------------------------------------

void CamView::setCam(const CamDeviceSetting& s)
      {
      setting = s;
      cam->init(s);
      updateSetting();
      }

//---------------------------------------------------------
//   setDevice
//---------------------------------------------------------

void CamView::changeCam(const CamDeviceSetting& s)
      {
      setting = s;
      cam->change(s);
      updateSetting();
      }

//---------------------------------------------------------
//   updateSetting
//---------------------------------------------------------

void CamView::updateSetting()
      {
      sizes->clear();
      fps->clear();
      for (auto& i : setting.device->formats) {
            sizes->addItem(QString("%1 x %2").arg(i.size.width()).arg(i.size.height()), i.size);
            if (i.size == setting.size) {
                  for (auto& ii : i.frameRates) {
                        fps->addItem(QString("%1").arg(ii), ii);
                        if (ii == setting.fps)
                              fps->setCurrentIndex(fps->count()-1);
                        }
                  sizes->setCurrentIndex(sizes->count()-1);
                  }
            }
      QSettings settings;
      settings.setValue("device", setting.device->shortName);
      settings.setValue("size", setting.size);
      settings.setValue("fps", setting.fps);
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

            CamDevice cd;
            cd.shortName = i;
            cd.name      = name;
            cd.device    = "/dev/" + i;

            char* videodevice = cd.device.toLocal8Bit().data();
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
                                 qPrintable(cd.device),
                                 errno, strerror(errno));
                              }
                        break;
                        }
                  if (s.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                        CamDeviceFormat fmt;
                        fmt.size = QSize(s.discrete.width, s.discrete.height);

                        struct v4l2_frmivalenum f;
                        memset(&f, 0, sizeof(f));
                        f.pixel_format = V4L2_PIX_FMT_MJPEG;
                        f.width        = s.discrete.width;
                        f.height       = s.discrete.height;

                        for (int k = 0;; ++k) {
                              f.index = k;
                              int ret = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &f);
                              if (ret == -1) {
                                    if (errno != EINVAL) {
                                          fprintf(stderr, "CamView: <%s>: %d ioctl failed %s\n",
                                             qPrintable(cd.device),
                                             errno, strerror(errno));
                                          }
                                    break;
                                    }
                              if (f.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                                    fmt.frameRates.push_back(f.discrete.denominator);
                                    }
                              else if (f.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
                                    ;
                                    }
                              else if (f.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
                                    ;
                                    }
                              }
                        cd.formats.push_back(fmt);
                        }
                  else if (s.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
                        break;
                        }
                  else if (s.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                        break;
                        }
                  }
            ::close(fd);
            // search for button device
            QDir ddd(dd.filePath("device/input"));
            for (auto i : ddd.entryList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
                  if (i.startsWith("input")) {
                        QDir dddd(ddd.filePath(i));
                        for (auto i : dddd.entryList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
                              if (i.startsWith("event")) {
                                    cd.buttonDevice = "/dev/input/" + i;
                                    break;
                                    }
                              }
                        }
                  break;
                  }

            devices.push_back(cd);
            }
      }

//---------------------------------------------------------
//   changeDevice
//---------------------------------------------------------

void CamView::changeDevice(int idx)
      {
      CamDevice* d = devs->itemData(idx).value<CamDevice*>();
      if (d == setting.device)
            return;
      setting.device = d;
      changeCam(setting);
      }

//---------------------------------------------------------
//   changeSize
//---------------------------------------------------------

void CamView::changeSize(int idx)
      {
      QSize s = sizes->itemData(idx).toSize();
      if (s == setting.size)
            return;
      setting.size = s;
      changeCam(setting);
      }

//---------------------------------------------------------
//   changeFps
//---------------------------------------------------------

void CamView::changeFps(int idx)
      {
      int i = fps->itemData(idx).toInt();
      if (i == setting.fps)
            return;
      setting.fps = i;
      changeCam(setting);
      }

