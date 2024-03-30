#include "Renderer.hpp"

#include <iostream>
#include <vector>
#include <cstdio>
#include "Vertex.hpp"
#include "Material.hpp"
#include "Camera.hpp"

// I know full well how cursed this is
#define TEX_ARRAY_SLOT 0
#define BRIGHT_TEXTURE_SLOT 1
#define SCENE_TEXTURE_SLOT 2
#define MATERIAL_TEXTURE_BUFFER_SLOT 3
#define POINTLIGHT_TEXTURE_BUFFER_SLOT 4
#define DIRLIGHT_TEXTURE_BUFFER_SLOT 5
#define SPOTLIGHT_TEXTURE_BUFFER_SLOT 6
#define CHUNKINFO_TEXTURE_BUFFER_SLOT 7

#define MAX_MATERIALS 8
#define MAX_LIGHTS 8

struct PointLight {
    glm::vec3 position;

    GLfloat constant;
    GLfloat linear;
    GLfloat quadratic; 

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

	// need one extra float for 16. read Material.h
	GLfloat padding_1;
};
static_assert(sizeof(PointLight) == 4 * sizeof(glm::vec4), "Error: PointLight has unexpected size");

struct DirLight {
    glm::vec3 direction;
  
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
static_assert(sizeof(DirLight) == 3 * sizeof(glm::vec4), "Error: DirLight has unexpected size");

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    GLfloat cutOff;
    GLfloat outerCutOff;
  
