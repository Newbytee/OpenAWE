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

#include <assert.h>
#include <stdexcept>
#include <iostream>
#include <fmt/format.h>
#include "texture.h"

namespace Graphics::OpenGL {

Texture::Texture(const ImageDecoder &decoder, GLuint id) : _id(id) {
	bool layered = decoder.getNumLayers() > 1;

	switch (decoder.getType()) {
		case ImageDecoder::kCubemap:
			_type = GL_TEXTURE_CUBE_MAP;
			break;
		case ImageDecoder::kTexture2D:
			if (layered)
				_type = GL_TEXTURE_2D_ARRAY;
			else
				_type = GL_TEXTURE_2D;
			break;
	}

	bind();

	assert(glIsTexture(id) == GL_TRUE);

	glTexParameteri(_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(_type, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLenum format, internalFormat = 0, type = 0;
	switch (decoder.getFormat()) {
		case ImageDecoder::kGrayScale:
			internalFormat = GL_R8;
			format = GL_RED;
			type = GL_UNSIGNED_BYTE;
			break;
		case ImageDecoder::RGB8:
			internalFormat = GL_RGB;
			format = GL_RGB;
			type = GL_UNSIGNED_BYTE;
			break;
		case ImageDecoder::kRGBA8:
			internalFormat = GL_RGBA;
			format = GL_BGRA;
			type = GL_UNSIGNED_BYTE;
			break;

		case ImageDecoder::kDXT1:
			format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			break;
		case ImageDecoder::kDXT3:
			format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			break;
		case ImageDecoder::kDXT5:
			format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;

		default:
			throw std::runtime_error("Invalid texture format");
	}

	if (layered) {
		unsigned int width = decoder.getMipmaps()[0].width;
		unsigned int height = decoder.getMipmaps()[0].height;
		glTexStorage3D(_type, 1, internalFormat, width, height, decoder.getNumLayers());
	}

	GLuint level = 0;
	for (int i = 0; i < decoder.getNumLayers(); ++i) {
		for (const auto &mipmap : decoder.getMipmaps(i)) {
			assert(mipmap.width != 0 && mipmap.height != 0);

			if (decoder.getType() == ImageDecoder::kCubemap) {
				if (decoder.isCompressed()) {
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, format, mipmap.width, mipmap.height, 0,
					                       mipmap.dataSize, mipmap.data[0]);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, format, mipmap.width, mipmap.height, 0,
					                       mipmap.dataSize, mipmap.data[1]);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, format, mipmap.width, mipmap.height, 0,
					                       mipmap.dataSize, mipmap.data[2]);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, format, mipmap.width, mipmap.height, 0,
					                       mipmap.dataSize, mipmap.data[3]);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, format, mipmap.width, mipmap.height, 0,
					                       mipmap.dataSize, mipmap.data[4]);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, format, mipmap.width, mipmap.height, 0,
					                       mipmap.dataSize, mipmap.data[5]);
				} else {
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, internalFormat, mipmap.width, mipmap.height, 0,
					             format, type, mipmap.data[0]);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, internalFormat, mipmap.width, mipmap.height, 0,
					             format, type, mipmap.data[1]);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, internalFormat, mipmap.width, mipmap.height, 0,
					             format, type, mipmap.data[2]);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, internalFormat, mipmap.width, mipmap.height, 0,
					             format, type, mipmap.data[3]);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, internalFormat, mipmap.width, mipmap.height, 0,
					             format, type, mipmap.data[4]);
					glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, internalFormat, mipmap.width, mipmap.height, 0,
					             format, type, mipmap.data[5]);
				}
			} else {
				if (decoder.isCompressed()) {
					glCompressedTexImage2D(
							_type,
							level,
							format,
							mipmap.width,
							mipmap.height,
							0,
							mipmap.dataSize,
							mipmap.data[0]
					);
				} else {
					if (layered) {
						glTexSubImage3D(
								_type,
								level,
								0,
								0,
								0,
								mipmap.width,
								mipmap.height,
								i,
								format,
								type,
								mipmap.data[0]
						);
					} else {
						glTexImage2D(
								_type,
								level,
								internalFormat,
								mipmap.width,
								mipmap.height,
								0,
								format,
								type,
								mipmap.data[0]
						);
					}
				}
			}

			level += 1;
		}
	}

	assert(glGetError() == GL_NO_ERROR);

	//if (decoder.getMipmaps().size() > 1)
	//	glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(unsigned int width, unsigned int height, GLuint id) : _id(id) {
	bind();

	assert(glIsTexture(id) == GL_TRUE);

	glTexParameteri(_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(_type, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA16F,
			width,
			height,
			0,
			GL_RGBA,
			GL_FLOAT,
			nullptr
	);
}

Texture::~Texture() {
	glDeleteTextures(1, &_id);
}

void Texture::attachToFramebuffer(GLuint attachmentType) {
	glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			attachmentType,
			_type,
			_id,
			0
	);
}

void Texture::bind() {
	//assert(glIsTexture(_id) == GL_TRUE);
	glBindTexture(_type, _id);
}

}
