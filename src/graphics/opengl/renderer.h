/* OpenAWE - A reimplementation of Remedys Alan Wake Engine
 *
 * OpenAWE is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * OpenAWE is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenAWE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenAWE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AWE_OPENGL_RENDERER_H
#define AWE_OPENGL_RENDERER_H

#include <list>
#include <map>
#include <memory>

#include "src/graphics/window.h"
#include "src/graphics/renderer.h"
#include "src/graphics/opengl/program.h"
#include "src/graphics/opengl/vbo.h"
#include "src/graphics/opengl/vao.h"
#include "src/graphics/opengl/texture.h"
#include "src/graphics/opengl/framebuffer.h"

namespace Graphics::OpenGL {

class Renderer : public Graphics::Renderer {
public:
	explicit Renderer(Graphics::Window &window);
	~Renderer();

	void drawFrame() override;

	Common::UUID registerVertices(byte *data, size_t length) override;
	Common::UUID registerIndices(byte *data, size_t length) override;

	Common::UUID
	registerVertexAttributes(const std::string &shader, const std::vector<VertexAttribute> &vertexAttributes,
							 Common::UUID vertexData) override;

	Common::UUID registerTexture(const ImageDecoder &decoder) override;

	void deregisterTexture(const Common::UUID &decoder) override;

private:
	void drawWorld();
	void drawVideo();
	void drawGUI();

	GLenum getTextureSlot(unsigned int slot);

	static void debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *userParam);

	Graphics::Window &_window;

	std::vector<GLuint> _texturePool;
	std::map<Common::UUID, GLuint> _texturePoolAssignments;

	std::unique_ptr<VBO> _videoQuad;
	std::unique_ptr<VAO> _videoQuadAttributes;

	std::unique_ptr<Framebuffer> _deferredBuffer;

	std::map<std::string, std::unique_ptr<Program>> _programs;

	std::map<Common::UUID, std::unique_ptr<VBO>> _vertices;
	std::map<Common::UUID, std::unique_ptr<VBO>> _indices;
	std::map<Common::UUID, std::unique_ptr<VAO>> _vaos;
	std::map<Common::UUID, std::unique_ptr<Texture>> _textures;
};

}

#endif //AWE_OPENGL_RENDERER_H