    GLfloat constant;
    GLfloat linear;
    GLfloat quadratic;
  
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
static_assert(sizeof(SpotLight) == 5 * sizeof(glm::vec4), "Error: SpotLight has unexpected size");


// quad filling entire screen
const ViewportVertex viewportVertices[] = {
	{-1.0f, -1.0f, 0.0f, 0.0f, 0.0f},
	{1.0f, -1.0f, 0.0f, 1.0f, 0.0f},
	{1.0f, 1.0f, 0.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 0.0f, 1.0f, 1.0f},
	{-1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
	{-1.0f, -1.0f, 0.0f, 0.0f, 0.0f}
};

void Renderer::drawAxis(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) {
	const AxisVertex vertices[] = {
		// x
		{-1000.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
		{1000.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},

		// y
		{0.0f, -1000.0f, 0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 1000.0f, 0.0f, 0.0f, 1.0f, 0.0f},

		// z
		{0.0f, 0.0f, -1000.0f, 0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1000.0f, 0.0f, 0.0f, 1.0f}
	};

	// bind VAO, VBO
	GLCall(glBindVertexArray(this->VAO_axis));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer_axis));

	// load vertices
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

	axisShader.use();
	axisShader.setMat4("u_MVP", projection * view * model);

	GLCall(glDrawArrays(GL_LINES, 0, 6)); // 6 pontos, 3 linhas
}

Renderer::Renderer(GLsizei viewport_width, GLsizei viewport_height)
: viewport_width(viewport_width), viewport_height(viewport_height), VAO(0), vertexBuffer(0),
  lightingShader("shaders/lighting.vert", "shaders/lighting.frag"),
  axisShader("shaders/basic.vert", "shaders/basic.frag"),
  blurShader("shaders/blur.vert", "shaders/blur.frag"),
  hdrBbloomMergeShader("shaders/hdrBloomMerge.vert", "shaders/hdrBloomMerge.frag")
{
	//////////////////////////// LOADING VAO ////////////////////////////
	GLCall(glGenVertexArrays(1, &this->VAO));
	GLCall(glBindVertexArray(this->VAO));

	//////////////////////////// LOADING VBOS ////////////////////////////////

	GLCall(glGenBuffers(1, &this->base_vertexBuffer));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->base_vertexBuffer));
	{
		const InstanceVertex baseVertices[] = {
			// position            // tex coords
			glm::vec3(0.0f, 0.0f, 0.0f),//  0.0f, 0.0f},
			glm::vec3(1.0f, 0.0f, 0.0f),//  1.0f, 0.0f},
			glm::vec3(1.0f, 1.0f, 0.0f),//  1.0f, 1.0f},
			glm::vec3(1.0f, 1.0f, 0.0f),//  1.0f, 1.0f},
			glm::vec3(0.0f, 1.0f, 0.0f),//  0.0f, 1.0f},
			glm::vec3(0.0f, 0.0f, 0.0f) //  0.0f, 0.0f}
		};
		GLuint vertex_position_layout = 0;
		GLCall(glEnableVertexAttribArray(vertex_position_layout));					// size appart				// offset
		GLCall(glVertexAttribPointer(vertex_position_layout, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceVertex), (const void *)offsetof(InstanceVertex, coords)));
		GLCall(glVertexAttribDivisor(vertex_position_layout, 0)); // these valuse are constant every vertex

		// GLuint vertex_texcoord_layout = 1;
		// GLCall(glEnableVertexAttribArray(vertex_position_layout));					// size appart				// offset
		// GLCall(glVertexAttribPointer(vertex_position_layout, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceVertex), (const void *)offsetof(InstanceVertex, tex_coords)));
		// GLCall(glVertexAttribDivisor(vertex_position_layout, 0)); // these valuse are constant every vertex

		GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(baseVertices), baseVertices, GL_STATIC_DRAW));
	}

	GLCall(glGenBuffers(1, &this->vertexBuffer));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer));
	// GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
	{
		// !!!!!!!!!!! the first 3 bytes of this uint will be the position, the remaining one will be the normal
		GLuint vertex_position_and_normal_layout = 1;
		GLCall(glEnableVertexAttribArray(vertex_position_and_normal_layout));					// size appart				// offset
		// WHAT THE FUCK????? why do I have to use glVertexAttribIPointer????????? and not glVertexAttribPointer????????????????// who tf designed this
		GLCall(glVertexAttribIPointer(vertex_position_and_normal_layout, 1, GL_INT, sizeof(Quad), (const void *)offsetof(Quad, position_and_normal)));
		GLCall(glVertexAttribDivisor(vertex_position_and_normal_layout, 1)); // values are per instance

		// !!!!!!!!!!! only the first byte has data
		GLuint vertex_matid_layout = 2;
		GLCall(glEnableVertexAttribArray(vertex_matid_layout));					// size appart				// offset
		GLCall(glVertexAttribIPointer(vertex_matid_layout, 1, GL_INT, sizeof(Quad), (const void *)offsetof(Quad, material_and_chunkid)));
		GLCall(glVertexAttribDivisor(vertex_matid_layout, 1)); // values are per instance
	}

	//////////////////////////// LOADING VAO FOR AXIS ////////////////////////////
	GLCall(glGenVertexArrays(1, &this->VAO_axis));
	GLCall(glBindVertexArray(this->VAO_axis));

	//////////////////////////// LOADING VBO FOR AXIS ////////////////////////////////
	GLCall(glGenBuffers(1, &this->vertexBuffer_axis));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer_axis));
	// GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
	{
		GLuint vertex_position_layout = 0;
		GLCall(glEnableVertexAttribArray(vertex_position_layout));					// size appart				// offset
		GLCall(glVertexAttribPointer(vertex_position_layout, 4, GL_FLOAT, GL_FALSE, sizeof(AxisVertex), (const void *)offsetof(AxisVertex, coords)));
		// GLCall(glVertexAttribDivisor(vertex_position_layout, 0)); // values are per vertex

		GLuint vertex_color_layout = 1;
		GLCall(glEnableVertexAttribArray(vertex_color_layout));					// size appart				// offset
		GLCall(glVertexAttribPointer(vertex_color_layout, 3, GL_FLOAT, GL_FALSE, sizeof(AxisVertex), (const void *)offsetof(AxisVertex, color)));
		// GLCall(glVertexAttribDivisor(vertex_color_layout, 0)); // values are per vertex
	}

	//////////////////////////// LOADING VAO FOR HDR ////////////////////////////
	GLCall(glGenVertexArrays(1, &this->VAO_viewport));
	GLCall(glBindVertexArray(this->VAO_viewport));

	//////////////////////////// LOADING VBO FOR HDR ////////////////////////////
	GLCall(glGenBuffers(1, &this->vertexBuffer_viewport));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer_viewport));
	// GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
	{
		GLuint vertex_position_layout = 0;
		GLCall(glEnableVertexAttribArray(vertex_position_layout));					// size appart				// offset
		GLCall(glVertexAttribPointer(vertex_position_layout, 4, GL_FLOAT, GL_FALSE, sizeof(ViewportVertex), (const void *)offsetof(ViewportVertex, coords)));
		// GLCall(glVertexAttribDivisor(vertex_position_layout, 0)); // values are per vertex

		GLuint vertex_texcoord_layout = 1;
		GLCall(glEnableVertexAttribArray(vertex_texcoord_layout));					// size appart				// offset
		GLCall(glVertexAttribPointer(vertex_texcoord_layout, 2, GL_FLOAT, GL_FALSE, sizeof(ViewportVertex), (const void *)offsetof(ViewportVertex, tex_coord)));
		// GLCall(glVertexAttribDivisor(vertex_normal_layout, 0)); // values are per vertex
	}

	//////////////////////////// LOADING SHADER UNIFORMS ///////////////////////////
	lightingShader.use();
	lightingShader.setInt("u_TextureArraySlot", TEX_ARRAY_SLOT);
	lightingShader.setMat4("u_Model", glm::mat4(1.0f)); // load identity just for safety
	lightingShader.setMat4("u_View", glm::mat4(1.0f)); // load identity just for safety
	lightingShader.setMat4("u_Projection", glm::mat4(1.0f)); // load identity just for safety


	//////////////////////////// load textures with info on materials and lights and chunk info
	GLCall(glGenBuffers(1, &materialBuffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, materialBuffer));
	GLCall(glGenTextures(1, &materialTBO));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, materialTBO));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, materialBuffer)); // bind the buffer to the texture

	GLCall(glGenBuffers(1, &chunkInfoBuffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, chunkInfoBuffer));
	GLCall(glGenTextures(1, &chunkInfoTBO));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, chunkInfoTBO));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, chunkInfoBuffer)); // bind the buffer to the texture

	GLCall(glGenBuffers(1, &pointLightBuffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, pointLightBuffer));
	GLCall(glGenTextures(1, &pointLightTBO));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, pointLightTBO));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, pointLightBuffer)); // bind the buffer to the texture

	GLCall(glGenBuffers(1, &dirLightBuffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, dirLightBuffer));
	GLCall(glGenTextures(1, &dirLightTBO));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, dirLightTBO));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, dirLightBuffer)); // bind the buffer to the texture

	GLCall(glGenBuffers(1, &spotLightBuffer));
	GLCall(glBindBuffer(GL_TEXTURE_BUFFER, spotLightBuffer));
	GLCall(glGenTextures(1, &spotLightTBO));
	GLCall(glBindTexture(GL_TEXTURE_BUFFER, spotLightTBO));
	GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, spotLightBuffer)); // bind the buffer to the texture

	//////////////////////////// load texture with info about chunks
	// ................


	// for axis shader
	axisShader.use();
	axisShader.setMat4("u_MVP", glm::mat4(1.0f));  // load identity just for safety

	//////////////////////////// LOADING TEXTURES ///////////////////////////
	loadTextures();


	//////////////////////////// LOADING FRAMEBUFFERS AND TEXTURE ATTACHMENTS ////////////////////////////
	GLCall(glGenFramebuffers(1, &lightingFBO));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, lightingFBO));
		generate_FBO_depth_buffer(&lightingFBODepthBuffer);
		generate_FBO_texture(&lightingTexture, GL_COLOR_ATTACHMENT0);
		generate_FBO_texture(&brightTexture, GL_COLOR_ATTACHMENT1);
		GLCall(checkFrameBuffer());
	GLCall(glGenFramebuffers(2, pingpongFBO));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]));
		generate_FBO_texture(&pingpongTextures[0], GL_COLOR_ATTACHMENT0);
		GLCall(checkFrameBuffer());
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[1]));
		generate_FBO_texture(&pingpongTextures[1], GL_COLOR_ATTACHMENT0);
		GLCall(checkFrameBuffer());
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));



	//////////////////////////// CLEANUP ///////////////////////////
	GLCall(glUseProgram(0));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	GLCall(glBindVertexArray(0));
}

