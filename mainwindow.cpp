#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QWheelEvent>
#include <QLabel>
#include <QToolBar>
#include <QDebug>
#include <QImage>
#include <QTime>
#include <QPixmap>
#include <QGraphicsEllipseItem>

#define FOREGROUND	1
#define BACKGROUND	0

#define BF			1
#define FB			2
#define FBF_OR_BFB	3

/* Viewport */
Viewport::Viewport(QWidget *parent):
	QGraphicsView(parent),
	initialMarking(true)
{

}

void Viewport::setPixmap(const QPixmap& pixmap_) {
	pixmap = pixmap_;
	scene()->addPixmap(pixmap);
}

void Viewport::redrawNotes() {
	auto par = dynamic_cast<MainWindow*>(parent());
	if (!par->showLabels->isChecked()) return;
	QSize size(6, 6);

	for (auto &e : source) {
		QRect aabb(QRect(QPoint(e % par->image.cols - 3, e / par->image.cols - 3), size));
		QColor color(255, 0, 0, 255);
		auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
		scene()->addEllipse(aabb, QPen(color), brush);
	}

	for (auto &e : sink) {
		QRect aabb(QRect(QPoint(e % par->image.cols - 3, e / par->image.cols - 3), size));
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
	auto par = dynamic_cast<MainWindow*>(parent());
	if (pos.x() < 0 || pos.x() >= par->image.cols) return;
	if (pos.y() < 0 || pos.y() >= par->image.rows) return;
	if (!scene() || !scene()->sceneRect().contains(pos)) return;

	QRect aabb(QRect(pos - QPoint(3, 3), QSize(6, 6)));
	if (ev->buttons() & Qt::MouseButton::LeftButton) {
		QColor color(255, 0, 0, 255);
		auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
		scene()->addEllipse(aabb, QPen(color), brush);
		currentSource.push_back(pos.x() + pos.y()*par->image.cols);
	}
	else if (ev->buttons() & Qt::MouseButton::RightButton) {
		QColor color(0, 0, 255, 255);
		auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
		scene()->addEllipse(aabb, QPen(color), brush);
		currentSink.push_back(pos.x() + pos.y()*par->image.cols);
	}
}

void Viewport::mouseMoveEvent(QMouseEvent* ev) {
	QGraphicsView::mouseMoveEvent(ev);
	auto pos = mapToScene(ev->pos()).toPoint();
	auto par = dynamic_cast<MainWindow*>(parent());
	if (pos.x() < 0 || pos.x() >= par->image.cols) return;
	if (pos.y() < 0 || pos.y() >= par->image.rows) return;
	if (!scene() || !scene()->sceneRect().contains(pos)) return;

	QRect aabb(QRect(pos - QPoint(3, 3), QSize(6, 6)));
	if (ev->buttons() & Qt::MouseButton::LeftButton) {
		QColor color(255, 0, 0, 255);
		auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
		scene()->addEllipse(aabb, QPen(color), brush);
		currentSource.push_back(pos.x() + pos.y()*par->image.cols);
	}
	else if (ev->buttons() & Qt::MouseButton::RightButton) {
		QColor color(0, 0, 255, 255);
		auto brush = QBrush(color, Qt::BrushStyle::SolidPattern);
		scene()->addEllipse(aabb, QPen(color), brush);
		currentSink.push_back(pos.x() + pos.y()*par->image.cols);
	}
}

/* MainWindow */
MainWindow::MainWindow(const char* filename, QWidget *parent):
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	viewport(new Viewport(this))
{
	ui->setupUi(this);

	setCentralWidget(viewport);

	connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(slotLoadFile()));
	ui->actionOpen->setShortcut(QKeySequence("CTRL+O"));

	viewport->setScene(new QGraphicsScene());

	if (filename) {
		source = QImage(filename);
		if (!source.isNull()) {
			viewport->setScene(new QGraphicsScene());
			viewport->setPixmap(QPixmap::fromImage(source));
			image = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
		}
	}

	ui->mainToolBar->addAction(QIcon("open.png"), "Open file", this, SLOT(slotLoadFile()));
	ui->mainToolBar->addSeparator();
	ui->mainToolBar->addAction(QIcon("new.png"), "Clear", this, SLOT(slotClear()));
	ui->mainToolBar->addAction(QIcon("run.png"), "Run", this, SLOT(slotRun()));
	ui->mainToolBar->addSeparator();
	showLabels = ui->mainToolBar->addAction(QIcon("draw-labels.png"), "Show Labels", this, SLOT(slotSetVisibleLabels()));
	showLabels->setCheckable(true);
	showLabels->setChecked(true);
}

