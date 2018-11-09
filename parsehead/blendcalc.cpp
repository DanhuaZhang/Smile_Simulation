#include "blendcalc.h"
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
