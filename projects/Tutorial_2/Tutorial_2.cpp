#include "Tutorial_2.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

Tutorial_2::Tutorial_2()
{

}

Tutorial_2::~Tutorial_2()
{

}

bool Tutorial_2::onCreate(int a_argc, char* a_argv[]) 
{
	// create a world-space matrix for a camera
	m_cameraMatrix = glm::inverse( glm::lookAt(glm::vec3(10,10,10),glm::vec3(0,0,0), glm::vec3(0,1,0)) );

	// get window dimensions to calculate aspect ratio
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);

	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, width / (float)height, 0.1f, 1000.0f);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);

	GenerateGrid(8, 10);
	m_time = 0;

	// create shader
	const char* vsSource = "#version 150\n \
	                        in vec4 Position; \
	                        in vec4 Colour; \
	                        out vec4 vColour; \
	                        uniform mat4 ProjectionView; \
							uniform float time; \
							uniform float heightScale; \
	                        void main() \
							{ \
							    vColour = Colour; \
								vec4 P = Position; \
								P.y += sin( time + Position.x) * heightScale; \
								gl_Position = ProjectionView * P; \
							}";

	const char* fsSource = "#version 150\n \
	                        in vec4 vColour; \
	                        out vec4 FragColor; \
	                        void main()	{ FragColor = vColour; }";

	int success = GL_FALSE;

	//Sets aside memory for our shaders	
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShader, 1, (const char**)&vsSource, 0);
	glCompileShader(vertexShader);

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	//check if there is an error in the shader code and printf it out
	if (success == GL_FALSE)
	{
		int infoLogLength = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* infoLog = new char[infoLogLength];

		glGetShaderInfoLog(vertexShader, infoLogLength, 0, infoLog);
		printf("Error: Failed to compile vertex shader!\n");
		printf("%s", infoLog);
		printf("\n");
		delete[] infoLog;
	}

	glShaderSource(fragmentShader, 1, (const char**)&fsSource, 0);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE)
	{
		int infoLogLength = 0;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* infoLog = new char[infoLogLength];

		glGetShaderInfoLog(fragmentShader, infoLogLength, 0, infoLog);
		printf("Error: Failed to compile fragment shader!\n");
		printf("%s", infoLog);
		printf("\n");
		delete[] infoLog;
	}

	//Where we are going to bind in outs completed shader program	
	m_programID = glCreateProgram();
	glAttachShader(m_programID, vertexShader);
	glAttachShader(m_programID, fragmentShader);
	glBindAttribLocation(m_programID, 0, "Position");
	glBindAttribLocation(m_programID, 1, "Colour");
	glLinkProgram(m_programID);

	glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
	if (success == GL_FALSE)
	{
		int infoLogLength = 0;
		glGetShaderiv(m_programID, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* infoLog = new char[infoLogLength];

		glGetShaderInfoLog(m_programID, infoLogLength, 0, infoLog);
		printf("Error: Failed to link shader program!\n");
		printf("%s", infoLog);
		printf("\n");
		delete[] infoLog;
	}

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	return true;
}

void Tutorial_2::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement( m_cameraMatrix, a_deltaTime, 10 );
	m_time += a_deltaTime;

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

void Tutorial_2::onDraw() 
{
	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse( m_cameraMatrix );
	viewMatrix = m_projectionMatrix * viewMatrix;

	glUseProgram(m_programID);
	unsigned int projectionViewUniform = glGetUniformLocation(m_programID, "ProjectionView");
	glUniformMatrix4fv(projectionViewUniform, 1, false, glm::value_ptr(viewMatrix));
	unsigned int timeUniform = glGetUniformLocation(m_programID, "time");
	glUniform1f(timeUniform, m_time);
	unsigned int heightScaleUniform = glGetUniformLocation(m_programID, "heightScale");
	glUniform1f(heightScaleUniform, 1);

	glBindVertexArray(m_VAO);
	int rows = 8;// number of rows
	int cols = 10;// number of cols
	unsigned int indexCount = (rows - 1) * (cols - 1) * 6;
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}

void Tutorial_2::onDestroy()
{
	
}

// function to create a grid
void Tutorial_2::GenerateGrid(unsigned int rows, unsigned int cols)
{
	Vertex4p4c* aoVertices = new Vertex4p4c[rows * cols];

	for (unsigned int r = 0; r < rows; ++r)
	{
		for (unsigned int c = 0; c < cols; ++c)
		{
			aoVertices[r * cols + c].position = glm::vec4((float)c, 0, (float)r, 1);
			//Create some arbitrary colour based off something 
			//that might not be related to tiling a texture
			glm::vec3 v3Colour = glm::vec3(sinf((c / (float)(cols - 1)) *  (r / (float)(rows - 1))));
			aoVertices[r * cols + c].colour = glm::vec4(v3Colour, 1.0f);
		}
	}

	// defining index count based off quad count (2 triangles per quad)
	unsigned int* auiIndices = new unsigned int[(rows - 1) * (cols - 1) * 6];

	unsigned int index = 0;
	for (unsigned int r = 0; r < (rows - 1); ++r)
	{
		for (unsigned int c = 0; c < (cols - 1); ++c)
		{
			// triangle 1
			auiIndices[index++] = (r + 1) * cols + (c + 1);
			auiIndices[index++] = (r + 1) * cols + c;
			auiIndices[index++] = r * cols + c;

			// triangle 2
			auiIndices[index++] = r * cols + (c + 1);
			auiIndices[index++] = (r + 1) * cols + (c + 1);
			auiIndices[index++] = r * cols + c;
		}
	}

	//Generate our GL Buffers
	//Lets move these so that they are all generated together
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_IBO);
	//Add the following line to generate a VertexArrayObject
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, (rows * cols) * sizeof(Vertex4p4c), aoVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex4p4c), 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex4p4c), (void *)(4 * sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (rows - 1) * (cols - 1) * 6 * sizeof(unsigned int), auiIndices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	delete[] aoVertices;
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new Tutorial_2();
	
	if (app->create("AIE - Tutorial_2",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}