
#include <SDL2/SDL.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#ifdef __RPI__
#include <EGL/eglext_brcm.h>
#endif

#include <GLES2/gl2platform.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#include <stdio.h>
#include <assert.h>

#include "shared.h"


static void* eglImage = 0;

static GLuint g_FrameBuffer = 0;
static GLuint g_VertexBuffer = 0;
static GLuint programObject  = 0;
static GLuint positionLoc    = 0;
static GLuint texCoordLoc    = 0;
static GLuint samplerLoc     = 0;

static int g_buf_width = -1;
static int g_buf_height = -1;

static void *fbo;

static int resizeFilter = GL_NEAREST;

static SDL_Window *sdl_window;

static int error;

static float vertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};

#ifdef __RPI__
EGLNativeWindowType create_rpi_native_window(int *width, int *height)
{
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    uint32_t display_width;
    uint32_t display_height;
    
    uint32_t success;
    success = graphics_get_display_size(0 /* LCD */, &display_width, &display_height);
    if (success < 0)
    {
         return 0;
    }

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = display_width;
    dst_rect.height = display_height;
      
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = display_width << 16;
    src_rect.height = display_height << 16;

    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );
         
    dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
					      0 /*layer*/, &dst_rect, 0 /*src*/,
					      &src_rect, DISPMANX_PROTECTION_NONE,
					      0 /*alpha*/, 0 /*clamp*/, 0 /*transform*/);
      
    dispman_window.element = dispman_element;
    dispman_window.width = display_width;
    dispman_window.height = display_height;
    vc_dispmanx_update_submit_sync(dispman_update);

    *width = display_width;
    *height = display_height;

    return &dispman_window;
}
#else
EGLNativeWindowType create_native_window(int *width, int *height)
{

     Display *display;
     Window  frame_window;
     XSetWindowAttributes attributes;
     Visual *visual;
     int screen;
     int depth;

     display = XOpenDisplay(NULL);
     screen = DefaultScreen(display);
     *width  = DisplayWidth(display, screen);
     *height = DisplayHeight(display, screen);
     visual = DefaultVisual(display,screen);
     depth  = DefaultDepth(display,screen);
     attributes.background_pixel = XWhitePixel(display,screen);
 
     frame_window = XCreateWindow( display,XRootWindow(display,screen),
                            *width/4, *height/4, *width/2, *height/2, 5, depth,  InputOutput,
                            visual ,CWBackPixel, &attributes);

 
    XMapWindow(display, frame_window);
	 
    return (EGLNativeWindowType)frame_window;
}
#endif

Context create_context(EGLDisplay display, EGLint attribList[], EGLint contextAttribs[])
{
    Context ctx;

    EGLint majorVersion;
    EGLint minorVersion;
   
    // Get Display
    if (display == EGL_NO_DISPLAY)
        ctx.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(ctx.display != EGL_NO_DISPLAY);
     
    // Initialize EGL
    assert(eglInitialize(ctx.display, &majorVersion, &minorVersion));

    // Get configs
    assert(eglGetConfigs(ctx.display, NULL, 0, &ctx.numConfigs));

    // Choose config
    assert(eglChooseConfig(ctx.display, attribList, &ctx.config, 1, &ctx.numConfigs));

    // Create a GL context
    ctx.context = eglCreateContext(ctx.display, ctx.config, EGL_NO_CONTEXT, contextAttribs);
    assert(ctx.context != EGL_NO_CONTEXT);

    return ctx;
}


static void Draw(void) {

    int buf_width, buf_height;
    int screen_width, screen_height;
    int error;

    if( sdl_window == NULL ){
      return;
    }

    glUseProgram(programObject);

//    glClearColor( 0.0f,0.0f,0.0f,1.0f);
//    glClear(GL_COLOR_BUFFER_BIT);

    if( g_FrameBuffer == 0 )
    {
       glGenTextures(1,&g_FrameBuffer);
       glActiveTexture ( GL_TEXTURE0 );
       glBindTexture(GL_TEXTURE_2D, g_FrameBuffer);
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
       glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, resizeFilter );
       glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, resizeFilter );
       error = glGetError();
       if( error != GL_NO_ERROR )
       {
          fprintf(stderr,"g_FrameBuffer gl error %04X", error );
          return;
       }
    }else{
       glBindTexture(GL_TEXTURE_2D, g_FrameBuffer);
    }

    SDL_GetWindowSize(sdl_window,&buf_width, &buf_height);
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,buf_width,buf_height,GL_RGBA,GL_UNSIGNED_BYTE,fbo);


    if( g_VertexBuffer == 0 )
    {
       glGenBuffers(1, &g_VertexBuffer);
       glBindBuffer(GL_ARRAY_BUFFER, g_VertexBuffer);
       glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),vertices,GL_STATIC_DRAW);
       error = glGetError();
       if( error != GL_NO_ERROR )
       {
          fprintf(stderr,"g_VertexBuffer gl error %04X", error );
          return;
       }
    }else{
       glBindBuffer(GL_ARRAY_BUFFER, g_VertexBuffer);
    }

   if( buf_width != g_buf_width ||  buf_height != g_buf_height )
   {
      vertices[6]=vertices[10]=(float)buf_width/1024.f;
      vertices[11]=vertices[15]=(float)buf_height/1024.f;
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),vertices,GL_STATIC_DRAW);
      glVertexAttribPointer ( positionLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), 0 );
      glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), (void*)(sizeof(float)*2) );
      glEnableVertexAttribArray ( positionLoc );
      glEnableVertexAttribArray ( texCoordLoc );
      g_buf_width  = buf_width;
      g_buf_height = buf_height;
      error = glGetError();
      if( error != GL_NO_ERROR )
      {
         fprintf(stderr, "gl error %d", error );
         return;
      }
   }else{
      glBindBuffer(GL_ARRAY_BUFFER, g_VertexBuffer);
   }

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

   SDL_GL_SwapWindow(sdl_window);
}

