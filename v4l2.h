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

#ifndef __V4L2_H__
#define __V4L2_H__

#include <QString>
#include <QImage>

#define NB_BUFFER 4

//---------------------------------------------------------
//   V4l2
//    video for linux II c++ wrapper
//---------------------------------------------------------

class V4l2 {
      int     fd           { -1 };
      QString path;
      void* mem[NB_BUFFER];

      int isControl(int control, struct v4l2_queryctrl* queryctrl);

   public:
      V4l2();
      ~V4l2();
      bool open(const QString&);
      bool close();

      int getFd()  { return fd; }

      int getControl(int control);
      int setControl(int control, int value);
      int upControl(int control);
      int downControl(int control);
      int toggleControl(int control);
      bool resetControl(int control);

      bool canVideoCapture();
      bool canStreaming();

      bool setMjpegFormat(int w, int h);
      bool setFramerate(int fps);

      QImage grab();
      bool initBuffers();
      bool freeBuffers();
      };

#endif

