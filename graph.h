#pragma once
#include <limits>
#include <QVector>
#include <stdint.h>
#include <QImage>
#include <QPair>
#include <QMap>

using Float = std::numeric_limits<float>;

#include "matrix.h"

class Graph {
public:
  using flow_t = float;
  using cut_t = QVector<QPair<int, int>>;
  using mask_t = Matrix<bool>;

  enum class Connectivity {
    Four = 4,
    Eight = 8
  };

private:
  QVector<int> parent_;
  QVector<bool> visited_;
  QSize image_size_;
  mask_t mask_;
  int size_;

  QVector<QMap<int, flow_t>> edges_;
  QVector<QMap<int, flow_t>> r_edges_;

  int bfs(int s, int t);
  void dfs(int s);

public:
  Graph(int size, const QSize& image_size);

  static Graph fromImage(const QImage& image, const Matrix<uint8_t>& mask, const Connectivity& connectivity = Connectivity::Four);

  void setMask(const mask_t& mask);

  void addEdge(int i, int j, float capacity);

  cut_t minCut(const QVector<int>& sources, const QVector<int>& sinks);

  QVector<int> getForeground(const cut_t& indices);
  QVector<int> getBackground(const cut_t& indices);
};
