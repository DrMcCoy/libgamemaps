/**
 * @file   fmt-map-ccaves.hpp
 * @brief  MapType and Map2D implementation for Crystal Caves maps.
 *
 * Copyright (C) 2010-2012 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef _CAMOTO_GAMEMAPS_MAP_CCAVES_HPP_
#define _CAMOTO_GAMEMAPS_MAP_CCAVES_HPP_

#include "base-maptype.hpp"
#include "map2d-generic.hpp"

namespace camoto {
namespace gamemaps {

/// Crystal Caves level reader/writer.
class CCavesMapType: virtual public BaseMapType
{
	public:
		virtual std::string getMapCode() const;
		virtual std::string getFriendlyName() const;
		virtual std::vector<std::string> getFileExtensions() const;
		virtual std::vector<std::string> getGameList() const;
		virtual Certainty isInstance(stream::input_sptr psMap) const;
		virtual MapPtr create(SuppData& suppData) const;
		virtual MapPtr open(stream::input_sptr input, SuppData& suppData) const;
		virtual void write(MapPtr map, stream::expanding_output_sptr output,
			ExpandingSuppData& suppData) const;
		virtual SuppFilenames getRequiredSupps(stream::input_sptr input,
			const std::string& filename) const;
};

class CCavesBackgroundLayer: virtual public GenericMap2D::Layer
{
	public:
		CCavesBackgroundLayer(ItemPtrVectorPtr& items,
			ItemPtrVectorPtr& validItems);

		virtual gamegraphics::ImagePtr imageFromCode(unsigned int code,
			camoto::gamegraphics::VC_TILESET& tileset);
};

} // namespace gamemaps
} // namespace camoto

#endif // _CAMOTO_GAMEMAPS_MAP_CCAVES_HPP_
