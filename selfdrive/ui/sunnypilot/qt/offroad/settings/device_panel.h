/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/settings.h"
#include "selfdrive/ui/sunnypilot/qt/widgets/controls.h"
#include "selfdrive/ui/ui_scale.h"

class DevicePanelSP : public DevicePanel {
  Q_OBJECT

public:
  explicit DevicePanelSP(SettingsWindowSP *parent = 0);
  void showEvent(QShowEvent *event) override;
  void setOffroadMode();
  void updateState();
  void resetSettings();

private:
  std::map<QString, PushButtonSP*> buttons;
  PushButtonSP *offroadBtn;

  const QString alwaysOffroadStyle = QString("PushButtonSP { border-radius: %1px; font-size: %2px; font-weight: 166; height: %3px; padding: 0 %4px 0 %4px; color: #FFFFFF; background-color: #393939; } PushButtonSP:pressed { background-color: #4A4A4A; }")
    .arg(ui_scale::px_w(9)).arg(ui_scale::px_w(23)).arg(ui_scale::px_h(60)).arg(ui_scale::px_w(9));

  const QString autoOffroadStyle = QString("PushButtonSP { border-radius: %1px; font-size: %2px; font-weight: 166; height: %3px; padding: 0 %4px 0 %4px; color: #FFFFFF; background-color: #E22C2C; } PushButtonSP:pressed { background-color: #FF2424; }")
    .arg(ui_scale::px_w(9)).arg(ui_scale::px_w(23)).arg(ui_scale::px_h(60)).arg(ui_scale::px_w(9));

  const QString rebootButtonStyle = QString("PushButtonSP { border-radius: %1px; font-size: %2px; font-weight: 166; height: %3px; padding: 0 %4px 0 %4px; color: #FFFFFF; background-color: #393939; } PushButtonSP:pressed { background-color: #4A4A4A; }")
    .arg(ui_scale::px_w(9)).arg(ui_scale::px_w(23)).arg(ui_scale::px_h(60)).arg(ui_scale::px_w(9));

  const QString powerOffButtonStyle = QString("PushButtonSP { border-radius: %1px; font-size: %2px; font-weight: 166; height: %3px; padding: 0 %4px 0 %4px; color: #FFFFFF; background-color: #E22C2C; } PushButtonSP:pressed { background-color: #FF2424; }")
    .arg(ui_scale::px_w(9)).arg(ui_scale::px_w(23)).arg(ui_scale::px_h(60)).arg(ui_scale::px_w(9));
};
