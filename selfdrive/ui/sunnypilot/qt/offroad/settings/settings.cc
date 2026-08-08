/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/settings.h"

#include "selfdrive/ui/ui_scale.h"
#include "selfdrive/ui/sunnypilot/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/offroad/developer_panel.h"
#include "selfdrive/ui/qt/offroad/firehose.h"
#include "selfdrive/ui/sunnypilot/qt/network/networking.h"

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/device_panel.h"
#include "selfdrive/ui/sunnypilot/qt/offroad/settings/software_panel.h"
#include "selfdrive/ui/sunnypilot/qt/offroad/settings/sunnylink_panel.h"
#include "selfdrive/ui/sunnypilot/qt/offroad/settings/lateral_panel.h"
#include "selfdrive/ui/sunnypilot/qt/offroad/settings/trips_panel.h"
#include "selfdrive/ui/sunnypilot/qt/offroad/settings/vehicle_panel.h"

TogglesPanelSP::TogglesPanelSP(SettingsWindowSP *parent) : TogglesPanel(parent) {
  QObject::connect(uiStateSP(), &UIStateSP::uiUpdate, this, &TogglesPanelSP::updateState);
}

void TogglesPanelSP::updateState(const UIStateSP &s) {
  TogglesPanel::updateState(s);
}

