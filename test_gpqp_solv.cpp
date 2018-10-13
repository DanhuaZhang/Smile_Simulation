#include "gpqp_solv.hpp"
#include <eigen3/Eigen/Core>
#include <iostream>
#include <chrono>

int main() {
  Eigen::MatrixXd V = Eigen::MatrixXd::Zero(2,2);
  V << -.5, 1, -1, .5;
  V = 1.0/sqrt(1.25) * V;
  Eigen::MatrixXd Sig = Eigen::MatrixXd::Zero(2,2);
  Sig << sqrt(10.0), 0.0, 0.0, sqrt(1.0);
  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(2,2);
  A << 1.0, 0.5, 0.5, 1.0;
  A = V*Sig;
  Eigen::VectorXd b = Eigen::VectorXd::Zero(2);
  b << -1.5, -1.5;
  b = A * b;

  //std::cout << "Unconst opt: " << -((A.transpose() * A).inverse() * A.transpose() * b).transpose() << std::endl;

  Eigen::VectorXd x_0 = Eigen::VectorXd::Zero(2);
  Eigen::VectorXd x_lb = Eigen::VectorXd::Zero(2);
  Eigen::VectorXd x_ub = Eigen::VectorXd::Ones(2);

  auto t1 = std::chrono::high_resolution_clock::now();
  Eigen::Vector2d x_opt = GPQPSolver::Solve(A, b, x_lb, x_ub, x_0, 10000);
  auto t2 = std::chrono::high_resolution_clock::now();
  auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
  float dur_ms = static_cast<float>(dur) / 1e6;
  std::cout << x_opt.transpose() << std::endl;
  std::cout << "Took " << dur_ms << " ms" << std::endl;

  return 0;
}
