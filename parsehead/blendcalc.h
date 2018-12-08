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
    glm::vec2 rendered_offset = glm::vec2( 0.f );
    glm::vec2 rendered_pos = glm::vec2( 0.f );
    const char* id;
};

struct FeatureControl
{
  float mouth_width;
  float mouth_height;
  float commisure_angle;
};

class NN_DataInput
{
public:
    FeatureControl* per_frame_data = nullptr;
    FeatureControl min_input;
    FeatureControl max_input;
    unsigned frame_count;

    ~NN_DataInput() { if( per_frame_data ) free( per_frame_data ); }
};

// Returns the index of the closest point to x_pos & y_pos based on search
// radius. Returns -1 on no match
int select_closest_point( const PointData p_data[],
                          const unsigned p_data_count,
                          const float search_radius,
						  const float x_pos,
						  const float y_pos );

// Calculates and return blend shape weights using target delta's set in
// feature data (face_count = # of blend shape weights)
Eigen::VectorXf calc_blend_values( blendcalc blend_data,
                                   Eigen::VectorXf feature_data,
                                   uint face_count,
                                   uint num_iterations );

// Get 2D eigen matrix from input file
Eigen::MatrixXf extract_matrix(const char *in_file);

// Get 1D eigen vector from input file
Eigen::VectorXf extract_vector(const char *in_file);

// Neural net
Eigen::VectorXf feed_forward( Eigen::VectorXf &inputs,
                              Eigen::MatrixXf weights[],
                              Eigen::VectorXf biases[],
                              uint layers_count);

NN_DataInput read_nn_data( const char* path_to_nn_input );

void print_nn_data( const NN_DataInput* nn_data );