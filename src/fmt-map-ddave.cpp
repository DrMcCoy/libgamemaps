/**
 * @file  fmt-map-ddave.cpp
 * @brief MapType and Map2D implementation for Dangerous Dave levels.
 *
 * This file format is fully documented on the ModdingWiki:
 *   http://www.shikadi.net/moddingwiki/DDave_Map_Format
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

#include <camoto/iostream_helpers.hpp>
#include "map2d-generic.hpp"
#include "fmt-map-ddave.hpp"

#define DD_MAP_WIDTH            100
#define DD_MAP_HEIGHT           10
#define DD_TILE_WIDTH           16
#define DD_TILE_HEIGHT          16

#define DD_LAYER_OFF_PATH       0
#define DD_LAYER_LEN_PATH       256
#define DD_LAYER_OFF_BG         (DD_LAYER_OFF_PATH + DD_LAYER_LEN_PATH)
#define DD_LAYER_LEN_BG         (DD_MAP_WIDTH * DD_MAP_HEIGHT)
#define DD_PAD_LEN              24 // to round DD_LAYER_LEN_BG to nearest power of two
#define DD_FILESIZE             (DD_LAYER_LEN_PATH + DD_LAYER_LEN_BG + DD_PAD_LEN)

/// Map code to write for locations with no tile set.
#define DD_DEFAULT_BGTILE     0x00

/// This is the largest valid tile code in the background layer.
#define DD_MAX_VALID_TILECODE 52

/// This is the code used in both X and Y coords to terminate a path.
#define DD_PATH_END  0xEA

namespace camoto {
namespace gamemaps {

using namespace camoto::gamegraphics;


class Layer_DDaveBackground: virtual public GenericMap2D::Layer
{
	public:
		Layer_DDaveBackground(ItemPtrVectorPtr& items, ItemPtrVectorPtr& validItems)
			:	GenericMap2D::Layer(
					"Background",
					Map2D::Layer::NoCaps,
					0, 0,
					0, 0,
					items, validItems
				)
		{
		}

		virtual Map2D::Layer::ImageType imageFromCode(
			const Map2D::Layer::ItemPtr& item, const TilesetCollectionPtr& tileset,
			ImagePtr *out) const
		{
			TilesetCollection::const_iterator t = tileset->find(BackgroundTileset1);
			if (t == tileset->end()) return Map2D::Layer::Unknown; // no tileset?!

			const Tileset::VC_ENTRYPTR& images = t->second->getItems();
			if (item->code >= images.size()) return Map2D::Layer::Unknown; // out of range
			*out = t->second->openImage(images[item->code]);
			return Map2D::Layer::Supplied;
		}
};


class Map2D_DDave: virtual public GenericMap2D
{
	public:
		Map2D_DDave(LayerPtrVector& layers, PathPtrVectorPtr& paths)
			:	GenericMap2D(
					Map::Attributes(), Map::GraphicsFilenames(),
					Map2D::HasViewport | Map2D::HasPaths | Map2D::FixedPathCount,
					20 * DD_TILE_WIDTH, 10 * DD_TILE_HEIGHT, // viewport size
					DD_MAP_WIDTH, DD_MAP_HEIGHT,
					DD_TILE_WIDTH, DD_TILE_HEIGHT,
					layers, paths
				)
		{
		}

		Map2D::ImageAttachment getBackgroundImage(
			const TilesetCollectionPtr& tileset, ImagePtr *outImage,
			PaletteEntry *outColour) const
		{
			outColour->red = 0;
			outColour->green = 0;
			outColour->blue = 0;
			outColour->alpha = 255;
			return Map2D::SingleColour;
		}
};


std::string MapType_DDave::getMapCode() const
{
	return "map-ddave";
}

std::string MapType_DDave::getFriendlyName() const
{
	return "Dangerous Dave level";
}

std::vector<std::string> MapType_DDave::getFileExtensions() const
{
	std::vector<std::string> vcExtensions;
	vcExtensions.push_back("dav");
	return vcExtensions;
}

std::vector<std::string> MapType_DDave::getGameList() const
{
	std::vector<std::string> vcGames;
	vcGames.push_back("Dangerous Dave");
	return vcGames;
}

MapType::Certainty MapType_DDave::isInstance(stream::input_sptr psMap) const
{
	stream::pos lenMap = psMap->size();

	// TESTED BY: fmt_map_ddave_isinstance_c01
	if (lenMap != DD_FILESIZE) return MapType::DefinitelyNo; // wrong size

	// Read in the layer and make sure all the tile codes are within range
	uint8_t bg[DD_LAYER_LEN_BG];
	psMap->seekg(DD_LAYER_OFF_BG, stream::start);
	stream::len r = psMap->try_read(bg, DD_LAYER_LEN_BG);
	if (r != DD_LAYER_LEN_BG) return MapType::DefinitelyNo; // read error
	for (unsigned int i = 0; i < DD_LAYER_LEN_BG; i++) {
		// TESTED BY: fmt_map_ddave_isinstance_c02
		if (bg[i] > DD_MAX_VALID_TILECODE) return MapType::DefinitelyNo; // invalid tile
	}

	// TESTED BY: fmt_map_ddave_isinstance_c00
	return MapType::DefinitelyYes;
}

MapPtr MapType_DDave::create(SuppData& suppData) const
{
	// TODO: Implement
	throw stream::error("Not implemented yet!");
}

MapPtr MapType_DDave::open(stream::input_sptr input, SuppData& suppData) const
{
	input->seekg(0, stream::start);

	// Read the path
	uint8_t pathdata[DD_LAYER_LEN_PATH];
	input->read((char *)pathdata, DD_LAYER_LEN_PATH);
	Map2D::PathPtrVectorPtr paths(new Map2D::PathPtrVector());
	Map2D::PathPtr pathptr(new Map2D::Path());
	pathptr->fixed = true;
	pathptr->forceClosed = false;
	pathptr->maxPoints = 128;
	unsigned int nextX = 0, nextY = 0;
	for (unsigned int i = 0; i < DD_LAYER_LEN_PATH; i += 2) {
		if ((pathdata[i] == DD_PATH_END) && (pathdata[i+1] == DD_PATH_END)) break; // end of path
		nextX += (int8_t)pathdata[i];
		nextY += (int8_t)pathdata[i + 1];
		pathptr->points.push_back(Map2D::Path::point(nextX, nextY));
	}
	// Hard-code the starting point and copies of each path
	unsigned int level = 3;
	switch (level) {
		case 3:
			pathptr->start.push_back(Map2D::Path::point(44 * DD_TILE_WIDTH, 4 * DD_TILE_HEIGHT));
			pathptr->start.push_back(Map2D::Path::point(59 * DD_TILE_WIDTH, 4 * DD_TILE_HEIGHT));
			break;
	}
	paths->push_back(pathptr);

	// Read the background layer
	uint8_t bg[DD_LAYER_LEN_BG];
	input->read((char *)bg, DD_LAYER_LEN_BG);

	Map2D::Layer::ItemPtrVectorPtr tiles(new Map2D::Layer::ItemPtrVector());
	tiles->reserve(DD_MAP_WIDTH * DD_MAP_HEIGHT);
	for (unsigned int i = 0; i < DD_LAYER_LEN_BG; i++) {
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		t->type = Map2D::Layer::Item::Default;
		t->x = i % DD_MAP_WIDTH;
		t->y = i / DD_MAP_WIDTH;
		t->code = bg[i];
		if (t->code != DD_DEFAULT_BGTILE) tiles->push_back(t);
	}

	// Populate the list of permitted tiles
	Map2D::Layer::ItemPtrVectorPtr validBGItems(new Map2D::Layer::ItemPtrVector());
	for (unsigned int i = 0; i <= DD_MAX_VALID_TILECODE; i++) {
		// The default tile actually has an image, so don't exclude it
		if (i == DD_DEFAULT_BGTILE) continue;

		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		t->type = Map2D::Layer::Item::Default;
		t->x = 0;
		t->y = 0;
		t->code = i;
		validBGItems->push_back(t);
	}

	// Create the map structures
	Map2D::LayerPtr bgLayer(new Layer_DDaveBackground(tiles, validBGItems));

	Map2D::LayerPtrVector layers;
	layers.push_back(bgLayer);

	Map2DPtr map(new Map2D_DDave(layers, paths));
	return map;
}

void MapType_DDave::write(MapPtr map, stream::expanding_output_sptr output,
	ExpandingSuppData& suppData) const
{
	Map2DPtr map2d = boost::dynamic_pointer_cast<Map2D>(map);
	if (!map2d) throw stream::error("Cannot write this type of map as this format.");
	if (map2d->getLayerCount() != 1)
		throw stream::error("Incorrect layer count for this format.");

	// Write the path
	uint8_t path[DD_LAYER_LEN_PATH];
	memset(path, 0, DD_LAYER_LEN_PATH);
	Map2D::PathPtrVectorPtr paths = map2d->getPaths();
	if (paths->size() != 1) throw stream::error("Incorrect path count for this format.");
	Map2D::PathPtr first_path = paths->at(0);
	unsigned int pathpos = 0;
	unsigned int lastX = 0, lastY = 0;
	int8_t x = 0, y = 0;
	for (Map2D::Path::point_vector::const_iterator i = first_path->points.begin();
		i != first_path->points.end();
		i++
	) {
		if (pathpos > 256) throw stream::error("Path too long (max 128 segments)");

		// Convert from relative to (0,0), to relative to previous point
		// Have to cast these to int8_t first so they're 8-bit but the sign is kept
		// intact (-1 still is -1) then to uint8_t so -1 becomes 255.
		x = i->first - lastX;
		y = i->second - lastY;
		lastX = i->first;
		lastY = i->second;

		if (((uint8_t)x == DD_PATH_END) && ((uint8_t)y == DD_PATH_END)) {
			// Can't write these magic values, so tweak the data slightly
			lastY++;
			y++;
			// This should work fine unless this is the last point in the path, but
			// the condition below will catch that.
		}
		path[pathpos++] = (uint8_t)x;
		path[pathpos++] = (uint8_t)y;
	}
	if (((uint8_t)x == DD_PATH_END) && ((uint8_t)y == DD_PATH_END)) {
		throw stream::error("The last point in the path happens to have a "
			"special magic offset that cannot be saved in a Dangerous Dave map.  "
			"Please move the last or second last point by at least one pixel.");
	}

	// Add terminator if there's enough room.
	// TODO: Test to see if this is correct, or if a terminator is always required
	if (pathpos < 256) {
		path[pathpos++] = DD_PATH_END;
		path[pathpos++] = DD_PATH_END;
	}
	output->write((char *)path, DD_LAYER_LEN_PATH);

	// Write the background layer
	uint8_t bg[DD_LAYER_LEN_BG];
	memset(bg, DD_DEFAULT_BGTILE, DD_LAYER_LEN_BG); // default background tile
	Map2D::LayerPtr layer = map2d->getLayer(0);
	const Map2D::Layer::ItemPtrVectorPtr items = layer->getAllItems();
	for (Map2D::Layer::ItemPtrVector::const_iterator i = items->begin();
		i != items->end();
		i++
	) {
		if (((*i)->x > DD_MAP_WIDTH) || ((*i)->y > DD_MAP_HEIGHT)) {
			throw stream::error("Layer has tiles outside map boundary!");
		}
		bg[(*i)->y * DD_MAP_WIDTH + (*i)->x] = (*i)->code;
	}
	output->write((char *)bg, DD_LAYER_LEN_BG);

	// Write out padding
	uint8_t pad[DD_PAD_LEN];
	memset(pad, 0, DD_PAD_LEN);
	output->write((char *)pad, DD_PAD_LEN);

	output->flush();
	return;
}

SuppFilenames MapType_DDave::getRequiredSupps(stream::input_sptr input,
	const std::string& filename) const
{
	SuppFilenames supps;
	return supps;
}

} // namespace gamemaps
} // namespace camoto
