#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDoubleSpinBox>
#include <QGraphicsView>
#include <QComboBox>
#include <QTime>

#include "graph.h"

using std::string;

class Viewport: public QGraphicsView {
	Q_OBJECT

public:
	QPixmap pixmap;
	bool initialMarking;
	QVector<int> source, sink;
	QVector<int> currentSource, currentSink;

	Viewport(QWidget *parent = 0);

	void setPixmap(const QPixmap& pixmap);
	void redrawNotes();

	void wheelEvent(QWheelEvent *event) override;
	void mousePressEvent(QMouseEvent* ev) override;
	void mouseMoveEvent(QMouseEvent* ev) override;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
	QImage source;
	cv::Mat image;
	cv::Mat model;
	cv::Mat marked;
	cv::Mat userIntention;
	QAction* showLabels;
	cv::Mat mask;

    explicit MainWindow(const char* filename, QWidget *parent = 0);
    ~MainWindow();

private:
	Ui::MainWindow *ui;
	Viewport* viewport;

	cv::Mat applyMask();
	int intention(cv::Mat model, const QVector<int>& source, const QVector<int>& sink);

public slots:
	void slotLoadFile();
	void slotClear();
	void slotRun();
	void slotSetVisibleLabels();
};

#endif // MAINWINDOW_H
