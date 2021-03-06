/**
 * @file  fmt-map-xargon.hpp
 * @brief MapType and Map2D implementation for Jill of the Jungle and Xargon.
 *
 * Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef _CAMOTO_GAMEMAPS_MAP_XARGON_HPP_
#define _CAMOTO_GAMEMAPS_MAP_XARGON_HPP_

#include <map>
#include "base-maptype.hpp"

namespace camoto {
namespace gamemaps {

/// Generic level reader/writer for games based on Tim Sweeney's Jill engine.
class MapType_Sweeney: virtual public MapType_Base
{
	public:
		virtual Certainty isInstance(stream::input_sptr psMap) const;
		virtual MapPtr create(SuppData& suppData) const;
		virtual MapPtr open(stream::input_sptr input, SuppData& suppData) const;
		virtual void write(MapPtr map, stream::expanding_output_sptr output,
			ExpandingSuppData& suppData) const;

		typedef std::map<uint16_t, uint16_t> image_map;
		typedef boost::shared_ptr<image_map> image_map_sptr;

	protected:
		unsigned int viewportWidth;
		unsigned int viewportHeight;
		unsigned int lenSavedata;
};

/// Jill of the Jungle level reader/writer.
class MapType_Jill: virtual public MapType_Sweeney
{
	public:
		MapType_Jill();

		virtual std::string getMapCode() const;
		virtual std::string getFriendlyName() const;
		virtual std::vector<std::string> getFileExtensions() const;
		virtual std::vector<std::string> getGameList() const;
		virtual SuppFilenames getRequiredSupps(stream::input_sptr input,
			const std::string& filename) const;
};

/// Xargon level reader/writer.
class MapType_Xargon: virtual public MapType_Sweeney
{
	public:
		MapType_Xargon();

		virtual std::string getMapCode() const;
		virtual std::string getFriendlyName() const;
		virtual std::vector<std::string> getFileExtensions() const;
		virtual std::vector<std::string> getGameList() const;
		virtual SuppFilenames getRequiredSupps(stream::input_sptr input,
			const std::string& filename) const;
};

} // namespace gamemaps
} // namespace camoto

#endif // _CAMOTO_GAMEMAPS_MAP_XARGON_HPP_
