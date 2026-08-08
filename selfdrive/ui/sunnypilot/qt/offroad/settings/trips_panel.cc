/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/trips_panel.h"

#include "selfdrive/ui/ui_scale.h"

TripsPanel::TripsPanel(QWidget* parent) : QFrame(parent) {
  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->setMargin(0);

  // main content
  main_layout->addSpacing(ui_scale::px_h(9));
  center_layout = new QStackedLayout();

  driveStatsWidget = new DriveStats;
  driveStatsWidget->setStyleSheet(QString("QLabel[type=\"title\"] { font-size: %1px; font-weight: 237; } QLabel[type=\"number\"] { font-size: %2px; font-weight: 237; } QLabel[type=\"unit\"] { font-size: %3px; font-weight: 142; color: #A0A0A0; }")
    .arg(ui_scale::px_w(24)).arg(ui_scale::px_w(37)).arg(ui_scale::px_w(24)));
  center_layout->addWidget(driveStatsWidget);

  main_layout->addLayout(center_layout, 1);

  setStyleSheet(QString("* { color: white; } TripsPanel > QLabel { font-size: %1px; }").arg(ui_scale::px_w(26)));
}
