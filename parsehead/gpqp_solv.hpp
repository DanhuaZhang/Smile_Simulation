#pragma once

#include "Eigen/Core"
#include <limits>
#include <algorithm>
#include <iostream>
#include <vector>

class GPQPSolver {
private:
  static std::vector<int> GetCauchyPoint(const Eigen::MatrixXf& A, const Eigen::VectorXf& b, const Eigen::VectorXf& x_k, const Eigen::VectorXf& g, const Eigen::VectorXf& x_lb, const Eigen::VectorXf& x_ub, Eigen::VectorXf& x_c) {
  std::vector<std::pair<double,int>> break_times;
  for (int i = 0; i < x_k.size(); ++i) {
    if (g[i] < 0.0) {
      break_times.emplace_back((x_k[i] - x_ub[i])/g[i], i);
    } else if (g[i] > 0.0) {
      break_times.emplace_back((x_k[i] - x_lb[i])/g[i], i);
    } else {
      break_times.emplace_back(std::numeric_limits<double>::max(), i);
    }
  }
  std::sort(break_times.begin(), break_times.end());
  /*for (auto &p : break_times) {
    std::cout << p.second << ": " << p.first << std::endl;
  }*/

  x_c = x_k;
  Eigen::VectorXf g_curr = g;
  double t_curr = 0.0;
  std::vector<int> locked_dims;
  for (int i = 0; i < break_times.size(); ++i) {
    double alpha_nb = std::max(break_times[i].first, 0.0);
    double ac_top1 = g_curr.transpose() * A.transpose() * A * x_c;
    double ac_top2 = b.transpose() * A * g;
    double ac_bot = g.transpose() * A.transpose() * A * g;
    double alpha_c = (ac_top1 + ac_top2) / ac_bot;
    //std::cout << "alpha_c: " << alpha_c << std::endl;

    if (alpha_nb < alpha_c + t_curr) {
      //std::cout << "Locking dim " << break_times[i].second << std::endl;
      x_c -= g_curr * (alpha_nb - t_curr);
      g_curr[break_times[i].second] = 0.0;

      locked_dims.push_back(break_times[i].second);
      t_curr = alpha_nb;
    } else {
      x_c -= g_curr * alpha_c;
      break;
    }
  }
  std::sort(locked_dims.begin(), locked_dims.end());
  return locked_dims;
};

public:
  // Problem of form:
  // min_x 0.5*||A*x + b||^2 s.th x_lb <= x <= x_ub
  static Eigen::VectorXf Solve(const Eigen::MatrixXf& A, const Eigen::VectorXf& b, const Eigen::VectorXf& x_lb, const Eigen::VectorXf& x_ub, const Eigen::VectorXf& x_0, int n_iters) {
    Eigen::VectorXf x_k = x_0;
    for (int iter = 0; iter < n_iters; ++iter) {// (true) {
      // Compute Gradient:
      Eigen::VectorXf g = A.transpose() * A * x_k + A.transpose() * b;
      
      // Get Cauchy Point:
      Eigen::VectorXf x_c;
      std::vector<int> locked_dims = GetCauchyPoint(A, b, x_k, g, x_lb, x_ub, x_c);

      // (Approx.) Solve constrained Problem:
      x_k = x_c;
      //std::cout << iter << ": " << (A * x_k + b).norm() << std::endl;
/*      int ld_k = 0;
      int b_idx = 0;
      int e_idx = x_k.size() - 1;
      Eigen::MatrixXf P = Eigen::MatrixXf::Zero(x_k.size(),x_k.size());
      int n_unlocked = 0;
      Eigen::VectorXf x_locked = x_c;
      for (int i = 0; i < x_k.size(); ++i) {
        if (ld_k < locked_dims.size() && i == locked_dims[ld_k]) {
          // Locked dim
          x_locked[ld_k] = x_c[i];
          ld_k++;
          P(e_idx,i) = 1;
          e_idx--;
        } else {
          // Not locked dim
          n_unlocked++;
          P(b_idx,i) = 1;
          b_idx++;
        }
      }
      if (n_unlocked == 0) {
        x_k = x_c;
        continue;
      }
      int n_locked = x_k.size() - n_unlocked;
      x_locked = x_locked.head(n_locked);
      Eigen::MatrixXf Q_comb = P.transpose() * A.transpose() * A * P;
      Eigen::VectorXf b_comb = b.transpose() * A * P;

      Eigen::MatrixXf Q_new = Q_comb.topLeftCorner(n_unlocked, n_unlocked);
      Eigen::MatrixXf b_new = x_locked.transpose() * Q_comb.topRightCorner(n_locked, n_unlocked);
      b_new += b_comb.topRows(n_unlocked).transpose();

      //Eigen::VectorXf x_new_pivoted = Q_comb.*/
    }
    return x_k;
  };

};
