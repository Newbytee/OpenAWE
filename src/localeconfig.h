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

#ifndef AWE_LOCALECONFIG_H
#define AWE_LOCALECONFIG_H

#include <string>

#include "src/common/readstream.h"
#include "src/common/xml.h"

class LocaleConfig {
public:
	struct Entry {
		std::string folder;
		unsigned int languageId;
		unsigned int voiceoverChannel;
		bool subtitlesDefault;
		bool useOnlyOneFont;
	};

	LocaleConfig(Common::ReadStream *locale);
};


#endif //AWE_LOCALECONFIG_H
