#include "blendcalc.h"
#include "gpqp_solv.hpp"
#include <float.h>

int select_closest_point(const PointData p_data[],
						 const unsigned p_data_count,
						 const float search_radius,
						 const float x_pos,
						 const float y_pos) {
	int closest_point_idx = -1;
	float closest_point_radius = FLT_MAX;

	const glm::vec2 mouse_pos( x_pos, y_pos );

	for( int i = 0; i < p_data_count; i++ ) {
		const float calc_dist = glm::distance( 
			mouse_pos, p_data[i].orginal_pos + p_data[i].offset );

		if( calc_dist <= search_radius && closest_point_radius > calc_dist ) {
			closest_point_idx = i;
			closest_point_radius = calc_dist;
		}
	}

	return closest_point_idx;
}

Eigen::VectorXf calc_blend_values( blendcalc blend_data, 
                                   Eigen::VectorXf feature_data,
                                   uint face_count,
                                   uint num_iterations )
{
	Eigen::VectorXf X = Eigen::VectorXf( face_count );

	blend_data.blend_b = feature_data;

	Eigen::VectorXf x_0 = Eigen::VectorXf::Ones(face_count) * 0.5f;
	Eigen::VectorXf x_lb = Eigen::VectorXf::Zero(face_count);
	Eigen::VectorXf x_ub = Eigen::VectorXf::Ones(face_count);

	blend_data.blend_x = GPQPSolver::Solve(blend_data.blend_a, blend_data.blend_b, x_lb, x_ub, x_0, num_iterations);
	
	// store calculated weights
	for (int i = 0; i < face_count; i++) {
		X(i) = blend_data.blend_x(i);
	}

	return X;
}