#include "stdafx.h"
#include "blendcalc.h"
#include "gpqp_solv.hpp"
#include <future>
#include <map>

int screen_width = 1000;
int screen_height = 600;
bool fullscreen = false;
int iter_num = 100;

static bool s_mouse_down = false;
static float s_mouse_xpos = -1.f;
static float s_mouse_ypos = -1.f;

static int s_selected_vert = -1;

static void mouseClicked(float mx, float my, PointData p_data[]);
static void mouseDragged(float mx, float my, PointData p_data[]);

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <io.h>
#include <fcntl.h>
#include <Windows.h>

static void create_console();
#endif

// edge stores the bound of each facial feature
// the basic points for canonical space, 1450 is used as origin and combine 3542 to make a line
static const int edgeidx[] = { 
	797, 
	133, 
	3542, 
	1450, 
	9807, 
	9928, 
	18028, 
	18150, 
	14813, 
	6565, 
	12, 
	14116, 
	5853, 
	15909, 
	7669, 
	14928, 
	6675, 
	22524, 
	5643 
};

static std::map<std::string, uint> s_features_map = {
	{"Oral commisure (L)", 0},
	{"Oral commisure (R)", 1},
	{"Lateral canthus (L)", 2},
	{"Lateral canthus (R)", 3},
	{"Palpebral fissure (RU)", 4},
	{"Palpebral fissure (RL)", 5},
	{"Palpebral fissure (LU)", 6},
	{"Palpebral fissure (LL)", 7},
	{"Depressor (L)", 8},
	{"Depressor (R)", 9},
	{"Depressor (M)", 10},
	{"Nasal ala (L)", 11},
	{"Nasal ala (R)", 12},
	{"Medial brow (L)", 13},
	{"Medial brow (R)", 14},
	{"Malar eminence (L)", 15},
	{"Malar eminence (R)", 16},
	{"Dental show (Top)", 17},
	{"Dental show (Bottom)", 18}
};

static const unsigned edge_numVerts = sizeof( edgeidx )/ sizeof( int );

static PointData s_modified_points[ edge_numVerts ];

std::future<Eigen::VectorXf> s_async_blend_calc;

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

#ifdef _WIN32
	create_console();
