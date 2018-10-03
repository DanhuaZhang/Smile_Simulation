#pragma once

#include "Eigen/Core"

/*  Ax = b := linear least squares
*   A -> feature point 2D slopes
*   x -> blend shape weights
*   b -> feature point 2D locations
*/
struct blendcalc
{
    Eigen::MatrixXd blend_a;
    Eigen::VectorXd blend_x;
    Eigen::VectorXd blend_b;
};
