#include "gles20programs.h"
//#include "program/pattern.h"


static GLint patternObject  = -1;
static GLint positionLoc    = -1;
static GLint texCoordLoc    = -1;
static GLint samplerLoc     = -1;
static GLint alphaValueLoc  = -1;

static GLint priorityProgram = -1;
static GLint prioPositionLoc = -1;
static GLint prioTexCoordLoc = -1;
static GLint prioSamplerLoc  = -1;
static GLint prioValueLoc = -1;


GLuint vertexSWBuffer = -1;

void createPriorityProgram() {
   GLbyte vShaderPriorityStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec3 a_texCoord;   \n"
      "varying vec3 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

   GLbyte fShaderPriorityStr[] =
      "uniform float u_priority;     \n"
      "varying vec3 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  vec4 color = texture2D( s_texture, v_texCoord.xy/v_texCoord.z);\n"  
      "  if (color.a < 0.1) discard;\n" 
      "  gl_FragColor.a = u_priority;\n"
      "}                                                   \n";
   if (priorityProgram > 0) return;
   priorityProgram = gles20_createProgram (vShaderPriorityStr, fShaderPriorityStr);

   if ( priorityProgram == 0 ){
      fprintf (stderr,"Can not create a program\n");
      return;
   }

   // Get the attribute locations
   prioPositionLoc = glGetAttribLocation ( priorityProgram, "a_position" );
   prioTexCoordLoc = glGetAttribLocation ( priorityProgram, "a_texCoord" );
   // Get the sampler location
   prioSamplerLoc = glGetUniformLocation ( priorityProgram, "s_texture" );
   prioValueLoc = glGetUniformLocation ( priorityProgram, "u_priority" );

   if (vertexSWBuffer == -1) 
       glGenBuffers(1, &vertexSWBuffer);
}

void createPatternProgram() {
   GLbyte vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec3 a_texCoord;   \n"
      "varying vec3 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

   GLbyte fShaderStr[] =
      "varying vec3 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  vec4 color = texture2D( s_texture, v_texCoord.xy/v_texCoord.z);\n"  
      "  if (color.a < 0.1) discard;\n" 
      "  gl_FragColor = color;\n"
      "}                                                   \n";
	if (patternObject > 0) return;
   // Create the program object
   patternObject = gles20_createProgram (vShaderStr, fShaderStr);

   if ( patternObject == 0 ){
      fprintf (stderr,"Can not create a program\n");
      return;
   }

   // Get the attribute locations
   positionLoc = glGetAttribLocation ( patternObject, "a_position" );
   texCoordLoc = glGetAttribLocation ( patternObject, "a_texCoord" );
   // Get the sampler location
   samplerLoc = glGetUniformLocation ( patternObject, "s_texture" );

   if (vertexSWBuffer == -1) 
       glGenBuffers(1, &vertexSWBuffer);
}

void drawPattern(Pattern* pattern, GLfloat* vertex, int nbVertex){
	glUseProgram(patternObject);
	glUniform1i(samplerLoc, 0);
        if (pattern->mesh != 0)
		glEnable(GL_STENCIL_TEST);

    	glBindBuffer(GL_ARRAY_BUFFER, vertexSWBuffer);

    	glBufferData(GL_ARRAY_BUFFER, 5*nbVertex*sizeof(GLfloat),vertex,GL_STATIC_DRAW);

    	if (positionLoc >= 0) glVertexAttribPointer ( positionLoc, 2, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), 0 );
    	if (texCoordLoc >= 0) glVertexAttribPointer ( texCoordLoc, 3, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), (void*)(sizeof(GLfloat)*2) );

    	if (positionLoc >= 0) glEnableVertexAttribArray ( positionLoc );
    	if (texCoordLoc >= 0) glEnableVertexAttribArray ( texCoordLoc );

    	glActiveTexture ( GL_TEXTURE0 );
    	glBindTexture(GL_TEXTURE_2D, pattern->tex);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	if (pattern->mesh != 0)
		glDisable(GL_STENCIL_TEST);
}


void drawPriority(Pattern* pattern, GLfloat* vertex, int priority, int nbVertex) {

    	glUseProgram(priorityProgram);
	glUniform1i(prioSamplerLoc, 0);

    	glBindBuffer(GL_ARRAY_BUFFER, vertexSWBuffer);

    	glBufferData(GL_ARRAY_BUFFER, 5*nbVertex*sizeof(GLfloat),vertex,GL_STATIC_DRAW);

    	if (prioPositionLoc >= 0) glVertexAttribPointer ( prioPositionLoc, 2, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), 0 );
    	if (prioTexCoordLoc >= 0) glVertexAttribPointer ( prioTexCoordLoc, 3, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), (void*)(sizeof(GLfloat)*2) );

    	if (prioPositionLoc >= 0) glEnableVertexAttribArray ( prioPositionLoc );
    	if (prioTexCoordLoc >= 0) glEnableVertexAttribArray ( prioTexCoordLoc );

    	glActiveTexture ( GL_TEXTURE0 );
    	glBindTexture(GL_TEXTURE_2D, pattern->tex);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    	glUniform1f(prioValueLoc, priority/255.0f);

    	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