MainWindow::~MainWindow() {
	delete ui;
	delete viewport;
}

void MainWindow::slotLoadFile() {
	auto path = QFileDialog::getOpenFileName(this, "Load file", "", "*.png; *.bmp; *.jpg; *.jpeg");
	if (!path.isEmpty()) {
		source = QImage(path);
		viewport->setScene(new QGraphicsScene());
		viewport->setPixmap(QPixmap::fromImage(source));
		image = cv::imread(path.toStdString(), CV_LOAD_IMAGE_COLOR);
		viewport->initialMarking = true;
		viewport->currentSource.clear();
		viewport->currentSink.clear();
		viewport->source.clear();
		viewport->sink.clear();
	}
}

void MainWindow::slotClear() {
	viewport->setScene(new QGraphicsScene());
	viewport->scene()->addPixmap(QPixmap::fromImage(source));
	viewport->initialMarking = true;
	viewport->currentSource.clear();
	viewport->currentSink.clear();
	viewport->source.clear();
	viewport->sink.clear();
}

cv::Mat MainWindow::applyMask() {
	static const int dx[] = {-1, 0, 1, 0};
	static const int dy[] = {0, 1, 0, -1};

	cv::Mat canvas = image.clone();
	model = cv::Mat::ones(image.size(), CV_32S);
	for (int x = 0; x < mask.cols; ++x) {
		for (int y = 0; y < mask.rows; ++y) {
			if (!mask.at<int>(y, x)) {
				model.at<int>(y, x) = 0;
				auto& pixel = canvas.at<cv::Vec3b>(y, x);
				pixel[0] = pixel[0] * 0.5 + 255 * 0.5;
				pixel[1] = pixel[1] * 0.5 + 255 * 0.5;
				pixel[2] = pixel[2] * 0.5 + 255 * 0.5;
			}
		}
	}

	for (int x = 0; x < mask.cols; ++x) {
		for (int y = 0; y < mask.rows; ++y) {
			if (mask.at<int>(y, x) == FOREGROUND) {
				auto& pixel = canvas.at<cv::Vec3b>(y, x);
				cv::Vec3b border(pixel[0] * 0.25, pixel[1] * 0.25, pixel[2] * 0.25 + 255 * 0.75);
				if (x == 0 || x == mask.cols - 1 || y == 0 || y == mask.rows - 1) {
					canvas.at<cv::Vec3b>(y, x) = border;
				}
				else {
					for (int i = 0; i < 4; ++i) {
						if (mask.at<int>(y + dy[i], x + dx[i]) == BACKGROUND) {
							canvas.at<cv::Vec3b>(y + dy[i], x + dx[i]) = border;
						}
					}
				}
			}
		}
	}

	/* метки регионов */
	int counter = 2;
	marked = model.clone();
	for (int x = 0; x < marked.cols; ++x) {
		for (int y = 0; y < marked.rows; ++y) {
			if (!marked.at<int>(y, x) || marked.at<int>(y, x) == 1) {
				cv::floodFill(marked, cv::Point(x, y), counter++);
			}
		}
	}

	auto temp = marked.clone();
	cv::normalize(temp, temp, 0, 255, CV_MINMAX);
	cv::imwrite("marked.png", marked);

	return canvas;
}

