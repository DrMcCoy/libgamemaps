/**
 * @file   fmt-map-wordresc.cpp
 * @brief  MapType and Map2D implementation for Word Rescue levels.
 *
 * This file format is fully documented on the ModdingWiki:
 *   http://www.shikadi.net/moddingwiki/Word_Rescue
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

#include <boost/scoped_array.hpp>
#include <camoto/iostream_helpers.hpp>
#include "map2d-generic.hpp"
#include <camoto/util.hpp>
#include "fmt-map-wordresc.hpp"

/// Width of tiles in background layer
#define WR_BGTILE_WIDTH           16
/// Height of tiles in background layer
#define WR_BGTILE_HEIGHT          16

/// Width of tiles in attribute layer
#define WR_ATTILE_WIDTH           8
/// Height of tiles in attribute layer
#define WR_ATTILE_HEIGHT          8

/// Map code to write for locations with no tile set.
#define WR_DEFAULT_BGTILE       0xFF

/// Map code to write for locations with no tile set.
#define WR_DEFAULT_ATTILE       0x20

/// This is the largest valid tile code in the background layer.
#define WR_MAX_VALID_TILECODE   240

// Internal codes for various items
#define WR_CODE_GRUZZLE  1
#define WR_CODE_SLIME    2
#define WR_CODE_BOOK     3
#define WR_CODE_ENTRANCE 4
#define WR_CODE_EXIT     5
#define WR_CODE_LETTER   6
#define WR_CODE_LETTER1  6 // same as WR_CODE_LETTER
#define WR_CODE_LETTER2  7
#define WR_CODE_LETTER3  8
#define WR_CODE_LETTER4  9
#define WR_CODE_LETTER5  10
#define WR_CODE_LETTER6  11
#define WR_CODE_LETTER7  12

/// Fixed number of letters in each map (to spell a word)
#define WR_NUM_LETTERS   7

// Values used when writing items (also in isinstance)
#define INDEX_GRUZZLE 0
#define INDEX_UNKNOWN 1
#define INDEX_SLIME   2
#define INDEX_BOOK    3
#define INDEX_LETTER  4
#define INDEX_ANIM    5
#define INDEX_END     6
#define INDEX_SIZE    7

namespace camoto {
namespace gamemaps {

using namespace camoto::gamegraphics;

WordRescueBackgroundLayer::WordRescueBackgroundLayer(ItemPtrVectorPtr& items,
	ItemPtrVectorPtr& validItems)
	:	GenericMap2D::Layer(
			"Background",
			Map2D::Layer::NoCaps,
			0, 0,
			0, 0,
			items, validItems
		)
{
}

ImagePtr WordRescueBackgroundLayer::imageFromCode(unsigned int code,
	VC_TILESET& tileset)
{
	if (tileset.size() <= 0) return ImagePtr();
	const Tileset::VC_ENTRYPTR& images = tileset[0]->getItems();
	if (code >= images.size()) return ImagePtr(); // out of range
	return tileset[0]->openImage(images[code]);
}


WordRescueObjectLayer::WordRescueObjectLayer(ItemPtrVectorPtr& items,
	ItemPtrVectorPtr& validItems)
	:	GenericMap2D::Layer(
			"Items",
			Map2D::Layer::NoCaps,
			0, 0,
			0, 0,
			items, validItems
		)
{
}

ImagePtr WordRescueObjectLayer::imageFromCode(unsigned int code,
	VC_TILESET& tileset)
{
	unsigned int t;
	switch (code) {
		case WR_CODE_GRUZZLE:  t = 1; code = 15; break;
		case WR_CODE_SLIME:    t = 0; code = 238; break;
		case WR_CODE_BOOK:     t = 0; code = 239; break;
		case WR_CODE_ENTRANCE: t = 1; code = 1; break;
		case WR_CODE_EXIT:     t = 1; code = 3; break;
		case WR_CODE_LETTER1:
		case WR_CODE_LETTER2:
		case WR_CODE_LETTER3:
		case WR_CODE_LETTER4:
		case WR_CODE_LETTER5:
		case WR_CODE_LETTER6:
		case WR_CODE_LETTER7:
			t = 1;
			code += 31;
			code -= WR_CODE_LETTER;
			break;
		default: return ImagePtr();
	}
	if (tileset.size() <= t) return ImagePtr();
	const Tileset::VC_ENTRYPTR& images = tileset[t]->getItems();
	if (code >= images.size()) return ImagePtr(); // out of range
	return tileset[t]->openImage(images[code]);
}

bool WordRescueObjectLayer::tilePermittedAt(unsigned int code,
	unsigned int x, unsigned int y, unsigned int *maxCodes)
{
	if ((code == WR_CODE_ENTRANCE) || (code == WR_CODE_EXIT)) {
		*maxCodes = 1; // only one level entrance/exit permitted
	} else {
		*maxCodes = 0; // unlimited
	}
	return true; // anything can be placed anywhere
}


WordRescueAttributeLayer::WordRescueAttributeLayer(ItemPtrVectorPtr& items,
	ItemPtrVectorPtr& validItems)
	:	GenericMap2D::Layer(
			"Attributes",
			Map2D::Layer::HasOwnTileSize,
			0, 0,
			WR_ATTILE_WIDTH, WR_ATTILE_HEIGHT,
			items, validItems
		)
{
}

ImagePtr WordRescueAttributeLayer::imageFromCode(unsigned int code,
	VC_TILESET& tileset)
{
	unsigned int t;
	switch (code) {
		case 0x0000: t = 1; code = 0; break; // first question mark box
		case 0x0001: t = 1; code = 0; break;
		case 0x0002: t = 1; code = 0; break;
		case 0x0003: t = 1; code = 0; break;
		case 0x0004: t = 1; code = 0; break;
		case 0x0005: t = 1; code = 0; break;
		case 0x0006: t = 1; code = 0; break; // last question mark box
		case 0x0073: t = 0; code = 50; break; // solid
		case 0x0074: t = 0; code = 91; break; // jump up through/climb
		case 0x00FD: return ImagePtr(); // what is this? end of layer flag?
		default: return ImagePtr();
	}
	if (tileset.size() <= t) return ImagePtr();
	const Tileset::VC_ENTRYPTR& images = tileset[t]->getItems();
	if (code >= images.size()) return ImagePtr(); // out of range
	return tileset[t]->openImage(images[code]);
}

bool WordRescueAttributeLayer::tilePermittedAt(unsigned int code,
	unsigned int x, unsigned int y, unsigned int *maxCodes)
{
	if (x == 0) return false; // can't place tiles in this column
	return true; // otherwise unrestricted
}


Map::FilenameVectorPtr wr_getGraphicsFilenames(const Map *map)
{
	Map::AttributePtrVectorPtr attributes = map->getAttributes();
	assert(attributes); // this map format always has attributes
	assert(attributes->size() == 3);

	Map::FilenameVectorPtr files(new Map::FilenameVector);
	Map::GraphicsFilename gf;
	gf.purpose = Map::GraphicsFilename::Tileset;
	gf.type = "tls-wordresc";
	gf.filename = createString("back" << (int)(attributes->at(1)->enumValue + 1)
		<< ".wr");
	files->push_back(gf); // bg tiles

	unsigned int dropNum = attributes->at(2)->enumValue;
	if (dropNum > 0) {
		gf.purpose = Map::GraphicsFilename::BackgroundImage;
		gf.filename = createString("drop" << dropNum << ".wr");
		files->push_back(gf); // fg tiles
	}

	return files;
}


/// Write the given data to the stream, RLE encoded
int rleWrite(stream::output_sptr output, uint8_t *data, int len)
{
	int lenWritten = 0;

	// RLE encode the data
	uint8_t lastCount = 0;
	uint8_t lastCode = data[0];
	for (int i = 0; i < len; i++) {
		if (data[i] == lastCode) {
			if (lastCount == 0xFF) {
				output
					<< u8(lastCount)
					<< u8(lastCode)
				;
				lenWritten += 2;
				lastCount = 1;
			} else {
				lastCount++;
			}
		} else {
			output
				<< u8(lastCount)
				<< u8(lastCode)
			;
			lenWritten += 2;
			lastCode = data[i];
			lastCount = 1;
		}
	}
	// Write out the last tile
	if (lastCount > 0) {
		output
			<< u8(lastCount)
			<< u8(lastCode)
		;
		lenWritten += 2;
	}

	return lenWritten;
}



std::string WordRescueMapType::getMapCode() const
{
	return "map-wordresc";
}

std::string WordRescueMapType::getFriendlyName() const
{
	return "Word Rescue level";
}

std::vector<std::string> WordRescueMapType::getFileExtensions() const
{
	std::vector<std::string> vcExtensions;
	vcExtensions.push_back("s0");
	vcExtensions.push_back("s1");
	vcExtensions.push_back("s2");
	vcExtensions.push_back("s3");
	vcExtensions.push_back("s4");
	vcExtensions.push_back("s5");
	vcExtensions.push_back("s6");
	vcExtensions.push_back("s7");
	vcExtensions.push_back("s8");
	vcExtensions.push_back("s9");
	vcExtensions.push_back("s10");
	vcExtensions.push_back("s11");
	vcExtensions.push_back("s12");
	vcExtensions.push_back("s13");
	vcExtensions.push_back("s14");
	vcExtensions.push_back("s15");
	vcExtensions.push_back("s16");
	vcExtensions.push_back("s17");
	vcExtensions.push_back("s18");
	vcExtensions.push_back("s19");
	return vcExtensions;
}

std::vector<std::string> WordRescueMapType::getGameList() const
{
	std::vector<std::string> vcGames;
	vcGames.push_back("Word Rescue");
	return vcGames;
}

MapType::Certainty WordRescueMapType::isInstance(stream::input_sptr psMap) const
{
	stream::pos lenMap = psMap->size();

#define WR_MIN_HEADER_SIZE (2*15 + 4*7) // includes INDEX_LETTER

	// Make sure file is large enough for the header
	// TESTED BY: fmt_map_wordresc_isinstance_c01
	if (lenMap < WR_MIN_HEADER_SIZE) return MapType::DefinitelyNo;

	uint16_t mapWidth, mapHeight;
	psMap->seekg(0, stream::start);
	psMap
		>> u16le(mapWidth)
		>> u16le(mapHeight)
	;
	psMap->seekg(2*7, stream::cur);

	// Check the items are each within range
	unsigned int minSize = WR_MIN_HEADER_SIZE;
	for (unsigned int i = 0; i < INDEX_SIZE; i++) {
		uint16_t count;
		if (i == INDEX_LETTER) {
			psMap->seekg(WR_NUM_LETTERS * 4, stream::cur);
			continue; // hard coded, included above
		}

		psMap >> u16le(count);
		minSize += count * 4;
		// Don't need to count the u16le in minSize, it's in WR_MIN_HEADER_SIZE

		// Make sure the item count is within range
		// TESTED BY: fmt_map_wordresc_isinstance_c02
		if (lenMap < minSize) return MapType::DefinitelyNo;
		psMap->seekg(count * 4, stream::cur);
	}

	// Read in the layer and make sure all the tile codes are within range
	for (int i = 0; i < mapWidth * mapHeight; ) {
		uint8_t num, code;
		minSize += 2;
		// Make sure the background layer isn't cut off
		// TESTED BY: fmt_map_wordresc_isinstance_c03
		if (lenMap < minSize) return MapType::DefinitelyNo;

		psMap >> u8(num) >> u8(code);
		i += num;

		// Ignore the default tile (otherwise it would be out of range)
		if (code == WR_DEFAULT_BGTILE) continue;

		// Make sure the tile values are within range
		// TESTED BY: fmt_map_wordresc_isinstance_c04
		if (code > WR_MAX_VALID_TILECODE) return MapType::DefinitelyNo;
	}

	// TESTED BY: fmt_map_wordresc_isinstance_c00
	return MapType::DefinitelyYes;
}

MapPtr WordRescueMapType::create(SuppData& suppData) const
{
	// TODO: Implement
	throw stream::error("Not implemented yet!");
}

MapPtr WordRescueMapType::open(stream::input_sptr input, SuppData& suppData) const
{
	input->seekg(0, stream::start);

	uint16_t mapWidth, mapHeight;
	uint16_t bgColour; // EGA 0-15
	uint16_t tileset; // 3 == suburban, 2 == medieval (backX.wr)
	uint16_t backdrop; // dropX.wr, 0 == none
	uint16_t startX, startY, endX, endY;
	input
		>> u16le(mapWidth)
		>> u16le(mapHeight)
		>> u16le(bgColour)
		>> u16le(tileset)
		>> u16le(backdrop)
		>> u16le(startX)
		>> u16le(startY)
		>> u16le(endX)
		>> u16le(endY)
	;

	Map::AttributePtrVectorPtr attributes(new Map::AttributePtrVector());
	Map::AttributePtr attrBGColour(new Map::Attribute);
	attrBGColour->type = Map::Attribute::Enum;
	attrBGColour->name = "Background colour";
	attrBGColour->desc = "Colour to draw where there are no tiles.  Only used if backdrop "
		"is not set.";
	attrBGColour->enumValue = bgColour;
	attrBGColour->enumValueNames.push_back("EGA 0 - Black");
	attrBGColour->enumValueNames.push_back("EGA 1 - Dark blue");
	attrBGColour->enumValueNames.push_back("EGA 2 - Dark green");
	attrBGColour->enumValueNames.push_back("EGA 3 - Dark cyan");
	attrBGColour->enumValueNames.push_back("EGA 4 - Dark red");
	attrBGColour->enumValueNames.push_back("EGA 5 - Dark magenta");
	attrBGColour->enumValueNames.push_back("EGA 6 - Brown");
	attrBGColour->enumValueNames.push_back("EGA 7 - Light grey");
	attrBGColour->enumValueNames.push_back("EGA 8 - Dark grey");
	attrBGColour->enumValueNames.push_back("EGA 9 - Light blue");
	attrBGColour->enumValueNames.push_back("EGA 10 - Light green");
	attrBGColour->enumValueNames.push_back("EGA 11 - Light cyan");
	attrBGColour->enumValueNames.push_back("EGA 12 - Light red");
	attrBGColour->enumValueNames.push_back("EGA 13 - Light magenta");
	attrBGColour->enumValueNames.push_back("EGA 14 - Yellow");
	attrBGColour->enumValueNames.push_back("EGA 15 - White");
	attributes->push_back(attrBGColour);

	Map::AttributePtr attrTileset(new Map::Attribute);
	attrTileset->type = Map::Attribute::Enum;
	attrTileset->name = "Tileset";
	attrTileset->desc = "Tileset to use for this map";
	if (tileset > 0) tileset--; // just in case it *is* ever zero
	attrTileset->enumValue = tileset;
	attrTileset->enumValueNames.push_back("Desert");
	attrTileset->enumValueNames.push_back("Castle");
	attrTileset->enumValueNames.push_back("Suburban");
	attrTileset->enumValueNames.push_back("Spooky (episode 3 only)");
	attrTileset->enumValueNames.push_back("Industrial");
	attrTileset->enumValueNames.push_back("Custom (back6.wr)");
	attrTileset->enumValueNames.push_back("Custom (back7.wr)");
	attrTileset->enumValueNames.push_back("Custom (back8.wr)");
	attributes->push_back(attrTileset);

	Map::AttributePtr attrBackdrop(new Map::Attribute);
	attrBackdrop->type = Map::Attribute::Enum;
	attrBackdrop->name = "Backdrop";
	attrBackdrop->desc = "Image to show behind map (overrides background colour)";
	attrBackdrop->enumValue = backdrop;
	attrBackdrop->enumValueNames.push_back("None (use background colour)");
	attrBackdrop->enumValueNames.push_back("Custom (drop1.wr)");
	attrBackdrop->enumValueNames.push_back("Cave (episodes 2-3 only)");
	attrBackdrop->enumValueNames.push_back("Desert");
	attrBackdrop->enumValueNames.push_back("Mountain");
	attrBackdrop->enumValueNames.push_back("Custom (drop5.wr)");
	attrBackdrop->enumValueNames.push_back("Custom (drop6.wr)");
	attrBackdrop->enumValueNames.push_back("Custom (drop7.wr)");
	attributes->push_back(attrBackdrop);

	Map2D::Layer::ItemPtrVectorPtr items(new Map2D::Layer::ItemPtrVector());

	// Add the map entrance and exit as special items
	{
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		t->x = startX / 2;
		t->y = startY / 2;
		t->code = WR_CODE_ENTRANCE;
		items->push_back(t);
	}
	{
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		t->x = endX / 2;
		t->y = endY / 2;
		t->code = WR_CODE_EXIT;
		items->push_back(t);
	}

	uint16_t gruzzleCount;
	input >> u16le(gruzzleCount);
	items->reserve(items->size() + gruzzleCount);
	for (unsigned int i = 0; i < gruzzleCount; i++) {
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		input
			>> u16le(t->x)
			>> u16le(t->y)
		;
		t->code = WR_CODE_GRUZZLE;
		items->push_back(t);
	}

	uint16_t unknownCount;
	input >> u16le(unknownCount);
	input->seekg(unknownCount * 4, stream::cur);

	uint16_t slimeCount;
	input >> u16le(slimeCount);
	items->reserve(items->size() + slimeCount);
	for (unsigned int i = 0; i < slimeCount; i++) {
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		input
			>> u16le(t->x)
			>> u16le(t->y)
		;
		t->code = WR_CODE_SLIME;
		items->push_back(t);
	}

	uint16_t bookCount;
	input >> u16le(bookCount);
	items->reserve(items->size() + bookCount + 7);
	for (unsigned int i = 0; i < bookCount; i++) {
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		input
			>> u16le(t->x)
			>> u16le(t->y)
		;
		t->code = WR_CODE_BOOK;
		items->push_back(t);
	}

	for (unsigned int i = 0; i < WR_NUM_LETTERS; i++) {
		Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
		input
			>> u16le(t->x)
			>> u16le(t->y)
		;
		t->code = WR_CODE_LETTER + i;
		items->push_back(t);
	}

	uint16_t animCount;
	input >> u16le(animCount);
	input->seekg(animCount * 4, stream::cur);
	// TODO: Figure out something with animated tiles

	// Skip over trailing 0x0000
	input->seekg(2, stream::cur);

	Map2D::Layer::ItemPtrVectorPtr validItemItems(new Map2D::Layer::ItemPtrVector());
	Map2D::LayerPtr itemLayer(new WordRescueObjectLayer(items, validItemItems));

	// Read the background layer
	Map2D::Layer::ItemPtrVectorPtr tiles(new Map2D::Layer::ItemPtrVector());
	tiles->reserve(mapWidth * mapHeight);
	for (int i = 0; i < mapWidth * mapHeight; ) {
		uint8_t num, code;
		input >> u8(num) >> u8(code);
		if (code == WR_DEFAULT_BGTILE) {
			i += num;
		} else {
			while (num-- > 0) {
				Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
				t->x = i % mapWidth;
				t->y = i / mapWidth;
				t->code = code;
				tiles->push_back(t);
				i++;
			}
		}
	}

	Map2D::Layer::ItemPtrVectorPtr validBGItems(new Map2D::Layer::ItemPtrVector());
	Map2D::LayerPtr bgLayer(new WordRescueBackgroundLayer(tiles, validBGItems));

	// Read the attribute layer
	Map2D::Layer::ItemPtrVectorPtr atItems(new Map2D::Layer::ItemPtrVector());
	uint16_t atWidth = mapWidth * 2;
	uint16_t atHeight = mapHeight * 2;
	atItems->reserve(atWidth * atHeight);
	for (int i = 0; i < atWidth * atHeight; ) {
		uint8_t num, code;
		input >> u8(num) >> u8(code);
		if (code == WR_DEFAULT_ATTILE) {
			i += num;
		} else {
			while (num-- > 0) {
				Map2D::Layer::ItemPtr t(new Map2D::Layer::Item());
				t->x = i % atWidth + 1;
				t->y = i / atWidth;
				t->code = code;
				atItems->push_back(t);
				i++;
			}
		}
	}
	Map2D::Layer::ItemPtrVectorPtr validAtItems(new Map2D::Layer::ItemPtrVector());
	Map2D::LayerPtr atLayer(new WordRescueAttributeLayer(atItems, validAtItems));

	Map2D::LayerPtrVector layers;
	layers.push_back(bgLayer);
	layers.push_back(atLayer);
	layers.push_back(itemLayer);

	Map2DPtr map(new GenericMap2D(
		attributes, wr_getGraphicsFilenames,
		Map2D::HasViewport,
		288, 152, // viewport
		mapWidth, mapHeight,
		WR_BGTILE_WIDTH, WR_BGTILE_HEIGHT,
		layers, Map2D::PathPtrVectorPtr()
	));

	return map;
}

void WordRescueMapType::write(MapPtr map, stream::expanding_output_sptr output,
	ExpandingSuppData& suppData) const
{
	Map2DPtr map2d = boost::dynamic_pointer_cast<Map2D>(map);
	if (!map2d) throw stream::error("Cannot write this type of map as this format.");
	if (map2d->getLayerCount() != 3)
		throw stream::error("Incorrect layer count for this format.");

	unsigned int mapWidth, mapHeight;
	map2d->getMapSize(&mapWidth, &mapHeight);

	Map::AttributePtrVectorPtr attributes = map->getAttributes();
	if (attributes->size() != 3) {
		throw stream::error("Cannot write map as there is an incorrect number "
			"of attributes set.");
	}

	Map::Attribute *attrBG = attributes->at(0).get();
	if (attrBG->type != Map::Attribute::Enum) {
		throw stream::error("Cannot write map as there is an attribute of the "
			"wrong type (bg != enum)");
	}
	uint16_t bgColour = attrBG->enumValue;

	Map::Attribute *attrTileset = attributes->at(1).get();
	if (attrTileset->type != Map::Attribute::Enum) {
		throw stream::error("Cannot write map as there is an attribute of the "
			"wrong type (tileset != enum)");
	}
	uint16_t tileset = attrTileset->enumValue + 1;

	Map::Attribute *attrBackdrop = attributes->at(2).get();
	if (attrBackdrop->type != Map::Attribute::Enum) {
		throw stream::error("Cannot write map as there is an attribute of the "
			"wrong type (backdrop != enum)");
	}
	uint16_t backdrop = attrBackdrop->enumValue;

	typedef std::pair<uint16_t, uint16_t> point;
	std::vector<point> itemLocations[INDEX_SIZE];

	// Prefill the letter vector with the fixed number of letters
	for (unsigned int i = 0; i < WR_NUM_LETTERS; i++) itemLocations[INDEX_LETTER].push_back(point(0, 0));

	uint16_t startX = 0;
	uint16_t startY = 0;
	uint16_t endX = 0;
	uint16_t endY = 0;

	Map2D::LayerPtr layer = map2d->getLayer(2);
	const Map2D::Layer::ItemPtrVectorPtr items = layer->getAllItems();
	for (Map2D::Layer::ItemPtrVector::const_iterator i = items->begin();
		i != items->end();
		i++
	) {
		switch ((*i)->code) {
			case WR_CODE_GRUZZLE: itemLocations[INDEX_GRUZZLE].push_back(point((*i)->x, (*i)->y)); break;
			case WR_CODE_SLIME:   itemLocations[INDEX_SLIME].push_back(point((*i)->x, (*i)->y)); break;
			case WR_CODE_BOOK:    itemLocations[INDEX_BOOK].push_back(point((*i)->x, (*i)->y)); break;
			case WR_CODE_LETTER1: itemLocations[INDEX_LETTER][0] = point((*i)->x, (*i)->y); break;
			case WR_CODE_LETTER2: itemLocations[INDEX_LETTER][1] = point((*i)->x, (*i)->y); break;
			case WR_CODE_LETTER3: itemLocations[INDEX_LETTER][2] = point((*i)->x, (*i)->y); break;
			case WR_CODE_LETTER4: itemLocations[INDEX_LETTER][3] = point((*i)->x, (*i)->y); break;
			case WR_CODE_LETTER5: itemLocations[INDEX_LETTER][4] = point((*i)->x, (*i)->y); break;
			case WR_CODE_LETTER6: itemLocations[INDEX_LETTER][5] = point((*i)->x, (*i)->y); break;
			case WR_CODE_LETTER7: itemLocations[INDEX_LETTER][6] = point((*i)->x, (*i)->y); break;
			case WR_CODE_ENTRANCE:
				startX = (*i)->x * 2;
				startY = (*i)->y * 2;
				break;
			case WR_CODE_EXIT:
				endX = (*i)->x * 2;
				endY = (*i)->y * 2;
				break;
		}
	}

	output
		<< u16le(mapWidth)
		<< u16le(mapHeight)
		<< u16le(bgColour)
		<< u16le(tileset)
		<< u16le(backdrop)
		<< u16le(startX)
		<< u16le(startY)
		<< u16le(endX)
		<< u16le(endY)
	;

	// Write out all the gruzzles, slime buckets and book positions
	for (unsigned int i = 0; i < INDEX_SIZE; i++) {

		// Write the number of items first, except for letters which are fixed at 7
		if (i != INDEX_LETTER) {
			uint16_t len = itemLocations[i].size();
			output << u16le(len);
		}

		// Write the X and Y coordinates for each item
		for (std::vector<point>::const_iterator j = itemLocations[i].begin();
			j != itemLocations[i].end(); j++
		) {
			output
				<< u16le(j->first)
				<< u16le(j->second)
			;
		}
	}

	// Write the background layer
	unsigned long lenTiles = mapWidth * mapHeight;
	uint8_t *tiles = new uint8_t[lenTiles];
	boost::scoped_array<uint8_t> stiles(tiles);
	// Set the default background tile
	memset(tiles, WR_DEFAULT_BGTILE, lenTiles);
	layer = map2d->getLayer(0);
	const Map2D::Layer::ItemPtrVectorPtr bgitems = layer->getAllItems();
	for (Map2D::Layer::ItemPtrVector::const_iterator i = bgitems->begin();
		i != bgitems->end();
		i++
	) {
		if (((*i)->x > mapWidth) || ((*i)->y > mapHeight)) {
			throw stream::error(createString("Layer has tiles outside map "
				"boundary at (" << (*i)->x << "," << (*i)->y << ")"));
		}
		tiles[(*i)->y * mapWidth + (*i)->x] = (*i)->code;
	}

	rleWrite(output, tiles, lenTiles);

	// Write the attribute layer
	unsigned long lenAttr = mapWidth * mapHeight * 4;
	uint8_t *attr = new uint8_t[lenAttr];
	boost::scoped_array<uint8_t> sattr(attr);
	// Set the default attribute tile
	memset(attr, WR_DEFAULT_ATTILE, lenAttr);
	layer = map2d->getLayer(1);
	const Map2D::Layer::ItemPtrVectorPtr atitems = layer->getAllItems();
	for (Map2D::Layer::ItemPtrVector::const_iterator i = atitems->begin();
		i != atitems->end();
		i++
	) {
		unsigned int xpos = (*i)->x;
		if (xpos < 1) continue; // skip first column, just in case
		xpos--;
		if ((xpos > mapWidth * 2) || ((*i)->y > mapHeight * 2)) {
			throw stream::error(createString("Layer has tiles outside map "
				"boundary at (" << xpos << "," << (*i)->y << ")"));
		}
		attr[(*i)->y * mapWidth * 2 + xpos] = (*i)->code;
	}

	rleWrite(output, attr, lenAttr);
	output->flush();
	return;
}

SuppFilenames WordRescueMapType::getRequiredSupps(stream::input_sptr input,
	const std::string& filename) const
{
	SuppFilenames supps;
	/// Add wr1.d0 (to wr1.d14) layer file
	std::string baseName = filename.substr(0, filename.find_last_of('.'))
		+ ".d" + filename.substr(filename.find_last_of('s') + 1);
	supps[SuppItem::Layer1] = baseName;
	return supps;
}


} // namespace gamemaps
} // namespace camoto