#endif

	//Load face model
	GLuint tex0 = AllocateTexture(TEX_DIR "/head_BaseColor.bmp", 0);
	int face_num = 7;
	int model_numVerts = 0;

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
	glm::vec3* iris = (glm::vec3*)malloc(sizeof(glm::vec3) * iris_numVerts);
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

	GLint uniColorFlag = glGetUniformLocation(PointShaderProgram, "color_flag");

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

	unsigned color_flag = 0;

	//parameters for face interpolation
	float* alpha = (float *)calloc(face_num, sizeof(float));

	float* face = (float *)malloc(sizeof(float)*model_numVerts * 8);
	for (int i = 0; i < model_numVerts * 8; i++) {
		face[i] = base_head[i];
	}

	glm::mat4 Xbox_camera = glm::perspective(3.14f / 3, tan(35 * 3.14f / 180)*sqrtf(3.0f), 50.0f, 450.0f); //FOV, aspect, near, far
	Xbox_camera[1][1] = -1;
	float face_length = 6.520 + 8.882; // the length of the face in original model space
	float real_face_length = 24; // in centimeter
	float face_ratio = real_face_length / face_length;
	glm::vec2 *bh_fpoint = (glm::vec2 *)malloc(sizeof(glm::vec2)*edge_numVerts);

	//convert the base_head to perspective plane
	for (int i = 0; i < edge_numVerts; i++) {
		glm::vec4 temp = Xbox_camera * glm::vec4(edge[face_num][i] * face_ratio, 1.0);
		bh_fpoint[i] = glm::vec2(temp.x, temp.y);
	}

	//transfer to canonical space
	trans = bh_fpoint[s_features_map["Lateral canthus (R)"]];
	scale = glm::distance(bh_fpoint[s_features_map["Lateral canthus (L)"]],
		bh_fpoint[s_features_map["Lateral canthus (R)"]]);
	for (int i = 0; i < edge_numVerts; i++) {
		bh_fpoint[i] -= trans;
		bh_fpoint[i] = bh_fpoint[i] / scale * unit;
	}

	//SET UP THE LS SYSTEM
	blendcalc blendsys;
	//Transfer the feature points into a perspective plane
	blendsys.blend_a = Eigen::MatrixXf(edge_numVerts * 2, face_num);
	for (int j = 0; j < face_num; j++) {
		for (int i = 0, k = 0; i < edge_numVerts * 2; i += 2, k++) {		
			glm::vec4 temp = Xbox_camera * glm::vec4(edge[j][k] * face_ratio, 1.0);
			blendsys.blend_a(i, j) = temp.x;
			blendsys.blend_a(i + 1, j) = temp.y;
		}
	}
	
	//Transfer to the canonical space
	for (int j = 0; j < face_num; j++) {
		trans = glm::vec2(
			blendsys.blend_a(s_features_map["Lateral canthus (R)"] * 2, j), 
			blendsys.blend_a(s_features_map["Lateral canthus (R)"] * 2 + 1, j));
		scale = glm::distance(trans, glm::vec2(
			blendsys.blend_a(s_features_map["Lateral canthus (L)"] * 2, j), 
			blendsys.blend_a(s_features_map["Lateral canthus (L)"] * 2 + 1, j)));
		for (int i = 0, k = 0; i < edge_numVerts * 2; i += 2, k++) {
			//translate
			blendsys.blend_a(i, j) -= trans.x;
			blendsys.blend_a(i + 1, j) -= trans.y;

			//scale
			blendsys.blend_a(i, j) = blendsys.blend_a(i, j) / scale * unit;
			blendsys.blend_a(i + 1, j) = blendsys.blend_a(i + 1, j) / scale * unit;

			//blendshape
			blendsys.blend_a(i, j) -= bh_fpoint[k].x;
			blendsys.blend_a(i + 1, j) -= bh_fpoint[k].y;
		}
	}

	//read the sample data
	//point: x, y, z
	float* point = (float *)calloc(edge_numVerts * 4, sizeof(float));
	Eigen::MatrixXf feature_csv;
	//read the feature points from .csv files
	GetCSVFile(feature_csv, CAN_DIR "/28162_s_canon.csv", edge_numVerts);
	blendsys.blend_b = feature_csv.block(1, 0, feature_csv.rows() - 1, 1);

	// initialize s_modified_points array for face
	for( const auto& key_val : s_features_map ) {
		s_modified_points[key_val.second].id = key_val.first.c_str();
		s_modified_points[key_val.second].orginal_pos.x = 
			blendsys.blend_b(2 * key_val.second);
		s_modified_points[key_val.second].orginal_pos.y = 
			blendsys.blend_b(2 * key_val.second + 1);

		s_modified_points[key_val.second].offset.x = 0.f;
		s_modified_points[key_val.second].offset.y = 0.f;
	}

	// get neural net data: weights && biases
	Eigen::MatrixXf nn_weights[] = { 
		extract_matrix( ROOT_DIR "/nn_data/w0.csv" ),
		extract_matrix( ROOT_DIR "/nn_data/w1.csv" ),
		extract_matrix( ROOT_DIR "/nn_data/w2.csv" ) 
	};

	Eigen::VectorXf nn_biases[] = {
		extract_vector( ROOT_DIR "/nn_data/b0.csv" ),
		extract_vector( ROOT_DIR "/nn_data/b1.csv" ),
		extract_vector( ROOT_DIR "/nn_data/b2.csv" )
	};

	bool show_window = false;
	bool check_progress = false;
	bool trigger_recalc = true;
	bool new_frame = true;
	bool set_original_pos = true;
	
	Eigen::VectorXf Calc_X;
	int frame_index = 0;
	const uint max_frames = feature_csv.cols();

	float scr_to_canon_w = scale;
	float scr_to_canon_h = scr_to_canon_w * 
		( (float)screen_height / (float)screen_width );
	
	scr_to_canon_w = 1.f / scr_to_canon_w;
	scr_to_canon_h = 1.f / scr_to_canon_h;

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
				trigger_recalc = new_frame = true;
			}

			// calculate
			if ( (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym ==
				 SDLK_c) || trigger_recalc ) 
			{
				if( !check_progress )
				{
					Eigen::VectorXf feature_pnt = feature_csv.block(
						1, frame_index, feature_csv.rows() - 1, 1);

					if( new_frame ) {
						for( int i = 0; i < edge_numVerts; i++ )
							s_modified_points[i].rendered_offset.x = 
								s_modified_points[i].rendered_offset.y = 
								s_modified_points[i].offset.x = 
								s_modified_points[i].offset.y = 0;
						
						new_frame = false;
						set_original_pos = true;
					}

					for( int i = 0; i < edge_numVerts; i++ )
					{
						feature_pnt(i * 2) -= bh_fpoint[i].x + 
							s_modified_points[i].offset.x * scr_to_canon_w;
						feature_pnt(i * 2 + 1) -= bh_fpoint[i].y -
							s_modified_points[i].offset.y * scr_to_canon_h;
						
						s_modified_points[i].rendered_offset.x = 
							s_modified_points[i].rendered_offset.y = 0.f; 
					}
					// Spawn calculation on separate thread
					s_async_blend_calc = std::async( std::launch::async, calc_blend_values, blendsys, feature_pnt, face_num, iter_num );
					
					check_progress = true;
					trigger_recalc = false;
				}
			}

			ImGui_ImplSdlGL3_ProcessEvent(&windowEvent);
		}

		if( check_progress )
		{
			if (s_async_blend_calc.wait_for(std::chrono::seconds( 0 )) == 
				std::future_status::ready) 
			{
				Calc_X = s_async_blend_calc.get();
				for (int i = 0; i < face_num; i++)
					alpha[i] = Calc_X(i);

				check_progress = false;
			}
		}

		int mx, my;
		if (SDL_GetMouseState(&mx, &my) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			if (s_mouse_down == false){
				mouseClicked( (float)mx, (float)(screen_height - my), s_modified_points);
			} 
			else{
				mouseDragged((float)mx, (float)(screen_height - my), s_modified_points);
			}
			s_mouse_down = true;
		} 
		else{
			s_mouse_down = false;
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int i = 0; i < model_numVerts * 8; i++) {
			face[i] = base_head[i];
			for (int j = 0; j < face_num; j++) {
				face[i] += alpha[j] * (faces[j][i] - base_head[i]);
			}
		}

		color_flag = 0;

		// face_num = 7
		for (int i = 0, k = 0; i < edge_numVerts; i++, k += 4) {
			point[k] = edge[face_num][i].x;
			point[k + 1] = edge[face_num][i].y;
			point[k + 2] = edge[face_num][i].z;

			for (int j = 0; j < face_num; j++) {
				point[k] += alpha[j] * (edge[j][i].x - edge[face_num][i].x);
				point[k + 1] += alpha[j] * (edge[j][i].y - edge[face_num][i].y);
				point[k + 2] += alpha[j] * (edge[j][i].z - edge[face_num][i].z);
			}

			if( i == s_selected_vert ) {
				color_flag |= ( 1 << s_selected_vert );
			}
		}

		// transform vertices to screen space
		const glm::mat4 MVP = proj * view;
		const float half_width = ( screen_width / 2.f );
		const float half_height = ( screen_height / 2.f );
		glm::vec4 NDC_space, clip_space;
		
		for( int i = 0; i < edge_numVerts; i++ ) {
			const unsigned idx = i * 4;

			clip_space = MVP * glm::vec4(
				point[idx], point[idx + 1], point[idx + 2], 1.0 );
			NDC_space = clip_space / clip_space.w;

			s_modified_points[i].rendered_pos.x = 
					( NDC_space.x + 1.f ) * half_width;
			s_modified_points[i].rendered_pos.y = 
				( NDC_space.y + 1.f ) * half_height;
			
			if( set_original_pos ) {
				s_modified_points[i].orginal_pos = 
					s_modified_points[i].rendered_pos;
			}

			// transform offset in clip space
			float ndc_offset_w = 
				(( s_modified_points[i].rendered_offset.x / 
					(float)screen_width ) * 2.f);
			float ndc_offset_h = 
				(( s_modified_points[i].rendered_offset.y / 
					(float)screen_height ) * 2.f);

			// modify rendered position
			point[idx] = clip_space.x + (ndc_offset_w * clip_space.w);
			point[idx + 1] = clip_space.y + ( ndc_offset_h * clip_space.w );
			point[idx + 2] = clip_space.z;
			point[idx + 3] = clip_space.w;
		}
		set_original_pos = false; // clear flag after all positions are updated

		if( s_mouse_down ) {
			s_selected_vert = select_closest_point( 
				s_modified_points, 
				(int)edge_numVerts, 
				10.f,
				s_mouse_xpos, 
				s_mouse_ypos );
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
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*edge_numVerts * 4, point, GL_DYNAMIC_DRAW);

		//draw the edge points
		glUseProgram(PointShaderProgram);

		// set color_flag
		glUniform1ui( uniColorFlag, color_flag );

		GLuint vao_point = LoadToVAO_Point(PointShaderProgram);
		glBindVertexArray(vao_point);

		inColor = glm::vec3(0.0, 1.0, 0.0);
		glUniform3f(uniColor, inColor.r, inColor.g, inColor.b);

		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_POINTS, 0, edge_numVerts);
		glBindVertexArray(0);
		glEnable(GL_DEPTH_TEST);

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
		
		//draw the slidebar
		ImGui_ImplSdlGL3_NewFrame(window);
		ImGui::Begin("Face Parameters", &show_window);

		ImGui::Text("The parameters to adjust the facial features.\n");
		ImGui::Text("Press key 's' to start auto-face.\n");
		ImGui::Text("Press key 'r' to clear modifications.\n");
		ImGui::Text("Press key 'c' to calculate new expression.\n");
		ImGui::Text("After reset, the last parameters will show up\n");
		ImGui::Text("in the command window.\n");
		ImGui::SliderFloat("open_smile", &alpha[0], 0.0f, 1.0f);
		ImGui::SliderFloat("blink_upper_lid", &alpha[1], 0.0f, 1.0f);
		ImGui::SliderFloat("closed_smile", &alpha[2], 0.0f, 1.0f);
		ImGui::SliderFloat("lower_lip_drop", &alpha[3], 0.0f, 1.0f);
		ImGui::SliderFloat("surprised", &alpha[4], 0.0f, 1.0f);
		ImGui::SliderFloat("wide_mouth_open", &alpha[5], 0.0f, 1.0f);
		ImGui::SliderFloat("frown", &alpha[6], 0.0f, 1.0f);

		ImGui::Separator();
		if( ImGui::SliderInt( "CSV frame", &frame_index, 0, max_frames - 1 ) ) {
			trigger_recalc = true;
			new_frame = true;
		}

		if( s_selected_vert >= 0 ) {
			ImGui::Separator();

			PointData& selected_point = s_modified_points[s_selected_vert];

			ImGui::Text( "Selected face vertex: %s (index: %d)", 
				selected_point.id, s_selected_vert );
			
			glm::vec2 canon_pos( 
				selected_point.orginal_pos + selected_point.offset );
			ImGui::InputFloat2( "2D screen pos", &canon_pos[0] );
			selected_point.offset = canon_pos - selected_point.orginal_pos;

			ImGui::Separator();
		}

		ImGui::InputText("frame", frame, sizeof(frame));

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

		ImGui::Render();
		ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

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
	free(bh_fpoint);
	free(faces);
	free(alpha);
	free(point);
	free(edge);
	return 0;
}

static void mouseClicked(float m_x, float m_y, PointData p_data[]) {
	s_mouse_xpos = m_x;
	s_mouse_ypos = m_y;
}

static void mouseDragged(float m_x, float m_y, PointData p_data[]) {
	s_mouse_xpos = m_x;
	s_mouse_ypos = m_y;

	if( s_selected_vert >= 0 ){
		p_data[s_selected_vert].offset = 
			glm::vec2( m_x, m_y ) - p_data[s_selected_vert].orginal_pos;

		p_data[s_selected_vert].rendered_offset = 
			glm::vec2( m_x, m_y ) - p_data[s_selected_vert].rendered_pos;
	}
}

#ifdef _WIN32

static void create_console() {
	FreeConsole();

	DWORD err_code = -1;
	const bool success = AllocConsole();

	if( !success ) {
		err_code = GetLastError();
	} else {
		HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
		int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
		FILE* hf_out = _fdopen(hCrt, "w");
		setvbuf(hf_out, NULL, _IONBF, 1);
		*stdout = *hf_out;

		HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
		hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
		FILE* hf_in = _fdopen(hCrt, "r");
		setvbuf(hf_in, NULL, _IONBF, 128);
		*stdin = *hf_in;
	}
}

#endif
