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

struct PointData
{
    glm::vec2 orginal_pos  = glm::vec2( 0.f );
    glm::vec2 offset = glm::vec2( 0.f );
    unsigned id;
};

int select_closest_point( const PointData p_data[],
                          const unsigned p_data_count,
                          const float search_radius,
						  const float x_pos,
						  const float y_pos );

Eigen::VectorXf calc_blend_values( blendcalc blend_data, 
                                   Eigen::VectorXf feature_data,
                                   uint face_count,
                                   uint num_iterations );
