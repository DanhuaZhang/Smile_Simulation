#pragma once

#include "Config.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"

#include "glad/glad.h"  //Include order can matter here
#ifndef _WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
//#include <SDL_image.h>

#define GLM_FORCE_RADIANS
#define MAX_CHARACTER 1024

#include <stdio.h>
#include <stdlib.h>

#if PLATFORM_FLAG == PF_WIN32
#include <tchar.h>
#endif // PLATFORM_FLAG == PF_LINUX

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/rotate_vector.hpp"

struct GT_Variable {
	int size;
	float* time;
	glm::vec2* Lateral_canthus_L;
	glm::vec2* Lateral_canthus_R;

	glm::vec2* Palpebral_fissure_RU;
	glm::vec2* Palpebral_fissure_RL;
	glm::vec2* Palpebral_fissure_LU;
	glm::vec2* Palpebral_fissure_LL;

	glm::vec2* Depressor_L;
	glm::vec2* Depressor_R;
	glm::vec2* Depressor_M;

	glm::vec2* Nasal_ala_L;
	glm::vec2* Nasal_ala_R;

	glm::vec2* Medial_brow_L;
	glm::vec2* Medial_brow_R;

	glm::vec2* Malar_eminence_L;
	glm::vec2* Malar_eminence_R;
};

void GTAllocate(GT_Variable &gt_var, int size);
float* ParseObj(const char* filename, int &model_numVerts, const int edgeidx[], glm::vec3** edge, const int face_id, const int edge_numVerts);
float* ParseObj(const char* filename, int &model_numVerts, const int edgeidx[], glm::vec3* edge, const int edge_numVerts);
float* ParseObj(const char* filename, int &model_numVerts);
GT_Variable ParseCSVwithSpace(const char* filename);
GLuint LoadToVBO(float* model, int model_numVerts);
GLuint LoadToVAO(GLuint shaderprogram);
static char* readShaderSource(const char* shaderFile);
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
GLuint AllocateTexture(const char* tex_filename, int i);
