#pragma once

// UI 自动缩放基础设施
//
// 所有 UI 像素尺寸（font-size / height / width / padding / margin 等）都应以
// 设计基准分辨率（UI_BASE_WIDTH x UI_BASE_HEIGHT）编写，并通过 ui_scale::px_w()
// / px_h() 换算，使 UI 在窗口尺寸变化时自动适配，无需手动逐个调整。
//
// 用法：
//   buttonStyle = QString("QPushButton { font-size: %1px; height: %2px; }")
//     .arg(ui_scale::px_w(23)).arg(ui_scale::px_h(60));
//
// 换分辨率/全屏自适应时无需手动调整，缩放因子自动跟随主屏幕实际分辨率
// （deviceScreenSize() 返回当前主屏幕尺寸）。例如屏幕为 1280x720 时，
// scale = (1.25, 1.2)，全部 UI 自动放大铺满屏幕。

#include <algorithm>
#include <cmath>

#include "selfdrive/ui/qt/qt_window.h"

namespace ui_scale {

// 设计基准分辨率（编写 UI 像素尺寸时的参照，缩放因子为 1.0）
constexpr int UI_BASE_WIDTH = 1024;
constexpr int UI_BASE_HEIGHT = 600;

// 水平缩放因子：当前屏幕宽度 / 设计基准宽度
inline float scale_w() {
  return (float)deviceScreenSize().width() / (float)UI_BASE_WIDTH;
}

// 垂直缩放因子：当前屏幕高度 / 设计基准高度
inline float scale_h() {
  return (float)deviceScreenSize().height() / (float)UI_BASE_HEIGHT;
}

// 按水平比例缩放（用于字号、宽度、水平间距）
inline int px_w(int v) { return std::lround(v * scale_w()); }

// 按垂直比例缩放（用于高度、纵向间距）
inline int px_h(int v) { return std::lround(v * scale_h()); }

// 按最小比例缩放（用于圆形元素，保证不超出）
inline int px_min(int v) { return std::lround(v * std::min(scale_w(), scale_h())); }

}  // namespace ui_scale
