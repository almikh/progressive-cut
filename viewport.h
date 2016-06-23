#ifndef VIEWPORT_H_INCLUDED__
#define VIEWPORT_H_INCLUDED__

#include <QGraphicsView>
#include <QPixmap>
#include <QVector>

class QWheelEvent;

class Viewport : public QGraphicsView {
  Q_OBJECT

public:
  QPixmap pixmap;
  bool initial_marking;
  QVector<int> source, sink;
  QVector<int> current_source, current_sink;

  explicit Viewport(QWidget* parent = nullptr);

  void setScene(const QImage& pixmap);
  void setScene(const QPixmap& pixmap);
  void setScene(QGraphicsScene* scene);

  void redrawNotes();

protected:
  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent* ev) override;
  void mouseMoveEvent(QMouseEvent* ev) override;
};

#endif // VIEWPORT_H_INCLUDED__