int MainWindow::intention(cv::Mat model, const QVector<int>& source, const QVector<int>& sink) {
	userIntention = cv::Mat::ones(model.size(), CV_32S);

	bool allForeground = true;
	bool allBackground = true;

	QVector<int> labels = source;
	for (auto &vert : (labels << sink)) {
		if (model.at<int>(vert / image.cols, vert % image.cols) != FOREGROUND) allForeground = false;
		if (model.at<int>(vert / image.cols, vert % image.cols) != BACKGROUND) allBackground = false;
	}

	if (allForeground) {
		for (int x = 0; x < model.cols; ++x) {
			for (int y = 0; y < model.rows; ++y) {
				if (model.at<int>(y, x) == BACKGROUND) {
					userIntention.at<int>(y, x) = 0;
				}
			}
		}

		qDebug() << "FB";
		return FB;
	}
	else if (allBackground) {
		for (int x = 0; x < model.cols; ++x) {
			for (int y = 0; y < model.rows; ++y) {
				if (model.at<int>(y, x) == FOREGROUND) {
					userIntention.at<int>(y, x) = 0;
				}
			}
		}

		qDebug() << "BF";
		return BF;
	}
	else {
		QSet<int> markers;
		for (auto vert : source + sink) {
			int mark = marked.at<int>(vert / marked.cols, vert % marked.cols);
			markers.insert(mark);
		}

		for (int x = 0; x < model.cols; ++x) {
			for (int y = 0; y < model.rows; ++y) {
				if (!markers.contains(marked.at<int>(y, x))) {
					userIntention.at<int>(y, x) = 0;
				}
			}
		}

		qDebug() << "FBF_OR_BFB";
		return FBF_OR_BFB;
	}

	return 0;
}

void MainWindow::slotRun() {
	cv::Mat target = image.clone();

	int mode = 0;
	qDebug() << "source:" << viewport->currentSource.size() << "sink:" << viewport->currentSink.size();
	viewport->sink << viewport->currentSink;
	viewport->source << viewport->currentSource;
	if (!viewport->initialMarking) {
		mode = intention(model, viewport->currentSource, viewport->currentSink);
		//cv::imwrite("intention.png", user * 255);
	}
	else {
		userIntention = cv::Mat::ones(target.size(), CV_32S);
		marked = cv::Mat::zeros(image.size(), CV_32S);
		mask = cv::Mat::ones(image.size(), CV_32S);
	}

	viewport->initialMarking = false;
	viewport->currentSource.clear();
	viewport->currentSink.clear();

	Graph graph = Graph::fromImage(target, userIntention);

	QTime timer;
	timer.start();
	auto cut = graph.minCut(viewport->source, viewport->sink);
	qDebug() << "elapsed:" << timer.elapsed();
	qDebug() << ">" << graph.elapsed;

	for (auto vert : graph.getForeground(cut)) {
		int x = vert % image.cols;
		int y = vert / image.cols;
		mask.at<int>(y, x) = 1;
	}
	for (auto vert : graph.getBackground(cut)) {
		int x = vert % image.cols;
		int y = vert / image.cols;
		mask.at<int>(y, x) = 0;
	}

	//cv::imwrite("mask.png", mask * 255);

	auto canvas = applyMask();

	//cv::imwrite("model.png", model*255);

	cv::imwrite("temp.png", canvas);
	viewport->setScene(new QGraphicsScene());
	viewport->setPixmap(QPixmap("temp.png"));
	viewport->redrawNotes();
	std::remove("temp.png");
}

void MainWindow::slotSetVisibleLabels() {
	viewport->setScene(new QGraphicsScene());
	viewport->scene()->addPixmap(viewport->pixmap);
	if (showLabels->isChecked()) {
		viewport->redrawNotes();
	}
}
