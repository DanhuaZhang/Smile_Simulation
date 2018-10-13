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

//point is feature point extracted from the face model 
//pnum is the number of points, which stands for the row number
//face_num is the number of face models, which stands for the column number 
void Get_Point_Matrix(Eigen::MatrixXf &matrix, int face_num, glm::vec3** edge, int pnum);
void SolveLeastSquare(blendcalc &blend);
