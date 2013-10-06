/**
 * @file   fmt-map-nukem2.cpp
 * @brief  MapType and Map2D implementation for Duke Nukem II levels.
 *
 * This file format is fully documented on the ModdingWiki:
 *   http://www.shikadi.net/moddingwiki/Duke_Nukem_II_Map_Format
 *
 * Copyright (C) 2010-2013 Adam Nielsen <malvineous@shikadi.net>
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

#include <boost/scoped_array.hpp>
#include <camoto/iostream_helpers.hpp>
#include "map2d-generic.hpp"
#include "fmt-map-nukem2.hpp"

/// Width of each tile in pixels
#define DN2_TILE_WIDTH 8

/// Height of each tile in pixels
#define DN2_TILE_HEIGHT 8

/// Width of map view during gameplay, in pixels
#define DN2_VIEWPORT_WIDTH 256

/// Height of map view during gameplay, in pixels
#define DN2_VIEWPORT_HEIGHT 160

/// Length of the map data, in bytes
#define DN2_LAYER_LEN_BG 65500u

/// Number of tiles in the map
#define DN2_NUM_TILES_BG (DN2_LAYER_LEN_BG / 2)

/// Number of tiles in the solid tileset
#define DN2_NUM_SOLID_TILES 1000

/// Number of tiles in the masked tileset
#define DN2_NUM_MASKED_TILES 160

/// Map code to write for locations with no tile set
#define DN2_DEFAULT_BGTILE 0x00

namespace camoto {
namespace gamemaps {

using namespace camoto::gamegraphics;

Map::GraphicsFilenamesPtr getNukem2GfxFilenames(const Map *map)
{
	Map::AttributePtrVectorPtr attributes = map->getAttributes();
	assert(attributes); // this map format always has attributes
	assert(attributes->size() > 0);

	Map::GraphicsFilenamesPtr files(new Map::GraphicsFilenames);
	Map::GraphicsFilename gf;
	gf.type = "tls-nukem2-czone";
	gf.filename = attributes->at(0)->filenameValue;
	if (!gf.filename.empty()) (*files)[BackgroundTileset1] = gf;

	return files;
}

class Nukem2ActorLayer: virtual public GenericMap2D::Layer
{
	public:
		Nukem2ActorLayer(ItemPtrVectorPtr& items, ItemPtrVectorPtr& validItems)
			:	GenericMap2D::Layer(
					"Actors",
					Map2D::Layer::NoCaps,
					0, 0,
					0, 0,
					items, validItems
				)
		{
		}

		virtual gamegraphics::ImagePtr imageFromCode(
			const Map2D::Layer::ItemPtr& item,
			const TilesetCollectionPtr& tileset) const
		{
			TilesetCollection::const_iterator t = tileset->find(SpriteTileset1);
			if (t == tileset->end()) return ImagePtr(); // no tileset?!

			// TODO
			const Tileset::VC_ENTRYPTR& images = t->second->getItems();
			if (item->code >= images.size()) return ImagePtr(); // out of range
			return t->second->openImage(images[item->code]);
		}
};

class Nukem2BackgroundLayer: virtual public GenericMap2D::Layer
{
	public:
		Nukem2BackgroundLayer(ItemPtrVectorPtr& items, ItemPtrVectorPtr& validItems)
			:	GenericMap2D::Layer(
					"Background",
					Map2D::Layer::NoCaps,
					0, 0,
					0, 0,
					items, validItems
				)
		{
		}

		virtual gamegraphics::ImagePtr imageFromCode(
			const Map2D::Layer::ItemPtr& item,
			const TilesetCollectionPtr& tileset) const
		{
			TilesetCollection::const_iterator t = tileset->find(BackgroundTileset1);
			if (t == tileset->end()) return ImagePtr(); // no tileset?!
			const Tileset::VC_ENTRYPTR& czoneTilesets = t->second->getItems();

			unsigned int index = item->code;
			unsigned int czoneTarget = 0;
			if (czoneTarget >= czoneTilesets.size()) return ImagePtr(); // out of range
			TilesetPtr ts = t->second->openTileset(czoneTilesets[czoneTarget]);
			const Tileset::VC_ENTRYPTR& images = ts->getItems();
			if (index >= images.size()) return ImagePtr(); // out of range
			return ts->openImage(images[index]);
		}
};

class Nukem2ForegroundLayer: virtual public GenericMap2D::Layer
{
	public:
		Nukem2ForegroundLayer(ItemPtrVectorPtr& items, ItemPtrVectorPtr& validItems)
			:	GenericMap2D::Layer(
					"Foreground",
					Map2D::Layer::NoCaps,
					0, 0,
					0, 0,
					items, validItems
				)
		{
		}

		virtual gamegraphics::ImagePtr imageFromCode(
			const Map2D::Layer::ItemPtr& item,
			const TilesetCollectionPtr& tileset) const
		{
			TilesetCollection::const_iterator t = tileset->find(BackgroundTileset1);
			if (t == tileset->end()) return ImagePtr(); // no tileset?!
			const Tileset::VC_ENTRYPTR& czoneTilesets = t->second->getItems();

			unsigned int index = item->code;
			unsigned int czoneTarget = 1;
			if (czoneTarget >= czoneTilesets.size()) return ImagePtr(); // out of range
			TilesetPtr ts = t->second->openTileset(czoneTilesets[czoneTarget]);
			const Tileset::VC_ENTRYPTR& images = ts->getItems();
			if (index >= images.size()) return ImagePtr(); // out of range
			return ts->openImage(images[index]);
		}
};


std::string Nukem2MapType::getMapCode() const
{
	return "map-nukem2";
}

std::string Nukem2MapType::getFriendlyName() const
{
	return "Duke Nukem II level";
}

std::vector<std::string> Nukem2MapType::getFileExtensions() const
{
	std::vector<std::string> vcExtensions;
	vcExtensions.push_back("mni");
	return vcExtensions;
}

std::vector<std::string> Nukem2MapType::getGameList() const
{
	std::vector<std::string> vcGames;
	vcGames.push_back("Duke Nukem II");
	return vcGames;
}

MapType::Certainty Nukem2MapType::isInstance(stream::input_sptr psMap) const
{
	stream::len lenMap = psMap->size();

	// TESTED BY: fmt_map_nukem2_isinstance_c01
	if (lenMap < 2+13+13+13+1+1+2+2 + 2+DN2_LAYER_LEN_BG) return MapType::DefinitelyNo; // too short

	psMap->seekg(0, stream::start);
	uint16_t bgOffset;
	std::string zoneFile, backFile, musFile;
	psMap
		>> u16le(bgOffset)
	;

	// TESTED BY: fmt_map_nukem2_isinstance_c02
	if (bgOffset > lenMap - (2+DN2_LAYER_LEN_BG)) return MapType::DefinitelyNo; // offset wrong

	psMap->seekg(13 * 3 + 4, stream::cur);

	uint16_t numActorInts;
	psMap >> u16le(numActorInts);

	// TESTED BY: fmt_map_nukem2_isinstance_c03
	if (2+13*3+6 + numActorInts * 2 + 2+DN2_LAYER_LEN_BG > lenMap) return MapType::DefinitelyNo; // too many actors

	psMap->seekg(bgOffset + 2+DN2_LAYER_LEN_BG, stream::start);
	uint16_t lenExtra;
	psMap >> u16le(lenExtra);
	// TESTED BY: fmt_map_nukem2_isinstance_c04
	if (bgOffset + 2+DN2_LAYER_LEN_BG + lenExtra+2 > lenMap) return MapType::DefinitelyNo; // extra data too long

	// TESTED BY: fmt_map_nukem2_isinstance_c00
	if (bgOffset + 2+DN2_LAYER_LEN_BG + lenExtra+2 + 13*3 == lenMap) return MapType::DefinitelyYes;

	// TESTED BY: fmt_map_nukem2_isinstance_c05
	return MapType::PossiblyYes;
}

MapPtr Nukem2MapType::create(SuppData& suppData) const
{
	// TODO: Implement
	throw stream::error("Not implemented yet!");
}

MapPtr Nukem2MapType::open(stream::input_sptr input, SuppData& suppData) const
{
	stream::pos lenMap = input->size();
	input->seekg(0, stream::start);

	uint16_t bgOffset, unk, numActorInts;
	std::string zoneFile, backFile, musFile;
	uint8_t flags, altBack;
	input
		>> u16le(bgOffset)
	;

	Map::AttributePtrVectorPtr attributes(new Map::AttributePtrVector());
	{
		Map::AttributePtr attr(new Map::Attribute);
		attr->type = Map::Attribute::Filename;
		attr->name = "CZone tileset";
		attr->desc = "Filename of the tileset to use for drawing the foreground and background layers";
		input >> nullPadded(attr->filenameValue, 13);
		// Trim off the padding spaces
		attr->filenameValue = attr->filenameValue.substr(0, attr->filenameValue.find_last_not_of(' ') + 1);
		attr->filenameValidExtension = "mni";
		attributes->push_back(attr);
	}

	{
		Map::AttributePtr attr(new Map::Attribute);
		attr->type = Map::Attribute::Filename;
		attr->name = "Backdrop";
		attr->desc = "Filename of the backdrop to draw behind the map";
		input >> nullPadded(attr->filenameValue, 13);
		// Trim off the padding spaces
		attr->filenameValue = attr->filenameValue.substr(0, attr->filenameValue.find_last_not_of(' ') + 1);
		attr->filenameValidExtension = "mni";
		attributes->push_back(attr);
	}

	{
		Map::AttributePtr attr(new Map::Attribute);
		attr->type = Map::Attribute::Filename;
		attr->name = "Music";
		attr->desc = "File to play as background music";
		input >> nullPadded(attr->filenameValue, 13);
		// Trim off the padding spaces
		attr->filenameValue = attr->filenameValue.substr(0, attr->filenameValue.find_last_not_of(' ') + 1);
		attr->filenameValidExtension = "imf";
		attributes->push_back(attr);
	}

	input
		>> u8(flags)
		>> u8(altBack)
		>> u16le(unk)
		>> u16le(numActorInts)
	;
	lenMap -= 2+13+13+13+1+1+2+2;

	{
		Map::AttributePtr attr(new Map::Attribute);
		attr->type = Map::Attribute::Integer;
		attr->name = "Alt backdrop";
		attr->desc = "Number of alternate backdrop file (DROPx.MNI)";
		attr->integerValue = altBack;
		attr->integerMinValue = 1;
		attr->integerMaxValue = 24;
		attributes->push_back(attr);
	}

	// Read in the actor layer
	unsigned int numActors = numActorInts / 3;
	if (lenMap < numActors * 6) throw stream::error("Map file has been truncated!");
	Map2D::Layer::ItemPtrVectorPtr actors(new Map2D::Layer::ItemPtrVector());
	actors->reserve(numActors);
	for (unsigned int i = 0; i < numActors; i++) {
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		input
			>> u16le(t->code)
			>> u16le(t->x)
			>> u16le(t->y)
		;
		actors->push_back(t);
	}
	lenMap -= 6 * numActors;

	Map2D::Layer::ItemPtrVectorPtr validActorItems(new Map2D::Layer::ItemPtrVector());
	Map2D::LayerPtr actorLayer(new Nukem2ActorLayer(actors, validActorItems));

	input->seekg(bgOffset, stream::start);
	uint16_t mapWidth;
	input
		>> u16le(mapWidth)
	;

	// Read the main layer
	Map2D::Layer::ItemPtrVectorPtr tilesBG(new Map2D::Layer::ItemPtrVector());
	Map2D::Layer::ItemPtrVectorPtr tilesFG(new Map2D::Layer::ItemPtrVector());
	unsigned int tileValues[DN2_NUM_TILES_BG];
	memset(tileValues, 0, sizeof(tileValues));

	unsigned int *v = tileValues;
	for (unsigned int i = 0; (i < DN2_NUM_TILES_BG) && (lenMap >= 2); i++) {
		input >> u16le(*v++);
		lenMap -= 2;
	}

	uint16_t lenExtra;
	input >> u16le(lenExtra);
	unsigned int extraValues[DN2_NUM_TILES_BG];
	memset(extraValues, 0, sizeof(extraValues));
	unsigned int *ev = extraValues;
	unsigned int *ev_end = extraValues + DN2_NUM_TILES_BG;
	for (unsigned int i = 0; i < lenExtra; i++) {
		uint8_t code;
		input >> u8(code);
		lenMap--;
		if (code & 0x80) {
			// Multiple bytes concatenated together
			// code == 0xFF for one byte, 0xFE for two bytes, etc.
			unsigned int len = 0x100 - code;
			while (len--) {
				input >> u8(code);
				lenMap--;
				i++;
				if (ev + 4 >= ev_end) break;
				*ev++ = (code << 5) & 0x60;
				*ev++ = (code << 3) & 0x60;
				*ev++ = (code << 1) & 0x60;
				*ev++ = (code >> 1) & 0x60;
			}
		} else {
			unsigned int len = code;
			input >> u8(code);
			lenMap--;
			i++;
			if (code == 0x00) {
				ev += len * 4; // faster
			} else {
				while (len--) {
					if (ev + 4 >= ev_end) break;
					*ev++ = (code << 5) & 0x60;
					*ev++ = (code << 3) & 0x60;
					*ev++ = (code << 1) & 0x60;
					*ev++ = (code >> 1) & 0x60;
				}
			}
		}
		if (ev + 4 > ev_end) {
			// This would read past the end of the array, so skip it
			lenMap -= lenExtra - i - 1;
			input->seekg(lenExtra - i - 1, stream::cur);
			break;
		}
	}

	v = tileValues;
	ev = extraValues;
	for (unsigned int i = 0; i < DN2_NUM_TILES_BG; i++) {
		if (*v & 0x8000) {
			// This cell has a foreground and background tile
			{
				Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
				t->type = Map2D::Layer::Item::Default;
				t->x = i % mapWidth;
				t->y = i / mapWidth;
				t->code = *v & 0x3FF;
				if (t->code != DN2_DEFAULT_BGTILE) tilesBG->push_back(t);
			}

			{
				Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
				t->type = Map2D::Layer::Item::Default;
				t->x = i % mapWidth;
				t->y = i / mapWidth;
				t->code = ((*v >> 10) & 0x1F) | *ev;
				tilesFG->push_back(t);
			}

		} else if (*v < DN2_NUM_SOLID_TILES * DN2_TILE_WIDTH) {
			// Background only tile

			Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
			t->type = Map2D::Layer::Item::Default;
			t->x = i % mapWidth;
			t->y = i / mapWidth;
			t->code = *v >> 3;
			if (t->code != DN2_DEFAULT_BGTILE) tilesBG->push_back(t);
		} else {
			// Foreground only tile

			Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
			t->type = Map2D::Layer::Item::Default;
			t->x = i % mapWidth;
			t->y = i / mapWidth;
			t->code = ((*v >> 3) - DN2_NUM_SOLID_TILES) / 5;
			tilesFG->push_back(t);
		}
		v++;
		ev++;
	}

	// Populate the list of permitted tiles
	Map2D::Layer::ItemPtrVectorPtr validBGItems(new Map2D::Layer::ItemPtrVector());
	for (unsigned int i = 0; i < DN2_NUM_SOLID_TILES; i++) {
		// Background tiles first
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		t->type = Map2D::Layer::Item::Default;
		t->x = 0;
		t->y = 0;
		t->code = i;
		validBGItems->push_back(t);
	}
	Map2D::Layer::ItemPtrVectorPtr validFGItems(new Map2D::Layer::ItemPtrVector());
	for (unsigned int i = 0; i < DN2_NUM_MASKED_TILES; i++) {
		// Then foreground tiles
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		t->type = Map2D::Layer::Item::Default;
		t->x = 0;
		t->y = 0;
		t->code = i;
		validFGItems->push_back(t);
	}

	// Trailing filenames
	{
		Map::AttributePtr attr(new Map::Attribute);
		attr->type = Map::Attribute::Filename;
		attr->name = "Zone attribute (unused)";
		attr->desc = "Filename of the zone tile attributes (unused)";
		input >> nullPadded(attr->filenameValue, 13);
		// Trim off the padding spaces
		attr->filenameValue = attr->filenameValue.substr(0, attr->filenameValue.find_last_not_of(' ') + 1);
		attr->filenameValidExtension = "mni";
		attributes->push_back(attr);
	}
	{
		Map::AttributePtr attr(new Map::Attribute);
		attr->type = Map::Attribute::Filename;
		attr->name = "Zone tileset (unused)";
		attr->desc = "Filename of the zone solid tileset (unused)";
		input >> nullPadded(attr->filenameValue, 13);
		// Trim off the padding spaces
		attr->filenameValue = attr->filenameValue.substr(0, attr->filenameValue.find_last_not_of(' ') + 1);
		attr->filenameValidExtension = "mni";
		attributes->push_back(attr);
	}
	{
		Map::AttributePtr attr(new Map::Attribute);
		attr->type = Map::Attribute::Filename;
		attr->name = "Zone masked tileset (unused)";
		attr->desc = "Filename of the zone masked tileset (unused)";
		input >> nullPadded(attr->filenameValue, 13);
		// Trim off the padding spaces
		attr->filenameValue = attr->filenameValue.substr(0, attr->filenameValue.find_last_not_of(' ') + 1);
		attr->filenameValidExtension = "mni";
		attributes->push_back(attr);
	}


	// Create the map structures
	Map2D::LayerPtr bgLayer(new Nukem2BackgroundLayer(tilesBG, validBGItems));
	Map2D::LayerPtr fgLayer(new Nukem2ForegroundLayer(tilesFG, validFGItems));

	Map2D::LayerPtrVector layers;
	layers.push_back(bgLayer);
	layers.push_back(fgLayer);
	layers.push_back(actorLayer);

	Map2DPtr map(new GenericMap2D(
		attributes, getNukem2GfxFilenames,
		Map2D::HasViewport,
		DN2_VIEWPORT_WIDTH, DN2_VIEWPORT_HEIGHT,
		mapWidth, DN2_NUM_TILES_BG / mapWidth,
		DN2_TILE_WIDTH, DN2_TILE_HEIGHT,
		layers, Map2D::PathPtrVectorPtr()
	));

	return map;
}

void Nukem2MapType::write(MapPtr map, stream::expanding_output_sptr output,
	ExpandingSuppData& suppData) const
{
	Map2DPtr map2d = boost::dynamic_pointer_cast<Map2D>(map);
	if (!map2d) throw stream::error("Cannot write this type of map as this format.");
	if (map2d->getLayerCount() != 3)
		throw stream::error("Incorrect layer count for this format.");

	// Figure out where the main data will start
	Map2D::LayerPtr layerA = map2d->getLayer(2);
	const Map2D::Layer::ItemPtrVectorPtr actors = layerA->getAllItems();
	stream::pos offBG = 2+13+13+13+1+1+2+2+6*actors->size();

	output
		<< u16le(offBG)
	;
	Map::AttributePtrVectorPtr attributes = map->getAttributes();
	// CZone
	Map::Attribute *attr = attributes->at(0).get();
	std::string val = attr->filenameValue;
	int padamt = 12 - val.length();
	val += std::string(padamt, ' '); // pad with spaces
	output << nullPadded(val, 13);

	// Backdrop
	attr = attributes->at(1).get();
	val = attr->filenameValue;
	padamt = 12 - val.length();
	val += std::string(padamt, ' '); // pad with spaces
	output << nullPadded(val, 13);

	// Song
	attr = attributes->at(2).get();
	val = attr->filenameValue;
	padamt = 12 - val.length();
	val += std::string(padamt, ' '); // pad with spaces
	output << nullPadded(val, 13);

	uint8_t flags = 0;
	output << u8(flags);

	attr = attributes->at(3).get();
	output << u8(attr->integerValue);

	output << u16le(0);

	unsigned int mapWidth, mapHeight;
	map2d->getMapSize(&mapWidth, &mapHeight);

	// Write the actor layer
	uint16_t numActorInts = actors->size() * 3;
	output << u16le(numActorInts);
	for (Map2D::Layer::ItemPtrVector::const_iterator
		i = actors->begin(); i != actors->end(); i++
	) {
		assert(((*i)->x < mapWidth) && ((*i)->y < mapHeight));
		output
			<< u16le((*i)->code)
			<< u16le((*i)->x)
			<< u16le((*i)->y)
		;
	}

	// Write the background layer
	uint16_t *bg = new uint16_t[DN2_NUM_TILES_BG];
	boost::scoped_array<uint16_t> sbg(bg);
	// Set the default background tile
	memset(bg, 0x00, DN2_NUM_TILES_BG * sizeof(uint16_t));

	uint16_t *fg = new uint16_t[DN2_NUM_TILES_BG];
	boost::scoped_array<uint16_t> sfg(fg);
	// Set the default foreground tile
	memset(fg, 0xFF, DN2_NUM_TILES_BG * sizeof(uint16_t));

	uint16_t *extra = new uint16_t[DN2_NUM_TILES_BG];
	boost::scoped_array<uint16_t> sextra(extra);
	// Set the default extra bits
	memset(extra, 0x00, DN2_NUM_TILES_BG * sizeof(uint16_t));

	Map2D::LayerPtr layerBG = map2d->getLayer(0);
	const Map2D::Layer::ItemPtrVectorPtr items = layerBG->getAllItems();
	for (Map2D::Layer::ItemPtrVector::const_iterator
		i = items->begin(); i != items->end(); i++
	) {
		assert(((*i)->x < mapWidth) && ((*i)->y < mapHeight));
		bg[(*i)->y * mapWidth + (*i)->x] = (*i)->code;
	}

	Map2D::LayerPtr layerFG = map2d->getLayer(1);
	const Map2D::Layer::ItemPtrVectorPtr itemsFG = layerFG->getAllItems();
	for (Map2D::Layer::ItemPtrVector::const_iterator
		i = itemsFG->begin(); i != itemsFG->end(); i++
	) {
		assert(((*i)->x < mapWidth) && ((*i)->y < mapHeight));
		fg[(*i)->y * mapWidth + (*i)->x] = (*i)->code;
	}

	output << u16le(mapWidth);

	assert(mapWidth * mapHeight < DN2_NUM_TILES_BG);
	for (unsigned int i = 0; i < DN2_NUM_TILES_BG; i++) {
		if (fg[i] == (uint16_t)-1) {
			// BG tile only
			output << u16le(bg[i] * 8);
		} else if (bg[i] == 0x00) {
			// FG tile only
			output << u16le((fg[i] * 5 + DN2_NUM_SOLID_TILES) * 8);
		} else {
			// BG and FG tile
			uint16_t code = 0x8000 | bg[i] | ((fg[i] & 0x1F) << 10);
			if (fg[i] & 0x60) {
				// Need to save these extra bits
				extra[i] = fg[i] & 0x60;
			}
			output << u16le(code);
		}
	}

	std::vector<int> rawExtra;
	for (unsigned int i = 0; i < DN2_NUM_TILES_BG / 4; i++) {
		rawExtra.push_back(
			  (extra[i * 4 + 0] >> 5)
			| (extra[i * 4 + 1] >> 3)
			| (extra[i * 4 + 2] >> 1)
			| (extra[i * 4 + 3] << 1)
		);
	}

	std::vector<int> rleExtra;
	std::vector<int> diffCount;
	std::vector<int>::iterator i = rawExtra.begin();
	assert(i != rawExtra.end());
	int lastByte = *i;
	int lastByteCount = 1;
	i++;
	for (; i != rawExtra.end(); i++) {
		if (lastByte == *i) {
			// Write out the different bytes so this byte will get written as at
			// least a duplicate
			while (!diffCount.empty()) {
				std::vector<int>::iterator end;
				int len;
				if (diffCount.size() > 0x7F) { // 0x80 length freezes the game
					len = 0x7F;
					end = diffCount.begin() + 0x7F;
				} else {
					len = diffCount.size();
					end = diffCount.end();
				}
				rleExtra.push_back(0x100 - len);
				rleExtra.insert(rleExtra.end(), diffCount.begin(), end);
				diffCount.erase(diffCount.begin(), end);
			}
			assert(diffCount.empty());
			lastByteCount++;
		} else {
			// This byte is different to the last
			if (lastByteCount > 1) {
				while (lastByteCount > 0) {
					int amt = std::min(0x7F, lastByteCount);
					rleExtra.push_back(amt);
					rleExtra.push_back(lastByte);
					lastByteCount -= amt;
				}
				// proceed with this new, different byte becoming lastByte
			} else {
				diffCount.push_back(lastByte);
			}
			lastByte = *i;
			lastByteCount = 1;
		}
	}
	while (!diffCount.empty()) {
		std::vector<int>::iterator end;
		int len;
		if (diffCount.size() > 0x80) {
			len = 0x80;
			end = diffCount.begin() + 0x80;
		} else {
			len = diffCount.size();
			end = diffCount.end();
		}
		rleExtra.push_back(0x100 - len);
		rleExtra.insert(rleExtra.end(), diffCount.begin(), end);
		diffCount.erase(diffCount.begin(), end);
	}
	if ((lastByte != 0x00) && (lastByteCount > 0)) {
		// Need to write out trailing repeated data
		while (lastByteCount > 0) {
			int amt = std::min(0x7F, lastByteCount);
			rleExtra.push_back(amt);
			rleExtra.push_back(lastByte);
			lastByteCount -= amt;
		}
	}
	// Last two bytes are always 0x00
	rleExtra.push_back(0x00);
	rleExtra.push_back(0x00);

	output << u16le(rleExtra.size());
	for (std::vector<int>::iterator i = rleExtra.begin(); i != rleExtra.end(); i++) {
		output << u8(*i);
	}

	// Zone attribute filename (null-padded, not space-padded)
	attr = attributes->at(4).get();
	output << nullPadded(attr->filenameValue, 13);

	// Zone solid tileset filename (null-padded, not space-padded)
	attr = attributes->at(5).get();
	output << nullPadded(attr->filenameValue, 13);

	// Zone masked tileset filename (null-padded, not space-padded)
	attr = attributes->at(6).get();
	output << nullPadded(attr->filenameValue, 13);

	output->flush();
	return;
}

SuppFilenames Nukem2MapType::getRequiredSupps(stream::input_sptr input,
	const std::string& filename) const
{
	SuppFilenames supps;
	return supps;
}

} // namespace gamemaps
} // namespace camoto