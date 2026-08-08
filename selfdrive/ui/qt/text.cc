#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>

#include "system/hardware/hw.h"
#include "selfdrive/ui/ui_scale.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"

int main(int argc, char *argv[]) {
  initApp(argc, argv);
  QApplication a(argc, argv);
  QWidget window;
  setMainWindow(&window);

  QGridLayout *main_layout = new QGridLayout(&window);
  main_layout->setMargin(ui_scale::px_min(23));

  QLabel *label = new QLabel(argv[1]);
  label->setWordWrap(true);
  label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  ScrollView *scroll = new ScrollView(label);
  scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  main_layout->addWidget(scroll, 0, 0, Qt::AlignTop);

  // Scroll to the bottom
  QObject::connect(scroll->verticalScrollBar(), &QAbstractSlider::rangeChanged, [=]() {
    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
  });

  QPushButton *btn = new QPushButton();
#ifdef __aarch64__
  btn->setText(QObject::tr("Reboot"));
  QObject::connect(btn, &QPushButton::clicked, [=]() {
    Hardware::reboot();
  });
#else
  btn->setText(QObject::tr("Exit"));
  QObject::connect(btn, &QPushButton::clicked, &a, &QApplication::quit);
#endif
  main_layout->addWidget(btn, 0, 0, Qt::AlignRight | Qt::AlignBottom);

  window.setStyleSheet(QString(R"(
    * {
      outline: none;
      color: white;
      background-color: black;
      font-size: %1px;
    }
    QPushButton {
      padding: %2px;
      padding-right: %3px;
      padding-left: %4px;
      border: 1px solid white;
      border-radius: %5px;
      margin-right: %6px;
    }
  )").arg(ui_scale::px_w(31)).arg(ui_scale::px_min(18)).arg(ui_scale::px_w(38)).arg(ui_scale::px_w(38)).arg(ui_scale::px_min(8)).arg(ui_scale::px_w(16)));

  return a.exec();
}
