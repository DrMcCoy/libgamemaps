/**
 * @file   util.hpp
 * @brief  Game map utility functions.
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

#ifndef _CAMOTO_GAMEMAPS_UTIL_HPP_
#define _CAMOTO_GAMEMAPS_UTIL_HPP_

#include <camoto/gamemaps/map2d.hpp>

namespace camoto {
namespace gamemaps {

/// Get the dimensions and tile size of a layer.
/**
 * @param layer
 *   Layer to query.
 *
 * @param layerWidth
 *   Stores layer width on return, in tiles.
 *
 * @param layerHeight
 *   Stores layer height on return, in tiles.
 *
 * @param tileWidth
 *   Stores tile width on return, in pixels.
 *
 * @param tileHeight
 *   Stores layer height on return, in pixels.
 */
void getLayerDims(Map2DPtr map, Map2D::LayerPtr layer, int *layerWidth,
	int *layerHeight, int *tileWidth, int *tileHeight);

} // namespace gamemaps
} // namespace camoto

#endif // _CAMOTO_GAMEMAPS_UTIL_HPP_