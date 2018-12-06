#include "blendcalc.h"
#include "gpqp_solv.hpp"
#include <cmath>
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
			mouse_pos, p_data[i].rendered_pos + p_data[i].rendered_offset );

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

Eigen::MatrixXf extract_matrix(const char *in_file)
{
	Eigen::MatrixXf out_mat;
	//ddIO mat_io;
	FILE* mat_file;

	mat_file = fopen( in_file, "r");// mat_io.open(in_file, ddIOflag::READ);

	if(!feof(mat_file) && !ferror(mat_file)) {
		// get matrix size
		unsigned long mat_size[2] = {0, 0};
		char* line = nullptr;
		size_t line_size = 0;

		bool keep_reading = (bool)getline( &line, &line_size, mat_file);

		char *nxt_num = nullptr;
		if(keep_reading) {
			// doesn't work if line doesn't have 2 number white-space separated
			mat_size[0] = std::strtoul(line, &nxt_num, 10);
			mat_size[1] = std::strtoul(nxt_num, NULL, 10);
		}

		printf("    Creating new matrix (%lu, %lu)...\n", mat_size[0], mat_size[1]);
		out_mat = Eigen::MatrixXf::Zero(mat_size[0], mat_size[1]);

		// populate matrix
		keep_reading = (bool)getline( &line, &line_size, mat_file);
		unsigned r_idx = 0;
		while(keep_reading && r_idx < mat_size[0]) {
			// loop thru columns per row
			unsigned c_idx = 0;
			const char *curr_row = line;
			while (*curr_row != '\n') {
				char *nxt_flt = nullptr;
				// printf("%s\n", curr_row);
				out_mat(r_idx, c_idx) = std::strtof(curr_row, &nxt_flt);
				curr_row = nxt_flt;

				c_idx++;
			}

			keep_reading = (bool)getline( &line, &line_size, mat_file);
			r_idx++;
		}
		//std::cout << out_mat << "\n";
	}

	return out_mat;
}

Eigen::VectorXf extract_vector(const char *in_file)
{
	Eigen::VectorXf out_vec;
	//ddIO vec_io;
	FILE* vec_file;

	vec_file = fopen( in_file, "r");

	if(!feof(vec_file) && !ferror(vec_file)) {
		// get vector size
		unsigned long vec_size = 0;
		char* line = nullptr;
		size_t line_size = 0;

		bool keep_reading = (bool)getline( &line, &line_size, vec_file);

		if(keep_reading) vec_size = std::strtoul(line, NULL, 10);

		printf("    Creating new vector (%lu)...\n", vec_size);
		out_vec = Eigen::VectorXf::Zero(vec_size);

		// populate vector
		keep_reading = (bool)getline( &line, &line_size, vec_file);
		unsigned idx = 0;
		while(keep_reading && idx < vec_size ) {
			out_vec(idx) = std::strtof(line, NULL);

			keep_reading = (bool)getline( &line, &line_size, vec_file);
			idx++;
		}
		
		//std::cout << out_vec << "\n";
	}

	return out_vec;
}


Eigen::VectorXf feedForward( Eigen::VectorXf &inputs,
                             Eigen::MatrixXf weights[],
                             Eigen::VectorXf biases[],
                             uint layers_count)
{
	// inputs are assumed normalized where appropriate
	// convert to row vector
	
	Eigen::VectorXf layerin = inputs;
	for (int i = 0; i < layers_count; i++) {
		if (weights[i].rows() != layerin.size()) {
			std::cout << "Input and weights have incompatible dimensions at layer "
					<< i << "!!";
			std::cout << "Input size: " << layerin.size()
					<< ", expected size: " << weights[i].rows() << std::endl;
			
			assert( weights[i].rows() != layerin.size() );
		}
		layerin = weights[i].transpose() * layerin + biases[i];
		
		// component wise RELU
		if (i < layers_count - 1) {
			for (int j = 0; j < layerin.size(); j++) {
				layerin[j] = std::max(0.f, layerin[j]);
			}
		}
	}
	
	Eigen::VectorXf output = Eigen::VectorXf::Zero(layerin.size());
	
	for (int i = 0; i < layerin.size(); i++) {
		output[i] = layerin[i];
	}
	return output;
}