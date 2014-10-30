#include "GPUParticleSystem.h"

#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <stb_image.h>

GPUParticleEmitter::GPUParticleEmitter()
: m_particles(nullptr), m_maxParticles(0),
m_position(0, 0, 0),
m_drawShader(0), m_updateShader(0)
{
	m_vao[0] = 0;
	m_vao[1] = 0;
	m_vbo[0] = 0;
	m_vbo[1] = 0;
}

GPUParticleEmitter::~GPUParticleEmitter()
{
	delete[] m_particles;

	// delete the texture
	if (0 != m_textureID)
	{
		glDeleteTextures(1, &m_textureID);
	}

	glDeleteVertexArrays(2, m_vao);
	glDeleteBuffers(2, m_vbo);

	// delete the shaders
	glDeleteProgram(m_drawShader);
	glDeleteProgram(m_updateShader);
}

void GPUParticleEmitter::initalise(unsigned int a_maxParticles,
	float a_lifetimeMin, float a_lifetimeMax,
	float a_velocityMin, float a_velocityMax,
	float a_startSize, float a_endSize,
	const glm::vec4& a_startColour, const glm::vec4& a_endColour,
	const char* a_textureFileName)
{
	m_maxParticles = a_maxParticles;

	// store all variables passed in
	m_startColour = a_startColour;
	m_endColour = a_endColour;
	m_startSize = a_startSize;
	m_endSize = a_endSize;
	m_velocityMin = a_velocityMin;
	m_velocityMax = a_velocityMax;
	m_lifespanMin = a_lifetimeMin;
	m_lifespanMax = a_lifetimeMax;

	// create particle array and store them in the dead pool
	m_particles = new GPUParticle[a_maxParticles];

	m_activeBuffer = 0;

	createBuffers();
	
	createTexture(a_textureFileName);

	createUpdateShader();

	createDrawShader();
}

void GPUParticleEmitter::createBuffers()
{
	// create opengl buffers
	glGenVertexArrays(2, m_vao);
	glGenBuffers(2, m_vbo);

	glBindVertexArray(m_vao[0]);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(GPUParticle), m_particles, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // velocity
	glEnableVertexAttribArray(2); // lifetime
	glEnableVertexAttribArray(3); // lifespan
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GPUParticle), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GPUParticle), ((char*)0) + 12);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(GPUParticle), ((char*)0) + 24);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GPUParticle), ((char*)0) + 28);

	glBindVertexArray(m_vao[1]);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, m_maxParticles * sizeof(GPUParticle), 0, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // velocity
	glEnableVertexAttribArray(2); // lifetime
	glEnableVertexAttribArray(3); // lifespan
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GPUParticle), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GPUParticle), ((char*)0) + 12);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(GPUParticle), ((char*)0) + 24);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GPUParticle), ((char*)0) + 28);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GPUParticleEmitter::createDrawShader()
{
	unsigned int vs = Utility::loadShader("shaders/particle.vert", GL_VERTEX_SHADER);
	unsigned int fs = Utility::loadShader("shaders/particle.frag", GL_FRAGMENT_SHADER);
	unsigned int gs = Utility::loadShader("shaders/particle.geom", GL_GEOMETRY_SHADER);
	const char* inputs[] = { "Position", "Velocity", "Lifetime", "Lifespan" };
	m_drawShader = Utility::createProgram(vs, 0, 0, gs, fs, 4, inputs);

	// remove unneeded handles
	glDeleteShader(vs);
	glDeleteShader(gs);
	glDeleteShader(fs);

	// bind the shader so that we can set some uniforms that don't change per-frame
	glUseProgram(m_drawShader);

	// bind size information for interpolation that wont change
	unsigned int location = glGetUniformLocation(m_drawShader, "sizeStart");
	glUniform1f(location, m_startSize);
	location = glGetUniformLocation(m_drawShader, "sizeEnd");
	glUniform1f(location, m_endSize);

	// bind colour information for interpolation that wont change
	location = glGetUniformLocation(m_drawShader, "colourStart");
	glUniform4fv(location, 1, glm::value_ptr(m_startColour));
	location = glGetUniformLocation(m_drawShader, "colourEnd");
	glUniform4fv(location, 1, glm::value_ptr(m_endColour));

	// bind texture information
	location = glGetUniformLocation(m_drawShader, "useTexture");
	glUniform1i(location, m_useTexture ? GL_TRUE : GL_FALSE);
	if (m_useTexture)
	{
		// bind texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		location = glGetUniformLocation(m_drawShader, "textureMap");
		glUniform1i(location, 0);
	}
}