Renderer::~Renderer() {
	GLCall(glDeleteBuffers(1, &vertexBuffer));
	GLCall(glDeleteBuffers(1, &materialBuffer));
	GLCall(glDeleteBuffers(1, &vertexBuffer_axis));
	GLCall(glDeleteBuffers(1, &vertexBuffer_viewport));

	GLCall(glDeleteVertexArrays(1, &VAO));
	GLCall(glDeleteVertexArrays(1, &VAO_axis));
	GLCall(glDeleteVertexArrays(1, &VAO_viewport));

	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	GLCall(glDeleteTextures(1, &lightingTexture));
	GLCall(glDeleteTextures(2, pingpongTextures));
	// delete the TBOs????????????????????????????????????????????

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, lightingFBO));
	GLCall(glDeleteRenderbuffers(1, &lightingFBODepthBuffer));

	GLCall(glDeleteFramebuffers(1, &lightingFBO));
	GLCall(glDeleteFramebuffers(2, pingpongFBO));
}

void Renderer::loadTextures() {
	// load texture array instance
	this->textureArray = std::make_unique<TextureArray>(TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_LAYERS);

	TextureArray *tex = textureArray.get();
	tex->addTexture("textures/missing_texture.png"); // 0
	tex->addTexture("textures/black.png"); // 1
	tex->addTexture("textures/white.png"); // 2
	tex->addTexture("textures/birch_planks.png"); // 3
	tex->setTextureArrayToSlot(TEX_ARRAY_SLOT);
}

