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

#include <QImage>
extern "C" {
      #include <libavcodec/avcodec.h>
      #include <libswscale/swscale.h>
      }
#include "mjpeg.h"

//---------------------------------------------------------
//   MjpegImageIOHandler
//---------------------------------------------------------

MjpegImageIOHandler::MjpegImageIOHandler()
      {
      avcodec_register_all();
      codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
      if (!codec) {
            printf("codec not found\n");
            abort();
            }
      c = avcodec_alloc_context3(codec);
      if (avcodec_open2(c, codec, 0) < 0) {
            printf("open codec failed\n");
            exit(1);
            }
      }

MjpegImageIOHandler::~MjpegImageIOHandler()
      {
      if (imgConvertCtx)
            sws_freeContext(imgConvertCtx);
      avcodec_close(c);
      av_free(c);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

bool MjpegImageIOHandler::read(QImage* image)
      {
      AVFrame* f = av_frame_alloc();

      AVPacket p;
      av_init_packet(&p);

      QByteArray b = device()->readAll();
      p.data      = (unsigned char*)b.data();
      p.size      = b.size();

      int len = avcodec_send_packet(c, &p);
      if (len < 0) {
            printf("send packet failed\n");
            b.clear();
            return false;
            }
      if (avcodec_receive_frame(c, f) < 0) {
            printf("receive frame failed\n");
            b.clear();
            return false;
            }
      if (!imgConvertCtx || (image->width() != f->width) || (image->height() != f->height)) {
            QImage i(f->width, f->height, QImage::Format_RGB32);
            *image = i;
            imgConvertCtx = sws_getContext(
               f->width, f->height,
               // c->pix_fmt,
               AV_PIX_FMT_YUV422P,  // hack to avoid deprecated warning
               f->width, f->height, AV_PIX_FMT_RGB32,
               SWS_BILINEAR,
               0,
               0,
               0);
            if (!imgConvertCtx) {
                  printf("no conversion context\n");
                  return false;
                  }
            }
      int stride    = image->bytesPerLine();
      uint8_t* data = image->bits();
      sws_scale(
            imgConvertCtx,
            f->data,
            f->linesize,
            0,
            f->height,
            &data,
            &stride
            );
      av_frame_free(&f);
      return true;
      }

//---------------------------------------------------------
//   capabilities
//---------------------------------------------------------

QImageIOPlugin::Capabilities MjpegImageIOPlugin::capabilities(QIODevice*, const QByteArray& ba) const
      {
      if (ba == "mjpeg")
            return CanRead;
      return QImageIOPlugin::Capabilities();
      }

//---------------------------------------------------------
//   create
//---------------------------------------------------------

QImageIOHandler* MjpegImageIOPlugin::create(QIODevice* device, const QByteArray& format) const
      {
      if (format.toLower() == "mjpeg") {
            MjpegImageIOHandler* h = new MjpegImageIOHandler();
            h->setDevice(device);
            return h;
            }
      return 0;
      }


