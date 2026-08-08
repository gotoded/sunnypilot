#include <QDebug>
#include <QTimer>
#include <QVBoxLayout>

#include "system/hardware/hw.h"
#include "selfdrive/ui/ui_scale.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/setup/updater.h"
#include "selfdrive/ui/qt/network/networking.h"

Updater::Updater(const QString &updater_path, const QString &manifest_path, QWidget *parent)
  : updater(updater_path), manifest(manifest_path), QStackedWidget(parent) {

  assert(updater.size());
  assert(manifest.size());

  // initial prompt screen
  prompt = new QWidget;
  {
    QVBoxLayout *layout = new QVBoxLayout(prompt);
    layout->setContentsMargins(ui_scale::px_w(128), ui_scale::px_h(139), ui_scale::px_w(128), ui_scale::px_h(56));

    QLabel *title = new QLabel(tr("Update Required"));
    title->setStyleSheet(QString("font-size: %1px; font-weight: bold;").arg(ui_scale::px_w(38)));
    layout->addWidget(title);

    layout->addSpacing(ui_scale::px_h(42));

    QLabel *desc = new QLabel(tr("An operating system update is required. Connect your device to Wi-Fi for the fastest update experience. The download size is approximately 1GB."));
    desc->setWordWrap(true);
    desc->setStyleSheet(QString("font-size: %1px;").arg(ui_scale::px_w(31)));
    layout->addWidget(desc);

    layout->addStretch();

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setSpacing(ui_scale::px_w(38));
    layout->addLayout(hlayout);

    QPushButton *connect = new QPushButton(tr("Connect to Wi-Fi"));
    connect->setObjectName("navBtn");
    QObject::connect(connect, &QPushButton::clicked, [=]() {
      setCurrentWidget(wifi);
    });
    hlayout->addWidget(connect);

    QPushButton *install = new QPushButton(tr("Install"));
    install->setObjectName("navBtn");
    install->setStyleSheet(R"(
      QPushButton {
        background-color: #465BEA;
      }
      QPushButton:pressed {
        background-color: #3049F4;
      }
    )");
    QObject::connect(install, &QPushButton::clicked, this, &Updater::installUpdate);
    hlayout->addWidget(install);
  }

  // wifi connection screen
  wifi = new QWidget;
  {
    QVBoxLayout *layout = new QVBoxLayout(wifi);
    layout->setContentsMargins(ui_scale::px_w(128), ui_scale::px_h(56), ui_scale::px_w(128), ui_scale::px_h(56));

    Networking *networking = new Networking(this, false);
    networking->setStyleSheet(QString("Networking { background-color: #292929; border-radius: %1px; }").arg(ui_scale::px_w(17)));
    layout->addWidget(networking, 1);

    QPushButton *back = new QPushButton(tr("Back"));
    back->setObjectName("navBtn");
    back->setStyleSheet(QString("padding-left: %1px; padding-right: %2px;").arg(ui_scale::px_w(77)).arg(ui_scale::px_w(77)));
    QObject::connect(back, &QPushButton::clicked, [=]() {
      setCurrentWidget(prompt);
    });
    layout->addWidget(back, 0, Qt::AlignLeft);
  }

  // progress screen
  progress = new QWidget;
  {
    QVBoxLayout *layout = new QVBoxLayout(progress);
    layout->setContentsMargins(ui_scale::px_w(83), ui_scale::px_h(183), ui_scale::px_w(83), ui_scale::px_h(83));
    layout->setSpacing(0);

    text = new QLabel(tr("Loading..."));
    text->setStyleSheet(QString("font-size: %1px; font-weight: 222;").arg(ui_scale::px_w(42)));
    layout->addWidget(text, 0, Qt::AlignTop);

    layout->addSpacing(ui_scale::px_h(100));

    bar = new QProgressBar();
    bar->setRange(0, 100);
    bar->setTextVisible(false);
    bar->setFixedHeight(ui_scale::px_h(103));
    layout->addWidget(bar, 0, Qt::AlignTop);

    layout->addStretch();

    reboot = new QPushButton(tr("Reboot"));
    reboot->setObjectName("navBtn");
    reboot->setStyleSheet(QString("padding-left: %1px; padding-right: %2px;").arg(ui_scale::px_w(22)).arg(ui_scale::px_w(22)));
    QObject::connect(reboot, &QPushButton::clicked, [=]() {
      Hardware::reboot();
    });
    layout->addWidget(reboot, 0, Qt::AlignLeft);
    reboot->hide();

    layout->addStretch();
  }

  addWidget(prompt);
  addWidget(wifi);
  addWidget(progress);

  setStyleSheet(QString(R"(
    * {
      color: white;
      outline: none;
      font-family: Inter;
    }
    Updater {
      color: white;
      background-color: black;
    }
    QPushButton#navBtn {
      height: %1px;
      font-size: %2px;
      font-weight: 148;
      border-radius: %3px;
      background-color: #333333;
    }
    QPushButton#navBtn:pressed {
      background-color: #444444;
    }
    QProgressBar {
      border: none;
      background-color: #292929;
    }
    QProgressBar::chunk {
      background-color: #364DEF;
    }
  )").arg(ui_scale::px_h(60)).arg(ui_scale::px_w(26)).arg(ui_scale::px_w(5)));
}

void Updater::installUpdate() {
  setCurrentWidget(progress);
  QObject::connect(&proc, &QProcess::readyReadStandardOutput, this, &Updater::readProgress);
  QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Updater::updateFinished);
  proc.setProcessChannelMode(QProcess::ForwardedErrorChannel);
  proc.start(updater, {"--swap", manifest});
}

void Updater::readProgress() {
  auto lines = QString(proc.readAllStandardOutput());
  for (const QString &line : lines.trimmed().split("\n")) {
    auto parts = line.split(":");
    if (parts.size() == 2) {
      text->setText(parts[0]);
      bar->setValue((int)parts[1].toDouble());
    } else {
      qDebug() << line;
    }
  }
  update();
}

void Updater::updateFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  qDebug() << "finished with " << exitCode;
  if (exitCode == 0) {
    Hardware::reboot();
  } else {
    text->setText(tr("Update failed"));
    reboot->show();
  }
}

int main(int argc, char *argv[]) {
  initApp(argc, argv);
  QApplication a(argc, argv);
  Updater updater(argv[1], argv[2]);
  setMainWindow(&updater);
  a.installEventFilter(&updater);
  return a.exec();
}
