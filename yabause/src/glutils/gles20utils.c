
#include "gles20utils.h"

static int error;

static float meshVertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};

GLuint LoadShader ( GLenum type, const char *shaderSrc )
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


int gles20_createProgram(GLbyte* vShader, GLbyte* fShader) {
   GLuint vertexShader;
   GLuint fragmentShader;
   GLint linked;
   GLuint program;

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vShader );
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShader );

   // Create the program object
   program = glCreateProgram ( );

   if ( program == 0 )
      return 0;

   glAttachShader ( program, vertexShader );
   glAttachShader ( program, fragmentShader );

   // Link the program
   glLinkProgram ( program );

   // Check the link status
   glGetProgramiv ( program, GL_LINK_STATUS, &linked );

   if ( !linked )
   {
      GLint infoLen = 0;

      glGetProgramiv ( program, GL_INFO_LOG_LENGTH, &infoLen );

      if ( infoLen > 1 )
      {
        char* infoLog = malloc (sizeof(char) * infoLen );
        glGetProgramInfoLog ( program, infoLen, NULL, infoLog );
        fprintf (stderr, "Error linking program:\n%s\n", infoLog );
        free ( infoLog );
         return GL_FALSE;
      }

      glDeleteProgram ( program );
      return GL_FALSE;
   }
   return program;

}

int gles20_createFBO(gl_fbo* fbo, int w, int h, int format)
{
   GLenum status;

   fbo->width = w;
   fbo->height = h;

   glGenTextures(1, &fbo->tex);
   glBindTexture(GL_TEXTURE_2D, fbo->tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   switch (format) {
	case 0:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	break;
	case 1:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
	break;
	default:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	break;
   }

   glGenRenderbuffers(1, &fbo->stencil);
   glBindRenderbuffer(GL_RENDERBUFFER, fbo->stencil);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, w, h);

   //-------------------------
   glGenFramebuffers(1, &fbo->fb);

   glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
   //Attach 2D texture to this FBO
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->tex, 0);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo->stencil);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   //Does the GPU support current FBO configuration?
   status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   switch(status)
   {
      case GL_FRAMEBUFFER_COMPLETE:
      	return 1;
      default:
        return 0;
   }

   return 0;
}

static GLint meshPatternObject  = -1;
static GLint meshPositionLoc    = -1;
static GLint meshTexCoordLoc    = -1;
static GLint meshSamplerLoc     = -1;
static GLint meshTex 		= -1;
static GLint meshVertexSWBuffer = -1;

static void createMeshPatternTexture() {
   	if (meshTex > 0) return;

   	int pix[4] = {0xFFFFFFFF,0x00,0x00,0xFFFFFFFF};
   	glGenTextures(1,&meshTex);
   	glBindTexture(GL_TEXTURE_2D, meshTex);
   	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);
}

static void createMeshPatternProgram() {
	createMeshPatternTexture();
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
      		"varying vec2 v_texCoord;                            \n"
      		"uniform sampler2D s_texture;                        \n"
      		"void main()                                         \n"
      		"{                                                   \n"
      		"  vec4 color = texture2D( s_texture, v_texCoord.xy);\n"  
      		"  if (color.a < 0.1) discard;\n"
      		"  gl_FragColor = color;\n"
		"}                                                   \n";
      	if (meshPatternObject > 0) return;
 	// Create the program object
   	meshPatternObject = gles20_createProgram (vShaderStr, fShaderStr);

   	if ( meshPatternObject == 0 ){
      		fprintf (stderr,"Can not create a program\n");
      		return;
   	}

   	// Get the attribute locations
   	meshPositionLoc = glGetAttribLocation ( meshPatternObject, "a_position" );
   	meshTexCoordLoc = glGetAttribLocation ( meshPatternObject, "a_texCoord" );
   	// Get the sampler location
   	meshSamplerLoc = glGetUniformLocation ( meshPatternObject, "s_texture" );

   	if (meshVertexSWBuffer == -1) 
       		glGenBuffers(1, &meshVertexSWBuffer);
}

void gles20_setupMeshStencil(gl_fbo* fbo) {
	int error;
        glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
        glViewport(0,0,fbo->width, fbo->height);

	glClearColor(0.0, 0.0, 0.0, 0.0);
        glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	if (meshPatternObject <= 0) createMeshPatternProgram();

	meshVertices[6] = meshVertices[10] = (float)fbo->width/4.0f;
	meshVertices[11] = meshVertices[15] = (float)fbo->height/4.0f;

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glEnable(GL_STENCIL_TEST);

	glUseProgram(meshPatternObject);

    	glBindBuffer(GL_ARRAY_BUFFER, meshVertexSWBuffer);

    	glBufferData(GL_ARRAY_BUFFER, sizeof(meshVertices),meshVertices,GL_STATIC_DRAW);

    	if (meshPositionLoc >= 0) glVertexAttribPointer ( meshPositionLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), 0 );
    	if (meshPositionLoc >= 0) glVertexAttribPointer ( meshTexCoordLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), (void*)(sizeof(GLfloat)*2) );

    	if (meshPositionLoc >= 0) glEnableVertexAttribArray ( meshPositionLoc );
    	if (meshPositionLoc >= 0) glEnableVertexAttribArray ( meshTexCoordLoc );

	glUniform1i(meshSamplerLoc, 0);
    	glActiveTexture ( GL_TEXTURE0 );
    	glBindTexture(GL_TEXTURE_2D, meshTex);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glDisable(GL_STENCIL_TEST);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
}

