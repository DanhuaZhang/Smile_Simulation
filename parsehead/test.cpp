#include "stdafx.h"

int screen_width = 1000;
int screen_height = 600;
bool fullscreen = false;

int main(int argc, char *argv[]) {	
	//Initialize SDL
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_Window* window = SDL_CreateWindow("Open_Smile", 100, 50, screen_width, screen_height, SDL_WINDOW_OPENGL);
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

	GLuint elements[] = {
		0, 1, 2,
		2, 3, 0
	};
	float vertices[] = {
		-0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
		 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
		 0.5f,-0.5f, 0.0f, 0.0f, 1.0f,
		-0.5f,-0.5f, 1.0f, 1.0f, 1.0f
	};

	// upload vertex data to graphics card
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	GLuint shaderprogram = InitShader("vertex.glsl", "fragment.glsl");

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLint posAttrib = glGetAttribLocation(shaderprogram, "position");
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	glEnableVertexAttribArray(posAttrib);

	GLint colAttrib = glGetUniformLocation(shaderprogram, "color");
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(colAttrib);

	SDL_Event windowEvent;
	bool quit = false;
	while (!quit) {
		while (SDL_PollEvent(&windowEvent)) {
			if (windowEvent.type == SDL_QUIT) quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; ; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) //If "f" is pressed
				fullscreen = !fullscreen;
			SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Set to full screen
		}

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

    return 0;
}
