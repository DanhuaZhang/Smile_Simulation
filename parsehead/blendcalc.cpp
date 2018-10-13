#include "blendcalc.h"

void Get_Point_Matrix(Eigen::MatrixXf &matrix, int face_num, glm::vec3** edge, int pnum) {
	matrix = Eigen::MatrixXf(pnum * 2, face_num);
	for (int j = 0; j < face_num; j++) {
		for (int i = 0, k = 0; i < pnum * 2; i += 2, k++) {
			matrix(i, j) = edge[j][k].x - edge[face_num][k].x;
			matrix(i + 1, j) = edge[j][k].y - edge[face_num][k].y;
			/*matrix(i, j) = edge[j][k].x;
			matrix(i + 1, j) = edge[j][k].y;*/
		}
	}
}

void SolveLeastSquare(blendcalc &blendsys) {
	printf("Solved result: \n");
	blendsys.blend_x = blendsys.blend_a.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(blendsys.blend_b);
	std::cout << blendsys.blend_x << std::endl;
}