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

#include <QApplication>
#include <QWidget>
#include <QtPlugin>
#include <QPluginLoader>
#include <QImageReader>
#include <QDir>
#include <QMainWindow>

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "camview.h"

Q_IMPORT_PLUGIN(MjpegImageIOPlugin)

//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      QApplication a(argc, argv);

      CamView* v = new CamView(0);
      v->show();
      return a.exec();
      }


