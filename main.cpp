//=============================================================================
//  Cam
//  Camera Modul
//
//  Copyright (C) 2016 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QApplication>
#include <QWidget>
#include <QtPlugin>
#include <QPluginLoader>
#include <QImageReader>
#include <QJsonArray>

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "cam.h"

Q_IMPORT_PLUGIN(MjpegImageIOPlugin)

//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      QApplication a(argc, argv);

      Camera* mw = new Camera(0);
      mw->init("/dev/video1", 1600, 1200, 30);
      mw->start();
      mw->resize(1600, 1200);
      mw->show();

      return a.exec();
      }

