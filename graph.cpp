#include "graph.h"
#include <assert.h>
#include <iostream>
#include <QDebug>
#include <QStack>
#include <QTime>
#include <queue>

using namespace std;

Graph::Graph(int size, const cv::Size& imageSize):
	size(size),
	parent(size),
	imageSize(imageSize),
	edges(size),
	elapsed(0)
{
	
}

Graph Graph::fromImage(const cv::Mat& image, cv::Mat mask) {
	assert(image.type() == CV_8UC3);
	//static const int dx[] = {-1, -1, -1, 0, 1, 1, 1, 0};
	//static const int dy[] = {-1, 0, 1, 1, 1, 0, -1, -1};
	static const int dx[] = {-1, 1, 0, 0};
	static const int dy[] = {0, 0, 1, -1};

	auto norm = [](const cv::Vec3b& lhs, const cv::Vec3b& rhs) -> double {
		return sqrt(1.0*(lhs[0] - rhs[0])*(lhs[0] - rhs[0]) + (lhs[1] - rhs[1])*(lhs[1] - rhs[1]) + (lhs[2] - rhs[2])*(lhs[2] - rhs[2]));
	};
	auto length = [](const int& x, const int& y) -> double {
		return sqrt(1.0*x*x + 1.0*y*y);
	};

	int edges = 0;
	double sigma = 2.0;
	Graph graph(image.rows*image.cols, image.size());
	for (int y = 0; y<image.rows; ++y) {
		for (int x = 0; x<image.cols; ++x) {
			if (mask.data && !mask.at<int>(y, x)) continue;

			for (int i = 0; i<4; ++i) {
				int index = x + y*image.cols;
				auto p = image.at<cv::Vec3b>(y, x);
				if (x + dx[i] >= 0 && x + dx[i]<image.cols && y + dy[i] >= 0 && y + dy[i]<image.rows) {
					auto q = image.at<cv::Vec3b>(y + dy[i], x + dx[i]);
					float weight = exp(-norm(p, q) / (2*sigma)) / length(dx[i], dy[i]);
					
					graph.addEdge(index, (x + dx[i]) + (y + dy[i])* image.cols, weight);
					++edges;
				}
			}		
		}
	}

	graph.mask = mask;
	qDebug() << "New graph:\n" << "  nodes:" << graph.size << "\n   edges:" << edges << "\n";
	return graph;
}

/* Returns true if there is a path from source 's' to sink 't' in
   residual graph. */
int Graph::bfs(int s, int t) {
	QTime timer;
	timer.start();

	visited.fill(false);

	queue<int> q;
	q.push(s);
	parent[s] = -1;
	visited[s] = true;

	while (!q.empty()) {
		int u = q.front();
		q.pop();

		for (auto &v : rEdges[u].keys()) {
			if (!visited[v] && qAbs(rEdges[u][v])>FLT_EPSILON) {
				q.push(v);
				parent[v] = u;
				visited[v] = true;
				if (v == t) q = queue<int>();
			}
		}
	}

	elapsed += timer.elapsed();
	// If we reached sink in BFS starting from source, then return true, else false
	return (visited[t] == true);
}

void Graph::dfs(int s, QVector<bool>& visited) {
	QStack<int> stack;
	stack.push(s);

	while (!stack.isEmpty()) {
		int s = stack.pop();
		visited[s] = true;

		for (auto e : rEdges[s].keys()) {
			if (!visited[e] && qAbs(rEdges[s][e])>FLT_EPSILON) {
				stack.push(e);
			}
		}
	}
}

void Graph::setMask(cv::Mat mask_) {
	mask = mask_;
}

void Graph::addEdge(int i, int j, float capacity) {
	edges[i][j] = capacity;
}

Graph::cut_t Graph::minCut(const QVector<int>& source_, const QVector<int>& sink_) {
	int source = edges.size(), sink = edges.size() + 1;
	
	edges.push_back(QMap<int, flow_t>());
	edges.push_back(QMap<int, flow_t>());
	for (auto s : source_) {
		if (!mask.data || mask.at<int>(s / imageSize.width, s % imageSize.width)) {
			edges[source][s] = 100500;
		}
	}
	for (auto t : sink_) {
		if (!mask.data || mask.at<int>(t / imageSize.width, t % imageSize.width)) {
			edges[t][sink] = 100500;
		}
	}

	size = edges.size();
	parent.resize(size);
	visited.resize(size);

	rEdges = edges;

	while (bfs(source, sink)) {
		float path_flow = FLT_MAX;
		for (int v = sink; v != source; v = parent[v]) {
			int u = parent[v];
			path_flow = min(path_flow, rEdges[u][v]);
		}

		// update residual capacities of the edges and reverse edges along the path
		for (int v = sink; v != source; v = parent[v]) {
			int u = parent[v];
			rEdges[u][v] -= path_flow;
			rEdges[v][u] += path_flow;
		}
	}

	visited.fill(false);
	dfs(source, visited);

	QVector<pair<int, int>> cut;
	for (int i = 0; i<size; ++i) {
		if (!visited[i]) continue;
		for (auto j : edges[i].keys()) {
			if (!visited[j] && edges[i][j]) {
				cut.push_back(make_pair(i, j));
			}
		}
	}

	return cut;
}

QVector<int> Graph::getForeground(const Graph::cut_t& cut_) {
	int source = edges.size() - 2;
	QVector<bool> cut(size, false);
	QStack<int> stack;
	stack.push(source);

	for (auto &e : cut_) {
		cut[e.first] = cut[e.second] = true;
	}

	visited.fill(false);
	while (!stack.isEmpty()) {
		int s = stack.pop();
		visited[s] = true;

		for (auto e : rEdges[s].keys()) {
			if (!visited[e] && !cut[e] && !cut[s]) {
				stack.push(e);
			}
		}
	}

	QVector<int> foreground;
	for (int i = 0; i<visited.size()-2; ++i) {
		if (visited[i]) {
			int y = i / imageSize.width, x = i % imageSize.width;
			if (!mask.data || mask.at<int>(y, x)) {
				foreground.push_back(i);
			}
		}
	}

	return foreground;
}

QVector<int> Graph::getBackground(const Graph::cut_t& cut_) {
	int source = edges.size() - 2;
	QVector<bool> cut(size, false);
	QStack<int> stack;
	stack.push(source);

	for (auto &e : cut_) {
		cut[e.first] = cut[e.second] = true;
	}

	visited.fill(false);
	while (!stack.isEmpty()) {
		int s = stack.pop();
		visited[s] = true;

		for (auto e : rEdges[s].keys()) {
			if (!visited[e] && !cut[e] && !cut[s]) {
				stack.push(e);
			}
		}
	}

	QVector<int> background;
	for (int i = 0; i<visited.size() - 2; ++i) {
		if (!visited[i]) {
			int y = i / imageSize.width, x = i % imageSize.width;
			if (!mask.data || mask.at<int>(y, x)) {
				background.push_back(i);
			}
		}
	}

	return background;
}
