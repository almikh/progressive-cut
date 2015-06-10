#pragma once
#include <QVector>
#include <QImage>
#include <QMap>
#include <QSet>
#include <utility>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;

class Graph {
    int size;
    QVector<int> parent;
    QVector<bool> visited;
    cv::Size imageSize;
    cv::Mat mask;

    int bfs(int s, int t);
    void dfs(int s, QVector<bool>& visited);
public:
    int elapsed;
    typedef float flow_t;
    typedef QVector<pair<int, int>> cut_t;

    QVector<QMap<int, flow_t>> edges;
    QVector<QMap<int, flow_t>> rEdges;

    Graph(int size, const cv::Size& imageSize);

    static Graph fromImage(const cv::Mat& image, cv::Mat mask);

    void setMask(cv::Mat mask);

    void addEdge(int i, int j, float capacity);

    cut_t minCut(const QVector<int>& source, const QVector<int>& sink);

    QVector<int> getForeground(const cut_t& cut);
    QVector<int> getBackground(const cut_t& cut);
};
