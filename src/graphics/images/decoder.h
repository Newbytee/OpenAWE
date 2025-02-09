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

#ifndef AWE_DECODER_H
#define AWE_DECODER_H

#include <src/common/types.h>
#include <vector>

namespace Graphics {

class ImageDecoder : public Common::Noncopyable {
public:
	enum Format {
		kGrayScale,
		RGB8,
		kRGBA8,

		kDXT1,
		kDXT3,
		kDXT5
	};

	enum Type {
		kTexture1D,
		kTexture2D,
		kTexture3D,
		kCubemap
	};

	struct Mipmap {
		std::vector<byte *> data;
		unsigned int dataSize, width, height, depth;
	};

	ImageDecoder();
	virtual ~ImageDecoder();

	size_t getNumLayers() const;
	[[nodiscard]] const std::vector<ImageDecoder::Mipmap> &getMipmaps(unsigned int layer = 0) const;
	[[nodiscard]] bool isCompressed() const;
	Format getFormat() const;
	Type getType() const;

protected:
	size_t getImageSize(unsigned int width, unsigned int height);

	std::vector<std::vector<Mipmap>> _layers;
	Format _format;
	Type _type;
	bool _compressed;
};

} // End of namespace Graphics

#endif //AWE_DECODER_H
