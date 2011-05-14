/**
 * @file   fmt-map-ddave.hpp
 * @brief  MapType and Map2D implementation for DDave maps.
 *
 * Copyright (C) 2010 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef _CAMOTO_GAMEMAPS_MAP_DDAVE_HPP_
#define _CAMOTO_GAMEMAPS_MAP_DDAVE_HPP_

#include <boost/shared_ptr.hpp>
#include <exception>
#include <iostream>
#include <sstream>
#include <vector>

#include <camoto/gamemaps/maptype.hpp>
#include <camoto/gamemaps/map2d.hpp>
#include <camoto/types.hpp>

namespace camoto {
namespace gamemaps {

/// Dangerous Dave level reader/writer.
class DDaveMapType: virtual public MapType {

	public:

		virtual std::string getMapCode() const
			throw ();

		virtual std::string getFriendlyName() const
			throw ();

		virtual std::vector<std::string> getFileExtensions() const
			throw ();

		virtual std::vector<std::string> getGameList() const
			throw ();

		virtual Certainty isInstance(istream_sptr psMap) const
			throw (std::ios::failure);

		virtual MapPtr create(SuppData& suppData) const
			throw (std::ios::failure);

		virtual MapPtr open(istream_sptr input, SuppData& suppData) const
			throw (std::ios::failure);

		virtual unsigned long write(MapPtr map, ostream_sptr output, SuppData& suppData) const
			throw (std::ios::failure);

};

} // namespace gamemaps
} // namespace camoto

#endif // _CAMOTO_GAMEMAPS_MAP_DDAVE_HPP_
