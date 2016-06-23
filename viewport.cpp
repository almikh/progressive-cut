#include "viewport.h"
#include <QApplication>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QAction>
#include <QPixmap>

#include "mainwindow.h"

/* Viewport */
Viewport::Viewport(QWidget* parent) :
  QGraphicsView(parent),
  initial_marking(true)
{

}

void Viewport::setScene(const QImage& image) {
  setScene(QPixmap::fromImage(image));
}

void Viewport::setScene(const QPixmap& p) {
  pixmap = p;

  setScene(new QGraphicsScene());
  scene()->addPixmap(pixmap);
}

void Viewport::setScene(QGraphicsScene* s) {
  if (scene()) {
    delete scene();
  }

  QGraphicsView::setScene(s);
}

void Viewport::redrawNotes() {
  auto parent = qobject_cast<MainWindow*>(this->parent());
  if (!parent->show_labels_action->isChecked()) return;

  QSize size(6, 6);

  for (auto& e : source) {
    QRect aabb(QPoint(e % parent->image.width() - 3, e / parent->image.width() - 3), size);

    QColor color(255, 0, 0, 255);
    auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
    scene()->addEllipse(aabb, QPen(color), brush);
  }

  for (auto &e : sink) {
    QRect aabb(QPoint(e % parent->image.width() - 3, e / parent->image.width() - 3), size);

    QColor color(0, 0, 255, 255);
    auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
    scene()->addEllipse(aabb, QPen(color), brush);
  }
}

void Viewport::wheelEvent(QWheelEvent *event) {
  if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
    qreal factor = std::pow(1.001, event->delta());
    scale(factor, factor);
  }
  else QGraphicsView::wheelEvent(event);
}

void Viewport::mousePressEvent(QMouseEvent* ev) {
  QGraphicsView::mousePressEvent(ev);
  auto pos = mapToScene(ev->pos()).toPoint();
  auto parent = qobject_cast<MainWindow*>(this->parent());
  if (pos.x() < 0 || pos.x() >= parent->image.width()) return;
  if (pos.y() < 0 || pos.y() >= parent->image.height()) return;
  if (!scene() || !scene()->sceneRect().contains(pos)) return;

  QRect aabb(QRect(pos - QPoint(3, 3), QSize(6, 6)));
  if (ev->buttons() & Qt::MouseButton::LeftButton) {
    QColor color(255, 0, 0, 255);
    auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
    scene()->addEllipse(aabb, QPen(color), brush);
    current_source.push_back(pos.x() + pos.y()*parent->image.width());
  }
  else if (ev->buttons() & Qt::MouseButton::RightButton) {
    QColor color(0, 0, 255, 255);
    auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
    scene()->addEllipse(aabb, QPen(color), brush);
    current_sink.push_back(pos.x() + pos.y()*parent->image.width());
  }
}

void Viewport::mouseMoveEvent(QMouseEvent* ev) {
  QGraphicsView::mouseMoveEvent(ev);
  auto pos = mapToScene(ev->pos()).toPoint();
  auto parent = qobject_cast<MainWindow*>(this->parent());
  if (pos.x() < 0 || pos.x() >= parent->image.width()) return;
  if (pos.y() < 0 || pos.y() >= parent->image.height()) return;
  if (!scene() || !scene()->sceneRect().contains(pos)) return;

  QRect aabb(QRect(pos - QPoint(3, 3), QSize(6, 6)));
  if (ev->buttons() & Qt::MouseButton::LeftButton) {
    QColor color(255, 0, 0, 255);
    auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
    scene()->addEllipse(aabb, QPen(color), brush);
    current_source.push_back(pos.x() + pos.y()*parent->image.width());
  }
  else if (ev->buttons() & Qt::MouseButton::RightButton) {
    QColor color(0, 0, 255, 255);
    auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
    scene()->addEllipse(aabb, QPen(color), brush);
    current_sink.push_back(pos.x() + pos.y()*parent->image.width());
  }
}