SettingsWindowSP::SettingsWindowSP(QWidget *parent) : SettingsWindow(parent) {
  // setup two main layouts
  sidebar_widget = new QWidget;
  QVBoxLayout *sidebar_layout = new QVBoxLayout(sidebar_widget);
  panel_widget = new QStackedWidget();

  // setup layout for close button
  QVBoxLayout *close_btn_layout = new QVBoxLayout;
  close_btn_layout->setContentsMargins(0, 0, 0, ui_scale::px_h(11));

  // close button
  QPushButton *close_btn = new QPushButton(tr("×"));
  close_btn->setStyleSheet(QString("QPushButton { font-size: %1px; padding-bottom: %2px; border-radius: %3px; background-color: #292929; font-weight: 148; } QPushButton:pressed { background-color: #3B3B3B; }")
    .arg(ui_scale::px_w(67)).arg(ui_scale::px_h(10)).arg(ui_scale::px_w(36)));
  close_btn->setFixedSize(ui_scale::px_w(72), ui_scale::px_h(80));
  close_btn_layout->addWidget(close_btn, 0, Qt::AlignLeft);
  QObject::connect(close_btn, &QPushButton::clicked, this, &SettingsWindowSP::closeSettings);

  // setup buttons widget
  QWidget *buttons_widget = new QWidget;
  QVBoxLayout *buttons_layout = new QVBoxLayout(buttons_widget);
  buttons_layout->setMargin(0);
  buttons_layout->addSpacing(ui_scale::px_h(6));

  // setup panels
  DevicePanelSP *device = new DevicePanelSP(this);
  QObject::connect(device, &DevicePanelSP::reviewTrainingGuide, this, &SettingsWindowSP::reviewTrainingGuide);
  QObject::connect(device, &DevicePanelSP::showDriverView, this, &SettingsWindowSP::showDriverView);

  TogglesPanelSP *toggles = new TogglesPanelSP(this);
  QObject::connect(this, &SettingsWindowSP::expandToggleDescription, toggles, &TogglesPanel::expandToggleDescription);

  auto networking = new NetworkingSP(this);
  QObject::connect(uiState()->prime_state, &PrimeState::changed, networking, &NetworkingSP::setPrimeType);

  QList<PanelInfo> panels = {
    PanelInfo("   " + tr("Device"), device, "../../sunnypilot/selfdrive/assets/offroad/icon_home.svg"),
    PanelInfo("   " + tr("Network"), networking, "../assets/offroad/icon_network.png"),
    PanelInfo("   " + tr("sunnylink"), new SunnylinkPanel(this), "../assets/offroad/icon_wifi_strength_full.svg"),
    PanelInfo("   " + tr("Toggles"), toggles, "../../sunnypilot/selfdrive/assets/offroad/icon_toggle.png"),
    PanelInfo("   " + tr("Software"), new SoftwarePanelSP(this), "../../sunnypilot/selfdrive/assets/offroad/icon_software.png"),
    PanelInfo("   " + tr("Steering"), new LateralPanel(this), "../../sunnypilot/selfdrive/assets/offroad/icon_lateral.png"),
    PanelInfo("   " + tr("Trips"), new TripsPanel(this), "../../sunnypilot/selfdrive/assets/offroad/icon_trips.png"),
    PanelInfo("   " + tr("Vehicle"), new VehiclePanel(this), "../../sunnypilot/selfdrive/assets/offroad/icon_vehicle.png"),
    PanelInfo("   " + tr("Firehose"), new FirehosePanel(this), "../../sunnypilot/selfdrive/assets/offroad/icon_firehose.svg"),
    PanelInfo("   " + tr("Developer"), new DeveloperPanel(this), "../assets/offroad/icon_shell.png"),
  };

  nav_btns = new QButtonGroup(this);
  for (auto &[name, panel, icon] : panels) {
    QPushButton *btn = new QPushButton(name);
    btn->setCheckable(true);
    btn->setChecked(nav_btns->buttons().size() == 0);
    btn->setIcon(QIcon(QPixmap(icon)));
    btn->setIconSize(QSize(ui_scale::px_w(26), ui_scale::px_h(26)));
    btn->setStyleSheet(QString("QPushButton { border-radius: %1px; width: %2px; height: %3px; color: #bdbdbd; border: none; background: none; font-size: %4px; font-weight: 237; text-align: left; padding-left: %5px; } QPushButton:checked { background-color: #696868; color: white; } QPushButton:pressed { color: #ADADAD; }")
      .arg(ui_scale::px_w(9)).arg(ui_scale::px_w(189)).arg(ui_scale::px_h(51)).arg(ui_scale::px_w(24)).arg(ui_scale::px_w(10)));
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    nav_btns->addButton(btn);
    buttons_layout->addWidget(btn, 0, Qt::AlignLeft | Qt::AlignBottom);

    const int lr_margin = (name != ("   " + tr("Network"))) ? 23 : 0;  // Network panel handles its own margins
    panel->setContentsMargins(ui_scale::px_w(lr_margin), ui_scale::px_h(13), ui_scale::px_w(lr_margin), ui_scale::px_h(13));

    ScrollViewSP *panel_frame = new ScrollViewSP(panel, this);
    panel_widget->addWidget(panel_frame);

    QObject::connect(btn, &QPushButton::clicked, [=, w = panel_frame]() {
      btn->setChecked(true);
      panel_widget->setCurrentWidget(w);
    });
  }
  sidebar_layout->setContentsMargins(ui_scale::px_w(23), ui_scale::px_h(26), ui_scale::px_w(12), ui_scale::px_h(26));

  // main settings layout, sidebar + main panel
  QHBoxLayout *main_layout = new QHBoxLayout(this);

  // add layout for close button
  sidebar_layout->addLayout(close_btn_layout);

  // add layout for buttons scrolling
  ScrollViewSP *buttons_scrollview = new ScrollViewSP(buttons_widget, this);
  sidebar_layout->addWidget(buttons_scrollview);

  sidebar_widget->setFixedWidth(ui_scale::px_w(237));
  main_layout->addWidget(sidebar_widget);
  main_layout->addWidget(panel_widget);

  setStyleSheet(QString("* { color: white; font-size: %1px; } SettingsWindow { background-color: black; } QStackedWidget, ScrollViewSP { background-color: black; border-radius: %2px; }")
    .arg(ui_scale::px_w(18)).arg(ui_scale::px_w(14)));
}
