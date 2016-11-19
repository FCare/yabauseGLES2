#include "gles20programs.h"


void createPriorityProgram(render_context *ctx) {
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
   if (ctx->tt_context->priorityProgram > 0) return;
   ctx->tt_context->priorityProgram = gles20_createProgram (vShaderPriorityStr, fShaderPriorityStr);

   if ( ctx->tt_context->priorityProgram == 0 ){
      fprintf (stderr,"Can not create a program\n");
      return;
   }

   // Get the attribute locations
   ctx->tt_context->prioPositionLoc = glGetAttribLocation ( ctx->tt_context->priorityProgram, "a_position" );
   ctx->tt_context->prioTexCoordLoc = glGetAttribLocation ( ctx->tt_context->priorityProgram, "a_texCoord" );
   // Get the sampler location
   ctx->tt_context->prioSamplerLoc = glGetUniformLocation ( ctx->tt_context->priorityProgram, "s_texture" );
   ctx->tt_context->prioValueLoc = glGetUniformLocation ( ctx->tt_context->priorityProgram, "u_priority" );

   if (ctx->tt_context->vertexSWBuffer == -1) 
       glGenBuffers(1, &ctx->tt_context->vertexSWBuffer);
}

void createPatternProgram(render_context *ctx) {
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
	if (ctx->tt_context->patternObject > 0) return;
   // Create the program object
   ctx->tt_context->patternObject = gles20_createProgram (vShaderStr, fShaderStr);

   if ( ctx->tt_context->patternObject == 0 ){
      fprintf (stderr,"Can not create a program\n");
      return;
   }

   // Get the attribute locations
   ctx->tt_context->patPositionLoc = glGetAttribLocation ( ctx->tt_context->patternObject, "a_position" );
   ctx->tt_context->patTexCoordLoc = glGetAttribLocation ( ctx->tt_context->patternObject, "a_texCoord" );
   // Get the sampler location
   ctx->tt_context->patSamplerLoc = glGetUniformLocation ( ctx->tt_context->patternObject, "s_texture" );

   if (ctx->tt_context->vertexSWBuffer == -1) 
       glGenBuffers(1, &ctx->tt_context->vertexSWBuffer);
}

void drawPattern(Pattern* pattern, GLfloat* vertex, render_context *ctx){
	int i;
	if (pattern->mesh != 0) {
		glBlendColor(0.0,0.0,0.0,0.5);
		glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	}
	glUseProgram(ctx->tt_context->patternObject);
	glUniform1i(ctx->tt_context->patSamplerLoc, 0);

        if (pattern->tw != 1.0f) {
		vertex[2]*=pattern->tw;
		vertex[7]*=pattern->tw;
		vertex[12]*=pattern->tw;
		vertex[17]*=pattern->tw;
	}
        if (pattern->th != 1.0f) {
		vertex[3]*=pattern->th;
		vertex[8]*=pattern->th;
		vertex[13]*=pattern->th;
		vertex[18]*=pattern->th;
	}

    	glBindBuffer(GL_ARRAY_BUFFER, ctx->tt_context->vertexSWBuffer);

    	glBufferData(GL_ARRAY_BUFFER, 20*sizeof(GLfloat),vertex,GL_STATIC_DRAW);

    	if (ctx->tt_context->patPositionLoc >= 0) glVertexAttribPointer ( ctx->tt_context->patPositionLoc, 2, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), 0 );
    	if (ctx->tt_context->patTexCoordLoc >= 0) glVertexAttribPointer ( ctx->tt_context->patTexCoordLoc, 3, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), (void*)(sizeof(GLfloat)*2) );

    	if (ctx->tt_context->patPositionLoc >= 0) glEnableVertexAttribArray ( ctx->tt_context->patPositionLoc );
    	if (ctx->tt_context->patTexCoordLoc >= 0) glEnableVertexAttribArray ( ctx->tt_context->patTexCoordLoc );

    	glActiveTexture ( GL_TEXTURE0 );
    	glBindTexture(GL_TEXTURE_2D, pattern->tex);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	if (pattern->mesh != 0) {
		glBlendColor(0.0,0.0,0.0,0.0);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void drawPriority(Pattern* pattern, GLfloat* vertex, int priority, render_context *ctx) {
	glDisable(GL_BLEND);
    	glUseProgram(ctx->tt_context->priorityProgram);
	glUniform1i(ctx->tt_context->prioSamplerLoc, 0);

    	glBindBuffer(GL_ARRAY_BUFFER, ctx->tt_context->vertexSWBuffer);

    	glBufferData(GL_ARRAY_BUFFER, 20*sizeof(GLfloat),vertex,GL_STATIC_DRAW);

    	if (ctx->tt_context->prioPositionLoc >= 0) glVertexAttribPointer ( ctx->tt_context->prioPositionLoc, 2, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), 0 );
    	if (ctx->tt_context->prioTexCoordLoc >= 0) glVertexAttribPointer ( ctx->tt_context->prioTexCoordLoc, 3, GL_FLOAT,  GL_FALSE, 5 * sizeof(GLfloat), (void*)(sizeof(GLfloat)*2) );

    	if (ctx->tt_context->prioPositionLoc >= 0) glEnableVertexAttribArray ( ctx->tt_context->prioPositionLoc );
    	if (ctx->tt_context->prioTexCoordLoc >= 0) glEnableVertexAttribArray ( ctx->tt_context->prioTexCoordLoc );

    	glActiveTexture ( GL_TEXTURE0 );
    	glBindTexture(GL_TEXTURE_2D, pattern->tex);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    	glUniform1f(ctx->tt_context->prioValueLoc, priority/255.0f);

    	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glEnable(GL_BLEND);
}
