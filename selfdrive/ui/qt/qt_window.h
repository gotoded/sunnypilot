#pragma once

#include <string>

#include <QApplication>
#include <QScreen>
#include <QWidget>

#ifdef QCOM2
#include <qpa/qplatformnativeinterface.h>
#include <wayland-client-protocol.h>
#include <QPlatformSurfaceEvent>
#endif

#include "system/hardware/hw.h"

const QString ASSET_PATH = ":/";
const QSize DEVICE_SCREEN_SIZE = {1024, 600};

// 获取当前主屏幕实际分辨率；未获取到屏幕信息时回退到 DEVICE_SCREEN_SIZE
inline QSize deviceScreenSize() {
  if (QGuiApplication::primaryScreen() != nullptr) {
    return QGuiApplication::primaryScreen()->size();
  }
  return DEVICE_SCREEN_SIZE;
}

void setMainWindow(QWidget *w);
