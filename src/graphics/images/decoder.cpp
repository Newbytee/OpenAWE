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

#include <cmath>
#include "decoder.h"

namespace Graphics {

ImageDecoder::ImageDecoder() : _format(kRGBA8), _compressed(false), _type(kTexture2D) {

}

ImageDecoder::~ImageDecoder() {
	for (const auto &layer : _layers) {
		for (const auto &mipmap : layer) {
			for (const auto &data : mipmap.data) {
				delete[] data;
			}
		}
	}
}

size_t ImageDecoder::getNumLayers() const {
	return _layers.size();
}

const std::vector<ImageDecoder::Mipmap> &ImageDecoder::getMipmaps(unsigned int layer) const {
	return _layers[layer];
}

bool ImageDecoder::isCompressed() const {
	return _compressed;
}

ImageDecoder::Format ImageDecoder::getFormat() const {
	return _format;
}

ImageDecoder::Type ImageDecoder::getType() const {
	return _type;
}

size_t ImageDecoder::getImageSize(unsigned int width, unsigned int height) {
	switch (_format) {
		case kGrayScale:
			return width * height;

		case RGB8:
			return width * height * 3;

		case kRGBA8:
			return width * height * 4;

		case kDXT1:
			return std::max(8u, ((width + 3) / 4) * ((height + 3) / 4) * 8);

		case kDXT3:
		case kDXT5:
			return std::max(16u, ((width + 3) / 4) * ((height + 3) / 4) * 16);

		default:
			return 0;
	}
}

}
