#include "stdafx.h"

int screen_width = 1000;
int screen_height = 600;
bool fullscreen = false;

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

	//edge stores the bound of each facial feature
	int edgeidx[] = { 133,1450,9807,9928,14813,656,12,5853,7669,6675,22524,5643 };
	int edge_numVerts = sizeof(edgeidx) / sizeof(int);
	glm::vec3** edge = (glm::vec3**)malloc(sizeof(glm::vec3*) * (face_num + 1));
	for (int i = 0; i < face_num + 1; i++) {
		edge[i] = (glm::vec3*)malloc(sizeof(glm::vec3) * edge_numVerts);
	}
	float* base_head = ParseObj(MESH_DIR "/base_head.obj", model_numVerts, edgeidx, edge, face_num, edge_numVerts);
	float** faces = (float**)malloc(sizeof(float*)*face_num);
	faces[0] = ParseObj(MESH_DIR "/open_smile.obj", model_numVerts, edgeidx, edge, 0, edge_numVerts);
	faces[1] = ParseObj(MESH_DIR "/blink_upper_lid.obj", model_numVerts, edgeidx, edge, 1, edge_numVerts);
	faces[2] = ParseObj(MESH_DIR "/closed_smile.obj", model_numVerts, edgeidx, edge, 2, edge_numVerts);
	faces[3] = ParseObj(MESH_DIR "/lower_lip_drop.obj", model_numVerts, edgeidx, edge, 3, edge_numVerts);
	faces[4] = ParseObj(MESH_DIR "/surprised.obj", model_numVerts, edgeidx, edge, 4, edge_numVerts);
	faces[5] = ParseObj(MESH_DIR "/wide_mouth_open.obj", model_numVerts, edgeidx, edge, 5, edge_numVerts);
	faces[6] = ParseObj(MESH_DIR "/frown.obj", model_numVerts, edgeidx, edge, 6, edge_numVerts);

	//Load eye model
	GLuint tex1 = AllocateTexture(TEX_DIR "/eye_BaseColor.bmp", 1);	
	int eye_numVerts = 0;
	int irisidx[] = { 257, 286 }; //257 is iris(L), 286 is iris(M)
	int iris_numVerts = sizeof(irisidx) / sizeof(int);
	glm::vec3* iris = (glm::vec3*)malloc(sizeof(glm::vec3) * edge_numVerts);
	float* eyes = ParseObj(MESH_DIR "/eyes.obj", eye_numVerts, irisidx, iris, iris_numVerts);

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
	float* alpha = (float *)calloc(face_num, sizeof(float));
	float* perfect = (float *)calloc(face_num, sizeof(float));

	float* face = (float *)malloc(sizeof(float)*model_numVerts * 8);
	for (int i = 0; i < model_numVerts * 8; i++) {
		face[i] = base_head[i];
	}

	glm::vec3* edge_point = (glm::vec3 *)malloc(sizeof(glm::vec3)*edge_numVerts);
	for (int i = 0; i < edge_numVerts; i++) {
		edge_point[i] = edge[face_num][i];
		//printf("edge_point[%d]=(%f,%f,%f)\n", i, edge_point[i].x, edge_point[i].y, edge_point[i].z);
	}
	
	// 8 values per edge (x, y, z, w) 
	// ? whats w/ the third value
	float* point = (float *)calloc(8 * edge_numVerts * 2, sizeof(float));

	bool show_window = false;

	bool start = false;
	bool quit = false;
	while (!quit) {
		while (SDL_PollEvent(&windowEvent)) {
			if (windowEvent.type == SDL_QUIT) quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; ; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) //If "f" is pressed
				fullscreen = !fullscreen;
			SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Set to full screen

			//start drawing
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_s) {
				start = true;
				for (int i = 0; i < face_num; i++) {
					perfect[i] = alpha[i];
					alpha[i] = 0;
				}
			}

			//reset
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_r) {
				printf("last perfect face:\n");
				for (int i = 0; i < face_num; i++) {
					printf("alpha%d: %f\n", i, alpha[i]);
				}
				for (int i = 0; i < face_num; i++) {
					alpha[i] = 0;
				}
				start = false;
			}

			ImGui_ImplSdlGL3_ProcessEvent(&windowEvent);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		for (int i = 0; i < model_numVerts * 8; i++) {
			face[i] = base_head[i];
			for (int j = 0; j < face_num; j++) {
				face[i] += alpha[j] * (faces[j][i] - base_head[i]);
			}
		}

		// face_num = 7
		// loops edge[face_num] -> glm::vec3 feature points of face to be rendered to screen
		// point[k + ...] -> points of feature points arranged as float array (x, y, z)
		// ? why is point[k + 5] set to 1 
		// Uses -x value of point to reflect features to the opposit side of faces
		for (int i = 0, k = 0; i < edge_numVerts; i++, k += 8) {
			point[k] = edge[face_num][i].x;
			point[k + 1] = edge[face_num][i].y;
			point[k + 2] = edge[face_num][i].z;
			point[k + 5] = 1;

			point[k + edge_numVerts*8] = -edge[face_num][i].x;
			point[k + edge_numVerts*8 + 1] = edge[face_num][i].y;
			point[k + edge_numVerts*8 + 2] = edge[face_num][i].z;
			point[k + edge_numVerts*8 + 5] = 1;

			for (int j = 0; j < face_num; j++) {
				point[k] += alpha[j] * (edge[j][i].x - edge[face_num][i].x);
				point[k + 1] += alpha[j] * (edge[j][i].y - edge[face_num][i].y);
				point[k + 2] += alpha[j] * (edge[j][i].z - edge[face_num][i].z);

				point[k + edge_numVerts*8] += alpha[j] * (-edge[j][i].x + edge[face_num][i].x);
				point[k + edge_numVerts*8 + 1] += alpha[j] * (edge[j][i].y - edge[face_num][i].y);
				point[k + edge_numVerts*8 + 2] += alpha[j] * (edge[j][i].z - edge[face_num][i].z);
			}
			//printf("point[%d]=(%f,%f,%f)\n", i, point[k], point[k + 1], point[k + 2]);
		}

		//Load the face model to buffer
		glBindBuffer(GL_ARRAY_BUFFER, vbo_face);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*model_numVerts * 8, face, GL_STATIC_DRAW);
		
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

		glDrawArrays(GL_TRIANGLES, 0, model_numVerts);
		glBindVertexArray(0);

		//Load the points to buffer
		glBindBuffer(GL_ARRAY_BUFFER, vbo_point);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*edge_numVerts * 8 * 2, point, GL_STATIC_DRAW);

		//draw the edge points
		glUseProgram(PointShaderProgram);
		GLuint vao_point = LoadToVAO(PointShaderProgram);
		glBindVertexArray(vao_point);

		inColor = glm::vec3(0.0, 1.0, 0.0);
		glUniform3f(uniColor, inColor.r, inColor.g, inColor.b);
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 pointmat(1.0f);
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(pointmat));

		glDrawArrays(GL_POINTS, 0, edge_numVerts*2);
		glBindVertexArray(0);

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

		static char frame[4] = { "20" };
		if (!start) {
			//draw the slidebar
			ImGui_ImplSdlGL3_NewFrame(window);
			ImGui::Begin("Face Parameters", &show_window);

			ImGui::Text("The parameters to adjust the facial features.\n");
			ImGui::Text("Press key 's' to start auto-face.\n");
			ImGui::Text("Press key 'r' to reset (neutral face).\n");
			ImGui::Text("After reset, the last parameters will show up\n");
			ImGui::Text("in the command window.\n");
			ImGui::SliderFloat("open_smile", &alpha[0], 0.0f, 1.0f);
			ImGui::SliderFloat("blink_upper_lid", &alpha[1], 0.0f, 1.0f);
			ImGui::SliderFloat("closed_smile", &alpha[2], 0.0f, 1.0f);
			ImGui::SliderFloat("lower_lip_drop", &alpha[3], 0.0f, 1.0f);
			ImGui::SliderFloat("surprised", &alpha[4], 0.0f, 1.0f);
			ImGui::SliderFloat("wide_mouth_open", &alpha[5], 0.0f, 1.0f);
			ImGui::SliderFloat("frown", &alpha[6], 0.0f, 1.0f);

			ImGui::InputText("frame", frame, sizeof(frame));
			//printf("frame: %f\n", (float)atof(frame));

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
			glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

			ImGui::Render();
			ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
		}
		else {
			for (int i = 0; i < face_num; i++) {
				if (alpha[i] < perfect[i]) {
					alpha[i] += (perfect[i] - alpha[i]) / (float)atof(frame);
				}
			}
		}

		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplSdlGL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	free(face);
	free(base_head);
	free(faces);
	free(alpha);
	free(perfect);
	free(point);
	free(edge);
    return 0;
}
