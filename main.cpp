#include "stdafx.h"
#include "blendcalc.h"
#include "gpqp_solv.hpp"

int screen_width = 1000;
int screen_height = 600;
bool fullscreen = false;
int iter_num = 100;

int main(int argc, char *argv[]) {	

	if( argc > 1 ) screen_width = (int)strtol(argv[1], NULL, 10);
	if( argc > 2 ) screen_height = (int)strtol(argv[2], NULL, 10);

	//Initialize SDL
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_Window* window = SDL_CreateWindow("Smile_Simulation", 100, 50, screen_width, screen_height, SDL_WINDOW_OPENGL);
	float aspect = screen_width / (float)screen_height; 

	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}

	//Load face model
	GLuint tex0 = AllocateTexture(TEX_DIR "/head_BaseColor.bmp", 0);
	int face_num = 7;
	int model_numVerts = 0;

	// edge stores the bound of each facial feature
	// the basic points for canonical space, 1450 is used as origin and combine 3542 to make a line
	// the index here has the same order as data in csv file
	int edgeidx[] = { 797, 133, 3542, 1450, 9807, 9928, 18028, 18150, 14813, 6565, 12, \
		              14116, 5853, 15909, 7669, 14928, 6675, 22524, 5643 };
	// 0: middle points, 1: left face, 2: right face
	int bitmap[] = { 1, 2, 1, 2, 2, 2, 1, 1, 1, 2, 0, \
					  1, 2, 1, 2, 1, 2, 0, 0};
	int edge_numVerts = sizeof(edgeidx) / sizeof(int);

	Face** faces = (Face**)malloc(sizeof(Face*) * (face_num + 1));
	faces[0] = ParseObj(MESH_DIR "/open_smile.obj", model_numVerts, edgeidx, edge_numVerts, bitmap);
	faces[1] = ParseObj(MESH_DIR "/blink_upper_lid.obj", model_numVerts, edgeidx, edge_numVerts, bitmap);
	faces[2] = ParseObj(MESH_DIR "/closed_smile.obj", model_numVerts, edgeidx, edge_numVerts, bitmap);
	faces[3] = ParseObj(MESH_DIR "/lower_lip_drop.obj", model_numVerts, edgeidx, edge_numVerts, bitmap);
	faces[4] = ParseObj(MESH_DIR "/surprised.obj", model_numVerts, edgeidx, edge_numVerts, bitmap);
	faces[5] = ParseObj(MESH_DIR "/wide_mouth_open.obj", model_numVerts, edgeidx, edge_numVerts, bitmap);
	faces[6] = ParseObj(MESH_DIR "/frown.obj", model_numVerts, edgeidx, edge_numVerts, bitmap);
	faces[7] = ParseObj(MESH_DIR "/base_head.obj", model_numVerts, edgeidx, edge_numVerts, bitmap);

	//Load eye model
	GLuint tex1 = AllocateTexture(TEX_DIR "/eye_BaseColor.bmp", 1);	
	int eye_numVerts = 0;
	float* eyes = ParseObj(MESH_DIR "/eyes.obj", eye_numVerts);

	//Load mouth model
	GLuint tex2 = AllocateTexture(TEX_DIR "/teeth_n_gums_BaseColor.bmp", 2);
	int teeth_numVerts = 0;
	float* teeth = ParseObj(MESH_DIR "/teeth.obj", teeth_numVerts);

	GLuint vbo_face;
	glGenBuffers(1, &vbo_face);
	GLuint vbo_eyes;
	glGenBuffers(1, &vbo_eyes);
	GLuint vbo_teeth;
	glGenBuffers(1, &vbo_teeth);
	GLuint vbo_point;
	glGenBuffers(1, &vbo_point);

	GLuint ShaderProgram = InitShader(SHADER_DIR "/vertexTex.glsl", SHADER_DIR "/fragmentTex.glsl");
	GLuint PointShaderProgram = InitShader(SHADER_DIR "/vertex.glsl", SHADER_DIR "/fragment.glsl");
	
	glEnable(GL_DEPTH_TEST);
	
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplSdlGL3_Init(window);

	// Setup style
	ImGui::StyleColorsDark();

	glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 40.0f);  //Cam Position
	glm::vec3 look_point = glm::vec3(0.0f, -8.0f, 0.0f);  //Look at point
	glm::vec3 up_vector = glm::vec3(0.0f, 1.0f, 0.0f); //Up

	glm::mat4 view = glm::lookAt(camera_position, look_point, up_vector);
	glm::mat4 proj = glm::perspective(3.14f / 4, aspect, 0.1f, 10000.0f); //FOV, aspect, near, far

	SDL_Event windowEvent;

	GLuint uniColor = glGetUniformLocation(ShaderProgram, "inColor");
	GLuint uniProj = glGetUniformLocation(ShaderProgram, "proj");
	GLuint uniView = glGetUniformLocation(ShaderProgram, "view");
	GLuint uniModel = glGetUniformLocation(ShaderProgram, "model");
	GLint uniTexID = glGetUniformLocation(ShaderProgram, "texID");
	
	glm::vec3 inColor;
	float line_width = 2.0f;
	glLineWidth(line_width);
	float point_size = 3.0f;
	glPointSize(point_size);

	//parameters for face interpolation
	float* alpha_left = (float *)calloc(face_num, sizeof(float));
	float* alpha_right = (float *)calloc(face_num, sizeof(float));

	int id_left = 0;
	int id_right = faces[face_num]->num_left * 24;

	// face is used for drawing, initialize it to be base_head
	float* face = (float *)malloc(sizeof(float)*(faces[face_num]->num_left + faces[face_num]->num_right)* 24);
	for (int i = 0; i < faces[face_num]->num_left * 24; i++) {
		face[id_left++] = faces[face_num]->left[i];
	}
	for (int i = 0; i < faces[face_num]->num_right * 24; i++) {
		face[id_right++] = faces[face_num]->right[i];
	}

	glm::mat4 Xbox_camera = glm::perspective(3.14f / 3, tan(35 * 3.14f / 180)*sqrtf(3.0f), 50.0f, 450.0f); //FOV, aspect, near, far
	Xbox_camera[1][1] = -1;
	float face_length = 6.520 + 8.882; // the length of the face in original model space
	float real_face_length = 24; // in centimeter
	float face_ratio = real_face_length / face_length;
	
	glm::vec2 *bh_fpoint_left = (glm::vec2 *)malloc(sizeof(glm::vec2)*faces[face_num]->num_left_fp);
	glm::vec2 *bh_fpoint_right = (glm::vec2 *)malloc(sizeof(glm::vec2)*faces[face_num]->num_right_fp);

	//convert the feature points in base_head to perspective plane
	for (int i = 0; i < faces[face_num]->num_left_fp; i++) {
		glm::vec3 temp_fp = glm::vec3(faces[face_num]->left_fp[3 * i], \
										faces[face_num]->left_fp[3 * i + 1], \
										faces[face_num]->left_fp[3 * i + 2]);
		glm::vec4 temp = Xbox_camera * glm::vec4(temp_fp * face_ratio, 1.0);
		bh_fpoint_left[i] = glm::vec2(temp.x, temp.y);
	}
	for (int i = 0; i < faces[face_num]->num_right_fp; i++) {
		glm::vec3 temp_fp = glm::vec3(faces[face_num]->right_fp[3 * i], \
										faces[face_num]->right_fp[3 * i + 1], \
										faces[face_num]->right_fp[3 * i + 2]);
		glm::vec4 temp = Xbox_camera * glm::vec4(temp_fp * face_ratio, 1.0);
		bh_fpoint_right[i] = glm::vec2(temp.x, temp.y);
	}

	//printf("perspective base head: \n");
	//printf("base_head left: \n");
	//for (int i = 0; i < faces[face_num]->num_left_fp; i++) {
	//	printf("(%f, %f)\n", bh_fpoint_left[i].x, bh_fpoint_left[i].y);
	//}
	//printf("base_head right: \n");
	//for (int i = 0; i < faces[face_num]->num_right_fp; i++) {
	//	printf("(%f, %f)\n", bh_fpoint_right[i].x, bh_fpoint_right[i].y);
	//}

	//transfer to canonical space
	trans = bh_fpoint_right[1];
	
	scale = glm::distance(bh_fpoint_left[1], bh_fpoint_right[1]);
	//printf("basehead trans: %f %f scale: %f\n", trans.x, trans.y, scale);

	for (int i = 0; i < faces[face_num]->num_left_fp; i++) {
		bh_fpoint_left[i] -= trans;
		bh_fpoint_left[i] = bh_fpoint_left[i] / scale * unit;
	}
	for (int i = 0; i < faces[face_num]->num_right_fp; i++) {
		bh_fpoint_right[i] -= trans;
		bh_fpoint_right[i] = bh_fpoint_right[i] / scale * unit;
	}

	//printf("base_head left: \n");
	//for (int i = 0; i < faces[face_num]->num_left_fp; i++) {
	//	printf("(%f, %f)\n", bh_fpoint_left[i].x, bh_fpoint_left[i].y);
	//}
	//printf("base_head right: \n");
	//for (int i = 0; i < faces[face_num]->num_right_fp; i++) {
	//	printf("(%f, %f)\n", bh_fpoint_right[i].x, bh_fpoint_right[i].y);
	//}

	//SET UP THE LS SYSTEM
	blendcalc blendsys_left;
	blendcalc blendsys_right;
	blendsys_left.blend_a = Eigen::MatrixXf(faces[face_num]->num_left_fp * 2, face_num);
	blendsys_right.blend_a = Eigen::MatrixXf(faces[face_num]->num_right_fp * 2, face_num);
	
	//Transfer the feature points into a perspective plane
	for (int j = 0; j < face_num; j++) {
		for (int i = 0, k = 0; i < faces[face_num]->num_left_fp; i++) {
			glm::vec3 temp_fp = glm::vec3(faces[j]->left_fp[3 * i], \
											faces[j]->left_fp[3 * i + 1], \
											faces[j]->left_fp[3 * i + 2]);
			glm::vec4 temp = Xbox_camera * glm::vec4(temp_fp * face_ratio, 1.0);
			blendsys_left.blend_a(k++, j) = temp.x;
			blendsys_left.blend_a(k++, j) = temp.y;
		}
		for (int i = 0, k = 0; i < faces[face_num]->num_right_fp; i++) {
			glm::vec3 temp_fp = glm::vec3(faces[j]->right_fp[3 * i], \
											faces[j]->right_fp[3 * i + 1], \
											faces[j]->right_fp[3 * i + 2]);
			glm::vec4 temp = Xbox_camera * glm::vec4(temp_fp * face_ratio, 1.0);
			blendsys_right.blend_a(k++, j) = temp.x;
			blendsys_right.blend_a(k++, j) = temp.y;
		}
	}

	//printf("perspective plane:\n");
	//printf("left face: \n");
	//for (int i = 0; i < blendsys_left.blend_a.rows(); i++) {
	//	printf("i: %d ", i);
	//	for (int j = 0; j < blendsys_left.blend_a.cols(); j++) {
	//		printf("%f ", blendsys_left.blend_a(i, j));
	//	}
	//	printf("\n");
	//}
	//printf("\nright face: \n");
	//for (int i = 0; i < blendsys_right.blend_a.rows(); i++) {
	//	printf("i: %d ", i);
	//	for (int j = 0; j < blendsys_right.blend_a.cols(); j++) {
	//		printf("%f ", blendsys_right.blend_a(i, j));
	//	}
	//	printf("\n");
	//}

	//Transfer to the canonical space
	for (int j = 0; j < face_num; j++) {
		trans = glm::vec2(blendsys_right.blend_a(2, j), blendsys_right.blend_a(3, j));
		scale = glm::distance(trans, 
							  glm::vec2(blendsys_left.blend_a(2, j),
								  blendsys_left.blend_a(3, j)));
		//printf("j: %d trans: (%f, %f)\tscale: %f\n", j, trans.x, trans.y, scale);
		
		// left face
		for (int i = 0, k = 0; i < faces[j]->num_left_fp * 2; i += 2, k++) {
			//printf("left face: i %d, k %d\n", i, k);
			//translate
			blendsys_left.blend_a(i, j) -= trans.x;
			blendsys_left.blend_a(i + 1, j) -= trans.y;
			//printf("left face %d (-trans): %f, %f\n", i, blendsys_left.blend_a(i, j), blendsys_left.blend_a(i+1, j));

			//scale
			blendsys_left.blend_a(i, j) = blendsys_left.blend_a(i, j) / scale * unit;
			blendsys_left.blend_a(i + 1, j) = blendsys_left.blend_a(i + 1, j) / scale * unit;

			//blendshape
			blendsys_left.blend_a(i, j) -= bh_fpoint_left[k].x;
			blendsys_left.blend_a(i + 1, j) -= bh_fpoint_left[k].y;
		}

		// right face
		for (int i = 0, k = 0; i < faces[j]->num_right_fp * 2; i += 2, k++) {
			//printf("right face: i %d, k %d\n", i, k);
			//translate
			blendsys_right.blend_a(i, j) -= trans.x;
			blendsys_right.blend_a(i + 1, j) -= trans.y;

			//scale
			blendsys_right.blend_a(i, j) = blendsys_right.blend_a(i, j) / scale * unit;
			blendsys_right.blend_a(i + 1, j) = blendsys_right.blend_a(i + 1, j) / scale * unit;

			//blendshape
			blendsys_right.blend_a(i, j) -= bh_fpoint_right[k].x;
			blendsys_right.blend_a(i + 1, j) -= bh_fpoint_right[k].y;
		}
	}

	//printf("left face: \n");
	//for (int i = 0; i < blendsys_left.blend_a.rows(); i++) {
	//	printf("i: %d ", i);
	//	for (int j = 0; j < blendsys_left.blend_a.cols(); j++) {
	//		printf("%f ", blendsys_left.blend_a(i, j));
	//	}
	//	printf("\n");
	//}
	//printf("\nright face: \n");
	//for (int i = 0; i < blendsys_right.blend_a.rows(); i++) {
	//	printf("i: %d ", i);
	//	for (int j = 0; j < blendsys_right.blend_a.cols(); j++) {
	//		printf("%f ", blendsys_right.blend_a(i, j));
	//	}
	//	printf("\n");
	//}


	//read the sample data
	//point: x, y, z
	float* point_left = (float *)calloc(faces[face_num]->num_left_fp * 3, sizeof(float));
	float* point_right = (float *)calloc(faces[face_num]->num_right_fp * 3, sizeof(float));
	Eigen::MatrixXf feature_csv;
	//read the feature points from .csv files, the first row is timestamp
	GetCSVFile(feature_csv, CAN_DIR "/28162_v_canon.csv", edge_numVerts);
	SplitBVector(feature_csv, bitmap, edge_numVerts);

	//printf("left x:\n");
	Eigen::MatrixXf X_left = Eigen::MatrixXf(face_num + 1, feature_csv.cols() / 2);
	Eigen::MatrixXf X_right = Eigen::MatrixXf(face_num + 1, feature_csv.cols() / 2);
	for (int j = 0; j < feature_csv.cols(); j++) {		
		Eigen::VectorXf x_0 = Eigen::VectorXf::Ones(X_left.rows() - 1) * 0.5f;
		Eigen::VectorXf x_lb = Eigen::VectorXf::Zero(X_left.rows() - 1);
		Eigen::VectorXf x_ub = Eigen::VectorXf::Ones(X_left.rows() - 1);

		if (j % 2 == 0) {
			// store time
			X_left(0, j/2) = feature_csv(0, j);
			blendsys_left.blend_b = feature_csv.block(1, j, feature_csv.rows() - 1, 1); 
			// i,j,p,q: start(i,j), size: p-by-q, the first line is time
			for (int i = 0; i < faces[face_num]->num_left_fp; i++) {
				blendsys_left.blend_b(2 * i) -= bh_fpoint_left[i].x;
				blendsys_left.blend_b(2 * i + 1) -= bh_fpoint_left[i].y;
				//printf("left %d: %f, %f\n", i, blendsys_left.blend_b(2 * i), blendsys_left.blend_b(2 * i + 1));
			}
			
			blendsys_left.blend_x = GPQPSolver::Solve(blendsys_left.blend_a, blendsys_left.blend_b, x_lb, x_ub, x_0, iter_num);
			
			// store calculated weights
			for (int i = 1; i < X_left.rows(); i++) {
				//printf("%f ", blendsys_left.blend_x(i - 1));
				X_left(i, j/2) = blendsys_left.blend_x(i - 1);
			}
			//printf("\n");
		}
		else if (j % 2 == 1) {
			// store time
			X_right(0, j/2) = feature_csv(0, j);
			blendsys_right.blend_b = feature_csv.block(1, j, feature_csv.rows() - 1, 1);
			for (int i = 0; i < faces[face_num]->num_right_fp; i++) {
				blendsys_right.blend_b(2 * i) -= bh_fpoint_right[i].x;
				blendsys_right.blend_b(2 * i + 1) -= bh_fpoint_right[i].y;
			}
			blendsys_right.blend_x = GPQPSolver::Solve(blendsys_right.blend_a, blendsys_right.blend_b, x_lb, x_ub, x_0, iter_num);
			// store calculated weights
			for (int i = 1; i < X_right.rows(); i++) {
				X_right(i, j/2) = blendsys_right.blend_x(i - 1);
			}
		}
	}
	free(bh_fpoint_left);
	free(bh_fpoint_right);

	//FILE *fpx = fopen(SHADER_DIR "/X.txt", "w+");
	//if (fpx == NULL) {
	//	printf("error open X.txt\n");
	//}
	//for (int i = 0; i < X.rows(); i++) {
	//	fprintf(fpx, "%d\t", i);
	//	for (int j = 0; j < X.cols(); j++) {
	//		fprintf(fpx, "%f ", X(i, j));
	//	}
	//	fprintf(fpx, "\n");
	//}
	//fclose(fpx);

	//interpolation between weights according to time
	Eigen::MatrixXf weights_left;
	Eigen::MatrixXf weights_right;
	float time_interval = 0.01f;
	Interpolation(X_left, weights_left, time_interval);
	Interpolation(X_right, weights_right, time_interval);
	//Interpolation_Split(X, weights, time_interval);

	FILE *fpw = fopen(SHADER_DIR "/Weights_left.txt", "w+");
	if (fpw == NULL) {
		printf("error open Weights_left.txt\n");
	}
	for (int i = 0; i < weights_left.rows(); i++) {
		fprintf(fpw, "%d\t", i);
		for (int j = 0; j < weights_left.cols(); j++) {
			fprintf(fpw, "%f ", weights_left(i, j));
		}
		fprintf(fpw, "\n");
	}
	fclose(fpw);

	fpw = fopen(SHADER_DIR "/Weights_right.txt", "w+");
	if (fpw == NULL) {
		printf("error open Weights_right.txt\n");
	}
	for (int i = 0; i < weights_right.rows(); i++) {
		fprintf(fpw, "%d\t", i);
		for (int j = 0; j < weights_right.cols(); j++) {
			fprintf(fpw, "%f ", weights_right(i, j));
		}
		fprintf(fpw, "\n");
	}
	fclose(fpw);

	int idx_loop = 0; // frame counter
	bool show_window = false;
	bool start = true;
	bool quit = false;
	while (!quit) {
		while (SDL_PollEvent(&windowEvent)) {
			if (windowEvent.type == SDL_QUIT) quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; ; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) //If "f" is pressed
				fullscreen = !fullscreen;
			SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Set to full screen

			//reset
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_r) {
				idx_loop = 0;
				start = true;
			}

			ImGui_ImplSdlGL3_ProcessEvent(&windowEvent);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		//if (idx_loop >= weights.cols()) {
		//	start = false;
		//}

		if (start) {
			/*if (idx_loop < weights_left.cols()) {
				for (int i = 0; i < face_num; i++) {
					alpha_left[i] = weights_left(i, idx_loop);
					alpha_right[i] = weights_right(i, idx_loop);
				}
			}*/

			// calculate data
			id_left = 0;
			//id_right = faces[face_num]->num_left * 24;
			//for (int i = 0; i < faces[face_num]->num_right * 24; i++, id_right++) {
			//	face[id_right] = faces[face_num]->right[i];
			//	for (int j = 0; j < face_num; j++) {
			//		face[id_right] += alpha_right[j] * (faces[j]->right[i] - faces[face_num]->right[i]);
			//	}
			//}

			alpha_left[0] = 0;
			alpha_left[1] = 0.694471;
			alpha_left[2] = 0.266363;
			alpha_left[3] = 0;
			alpha_left[4] = 0;
			alpha_left[5] = 0;
			alpha_left[6] = 0.430098;

			for (int i = 0; i < faces[face_num]->num_left * 24; i++, id_left++) {
				face[id_left] = faces[face_num]->left[i];
				for (int j = 0; j < face_num; j++) {
					face[id_left] += alpha_left[j] * (faces[j]->left[i] - faces[face_num]->left[i]);
				}
			}

			

			/*for (int i = 0, k = 0; i < edge_numVerts; i++, k += 3) {
				point[k] = edge[face_num][i].x;
				point[k + 1] = edge[face_num][i].y;
				point[k + 2] = edge[face_num][i].z;

				for (int j = 0; j < face_num; j++) {
					point[k] += alpha[j] * (edge[j][i].x - edge[face_num][i].x);
					point[k + 1] += alpha[j] * (edge[j][i].y - edge[face_num][i].y);
					point[k + 2] += alpha[j] * (edge[j][i].z - edge[face_num][i].z);
				}
			}*/

			//Load the face model to buffer
			glBindBuffer(GL_ARRAY_BUFFER, vbo_face);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(faces[face_num]->num_left + faces[face_num]->num_right) * 24, face, GL_STATIC_DRAW);
			//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*faces[face_num]->num_left * 24, face, GL_STATIC_DRAW);

			//draw face
			glUseProgram(ShaderProgram);
			GLuint vao_face = LoadToVAO(ShaderProgram);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex0);
			glUniform1i(glGetUniformLocation(ShaderProgram, "tex0"), 0);
			glUniform1i(uniTexID, 0);

			glBindVertexArray(vao_face);
			glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
			glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

			glm::mat4 modelmat(1.0f);
			glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(modelmat));

			glDrawArrays(GL_TRIANGLES, 0, (faces[face_num]->num_left + faces[face_num]->num_right) * 3);
			glBindVertexArray(0);

			////Load the points to buffer
			//glBindBuffer(GL_ARRAY_BUFFER, vbo_point);
			//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*edge_numVerts * 3, point, GL_STATIC_DRAW);

			////draw the edge points
			//glUseProgram(PointShaderProgram);
			//GLuint vao_point = LoadToVAO_Point(PointShaderProgram);
			//glBindVertexArray(vao_point);

			//inColor = glm::vec3(0.0, 1.0, 0.0);
			//glUniform3f(uniColor, inColor.r, inColor.g, inColor.b);
			//glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
			//glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

			//glm::mat4 pointmat(1.0f);
			//glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(pointmat));

			//glDrawArrays(GL_POINTS, 0, edge_numVerts);
			//glBindVertexArray(0);

			//Load the eyes to buffer
			glBindBuffer(GL_ARRAY_BUFFER, vbo_eyes);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)*eye_numVerts * 8, eyes, GL_STATIC_DRAW);

			//draw eyes
			glUseProgram(ShaderProgram);
			GLuint vao_eye = LoadToVAO(ShaderProgram);
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, tex1);
			glUniform1i(glGetUniformLocation(ShaderProgram, "tex1"), 1);
			glUniform1i(uniTexID, 1);

			glBindVertexArray(vao_eye);
			glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
			glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

			glm::mat4 eyemat(1.0f);
			glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(eyemat));

			glDrawArrays(GL_TRIANGLES, 0, eye_numVerts);
			glBindVertexArray(0);

			//Load the teeth to buffer
			glBindBuffer(GL_ARRAY_BUFFER, vbo_teeth);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)*teeth_numVerts * 8, teeth, GL_STATIC_DRAW);

			//draw teeth
			glUseProgram(ShaderProgram);
			GLuint vao_teeth = LoadToVAO(ShaderProgram);
			glActiveTexture(GL_TEXTURE0 + 2);
			glBindTexture(GL_TEXTURE_2D, tex2);
			glUniform1i(glGetUniformLocation(ShaderProgram, "tex2"), 2);
			glUniform1i(uniTexID, 2);

			glBindVertexArray(vao_teeth);
			glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
			glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

			glm::mat4 teethmat(1.0f);
			glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(teethmat));

			glDrawArrays(GL_TRIANGLES, 0, teeth_numVerts);
			glBindVertexArray(0);
		}

		//static char frame[4] = { "20" };
		
		////draw the slidebar
		//ImGui_ImplSdlGL3_NewFrame(window);
		//ImGui::Begin("Face Parameters", &show_window);

		//ImGui::Text("The parameters to adjust the facial features.\n");
		//ImGui::Text("Press key 's' to start auto-face.\n");
		//ImGui::Text("Press key 'r' to reset (neutral face).\n");
		//ImGui::Text("After reset, the last parameters will show up\n");
		//ImGui::Text("in the command window.\n");
		//ImGui::SliderFloat("open_smile", &alpha[0], 0.0f, 1.0f);
		//ImGui::SliderFloat("blink_upper_lid", &alpha[1], 0.0f, 1.0f);
		//ImGui::SliderFloat("closed_smile", &alpha[2], 0.0f, 1.0f);
		//ImGui::SliderFloat("lower_lip_drop", &alpha[3], 0.0f, 1.0f);
		//ImGui::SliderFloat("surprised", &alpha[4], 0.0f, 1.0f);
		//ImGui::SliderFloat("wide_mouth_open", &alpha[5], 0.0f, 1.0f);
		//ImGui::SliderFloat("frown", &alpha[6], 0.0f, 1.0f);

		//ImGui::InputText("frame", frame, sizeof(frame));
		////printf("frame: %f\n", (float)atof(frame));

		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		//ImGui::End();
		//glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

		//ImGui::Render();
		//ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

		idx_loop++;
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplSdlGL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	free(face);
	free(faces);
	//free(alpha);
	//free(point);
    return 0;
}
