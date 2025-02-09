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

#ifndef AWE_TEXTUREMAN_H
#define AWE_TEXTUREMAN_H

#include <map>
#include <string>
#include <variant>

#include "src/common/singleton.h"
#include "src/common/uuid.h"

#include "src/awe/types.h"

namespace Graphics {

class TextureManager : public Common::Singleton<TextureManager> {
public:
	Common::UUID getTexture(const std::string &path);

private:
	std::map<std::variant<std::string, rid_t>, Common::UUID> _textures;
};

}

#define TextureMan TextureManager::instance()

#endif //AWE_TEXTUREMAN_H
