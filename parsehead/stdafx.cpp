#include "stdafx.h"

void GTAllocate(GT_Variable &gt_var, int size){
	gt_var.size = size;
	gt_var.time = (float *)malloc(sizeof(float)*size);
	gt_var.Lateral_canthus_L = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Lateral_canthus_R = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);

	gt_var.Palpebral_fissure_RU = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Palpebral_fissure_RL = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Palpebral_fissure_LU = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Palpebral_fissure_LL = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);

	gt_var.Depressor_L = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Depressor_R = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Depressor_M = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);

	gt_var.Nasal_ala_L = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Nasal_ala_R = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);

	gt_var.Medial_brow_L = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Medial_brow_R = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);

	gt_var.Malar_eminence_L = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
	gt_var.Malar_eminence_R = (glm::vec2 *)malloc(sizeof(glm::vec2)*size);
}

float* ParseObj(const char* filename, int &model_numVerts, const int edgeidx[], glm::vec3** edge, const int face_id, const int edge_numVerts) {
	//printf("Parsing the obj file %s:\n", filename);
	FILE* fp = fopen(filename, "r");
	if (!fp) {
		printf("Error! Cannot open the file!\nPlease check the path\n");
		exit(0);
	}

	//estimate the length of the file
	fseek(fp, 0L, SEEK_END);
	int file_length = ftell(fp);
	rewind(fp);

	//get the size of each element
	int num_vertex = 0;
	int num_texture_coord = 0;
	int num_normal = 0;
	int num_face = 0;

	char *charline = (char*)malloc(MAX_CHARACTER * sizeof(char));
	int cnt = 0;
	while (fgets(charline, MAX_CHARACTER, fp)) {
		char parameter[50];
		int fieldsRead = sscanf(charline, "%s ", parameter); //Read first word in the line (i.e., the command type)

		if (fieldsRead < 1) { //No command read
							  //Blank line
			continue;
		}
		if (strcmp(parameter, "v") == 0) {
			num_vertex++;
		}
		else if (strcmp(parameter, "vt") == 0) {
			num_texture_coord++;
		}
		else if (strcmp(parameter, "vn") == 0) {
			num_normal++;
		}
		else if (strcmp(parameter, "f") == 0) {
			num_face++;
		}
	}
	rewind(fp);

	//read the file
	glm::vec3* vertex = (glm::vec3 *)malloc(sizeof(glm::vec3)*num_vertex);
	glm::vec3* normal = (glm::vec3 *)malloc(sizeof(glm::vec3)*num_normal);
	glm::vec2* texture = (glm::vec2 *)malloc(sizeof(glm::vec2)*num_texture_coord);
	int* face = (int *)malloc(sizeof(int)*num_face * 12);
	for (int i = 0; i < num_face * 12; i++) {
		face[i] = -1;
	}

	int idx_vertex = 0;
	int idx_normal = 0;
	int idx_texture = 0;
	int idx_face = 0;
	int num_triangle = 0;

	while (fgets(charline, MAX_CHARACTER, fp)) { //Assumes no line is longer than 1024 characters!
		char parameter[50];
		int fieldsRead = sscanf(charline, "%s ", parameter); //Read first word in the line (i.e., the command type)

		if (fieldsRead < 1) { //No command read
							  //Blank line
			continue;
		}

		if (strcmp(parameter, "v") == 0) {
			sscanf(charline, "v %f %f %f", &vertex[idx_vertex].x, &vertex[idx_vertex].y, &vertex[idx_vertex].z);
			//printf("v %f %f %f\n", vertex[idx_vertex].x, vertex[idx_vertex].y, vertex[idx_vertex].z);
			idx_vertex++;
		}
		else if (strcmp(parameter, "vt") == 0) {
			sscanf(charline, "vt %f %f", &texture[idx_texture].x, &texture[idx_texture].y);
			//printf("vt %f %f\n", texture[idx_texture].x, texture[idx_texture].y);
			idx_texture++;
		}
		else if (strcmp(parameter, "vn") == 0) {
			sscanf(charline, "vn %f %f %f", &normal[idx_normal].x, &normal[idx_normal].y, &normal[idx_normal].z);
			//printf("vn %f %f %f\n", normal[idx_normal].x, normal[idx_normal].y, normal[idx_normal].z);
			idx_normal++;
		}
		else if (strcmp(parameter, "f") == 0) {
			sscanf(charline, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", \
				&face[idx_face], &face[idx_face + 1], &face[idx_face + 2], \
				&face[idx_face + 3], &face[idx_face + 4], &face[idx_face + 5], \
				&face[idx_face + 6], &face[idx_face + 7], &face[idx_face + 8], \
				&face[idx_face + 9], &face[idx_face + 10], &face[idx_face + 11]);
			if (face[idx_face + 9] == -1) { num_triangle++; }
			idx_face += 12;
		}
	}
	fclose(fp);

	//load to opengl, stored in model
	//note that the indices in face starts from 1 rather than 0
	model_numVerts = (num_face - num_triangle) * 6 + num_triangle * 3;
	float* model = (float *)malloc(sizeof(float)*model_numVerts * 8);
	int i = 0;
	int j = 0;
	bool repeat = true;
	while (i < num_face * 12) {
		//printf("i: %d\tj: %d\n", i, j);
		//skip -1/-1/-1
		if (face[i] == -1) {
			i += 3;
			continue;
		}
		else {
			//vertex
			model[j] = vertex[face[i] - 1].x;
			model[j + 1] = vertex[face[i] - 1].y;
			model[j + 2] = vertex[face[i] - 1].z;

			//normal
			model[j + 3] = normal[face[i + 2] - 1].x;
			model[j + 4] = normal[face[i + 2] - 1].y;
			model[j + 5] = normal[face[i + 2] - 1].z;

			//texture
			model[j + 6] = texture[face[i + 1] - 1].x;
			model[j + 7] = 1-texture[face[i + 1] - 1].y;
		}

		//repeat the first and the third vertex in the face directive
		//i.e. i is 6, 6+12, ...
		if ((i % 12 == 6) && (face[i + 3] != -1)) {
			if (repeat) {
				repeat = false;

				//load the first vertex again
				j += 8; i -= 6;
				model[j] = vertex[face[i] - 1].x;
				model[j + 1] = vertex[face[i] - 1].y;
				model[j + 2] = vertex[face[i] - 1].z;

				//normal
				model[j + 3] = normal[face[i + 2] - 1].x;
				model[j + 4] = normal[face[i + 2] - 1].y;
				model[j + 5] = normal[face[i + 2] - 1].z;

				//texture
				model[j + 6] = texture[face[i + 1] - 1].x;
				model[j + 7] = 1-texture[face[i + 1] - 1].y;

				i += 6;
			}
			else {
				repeat = true;
				i += 3;
			}
		}
		else {
			i += 3;
		}
		j += 8;
	}

	for (int i = 0; i < edge_numVerts; i++) {
		edge[face_id][i] = vertex[edgeidx[i]];
	}

	free(charline);
	free(vertex);
	free(normal);
	free(texture);
	free(face);

	return model;
}

float* ParseObj(const char* filename, int &model_numVerts, const int edgeidx[], glm::vec3* edge, const int edge_numVerts) {
	//printf("Parsing the obj file %s:\n", filename);
	FILE* fp = fopen(filename, "r");
	if (!fp) {
		printf("Error! Cannot open the file!\nPlease check the path\n");
		exit(0);
	}

	//get the size of each element
	int num_vertex = 0;
	int num_texture_coord = 0;
	int num_normal = 0;
	int num_face = 0;

	char *charline = (char*)malloc(MAX_CHARACTER * sizeof(char));
	int cnt = 0;
	while (fgets(charline, MAX_CHARACTER, fp)) {
		char parameter[50];
		int fieldsRead = sscanf(charline, "%s ", parameter); //Read first word in the line (i.e., the command type)

		if (fieldsRead < 1) { //No command read
							  //Blank line
			continue;
		}
		if (strcmp(parameter, "v") == 0) {
			num_vertex++;
		}
		else if (strcmp(parameter, "vt") == 0) {
			num_texture_coord++;
		}
		else if (strcmp(parameter, "vn") == 0) {
			num_normal++;
		}
		else if (strcmp(parameter, "f") == 0) {
			num_face++;
		}
	}
	rewind(fp);

	//read the file
	glm::vec3* vertex = (glm::vec3 *)malloc(sizeof(glm::vec3)*num_vertex);
	glm::vec3* normal = (glm::vec3 *)malloc(sizeof(glm::vec3)*num_normal);
	glm::vec2* texture = (glm::vec2 *)malloc(sizeof(glm::vec2)*num_texture_coord);
	int* face = (int *)malloc(sizeof(int)*num_face * 12);
	for (int i = 0; i < num_face * 12; i++) {
		face[i] = -1;
	}

	int idx_vertex = 0;
	int idx_normal = 0;
	int idx_texture = 0;
	int idx_face = 0;
	int num_triangle = 0;

	while (fgets(charline, MAX_CHARACTER, fp)) { //Assumes no line is longer than 1024 characters!
		char parameter[50];
		int fieldsRead = sscanf(charline, "%s ", parameter); //Read first word in the line (i.e., the command type)

		if (fieldsRead < 1) { //No command read
							  //Blank line
			continue;
		}

		if (strcmp(parameter, "v") == 0) {
			sscanf(charline, "v %f %f %f", &vertex[idx_vertex].x, &vertex[idx_vertex].y, &vertex[idx_vertex].z);
			idx_vertex++;
		}
		else if (strcmp(parameter, "vt") == 0) {
			sscanf(charline, "vt %f %f", &texture[idx_texture].x, &texture[idx_texture].y);
			idx_texture++;
		}
		else if (strcmp(parameter, "vn") == 0) {
			sscanf(charline, "vn %f %f %f", &normal[idx_normal].x, &normal[idx_normal].y, &normal[idx_normal].z);
			idx_normal++;
		}
		else if (strcmp(parameter, "f") == 0) {
			sscanf(charline, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", \
				&face[idx_face], &face[idx_face + 1], &face[idx_face + 2], \
				&face[idx_face + 3], &face[idx_face + 4], &face[idx_face + 5], \
				&face[idx_face + 6], &face[idx_face + 7], &face[idx_face + 8], \
				&face[idx_face + 9], &face[idx_face + 10], &face[idx_face + 11]);
			if (face[idx_face + 9] == -1) { num_triangle++; }
			idx_face += 12;
		}
	}
	fclose(fp);

	//load to opengl, stored in model
	//note that the indices in face starts from 1 rather than 0
	model_numVerts = (num_face - num_triangle) * 6 + num_triangle * 3;
	float* model = (float *)malloc(sizeof(float)*model_numVerts * 8);
	int i = 0;
	int j = 0;
	bool repeat = true;
	while (i < num_face * 12) {
		if (face[i] == -1) {
			i += 3;
			continue;
		}
		else {
			//vertex
			model[j] = vertex[face[i] - 1].x;
			model[j + 1] = vertex[face[i] - 1].y;
			model[j + 2] = vertex[face[i] - 1].z;

			//normal
			model[j + 3] = normal[face[i + 2] - 1].x;
			model[j + 4] = normal[face[i + 2] - 1].y;
			model[j + 5] = normal[face[i + 2] - 1].z;

			//texture
			model[j + 6] = texture[face[i + 1] - 1].x;
			model[j + 7] = 1 - texture[face[i + 1] - 1].y;
		}

		//repeat the first and the third vertex in the face directive
		//i.e. i is 6, 6+12, ...
		if ((i % 12 == 6) && (face[i + 3] != -1)) {
			if (repeat) {
				repeat = false;

				//load the first vertex again
				j += 8; i -= 6;
				model[j] = vertex[face[i] - 1].x;
				model[j + 1] = vertex[face[i] - 1].y;
				model[j + 2] = vertex[face[i] - 1].z;

				//normal
				model[j + 3] = normal[face[i + 2] - 1].x;
				model[j + 4] = normal[face[i + 2] - 1].y;
				model[j + 5] = normal[face[i + 2] - 1].z;

				//texture
				model[j + 6] = texture[face[i + 1] - 1].x;
				model[j + 7] = 1 - texture[face[i + 1] - 1].y;

				i += 6;
			}
			else {
				repeat = true;
				i += 3;
			}
		}
		else {
			i += 3;
		}
		j += 8;
	}

	for (int i = 0; i < edge_numVerts; i++) {
		edge[i] = vertex[edgeidx[i]];
	}

	free(charline);
	free(vertex);
	free(normal);
	free(texture);
	free(face);

	return model;
}

float* ParseObj(const char* filename, int &model_numVerts) {
	//printf("Parsing the obj file %s:\n", filename);
	FILE* fp = fopen(filename, "r");
	if (!fp) {
		printf("Error! Cannot open the file!\nPlease check the path\n");
		exit(0);
	}

	//get the size of each element
	int num_vertex = 0;
	int num_texture_coord = 0;
	int num_normal = 0;
	int num_face = 0;

	char *charline = (char*)malloc(MAX_CHARACTER * sizeof(char));
	int cnt = 0;
	while (fgets(charline, MAX_CHARACTER, fp)) {
		char parameter[50];
		int fieldsRead = sscanf(charline, "%s ", parameter); //Read first word in the line (i.e., the command type)

		if (fieldsRead < 1) { //No command read
							  //Blank line
			continue;
		}
		if (strcmp(parameter, "v") == 0) {
			num_vertex++;
		}
		else if (strcmp(parameter, "vt") == 0) {
			num_texture_coord++;
		}
		else if (strcmp(parameter, "vn") == 0) {
			num_normal++;
		}
		else if (strcmp(parameter, "f") == 0) {
			num_face++;
		}
	}
	rewind(fp);

	//read the file
	glm::vec3* vertex = (glm::vec3 *)malloc(sizeof(glm::vec3)*num_vertex);
	glm::vec3* normal = (glm::vec3 *)malloc(sizeof(glm::vec3)*num_normal);
	glm::vec2* texture = (glm::vec2 *)malloc(sizeof(glm::vec2)*num_texture_coord);
	int* face = (int *)malloc(sizeof(int)*num_face * 12);
	for (int i = 0; i < num_face * 12; i++) {
		face[i] = -1;
	}

	int idx_vertex = 0;
	int idx_normal = 0;
	int idx_texture = 0;
	int idx_face = 0;
	int num_triangle = 0;

	while (fgets(charline, MAX_CHARACTER, fp)) { //Assumes no line is longer than 1024 characters!
		char parameter[50];
		int fieldsRead = sscanf(charline, "%s ", parameter); //Read first word in the line (i.e., the command type)

		if (fieldsRead < 1) { //No command read
							  //Blank line
			continue;
		}

		if (strcmp(parameter, "v") == 0) {
			sscanf(charline, "v %f %f %f", &vertex[idx_vertex].x, &vertex[idx_vertex].y, &vertex[idx_vertex].z);
			//printf("v %f %f %f\n", vertex[idx_vertex].x, vertex[idx_vertex].y, vertex[idx_vertex].z);
			idx_vertex++;
		}
		else if (strcmp(parameter, "vt") == 0) {
			sscanf(charline, "vt %f %f", &texture[idx_texture].x, &texture[idx_texture].y);
			//printf("vt %f %f\n", texture[idx_texture].x, texture[idx_texture].y);
			idx_texture++;
		}
		else if (strcmp(parameter, "vn") == 0) {
			sscanf(charline, "vn %f %f %f", &normal[idx_normal].x, &normal[idx_normal].y, &normal[idx_normal].z);
			//printf("vn %f %f %f\n", normal[idx_normal].x, normal[idx_normal].y, normal[idx_normal].z);
			idx_normal++;
		}
		else if (strcmp(parameter, "f") == 0) {
			sscanf(charline, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", \
				&face[idx_face], &face[idx_face + 1], &face[idx_face + 2], \
				&face[idx_face + 3], &face[idx_face + 4], &face[idx_face + 5], \
				&face[idx_face + 6], &face[idx_face + 7], &face[idx_face + 8], \
				&face[idx_face + 9], &face[idx_face + 10], &face[idx_face + 11]);
			if (face[idx_face + 9] == -1) { num_triangle++; }
			idx_face += 12;
		}
	}
	fclose(fp);

	//load to opengl, stored in model
	//note that the indices in face starts from 1 rather than 0
	model_numVerts = (num_face - num_triangle) * 6 + num_triangle * 3;
	float* model = (float *)malloc(sizeof(float)*model_numVerts * 8);
	int i = 0;
	int j = 0;
	bool repeat = true;
	while (i < num_face * 12) {
		//printf("i: %d\tj: %d\n", i, j);
		//skip -1/-1/-1
		if (face[i] == -1) {
			i += 3;
			continue;
		}
		else {
			//vertex
			model[j] = vertex[face[i] - 1].x;
			model[j + 1] = vertex[face[i] - 1].y;
			model[j + 2] = vertex[face[i] - 1].z;

			//normal
			model[j + 3] = normal[face[i + 2] - 1].x;
			model[j + 4] = normal[face[i + 2] - 1].y;
			model[j + 5] = normal[face[i + 2] - 1].z;

			//texture
			model[j + 6] = texture[face[i + 1] - 1].x;
			model[j + 7] = 1 - texture[face[i + 1] - 1].y;
		}

		//repeat the first and the third vertex in the face directive
		//i.e. i is 6, 6+12, ...
		if ((i % 12 == 6) && (face[i + 3] != -1)) {
			if (repeat) {
				repeat = false;

				//load the first vertex again
				j += 8; i -= 6;
				model[j] = vertex[face[i] - 1].x;
				model[j + 1] = vertex[face[i] - 1].y;
				model[j + 2] = vertex[face[i] - 1].z;

				//normal
				model[j + 3] = normal[face[i + 2] - 1].x;
				model[j + 4] = normal[face[i + 2] - 1].y;
				model[j + 5] = normal[face[i + 2] - 1].z;

				//texture
				model[j + 6] = texture[face[i + 1] - 1].x;
				model[j + 7] = 1 - texture[face[i + 1] - 1].y;

				i += 6;
			}
			else {
				repeat = true;
				i += 3;
			}
		}
		else {
			i += 3;
		}
		j += 8;
	}

	free(charline);
	free(vertex);
	free(normal);
	free(texture);
	free(face);

	return model;
}

GT_Variable ParseCSVwithSpace(const char* filename) {
	FILE* fp = fopen(filename, "r");
	if (!fp) {
		printf("Error! Cannot open the file!\nPlease check the path\n");
		exit(0);
	}

	char *charline = (char*)malloc(MAX_CHARACTER * sizeof(char));
	int cnt = 0;
	// estimate the total line numbers of the data file
	while (fgets(charline, MAX_CHARACTER, fp)) {
		cnt++;
	}
	rewind(fp);

	GT_Variable gt_var;
	GTAllocate(gt_var, cnt);
	cnt = 0;
	while (fgets(charline, MAX_CHARACTER, fp)) {
		sscanf(charline, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
			&gt_var.time[cnt], &gt_var.Lateral_canthus_L[cnt].x, &gt_var.Lateral_canthus_L[cnt].y,
			&gt_var.Lateral_canthus_R[cnt].x, &gt_var.Lateral_canthus_R[cnt].y, 
			&gt_var.Palpebral_fissure_RU[cnt].x, &gt_var.Palpebral_fissure_RU[cnt].y,
			&gt_var.Palpebral_fissure_RL[cnt].x, &gt_var.Palpebral_fissure_RL[cnt].y,
			&gt_var.Palpebral_fissure_LU[cnt].x, &gt_var.Palpebral_fissure_LU[cnt].y,
			&gt_var.Palpebral_fissure_LL[cnt].x, &gt_var.Palpebral_fissure_LL[cnt].y,
			&gt_var.Depressor_L[cnt].x, &gt_var.Depressor_L[cnt].y,
			&gt_var.Depressor_R[cnt].x, &gt_var.Depressor_R[cnt].y,
			&gt_var.Depressor_M[cnt].x, &gt_var.Depressor_M[cnt].y,
			&gt_var.Nasal_ala_L[cnt].x, &gt_var.Nasal_ala_L[cnt].y,
			&gt_var.Nasal_ala_R[cnt].x, &gt_var.Nasal_ala_R[cnt].y,
			&gt_var.Medial_brow_L[cnt].x, &gt_var.Medial_brow_L[cnt].y,
			&gt_var.Medial_brow_R[cnt].x, &gt_var.Medial_brow_R[cnt].y,
			&gt_var.Malar_eminence_L[cnt].x, &gt_var.Malar_eminence_L[cnt].y,
			&gt_var.Malar_eminence_R[cnt].x, &gt_var.Malar_eminence_R[cnt].y);
		/*printf(charline, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
			gt_var.time[cnt], gt_var.Lateral_canthus_L[cnt].x, gt_var.Lateral_canthus_L[cnt].y,
			gt_var.Lateral_canthus_R[cnt].x, gt_var.Lateral_canthus_R[cnt].y,
			gt_var.Palpebral_fissure_RU[cnt].x, gt_var.Palpebral_fissure_RU[cnt].y,
			gt_var.Palpebral_fissure_RL[cnt].x, gt_var.Palpebral_fissure_RL[cnt].y,
			gt_var.Palpebral_fissure_LU[cnt].x, gt_var.Palpebral_fissure_LU[cnt].y,
			gt_var.Palpebral_fissure_LL[cnt].x, gt_var.Palpebral_fissure_LL[cnt].y,
			gt_var.Depressor_L[cnt].x, gt_var.Depressor_L[cnt].y,
			gt_var.Depressor_R[cnt].x, gt_var.Depressor_R[cnt].y,
			gt_var.Depressor_M[cnt].x, gt_var.Depressor_M[cnt].y,
			gt_var.Nasal_ala_L[cnt].x, gt_var.Nasal_ala_L[cnt].y,
			gt_var.Nasal_ala_R[cnt].x, gt_var.Nasal_ala_R[cnt].y,
			gt_var.Medial_brow_L[cnt].x, gt_var.Medial_brow_L[cnt].y,
			gt_var.Medial_brow_R[cnt].x, gt_var.Medial_brow_R[cnt].y,
			gt_var.Malar_eminence_L[cnt].x, gt_var.Malar_eminence_L[cnt].y,
			gt_var.Malar_eminence_R[cnt].x, gt_var.Malar_eminence_R[cnt].y);*/
		cnt++;
	}
	fclose(fp);

	return gt_var;
}

GLuint LoadToVBO(float* model, int model_numVerts) {
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*model_numVerts * 8, model, GL_STATIC_DRAW);
	return vbo;
}

GLuint LoadToVAO(GLuint shaderprogram) {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLint posAttrib = glGetAttribLocation(shaderprogram, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
	glEnableVertexAttribArray(posAttrib);

	GLint nolAttrib = glGetAttribLocation(shaderprogram, "inNormal");
	glVertexAttribPointer(nolAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(nolAttrib);

	GLint texAttrib = glGetAttribLocation(shaderprogram, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	glBindVertexArray(0); //Unbind the VAO

	return vao;
}

GLuint AllocateTexture(const char* tex_filename, int i) {
	SDL_Surface* surface = SDL_LoadBMP(tex_filename);
	if (surface == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); 
		exit(1);
	}
	GLuint tex0;
	glGenTextures(1, &tex0);

	glActiveTexture(GL_TEXTURE0 + i);
	glBindTexture(GL_TEXTURE_2D, tex0);

	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Load the texture into memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(surface);

	return tex0;
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile) {
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position
	printf("length: %d\n", length);

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName) {
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	bool DEBUG_ON = true;

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	}
	else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	}
	else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders

	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);

	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}
