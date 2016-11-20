
#include "gles20utils.h"

// create a texture object
static GLuint textureId = 0;
static GLuint g_VertexBuffer = 0;
static GLuint programObject  = 0;
static GLuint positionLoc    = 0;
static GLuint texCoordLoc    = 0;
static GLuint samplerLoc     = 0;

static int g_buf_height = -1;
static int g_buf_width = -1;

static int error;

static float vertices [] = {
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
	case 2:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, NULL);
	break;
	default:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	break;
   }

  // glGenRenderbuffers(1, &fbo->stencil);
  // glBindRenderbuffer(GL_RENDERBUFFER, fbo->stencil);
  // glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, w, h);

   //-------------------------
   glGenFramebuffers(1, &fbo->fb);

   glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
   //Attach 2D texture to this FBO
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->tex, 0);
   //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo->stencil);

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

