#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGraphicsEllipseItem>
#include <QFileDialog>
#include <QToolBar>
#include <QPixmap>
#include <QLabel>
#include <QDebug>
#include <QImage>
#include <QStack>
#include <QTime>

#include "viewport.h"

template<class T>
void floodFill(Matrix<uint8_t>& src, int x, int y, T color, int connectivity = 4) {
  static const int dx[] = {-1, 1, 0, 0, 1, -1, 1, 1 };
  static const int dy[] = {0, 0, 1, -1, 1, 1, -1, -1};

  QStack<QPoint> stack;
  stack.push_back(QPoint(x, y));

  T field_color = src(x, y);
  src(x, y) = color;
  do {
    auto cur = stack.back();
    stack.pop_back();

    for (int i = 0; i < connectivity; ++i) {
      QPoint tmp(cur.x() + dx[i], cur.y() + dy[i]);
      if (tmp.x() < 0 || tmp.x() >= src.width()) continue;
      if (tmp.y() < 0 || tmp.y() >= src.height()) continue;
      if (src(tmp) == field_color) {
        stack.push_back(tmp);
        src(tmp) = color;
      }
    }
  } while (!stack.isEmpty());
}

/* MainWindow */
MainWindow::MainWindow(const QString& filename, QWidget* parent):
  QMainWindow(parent),
  ui_(new Ui::MainWindow),
  viewport_(new Viewport(this))
{
  ui_->setupUi(this);

  createToolbar();
  setCentralWidget(viewport_);

  connect(ui_->actionOpen, SIGNAL(triggered()), this, SLOT(slotLoadFile()));
  ui_->actionOpen->setShortcut(QKeySequence("CTRL+O"));

  viewport_->setScene(new QGraphicsScene());

  if (!filename.isEmpty()) {
    load(filename);
  }
}

MainWindow::~MainWindow() {
  delete ui_;
}

void MainWindow::createToolbar() {
  ui_->mainToolBar->addAction(QIcon("open.png"), "Open file", this, SLOT(slotLoadFile()));
  ui_->mainToolBar->addSeparator();
  ui_->mainToolBar->addAction(QIcon("new.png"), "Clear", this, SLOT(slotClear()));
  ui_->mainToolBar->addAction(QIcon("run.png"), "Run", this, SLOT(slotRun()));
  ui_->mainToolBar->addSeparator();

  show_labels_action = ui_->mainToolBar->addAction(QIcon("draw-labels.png"), "Show Labels", this, SLOT(slotSetVisibleLabels()));
  show_labels_action->setCheckable(true);
  show_labels_action->setChecked(true);
}

void MainWindow::load(const QString& filename) {
  image = source = QImage(filename);
  viewport_->setScene(source);

  image = image.convertToFormat(QImage::Format_RGB888);

  viewport_->initial_marking = true;
  viewport_->current_source.clear();
  viewport_->current_sink.clear();
  viewport_->source.clear();
  viewport_->sink.clear();
}

QImage MainWindow::applyMask() {
  static const int dx[] = {-1, 0, 1, 0};
  static const int dy[] = {0, 1, 0, -1};

  auto canvas = image;
  model = std::move(Matrix<uint8_t>(image.size(), 1));
  for (int y = 0; y < mask.height(); ++y) {
    for (int x = 0; x < mask.width(); ++x) {
      if (!mask(x, y)) {
        model(x, y) = 0;
        auto pixel = canvas.pixel(x, y);
        auto r = qRed(pixel) * 0.5 + 255 * 0.5;
        auto g = qGreen(pixel) * 0.5 + 255 * 0.5;
        auto b = qBlue(pixel) * 0.5 + 255 * 0.5;
        canvas.setPixel(x, y, qRgb(r, g, b));
      }
    }
  }

  for (int x = 0; x < mask.width(); ++x) {
    for (int y = 0; y < mask.height(); ++y) {
      if (mask(x, y) == PixelClass::Foreground) {
        auto pixel = canvas.pixel(x, y);
        auto border = qRgb(qRed(pixel) * 0.25, qGreen(pixel) * 0.25, qBlue(pixel) * 0.25 + 255 * 0.75);

        if (x == 0 || x == mask.width() - 1 || y == 0 || y == mask.height() - 1) {
          canvas.setPixel(x, y, border);
        }
        else {
          for (int i = 0; i < 4; ++i) {
            if (mask(x + dx[i], y + dy[i]) == PixelClass::Background) {
              canvas.setPixel(x + dx[i], y + dy[i], border);
            }
          }
        }
      }
    }
  }

  // labels of regions
  marked = model;
  int counter = 2;
  for (int x = 0; x < marked.width(); ++x) {
    for (int y = 0; y < marked.height(); ++y) {
      if (marked(x, y) < 2) {
        floodFill(marked, x, y, counter++);
      }
    }
  }

  return canvas;
}