static void SDLInit(void) {
	SDL_InitSubSystem(SDL_INIT_EVENTS);
}

///
// Create a shader object, load the shader source, and
// compile the shader.
//
static GLuint LoadShader ( GLenum type, const char *shaderSrc )
{
   GLuint shader;
   GLint compiled;
   
   // Create the shader object
   shader = glCreateShader ( type );

   if ( shader == 0 )
   	return 0;

   // Load the shader source
   glShaderSource ( shader, 1, &shaderSrc, NULL );
   
   // Compile the shader
   glCompileShader ( shader );

   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if ( !compiled ) 
   {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );
      
      if ( infoLen > 1 )
      {
         char* infoLog = malloc (sizeof(char) * infoLen );

         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
         printf ( "Error compiling shader:\n%s\n", infoLog );            
         
         free ( infoLog );
      }

      glDeleteShader ( shader );
      return 0;
   }

   return shader;

}

static int SetupOpenGL()
{
   GLbyte vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

   GLbyte fShaderStr[] =
      "precision mediump float;     \n"
      "varying vec2 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"    
      "}                                                   \n";

   GLuint vertexShader;
   GLuint fragmentShader;
   GLint linked;

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
    };

    EGLint attribList[] =
    {
        EGL_RED_SIZE,                 8,
	EGL_GREEN_SIZE,               8,
	EGL_BLUE_SIZE,                8,
	EGL_ALPHA_SIZE,               8,
	EGL_DEPTH_SIZE,   EGL_DONT_CARE,
	EGL_STENCIL_SIZE, EGL_DONT_CARE,
	EGL_SAMPLE_BUFFERS,           0,
	EGL_NONE,              EGL_NONE
    };

    // Creating EGL context
    Context ctx = create_context(EGL_NO_DISPLAY, attribList, contextAttribs);
    int width;
    int height;

    // Creating Native Window Surface
#ifdef __RPI__
    EGLNativeWindowType window = create_rpi_native_window(&width, &height);
#else
    EGLNativeWindowType window = create_native_window(&width, &height);
#endif
    assert(window);
    ctx.surface = eglCreateWindowSurface(ctx.display, ctx.config, window, NULL);
    assert(ctx.surface != EGL_NO_SURFACE);
    assert(eglMakeCurrent(ctx.display, ctx.surface, ctx.surface, ctx.context));
#if 0
SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetSwapInterval(1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	window = SDL_CreateWindow("OpenGL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
	if (!window) {
    		fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
    		return;
	}

	context = SDL_GL_CreateContext(window);
	if (!context) {
    		fprintf(stderr, "Couldn't create context: %s\n", SDL_GetError());
    		return;
	}
#endif


   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vShaderStr );
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

   // Create the program object
   programObject = glCreateProgram ( );

   if ( programObject == 0 ){
      fprintf (stderr,"Can not create a program\n");
      return 0;
   }

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

   // Link the program
   glLinkProgram ( programObject );

   // Check the link status
   glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

   if ( !linked )
   {
      GLint infoLen = 0;

      glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
        char* infoLog = malloc (sizeof(char) * infoLen );
        glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
        fprintf (stderr, "Error linking program:\n%s\n", infoLog );
        free ( infoLog );
         return GL_FALSE;
      }

      glDeleteProgram ( programObject );
      return GL_FALSE;
   }


   // Get the attribute locations
   positionLoc = glGetAttribLocation ( programObject, "a_position" );
   texCoordLoc = glGetAttribLocation ( programObject, "a_texCoord" );

   // Get the sampler location
   samplerLoc = glGetUniformLocation ( programObject, "s_texture" );

   glUseProgram(programObject);
#if 0
   /* Create EGL Image */
   eglImage = eglCreateImageKHR(
                esContext->eglDisplay,
                esContext->eglContext,
                EGL_GL_TEXTURE_2D_KHR,
                textureId, // (EGLClientBuffer)esContext->texture,
                0);
    
   if (eglImage == EGL_NO_IMAGE_KHR)
   {
      printf("eglCreateImageKHR failed.\n");
      exit(1);
   }
#endif

   return GL_TRUE;
}

int main(int argc, char *argv[]) {
	int i;
	SDL_Event event;
	int quit = SDL_FALSE;

	//handle command line arguments
	for (i = 1; i < argc; ++i) {
		if (argv[i]) {
			if (strcmp(argv[i], "-rb") == 0 || strcmp(argv[i], "--resizebilinear") == 0) {
			resizeFilter = GL_LINEAR;
			}
		}
	}

        fbo = calloc (800 * 600, 4);
	for (i = 0; i<800*600; i++) {
		((int*)fbo)[i] = 0xFF0000FF;
	}

#ifdef __RPI__
	bcm_host_init()
#endif

        SDLInit();
	if( SetupOpenGL() != GL_TRUE ){
		fprintf(stderr, "Fail to SetupOpenGL\n");
                return -1;
	}
	while (quit == SDL_FALSE)
	{
		if (SDL_PollEvent(&event))
		{
   			switch(event.type)
    			{
     				case SDL_QUIT:
      					quit = SDL_TRUE;
      				break;
				case SDL_KEYDOWN:
					printf("Key down!");
				break;
     			}
  		}
		Draw();
	}
	SDL_Quit();

	return 0;
}
