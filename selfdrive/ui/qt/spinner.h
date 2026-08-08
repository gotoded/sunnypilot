#include <array>

#include <QLabel>
#include <QPixmap>
#include <QProgressBar>
#include <QSocketNotifier>
#include <QVariantAnimation>
#include <QWidget>

#include "selfdrive/ui/ui_scale.h"

constexpr int spinner_fps = 30;
inline QSize spinner_size() { return QSize(ui_scale::px_w(170), ui_scale::px_h(170)); }

class TrackWidget : public QWidget  {
  Q_OBJECT
public:
  TrackWidget(QWidget *parent = nullptr);

private:
  void paintEvent(QPaintEvent *event) override;
  std::array<QPixmap, spinner_fps> track_imgs;
  QVariantAnimation m_anim;
};

class Spinner : public QWidget {
  Q_OBJECT

public:
  explicit Spinner(QWidget *parent = 0);

private:
  QLabel *text;
  QProgressBar *progress_bar;
  QSocketNotifier *notifier;

public slots:
  void update(int n);
};
