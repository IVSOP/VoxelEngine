#ifndef RENDERER_H
#define RENDERER_H

#include "common.hpp"
#include "Chunk.hpp"
#include "Camera.hpp"
#include "Shader.hpp"
#include "TextureArray.hpp"
#include <memory>

#include <vector>

class Renderer {
public:

	Renderer() = delete;
	Renderer(GLsizei viewport_width, GLsizei viewport_height);
	~Renderer();

	GLsizei viewport_width, viewport_height;

	GLuint materialBuffer, materialTBO;
	GLuint chunkInfoBuffer, chunkInfoTBO;
	GLuint pointLightBuffer, pointLightTBO;
	GLuint dirLightBuffer, dirLightTBO;
	GLuint spotLightBuffer, spotLightTBO;


	// lighting FBO into wich scene gets rendered normally, but bright colors are extracted for bloom
	GLuint VAO, VAO_axis;
	GLuint indirectBuffer;
	GLuint base_vertexBuffer, // VBO for base vertex positions of a quad instance
		   vertexBuffer,      // VBO for all other information, needs to be separate due to instancing
		   vertexBuffer_axis; // VBO for axis vertices

	GLuint VAO_highlight;
	GLuint VBO_highlight, base_highlight_buffer;

	Shader lightingShader, axisShader, highlightShader;
	GLuint lightingFBO = 0, lightingFBODepthBuffer = 0;
	GLuint lightingTexture = 0; // color atttachment 0, scene renders into this
	GLuint brightTexture = 0; // color atttachment 1, extraction of brightly lit areas

	// ping pong FBOs, to perform two-pass Gaussian blur on the extracted bright colors
	GLuint VAO_viewport;
	GLuint vertexBuffer_viewport;
	GLuint pingpongFBO[2];
	GLuint pingpongTextures[2];
	Shader blurShader;

	// finally, in the normal FBO we merge the resulting blur with the original scene and apply gamma and exposure to everything at once
	// GLuint VAO_viewport;
	// GLuint vertexBuffer_viewport;
	Shader hdrBbloomMergeShader;
	// !! could have reused fbo and textures, but this is simpler and more flexible and less painful to manage
	

	GLfloat gamma = 2.2f, exposure = 1.0f, bloomThreshold = 1.0f, bloomOffset = 1.0f, explodeCoeff = 0.0f;
	int bloomBlurPasses = 5;
	bool showAxis = true;
	bool wireframe = false;
	bool limitFPS = false;
	double fps = 60.0f;
	GLfloat break_radius = 5.0f;
	GLfloat break_range = 10.0f;

	std::unique_ptr<TextureArray> textureArray = nullptr; // pointer since it starts as null and gets initialized later. unique_ptr so it always gets deleted

	void draw(const QuadContainer<Quad> &quads, const std::vector<IndirectData> &indirect, const std::vector<ChunkInfo> &chunkInfo, const SelectedBlockInfo &selectedBlock, const glm::mat4 &projection, Camera &camera, GLFWwindow * window, GLfloat deltaTime); // const
	void drawAxis(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection);

	void loadTextures();
	void resizeViewport(GLsizei viewport_width, GLsizei viewport_height);
	void generate_FBO_depth_buffer(GLuint *depthBuffer) const;
	void generate_FBO_texture(GLuint *textureID, GLenum attachmentID); // makes the texture, needs to be called whenever viewport is resized (for now)
	void checkFrameBuffer();

private:
	void prepareFrame(Camera &camera, GLfloat deltatime);
	void drawLighting(const QuadContainer<Quad> &quads, const std::vector<IndirectData> &indirect, const std::vector<ChunkInfo> &chunkInfo, const glm::mat4 &projection, const glm::mat4 &view, const Camera &camera); // camera is for debugging
	void drawSelectedBlock(const SelectedBlockInfo &selectedBlock, const glm::mat4 &projection, const glm::mat4 &view); // will draw over the FBO used in drawLighting
	void bloomBlur(int passes);
	void merge();
	void endFrame(GLFWwindow * window);
};

#endif