void GPUParticleEmitter::createUpdateShader()
{
	// load a text file into an unsigned char buffer
	unsigned char* source = Utility::fileToBuffer("shaders/particleUpdate.vert");

	// create a shader ourselves
	unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, (const char**)&source, 0);
	glCompileShader(vs);

	delete[] source;

	m_updateShader = glCreateProgram();
	glAttachShader(m_updateShader, vs);

	// set the inputs (not needed if using layout(location) shader tags)
	glBindAttribLocation(m_updateShader, 0, "Position");
	glBindAttribLocation(m_updateShader, 1, "Velocity");
	glBindAttribLocation(m_updateShader, 2, "Lifetime");
	glBindAttribLocation(m_updateShader, 3, "Lifespan");

	const char* varyings[] = { "position", "velocity", "lifetime", "lifespan" };
	glTransformFeedbackVaryings(m_updateShader, 4, varyings, GL_INTERLEAVED_ATTRIBS);

	glLinkProgram(m_updateShader);

	// remove unneeded handles
	glDeleteShader(vs);

	// bind the shader so that we can set some uniforms that don't change per-frame
	glUseProgram(m_updateShader);

	// bind lifetime minimum and maximum
	unsigned int location = glGetUniformLocation(m_updateShader, "lifeMin");
	glUniform1f(location, m_lifespanMin);
	location = glGetUniformLocation(m_updateShader, "lifeMax");
	glUniform1f(location, m_lifespanMax);
}

void GPUParticleEmitter::draw(const glm::mat4& a_cameraTransform, const glm::mat4& a_projection)
{
	//////////////////////////////////////////////////////////////////////////
	// update the particles using a vertex shader and transform feedback
	glUseProgram(m_updateShader);

	// bind time information
	unsigned int location = glGetUniformLocation(m_updateShader, "time");
	glUniform1f(location, Utility::getTotalTime());
	location = glGetUniformLocation(m_updateShader, "deltaTime");
	glUniform1f(location, Utility::getDeltaTime());

	// bind emitter's position
	location = glGetUniformLocation(m_updateShader, "emitterPosition");
	glUniform3fv(location, 1, glm::value_ptr(m_position));

	// disable rasterisation
	glEnable(GL_RASTERIZER_DISCARD);

	// bind the buffer we will update
	glBindVertexArray(m_vao[m_activeBuffer]);

	// work out the "other" buffer
	unsigned int otherBuffer = (m_activeBuffer + 1) % 2;

	// bind the buffer we will update into as points and begin transform feedback
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_vbo[otherBuffer]);
	glBeginTransformFeedback(GL_POINTS);

	glDrawArrays(GL_POINTS, 0, m_maxParticles);

	// disable transform feedback and enable rasterisation again
	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

	//////////////////////////////////////////////////////////////////////////
	// draw the particles using the Geometry SHader to billboard them
	glUseProgram(m_drawShader);

	glm::mat4 viewMatrix = glm::inverse(a_cameraTransform);

	location = glGetUniformLocation(m_drawShader, "projectionView");
	glUniformMatrix4fv(location, 1, false, glm::value_ptr(a_projection * viewMatrix));

	location = glGetUniformLocation(m_drawShader, "cameraTransform");
	glUniformMatrix4fv(location, 1, false, glm::value_ptr(a_cameraTransform));

	// draw particles in the "other" buffer
	glBindVertexArray(m_vao[otherBuffer]);
	glDrawArrays(GL_POINTS, 0, m_maxParticles);

	// swap for next frame
	m_activeBuffer = otherBuffer;
}


void GPUParticleEmitter::createTexture(const char* a_textureFileName)
{
	// if no file name is provided for the particle texture, texture ID will remain zero
	m_textureID = 0;
	m_useTexture = nullptr != a_textureFileName && 0 < strlen(a_textureFileName);
	if (m_useTexture)
	{
		// load particle texture
		int width = 0;
		int height = 0;
		int format = 0;
		unsigned char* pixelData = stbi_load(a_textureFileName, &width, &height, &format, STBI_default);

		// create OpenGL texture handle
		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);

		// set pixel data for texture
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);

		// set filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// clear bound texture state so we don't accidentally change it
		glBindTexture(GL_TEXTURE_2D, 0);

		// delete pixel data
		delete[] pixelData;
	}
}
