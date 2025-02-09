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

#include <stdexcept>
#include <assert.h>

#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include <vector>

#include "src/graphics/opengl/program.h"

namespace Graphics::OpenGL {

Program::Program() {
	_id = glCreateProgram();
}

Program::~Program() {
	glDeleteProgram(_id);
}

void Program::attach(const Shader &shader) const {
	glAttachShader(_id, shader._id);
}

void Program::validate() const {
	glValidateProgram(_id);

	GLint result, infoLogLength;

	glGetProgramiv(_id, GL_VALIDATE_STATUS, &result);
	glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (result != GL_TRUE) {
		std::string log;
		log.resize(infoLogLength);

		glGetProgramInfoLog(_id, infoLogLength, nullptr, log.data());

		throw std::runtime_error(log);
	}
}

void Program::link() {
	glLinkProgram(_id);

	GLint result, infoLogLength;

	glGetProgramiv(_id, GL_LINK_STATUS, &result);
	glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (result != GL_TRUE) {
		std::string log;
		log.resize(infoLogLength);

		glGetProgramInfoLog(_id, infoLogLength, nullptr, log.data());

		throw std::runtime_error(log);
	}

	// Determine Attribute locations
	GLint numAttributes;
	GLint maxAttributeNameLength;
	glGetProgramiv(_id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttributeNameLength);
	glGetProgramiv(_id, GL_ACTIVE_ATTRIBUTES, &numAttributes);
	GLchar *name = new GLchar[maxAttributeNameLength];
	for (int i = 0; i < numAttributes; ++i) {
		GLsizei actualLength;
		GLenum type;
		GLint size;
		glGetActiveAttrib(_id, i, maxAttributeNameLength, &actualLength, &size, &type, name);
		std::string attributeName(name, actualLength);
		_attributes[attributeName] = i;
	}
	delete [] name;

	// Determine Uniform locations
	GLint numUniforms;
	GLint maxUniformNameLength;
	glGetProgramiv(_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLength);
	glGetProgramiv(_id, GL_ACTIVE_UNIFORMS, &numUniforms);
	name = new GLchar[maxUniformNameLength];
	for (int i = 0; i < numUniforms; ++i) {
		GLsizei actualLength;
		GLenum type;
		GLint size;
		glGetActiveUniform(_id, i, maxUniformNameLength, &actualLength, &size, &type, name);
		std::string uniformName(name, actualLength);
		_uniforms[uniformName] = i;
	}
	delete [] name;
}

void Program::bind() const {
	glUseProgram(_id);
}

void Program::setSamplerMappings(const std::map<std::string, std::string> &mappings) {
	_samplerMappings = mappings;
}

void Program::setAttributeMappings(const std::map<AttributeType, std::string> &mappings) {
	_attributeMappings = mappings;
}

std::optional<GLint> Program::getAttributeLocation(const AttributeType &type) {
	std::string attributeName;
	switch (type) {
		case kPosition:
			attributeName = "in_Position";
			break;
		case kNormal:
			attributeName = "in_Normal";
			break;
		case kColor:
			attributeName = "in_Color";
			break;
		case kBoneWeight:
			attributeName = "in_BoneWeight";
			break;
		case kBoneIndex:
			attributeName = "in_BoneID";
			break;
		case kDisplacementFactor:
			attributeName = "in_DisplacementFactor";
			break;
		case kTexCoord0:
			attributeName = "in_UV0";
			break;
		case kTexCoord1:
			attributeName = "in_UV1";
			break;
		case kTexCoord2:
			attributeName = "in_UV2";
			break;
		case kTexCoord3:
			attributeName = "in_UV3";
			break;
		default:
			throw std::runtime_error("Unknown attribute");
	}

	const auto attributeIndex = getAttributeLocation(attributeName);
	if (attributeIndex)
		return *attributeIndex;

	const auto attributeIndexMapped = getAttributeLocation(_attributeMappings[type]);
	if (attributeIndexMapped)
		return *attributeIndexMapped;

	return std::optional<GLint>();
}

void Program::setSymbols(const std::vector<ShaderConverter::Symbol> &symbols) {
	for (const auto &symbol : symbols) {
		_symbols[symbol.name] = symbol;
	}
}

void Program::setUniform1f(const std::string &name, const glm::vec1 &value) const {
	const auto uniformLocation = getUniformLocation(name);
	if (uniformLocation) {
		glUniform1f(*uniformLocation, value.x);
		return;
	}

	if (_symbols.empty())
		return;

	const auto symbol = _symbols.find(name);
	if (symbol != _symbols.end())
		glUniform1f(*getUniformArraySymbolLocation(symbol->second), value.x);
}

void Program::setUniform2f(const std::string &name, const glm::vec2 &value) const {
	const auto uniformLocation = getUniformLocation(name);
	if (uniformLocation) {
		glUniform2fv(*uniformLocation, 1, glm::value_ptr(value));
		return;
	}

	if (_symbols.empty())
		return;

	const auto symbol = _symbols.find(name);
	if (symbol != _symbols.end())
		glUniform2fv(*getUniformArraySymbolLocation(symbol->second), 1, glm::value_ptr(value));
}

void Program::setUniform3f(const std::string &name, const glm::vec3 &value) const {
	const auto uniformLocation = getUniformLocation(name);
	if (uniformLocation) {
		glUniform3fv(*uniformLocation, 1, glm::value_ptr(value));
		return;
	}

	if (_symbols.empty())
		return;

	const auto symbol = _symbols.find(name);
	if (symbol != _symbols.end())
		glUniform3fv(*getUniformArraySymbolLocation(symbol->second), 1, glm::value_ptr(value));
}

void Program::setUniform4f(const std::string &name, const glm::vec4 &value) const {
	const auto uniformLocation = getUniformLocation(name);
	if (uniformLocation) {
		glUniform4fv(*uniformLocation, 1, glm::value_ptr(value));
		return;
	}

	if (_symbols.empty())
		return;

	const auto symbol = _symbols.find(name);
	if (symbol != _symbols.end())
		glUniform4fv(*getUniformArraySymbolLocation(symbol->second), 1, glm::value_ptr(value));
}

void Program::setUniformMatrix4f(const std::string &name, const glm::mat4 &value) const {
	const auto uniformLocation = getUniformLocation(name);
	if (uniformLocation) {
		glUniformMatrix4fv(*uniformLocation, 1, GL_FALSE, glm::value_ptr(value));
		return;
	}

	if (_symbols.empty())
		return;

	const auto symbol = _symbols.find(name);
	if (symbol != _symbols.end()) {
		glUniform4fv(*getUniformArraySymbolLocation(symbol->second), 1, glm::value_ptr(value[0]));
		glUniform4fv(*getUniformArraySymbolLocation(symbol->second, 1), 1, glm::value_ptr(value[1]));
		glUniform4fv(*getUniformArraySymbolLocation(symbol->second, 2), 1, glm::value_ptr(value[2]));
		glUniform4fv(*getUniformArraySymbolLocation(symbol->second, 3), 1, glm::value_ptr(value[3]));
	}
}

void Program::setUniformSampler(const std::string &name, const GLuint value) const {
	const auto uniformLocation = getUniformLocation(name);
	if (uniformLocation) {
		glUniform1i(*uniformLocation, value);
		return;
	}

	const auto samplerName = _samplerMappings.find(name);
	if (samplerName != _samplerMappings.end()) {
		const auto samplerLocation = getUniformLocation(samplerName->second);
		if (!samplerLocation)
			throw std::runtime_error("Sampler location not found");

		glUniform1i(*samplerLocation, value);
	}
}

std::optional<GLint> Program::getUniformArraySymbolLocation(const ShaderConverter::Symbol &symbol, unsigned int offset) const {
	const std::string uniformArrayName =
			(symbol.shaderType == ShaderConverter::kVertex) ? "vs_uniforms_vec4" : "ps_uniforms_vec4";
	const std::string uniformArrayElementName = fmt::format("{}[{}]", uniformArrayName, symbol.index + offset);
	const auto uniformArrayLocation = getUniformLocation(uniformArrayElementName);
	if (!uniformArrayLocation)
		return std::optional<GLint>();

	return uniformArrayLocation;
}

std::optional<GLint> Program::getUniformLocation(const std::string &name) const {
	// First try to find the uniform in the cached uniforms
	auto iter = _uniforms.find(name);
	if (iter == _uniforms.end()) {
		// If that doesn't work, try to find it usng glGetUniformLocation, for example for specific array offsets
		GLint location = glGetUniformLocation(_id, name.c_str());
		if (location == -1)
			return std::optional<GLint>();
		else
			return location;
	}
	return iter->second;
}

std::optional<GLint> Program::getAttributeLocation(const std::string &name) const {
	auto iter = _attributes.find(name);
	if (iter == _attributes.end())
		return std::optional<GLint>();
	return iter->second;
}

}