int MainWindow::intention(Matrix<uint8_t> model, const QVector<int>& source, const QVector<int>& sink) {
  user_intention = std::move(Matrix<uint8_t>(image.size(), 1));

  bool all_foreground = true;
  bool all_background = true;

  QVector<int> labels = source;
  for (auto &vert : (labels << sink)) {
    if (model(vert % image.width(), vert / image.width()) != PixelClass::Foreground) all_foreground = false;
    if (model(vert % image.width(), vert / image.width()) != PixelClass::Background) all_background = false;
  }

  if (all_foreground) {
    for (int x = 0; x < model.width(); ++x) {
      for (int y = 0; y < model.height(); ++y) {
        if (model(x, y) == PixelClass::Background) {
          user_intention(x, y) = 0;
        }
      }
    }

    return UserAction::FB;
  }
  else if (all_background) {
    for (int x = 0; x < model.width(); ++x) {
      for (int y = 0; y < model.height(); ++y) {
        if (model(x, y) == PixelClass::Foreground) {
          user_intention(x, y) = 0;
        }
      }
    }

    return UserAction::BF;
  }
  else {
    QSet<int> markers;
    for (auto vert : source + sink) {
      int mark = marked(vert % marked.width(), vert / marked.width());
      markers.insert(mark);
    }

    for (int x = 0; x < model.width(); ++x) {
      for (int y = 0; y < model.height(); ++y) {
        if (!markers.contains(marked(x, y))) {
          user_intention(x, y) = 0;
        }
      }
    }

    return UserAction::FBF_OR_BFB;
  }

  return 0;
}

void MainWindow::slotLoadFile() {
  auto title = "Load file";
  auto filters = "*.png; *.bmp; *.jpg; *.jpeg";
  auto filename = QFileDialog::getOpenFileName(this, title, "", filters);
  if (!filename.isEmpty()) {
    load(filename);
  }
}

void MainWindow::slotClear() {
  viewport_->setScene(source);
  viewport_->initial_marking = true;
  viewport_->current_source.clear();
  viewport_->current_sink.clear();
  viewport_->source.clear();
  viewport_->sink.clear();
}

void MainWindow::slotRun() {
  int mode = 0;
  viewport_->sink << viewport_->current_sink;
  viewport_->source << viewport_->current_source;
  if (!viewport_->initial_marking) {
    mode = intention(model, viewport_->current_source, viewport_->current_sink);
  }
  else {
    user_intention = std::move(Matrix<uint8_t>(image.size(), 1));
    marked = std::move(Matrix<uint8_t>(image.size(), 0));
    mask = std::move(Matrix<bool>(image.size(), true));
  }

  viewport_->initial_marking = false;
  viewport_->current_source.clear();
  viewport_->current_sink.clear();

  Graph graph = Graph::fromImage(image, user_intention);

  QTime timer;
  timer.start();
  auto cut = graph.minCut(viewport_->source, viewport_->sink);
  qDebug() << "elapsed:" << timer.elapsed();

  for (auto vert : graph.getForeground(cut)) {
    int x = vert % image.width();
    int y = vert / image.width();
    mask(x, y) = 1;
  }
  for (auto vert : graph.getBackground(cut)) {
    int x = vert % image.width();
    int y = vert / image.width();
    mask(x, y) = 0;
  }

  viewport_->setScene(applyMask());
  viewport_->redrawNotes();
}

void MainWindow::slotSetVisibleLabels() {
  viewport_->setScene(viewport_->pixmap);
  if (show_labels_action->isChecked()) {
    viewport_->redrawNotes();
  }
}
