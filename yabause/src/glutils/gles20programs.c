#include "gles20programs.h"
//#include "program/pattern.h"


static GLint patternObject  = -1;
static GLint positionLoc    = -1;
static GLint texCoordLoc    = -1;
static GLint samplerLoc     = -1;
static GLint alphaValueLoc  = -1;

static GLint meshPatternObject  = -1;
static GLint meshPositionLoc    = -1;
static GLint meshTexCoordLoc    = -1;
static GLint meshSamplerLoc     = -1;

static GLint priorityProgram = -1;
static GLint prioPositionLoc = -1;
static GLint prioTexCoordLoc = -1;
static GLint prioSamplerLoc  = -1;
static GLint prioValueLoc = -1;

static GLint meshTex = -1;

static void setupMeshStencil(Pattern* pattern, GLfloat* vertex, int nbVertex);
static void createMeshPatternTexture();
static void createMeshPatternProgram();
static void disableMeshStencil();


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
	createMeshPatternProgram();
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

static void createMeshPatternTexture() {
   if (meshTex > 0) return;

   u8 pix[4] = {0xFF,0x00,0x00,0xFF};
   glGenTextures(1,&meshTex);
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture(GL_TEXTURE_2D, meshTex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 2, 2, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pix);
}



void drawPattern(Pattern* pattern, GLfloat* vertex, int nbVertex){
        if (pattern->mesh != 0) {
		setupMeshStencil(pattern, vertex, nbVertex);
	}

	glUseProgram(patternObject);
	glUniform1i(samplerLoc, 0);

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

	if (pattern->mesh != 0) {
		disableMeshStencil();
	}
}

static void createMeshPatternProgram() {
	createMeshPatternTexture();
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
      "  gl_FragColor.a = color.a;\n"
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

   if (vertexSWBuffer == -1) 
       glGenBuffers(1, &vertexSWBuffer);
}

static void setupMeshStencil(Pattern* pattern, GLfloat* vertex, int nbVertex){
	if (meshPatternObject <= 0) createMeshPatternProgram();

	glStencilFunc(GL_NEVER, 1, 0xFF);
	glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
	glEnable(GL_STENCIL_TEST);

	glUseProgram(meshPatternObject);

    	glBindBuffer(GL_ARRAY_BUFFER, vertexSWBuffer);

    	glBufferData(GL_ARRAY_BUFFER, 5*nbVertex*sizeof(GLfloat),vertex,GL_STATIC_DRAW);

    	if (meshPositionLoc >= 0) glVertexAttribPointer ( meshPositionLoc, 2, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), 0 );
    	if (meshPositionLoc >= 0) glVertexAttribPointer ( meshTexCoordLoc, 3, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), (void*)(sizeof(GLfloat)*2) );

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
}

static void disableMeshStencil(){
	glDisable(GL_STENCIL_TEST);
}

void drawPriority(Pattern* pattern, GLfloat* vertex, int priority, int nbVertex) {
	glDisable(GL_BLEND);
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
	glEnable(GL_BLEND);
}
