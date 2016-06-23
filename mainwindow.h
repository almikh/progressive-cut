#ifndef MAINWINDOW_H_INCLUDED__
#define MAINWINDOW_H_INCLUDED__

#include <QMainWindow>
#include <stdint.h>

#include "graph.h"
#include "matrix.h"

class Viewport;

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  enum PixelClass {
    Background = 0,
    Foreground = 1
  };

  enum UserAction {
    BF = 1,
    FB = 2,
    FBF_OR_BFB = 3
  };

public:
  QImage source;
  QImage image;
  Matrix<bool> mask;
  Matrix<uint8_t> model;
  Matrix<uint8_t> marked;
  Matrix<uint8_t> user_intention;
  QAction* show_labels_action;

  MainWindow(const QString& filename, QWidget* parent = nullptr);
  ~MainWindow();

private:
  Ui::MainWindow* ui_;
  Viewport* viewport_;

  void createToolbar();

  void load(const QString& filename);

  QImage applyMask();
  int intention(Matrix<uint8_t> model, const QVector<int>& source, const QVector<int>& sink);

public slots:
  void slotLoadFile();
  void slotClear();
  void slotRun();
  void slotSetVisibleLabels();
};

#endif // MAINWINDOW_H_INCLUDED__
