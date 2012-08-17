/**
 * @file   maptype.cpp
 * @brief  Implementation of base functions in Map class.
 *
 * Copyright (C) 2010-2011 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <camoto/gamemaps/maptype.hpp>

namespace camoto {
namespace gamemaps {

MapType::~MapType()
{
}

SuppFilenames MapType::getRequiredSupps(const std::string& filenameMap) const
{
	// No supplemental types/empty list
	return SuppFilenames();
}

} // namespace gamemaps
} // namespace camoto