void Renderer::prepareFrame(Camera &camera, GLfloat deltaTime) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Debug");
	// ImGui::ShowDemoWindow();
	ImGui::Text("FPS: %lf", 1.0f / deltaTime);
	ImGui::Text("Facing x:%f y:%f z:%f", camera.Front.x, camera.Front.y, camera.Front.z);
	ImGui::InputFloat3("Position", glm::value_ptr(camera.Position));
	ImGui::SliderFloat("##Camera_speed", &camera.MovementSpeed, 0.0f, 1000.0f, "Camera speed = %.3f");
	ImGui::SameLine();
	ImGui::InputFloat("Camera speed", &camera.MovementSpeed, 1.0f, 10.0f);
	ImGui::SliderFloat("gamma", &gamma, 0.0f, 10.0f, "gamma = %.3f");
	ImGui::SliderFloat("exposure", &exposure, 0.0f, 10.0f, "exposure = %.3f");
	ImGui::InputInt("bloomPasses", &bloomBlurPasses, 1, 1); if (bloomBlurPasses < 0) bloomBlurPasses = 0;
	// ImGui::InputInt("bloomPasses", &bloomBlurPasses, 1, 1, "bloomPasses = %d");
	ImGui::SliderFloat("bloomThreshold", &bloomThreshold, 0.0f, 5.0f);
	// texOffsetCoeff = static_cast<GLfloat>(rand()) / static_cast<GLfloat>(RAND_MAX) * 10.0f;
	ImGui::SliderFloat("bloomOffset", &bloomOffset, 0.0f, 10.0f, "bloomOffset = %.3f");
	ImGui::Checkbox("Show axis", &showAxis);
}

