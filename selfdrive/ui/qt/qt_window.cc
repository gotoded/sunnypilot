#include "selfdrive/ui/qt/qt_window.h"

void setMainWindow(QWidget *w) {
  const float scale = util::getenv("SCALE", 1.0f);
  // 获取当前屏幕大小，主窗口自适应全屏
  const QSize sz = deviceScreenSize();

  if (scale == 1.0) {
    w->setMinimumSize(QSize(240, 180)); // allow resize smaller than fullscreen
    w->setMaximumSize(sz);
    w->resize(sz);
  } else {
    w->setFixedSize(sz * scale);
  }

  // 获取屏幕大小，自适应全屏
  w->setWindowState(Qt::WindowFullScreen);
  w->show();

#ifdef QCOM2
  QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
  wl_surface *s = reinterpret_cast<wl_surface*>(native->nativeResourceForWindow("surface", w->windowHandle()));
  wl_surface_set_buffer_transform(s, WL_OUTPUT_TRANSFORM_270);
  wl_surface_commit(s);

  w->setWindowState(Qt::WindowFullScreen);
  w->setVisible(true);

  // ensure we have a valid eglDisplay, otherwise the ui will silently fail
  void *egl = native->nativeResourceForWindow("egldisplay", w->windowHandle());
  assert(egl != nullptr);
#endif
}


extern "C" {
  void set_main_window(void *w) {
    setMainWindow((QWidget*)w);
  }
}
