#include "graph.h"
#include <QDebug>
#include <QQueue>
#include <QStack>
#include <QtMath>
#include <QTime>

using namespace std;

Graph::Graph(int size, const QSize& image_size):
  size_(size),
  parent_(size),
  image_size_(image_size),
  edges_(size)
{

}

Graph Graph::fromImage(const QImage& image, const Matrix<uint8_t>& mask, const Connectivity& connectivity) {
  Q_ASSERT(image.format() == QImage::Format::Format_RGB888);

  static const int dx[] = {-1, 1, 0, 0, 1, -1, 1, 1 };
  static const int dy[] = {0, 0, 1, -1, 1, 1, -1, -1};

  auto norm = [](const QRgb& lhs, const QRgb& rhs) -> float {
    float r1 = qRed(lhs), g1 = qGreen(lhs), b1 = qBlue(lhs);
    float r2 = qRed(rhs), g2 = qGreen(rhs), b2 = qBlue(rhs);
    return qSqrt((r1-r2)*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2));
  };
  auto length = [](const int& x, const int& y) -> float {
    return qSqrt(1.0f*x*x + 1.0f*y*y);
  };

  int edges = 0;
  float sigma = 2.0f;
  Graph graph(image.width()*image.height(), image.size());
  for (int y = 0; y<image.height(); ++y) {
    for (int x = 0; x<image.width(); ++x) {
      if (!mask.isNull() && !mask(x, y)) continue;

      for (int i = 0; i<static_cast<int>(connectivity); ++i) {
        int index = x + y*image.width();
        auto p = image.pixel(x, y);
        if (x + dx[i] >= 0 && x + dx[i]<image.width() && y + dy[i] >= 0 && y + dy[i]<image.height()) {
          auto q = image.pixel(x + dx[i], y + dy[i]);
          float weight = exp(-norm(p, q) / (2.0f*sigma)) / length(dx[i], dy[i]);

          graph.addEdge(index, (x + dx[i]) + (y + dy[i])* image.width(), weight);
          ++edges;
        }
      }
    }
  }

  graph.mask_ = mask.to<bool>();
  qDebug() << "New graph:\n" << "  nodes:" << graph.size_ << "\n   edges:" << edges << "\n";
  return graph;
}

// Returns true if there is a path from source 's'
// to sink 't' in residual graph.
int Graph::bfs(int s, int t) {
  visited_.fill(false);

  QQueue<int> q;
  q << s;

  parent_[s] = -1;
  visited_[s] = true;

  while (!q.empty()) {
    int u = q.front();
    q.pop_front();

    for (auto& v : r_edges_[u].keys()) {
      if (!visited_[v] && qAbs(r_edges_[u][v])>Float::epsilon()) {
        q << v;
        parent_[v] = u;
        visited_[v] = true;
        if (v == t) q.clear();
      }
    }
  }

  // if we reached sink in BFS starting from source, then return true, else false
  return (visited_[t] == true);
}

void Graph::dfs(int s) {
  QStack<int> stack;
  stack.push(s);

  while (!stack.isEmpty()) {
    int s = stack.pop();
    visited_[s] = true;

    for (auto e : r_edges_[s].keys()) {
      if (!visited_[e] && qAbs(r_edges_[s][e])>Float::epsilon()) {
        stack.push(e);
      }
    }
  }
}

void Graph::setMask(const mask_t& mask) {
  mask_ = mask;
}

void Graph::addEdge(int i, int j, float capacity) {
  edges_[i][j] = capacity;
}

Graph::cut_t Graph::minCut(const QVector<int>& sources, const QVector<int>& sinks) {
  int source = edges_.size(), sink = edges_.size() + 1;

  edges_ << QMap<int, flow_t>();
  edges_ << QMap<int, flow_t>();

  for (auto s : sources) {
    if (mask_.isNull() || mask_(s % image_size_.width(), s / image_size_.width())) {
      edges_[source][s] = 100500;
    }
  }

  for (auto t : sinks) {
    if (mask_.isNull() || mask_(t % image_size_.width(), t / image_size_.width())) {
      edges_[t][sink] = 100500;
    }
  }

  size_ = edges_.size();
  parent_.resize(size_);
  visited_.resize(size_);

  r_edges_ = edges_;

  while (bfs(source, sink)) {
    float path_flow = Float::max();
    for (int v = sink; v != source; v = parent_[v]) {
      int u = parent_[v];
      path_flow = qMin(path_flow, r_edges_[u][v]);
    }

    // update residual capacities of the edges and reverse edges along the path
    for (int v = sink; v != source; v = parent_[v]) {
      int u = parent_[v];
      r_edges_[u][v] -= path_flow;
      r_edges_[v][u] += path_flow;
    }
  }

  visited_.fill(false);
  dfs(source);

  QVector<QPair<int, int>> cut;
  for (int i = 0; i<size_; ++i) {
    if (!visited_[i]) continue;
    for (auto j : edges_[i].keys()) {
      if (!visited_[j] && edges_[i][j]) {
        cut.push_back(qMakePair(i, j));
      }
    }
  }

  return cut;
}

QVector<int> Graph::getForeground(const Graph::cut_t& indices) {
  int source = edges_.size() - 2;
  QVector<bool> cut(size_, false);
  QStack<int> stack;
  stack.push(source);

  for (auto &e : indices) {
    cut[e.first] = cut[e.second] = true;
  }

  visited_.fill(false);
  while (!stack.isEmpty()) {
    int s = stack.pop();
    visited_[s] = true;

    for (auto e : r_edges_[s].keys()) {
      if (!visited_[e] && !cut[e] && !cut[s]) {
        stack.push(e);
      }
    }
  }

  QVector<int> foreground;
  for (int i = 0; i<visited_.size()-2; ++i) {
    if (visited_[i]) {
      int y = i / image_size_.width(), x = i % image_size_.width();
      if (mask_.isNull() || mask_(x, y)) {
        foreground.push_back(i);
      }
    }
  }

  return foreground;
}

QVector<int> Graph::getBackground(const Graph::cut_t& indices) {
  int source = edges_.size() - 2;
  QVector<bool> cut(size_, false);
  QStack<int> stack;
  stack.push(source);

  for (auto &e : indices) {
    cut[e.first] = cut[e.second] = true;
  }

  visited_.fill(false);
  while (!stack.isEmpty()) {
    int s = stack.pop();
    visited_[s] = true;

    for (auto e : r_edges_[s].keys()) {
      if (!visited_[e] && !cut[e] && !cut[s]) {
        stack.push(e);
      }
    }
  }

  QVector<int> background;
  for (int i = 0; i<visited_.size() - 2; ++i) {
    if (!visited_[i]) {
      int y = i / image_size_.width(), x = i % image_size_.width();
      if (mask_.isNull() || mask_(x, y)) {
        background.push_back(i);
      }
    }
  }

  return background;
}