void Renderer::drawLighting(const std::vector<Quad> &quads, const std::vector<ChunkInfo> &chunkInfo, const glm::mat4 &projection, const glm::mat4 &view, const Camera &camera) {
	constexpr glm::mat4 model = glm::mat4(1.0f);
	// const glm::mat4 MVP = projection * view * model;

	//////////////////////////////////////////////// he normal scene is drawn into the lighting framebuffer, where the bright colors are then separated
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, lightingFBO));
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
		// glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

		GLCall(glBindVertexArray(this->VAO));
		// GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->base_vertexBuffer)); // is this needed??????????
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer));

		// load vertices
		GLCall(glBufferData(GL_ARRAY_BUFFER, quads.size() * sizeof(Quad), quads.data(), GL_STATIC_DRAW));

		// bind program, load uniforms
		lightingShader.use();

		// load MVP, texture array and view
		this->textureArray.get()->setTextureArrayToSlot(TEX_ARRAY_SLOT);
		lightingShader.setInt("u_TextureArraySlot", TEX_ARRAY_SLOT);
		lightingShader.setMat4("u_Model", model);
		lightingShader.setMat4("u_View", view);
		lightingShader.setMat4("u_Projection", projection);
		lightingShader.setMat3("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(view * model))));

		lightingShader.setFloat("u_BloomThreshold", bloomThreshold);

		// load UBO
		Material materials[8];
		materials[0] = {
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			// glm::vec3(2.99f, 0.72f, 0.0745f),
			glm::vec3(0.0f),
			32.0f,
			3
		};

		GLCall(glBindBuffer(GL_TEXTURE_BUFFER, materialBuffer));
		GLCall(glBufferData(GL_TEXTURE_BUFFER, MAX_MATERIALS * sizeof(Material), materials, GL_STATIC_DRAW));
		GLCall(glActiveTexture(GL_TEXTURE0 + MATERIAL_TEXTURE_BUFFER_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_BUFFER, materialTBO));
		// GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, materialBuffer)); // bind the buffer to the texture (has been done while setting up)
		lightingShader.setInt("u_MaterialTBO", MATERIAL_TEXTURE_BUFFER_SLOT);

		PointLight pointLights[MAX_LIGHTS];
		pointLights[0] = {
			.position = glm::vec3(0.0f, 2.0f, 5.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
			.ambient = glm::vec3(0.2f, 0.2f, 0.0f),
			.diffuse = glm::vec3(0.78f, 0.78f, 0.0f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.padding_1 = 0.0f
		};

		GLCall(glBindBuffer(GL_TEXTURE_BUFFER, pointLightBuffer));
		GLCall(glBufferData(GL_TEXTURE_BUFFER, MAX_LIGHTS * sizeof(PointLight), pointLights, GL_STATIC_DRAW));
		GLCall(glActiveTexture(GL_TEXTURE0 + POINTLIGHT_TEXTURE_BUFFER_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_BUFFER, pointLightTBO));
		// GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, pointLightBuffer)); // bind the buffer to the texture (has been done while setting up)
		lightingShader.setInt("u_PointLightTBO", POINTLIGHT_TEXTURE_BUFFER_SLOT);
		lightingShader.setInt("u_NumPointLights", 0);

		DirLight dirLights[MAX_LIGHTS];
		dirLights[0] = {
			// .direction = glm::normalize(glm::vec3(0.5f, -0.45f, 0.5f)),
			.direction = glm::normalize(glm::vec3(1.0f, 0.1f, 0.0f)),
			.ambient = glm::vec3(0.2f, 0.2f, 0.2f),
			.diffuse = glm::vec3(0.78f, 0.78f, 0.78f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f)
		};

		GLCall(glBindBuffer(GL_TEXTURE_BUFFER, dirLightBuffer));
		GLCall(glBufferData(GL_TEXTURE_BUFFER, MAX_LIGHTS * sizeof(DirLight), dirLights, GL_STATIC_DRAW));
		GLCall(glActiveTexture(GL_TEXTURE0 + DIRLIGHT_TEXTURE_BUFFER_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_BUFFER, dirLightTBO));
		// GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, pointLightBuffer)); // bind the buffer to the texture (has been done while setting up)
		lightingShader.setInt("u_DirLightTBO", DIRLIGHT_TEXTURE_BUFFER_SLOT);
		lightingShader.setInt("u_NumDirLights", 1);

		SpotLight spotLights[MAX_LIGHTS];
		spotLights[0] = {
			.position = camera.Position,
			// .position = glm::vec3(0.0f, 1.0f, 3.0f),
			.direction = camera.Front,
			// .direction = glm::vec3(0.0f, -0.25f, -0.97f),
			.cutOff = glm::cos(glm::radians(12.5f)),
			.outerCutOff = glm::cos(glm::radians(17.5f)),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
			.ambient = glm::vec3(0.1f, 0.1f, 0.1f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f)
		};

		GLCall(glBindBuffer(GL_TEXTURE_BUFFER, spotLightBuffer));
		GLCall(glBufferData(GL_TEXTURE_BUFFER, MAX_LIGHTS * sizeof(SpotLight), spotLights, GL_STATIC_DRAW));
		GLCall(glActiveTexture(GL_TEXTURE0 + SPOTLIGHT_TEXTURE_BUFFER_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_BUFFER, spotLightTBO));
		// GLCall(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, spotLightBuffer)); // bind the buffer to the texture (has been done while setting up)
		lightingShader.setInt("u_SpotLightTBO", SPOTLIGHT_TEXTURE_BUFFER_SLOT);
		lightingShader.setInt("u_NumSpotLights", 0);


		GLCall(glBindBuffer(GL_TEXTURE_BUFFER, chunkInfoBuffer));
		GLCall(glBufferData(GL_TEXTURE_BUFFER, chunkInfo.size() * sizeof(ChunkInfo), chunkInfo.data(), GL_STATIC_DRAW));
		GLCall(glActiveTexture(GL_TEXTURE0 + CHUNKINFO_TEXTURE_BUFFER_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_BUFFER, chunkInfoTBO));
		lightingShader.setInt("u_ChunkInfoTBO", CHUNKINFO_TEXTURE_BUFFER_SLOT);


		// bind the render buffer to this FBO (maybe this is missing actualy binding it, idk, but it gets regenerated automatically when screen is resized)
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, lightingFBODepthBuffer);


		// specify 2 attachments
		constexpr GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		GLCall(glDrawBuffers(2, attachments));

		// lightingShader.validate();

		GLCall(glDrawArraysInstanced(GL_TRIANGLES, 0, 6, quads.size()))


		if (showAxis) {
			drawAxis(glm::mat4(1.0f), view, projection);
		}

		// GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
}

void Renderer::bloomBlur(int passes) {
	//////////////////////////////////////////////// run the ping pong gaussian blur several times
	blurShader.use();
	GLCall(glBindVertexArray(this->VAO_viewport));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer_viewport));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(viewportVertices), viewportVertices, GL_STATIC_DRAW));

	if (passes == 0) {
		// if this happens, instead of switching verything just clear pingpongTextures[1], which will be used in merging the textures
		// bind FBO, attach texture, clear color buffer
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[1]));
		GLCall(glActiveTexture(GL_TEXTURE0 + BRIGHT_TEXTURE_SLOT));
		GLCall(glActiveTexture(GL_TEXTURE0 + BRIGHT_TEXTURE_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_2D, pingpongTextures[1])); // use texture from pong
		blurShader.setInt("u_BlurBuffer", BRIGHT_TEXTURE_SLOT);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}


	// manually doing the first passes, since I need to get the texture from the scene
	// horizontal pass
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // isto devia ser chamado sequer???????????????????????????????????? acho que a imagem e 100% overwritten
		blurShader.setInt("u_Horizontal", 0);
		GLCall(glActiveTexture(GL_TEXTURE0 + BRIGHT_TEXTURE_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_2D, this->brightTexture)); // use texture from scene
		blurShader.setInt("u_BlurBuffer", BRIGHT_TEXTURE_SLOT);
		GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

	// vertical pass
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[1]));
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		blurShader.setInt("u_Horizontal", 1);
		GLCall(glActiveTexture(GL_TEXTURE0 + BRIGHT_TEXTURE_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_2D, pingpongTextures[0])); // use texture from ping
		blurShader.setInt("u_BlurBuffer", BRIGHT_TEXTURE_SLOT);
		GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

	for (GLint i = 0; i < passes - 1; i++) {
		blurShader.setFloat("u_TexOffsetCoeff", bloomOffset);

		// horizontal pass
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]));
			blurShader.setInt("u_Horizontal", 0);
			GLCall(glActiveTexture(GL_TEXTURE0 + BRIGHT_TEXTURE_SLOT));
			GLCall(glBindTexture(GL_TEXTURE_2D, pingpongTextures[1])); // use texture from pong
			blurShader.setInt("u_BlurBuffer", BRIGHT_TEXTURE_SLOT);
			GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

		// vertical pass
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[1]));
			blurShader.setInt("u_Horizontal", 1);
			GLCall(glActiveTexture(GL_TEXTURE0 + BRIGHT_TEXTURE_SLOT));
			GLCall(glBindTexture(GL_TEXTURE_2D, pingpongTextures[0])); // use texture from ping
			blurShader.setInt("u_BlurBuffer", BRIGHT_TEXTURE_SLOT);
			GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
	}
}

