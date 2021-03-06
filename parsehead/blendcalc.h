#pragma once

#include "Eigen/Core"
#include "Eigen/Dense"
#include "glm/glm.hpp"
#include <iostream>

/*  Ax = b := linear least squares
*   A -> feature point 2D slopes
*   x -> blend shape weights
*   b -> feature point 2D locations
*/
struct blendcalc
{
    Eigen::MatrixXf blend_a;
    Eigen::VectorXf blend_x; //column vector
    Eigen::VectorXf blend_b; //column vector
};
