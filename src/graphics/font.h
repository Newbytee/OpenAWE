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

#ifndef AWE_FONT_H
#define AWE_FONT_H

#include "src/common/uuid.h"

namespace Graphics {

class Font {
public:
	Common::UUID getTexture() const;
	Common::UUID getVertices() const;
	Common::UUID getIndices() const;
	Common::UUID getVAO() const;

	virtual void getCharacterValues(char16_t c, uint16_t &vertexOffset, uint16_t &indexOffset, uint16_t &indexCount,
									float &characterWidth) = 0;

protected:
	Font();

	Common::UUID _vertices, _indices, _texture, _vao;
};

} // End of namespace Graphics

#endif //AWE_FONT_H