void Renderer::merge() {
	//////////////////////////////////////////////// join the blur to the scene and apply gamma and exposure
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// GLCall(glBindVertexArray(this->VAO_viewport));
		// GLCall(glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer_viewport));
		// GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(viewportVertices), viewportVertices, GL_STATIC_DRAW));

		hdrBbloomMergeShader.use();
		GLCall(glActiveTexture(GL_TEXTURE0 + SCENE_TEXTURE_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_2D, this->lightingTexture));
		GLCall(glActiveTexture(GL_TEXTURE0 + BRIGHT_TEXTURE_SLOT));
		GLCall(glBindTexture(GL_TEXTURE_2D, pingpongTextures[1]));

		hdrBbloomMergeShader.setInt("u_SceneBuffer", SCENE_TEXTURE_SLOT);
		hdrBbloomMergeShader.setInt("u_BrightBuffer", BRIGHT_TEXTURE_SLOT);
		hdrBbloomMergeShader.setFloat("u_Gamma", gamma);
		hdrBbloomMergeShader.setFloat("u_Exposure", exposure);

		// hdrBbloomMergeShader.validate();
		GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
}

void Renderer::endFrame(GLFWwindow * window) {
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void Renderer::draw(const std::vector<Quad> &quads, const std::vector<ChunkInfo> &chunkInfo, const glm::mat4 &projection, Camera &camera, GLFWwindow * window, GLfloat deltaTime) {
	prepareFrame(camera, deltaTime);
	const glm::mat4 view = camera.GetViewMatrix();
	drawLighting(quads, chunkInfo, projection, view, camera);
	bloomBlur(this->bloomBlurPasses);
	merge();

	ImGui::Checkbox("Limit FPS", &limitFPS);
	if (limitFPS) {
		const double f64_min = 0.0, f64_max = 240.0;
		ImGui::SliderScalar("Target FPS", ImGuiDataType_Double, &fps, &f64_min, &f64_max, "Target FPS = %.0f");
	}


	endFrame(window);
}

// make sure fbo is bound before calling this
void Renderer::generate_FBO_depth_buffer(GLuint *depthBuffer) const {

	if (*depthBuffer > 0) {
		glDeleteRenderbuffers(1, depthBuffer);
	}

	glGenRenderbuffers(1, depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, *depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, this->viewport_width, this->viewport_height);
}

// this does NOT take into acount currently used textures slots etc, only here for organisation
// make sure fbo is bound before calling this
void Renderer::generate_FBO_texture(GLuint *textureID, GLenum attachmentID) {
	// delete existing texture, if needed
	if (*textureID != 0) { // for safety, delete the texture entirely. maybe does not need to be done
		GLCall(glBindTexture(GL_TEXTURE_2D, 0));
		GLCall(glDeleteTextures(1, textureID));
	}

	GLCall(glGenTextures(1, textureID));
	GLCall(glBindTexture(GL_TEXTURE_2D, *textureID));
	// you can get the default behaviour by either not using the framebuffer or setting this to GL_RGBA
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, this->viewport_width, this->viewport_height, 0, GL_RGBA, GL_FLOAT, NULL));  // change to 32float if needed

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	// attach to fbo
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentID, GL_TEXTURE_2D, *textureID, 0));
}

// regenerate the textures for all the FBOs
void Renderer::resizeViewport(GLsizei viewport_width, GLsizei viewport_height) {
	this->viewport_width = viewport_width;
	this->viewport_height = viewport_height;

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, lightingFBO));
	generate_FBO_depth_buffer(&lightingFBODepthBuffer);
	generate_FBO_texture(&lightingTexture, GL_COLOR_ATTACHMENT0);
	generate_FBO_texture(&brightTexture, GL_COLOR_ATTACHMENT1);
	GLCall(checkFrameBuffer());

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]));
	generate_FBO_texture(&pingpongTextures[0], GL_COLOR_ATTACHMENT0);
	GLCall(checkFrameBuffer());
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[1]));
	generate_FBO_texture(&pingpongTextures[1], GL_COLOR_ATTACHMENT0);
	GLCall(checkFrameBuffer());

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

// needs to be improved
void Renderer::checkFrameBuffer() {
	GLCall(GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER));

	if (status != GL_FRAMEBUFFER_COMPLETE) {
    	puts("Error in fbo");
		exit(1);
	}
}
