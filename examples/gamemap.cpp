/**
 * @file  gamemap.cpp
 * @brief Command-line interface to libgamemaps.
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

#include <boost/algorithm/string.hpp> // for case-insensitive string compare
#include <boost/program_options.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/bind.hpp>
#include <camoto/gamegraphics.hpp>
#include <camoto/gamemaps.hpp>
#include <camoto/util.hpp>
#include <camoto/stream_file.hpp>
#include <iostream>
#include <iomanip>
#include <png++/png.hpp>

namespace po = boost::program_options;
namespace gm = camoto::gamemaps;
namespace gg = camoto::gamegraphics;
namespace stream = camoto::stream;

#define PROGNAME "gamemap"

/// Return value: All is good
#define RET_OK                 0
/// Return value: Bad arguments (missing/invalid parameters)
#define RET_BADARGS            1
/// Return value: Major error (couldn't open map file, etc.)
#define RET_SHOWSTOPPER        2
/// Return value: More info needed (-t auto didn't work, specify a type)
#define RET_BE_MORE_SPECIFIC   3
/// Return value: One or more files failed, probably user error (file not found, etc.)
#define RET_NONCRITICAL_FAILURE 4
/// Return value: Some files failed, but not in a common way (cut off write, disk full, etc.)
#define RET_UNCOMMON_FAILURE   5

// mingw32 doesn't have __STRING
#ifndef __STRING
#define __STRING(x) #x
#endif

/// Place to cache tiles when rendering a map to a .png file
struct CachedTile {
	unsigned int code;
	gg::StdImageDataPtr data;
	gg::StdImageDataPtr mask;
	unsigned int width;
	unsigned int height;
};

/// Open a tileset.
/**
 * @param filename
 *   File to open.
 *
 * @param type
 *   File type if it can't be autodetected.
 *
 * @return Shared pointer to the tileset.
 *
 * @throw stream::error on error
 */
gg::TilesetPtr openTileset(const std::string& filename, const std::string& type)
{
	gg::ManagerPtr pManager(gg::getManager());

	stream::file_sptr psTileset(new stream::file());
	try {
		psTileset->open(filename.c_str());
	} catch (const stream::open_error& e) {
		std::cerr << "Error opening " << filename << ": " << e.what()
			<< std::endl;
		throw stream::error("Unable to open tileset " + filename + ": "
			+ e.get_message());
	}

	gg::TilesetTypePtr pGfxType;
	if (type.empty()) {
		// Need to autodetect the file format.
		gg::TilesetTypePtr pTestType;
		unsigned int i = 0;
		while ((pTestType = pManager->getTilesetType(i++))) {
			gg::TilesetType::Certainty cert = pTestType->isInstance(psTileset);
			switch (cert) {
				case gg::TilesetType::DefinitelyNo:
					break;
				case gg::TilesetType::Unsure:
					// If we haven't found a match already, use this one
					if (!pGfxType) pGfxType = pTestType;
					break;
				case gg::TilesetType::PossiblyYes:
					// Take this one as it's better than an uncertain match
					pGfxType = pTestType;
					break;
				case gg::TilesetType::DefinitelyYes:
					pGfxType = pTestType;
					// Don't bother checking any other formats if we got a 100% match
					goto finishTesting;
			}
		}
finishTesting:
		if (!pGfxType) {
			std::cerr << "Unable to automatically determine the graphics file "
				"type.  Use the --graphicstype option to manually specify the file "
				"format." << std::endl;
			throw stream::error("Unable to open tileset");
		}
	} else {
		gg::TilesetTypePtr pTestType(pManager->getTilesetTypeByCode(type));
		if (!pTestType) {
			std::cerr << "Unknown file type given to -y/--graphicstype: " << type
				<< std::endl;
			throw stream::error("Unable to open tileset");
		}
		pGfxType = pTestType;
	}

	assert(pGfxType != NULL);

	// See if the format requires any supplemental files
	camoto::SuppFilenames suppList = pGfxType->getRequiredSupps(filename);
	camoto::SuppData suppData;
	if (suppList.size() > 0) {
		for (camoto::SuppFilenames::iterator i = suppList.begin(); i != suppList.end(); i++) {
			try {
				std::cerr << "Opening supplemental file " << i->second << std::endl;
				stream::file_sptr suppStream(new stream::file());
				suppStream->open(i->second);
				suppData[i->first] = suppStream;
			} catch (const stream::open_error& e) {
				std::cerr << "Error opening supplemental file " << i->second << ": "
					<< e.what() << std::endl;
				throw stream::error("Unable to open supplemental file " + i->second
					+ ": " +  e.get_message());
			}
		}
	}

	// Open the graphics file
	std::cout << "Opening tileset " << filename << " as "
		<< pGfxType->getCode() << std::endl;
	gg::TilesetPtr pTileset(pGfxType->open(psTileset, suppData));
	assert(pTileset);

	return pTileset;
}

/// Export a map to .png file
/**
 * Convert the given map into a PNG file on disk, by rendering the map as it
 * would appear in the game.
 *
 * @param map
 *   Map file to export.
 *
 * @param allTilesets
 *   Collection of tilesets to use when rendering the map.
 *
 * @param destFile
 *   Filename of destination (including ".png")
 *
 * @throw stream::error on error
 */
void map2dToPng(gm::Map2DPtr map, const gm::TilesetCollectionPtr& allTilesets,
	const std::string& destFile)
{
	unsigned int outWidth, outHeight; // in pixels
	unsigned int globalTileWidth, globalTileHeight;
	map->getTileSize(&globalTileWidth, &globalTileHeight);
	map->getMapSize(&outWidth, &outHeight);
	outWidth *= globalTileWidth;
	outHeight *= globalTileHeight;

	png::image<png::index_pixel> png(outWidth, outHeight);

	bool useMask;
	gg::PaletteTablePtr srcPal;
	for (gm::TilesetCollection::const_iterator
		i = allTilesets->begin(); i != allTilesets->end(); i++
	) {
		if (i->second->getCaps() & gg::Tileset::HasPalette) {
			srcPal = i->second->getPalette();
			break;
		}
	}
	if (!srcPal) {
		srcPal = gg::createPalette_DefaultVGA();
		// Force last colour to be transparent
		srcPal->at(255).red = 255;
		srcPal->at(255).green = 0;
		srcPal->at(255).blue = 192;
		srcPal->at(255).alpha = 0;
	}
	png::palette pal(srcPal->size());
	int j = 0;
	png::tRNS transparency;
	for (gg::PaletteTable::iterator
		i = srcPal->begin(); i != srcPal->end(); i++, j++
	) {
		pal[j] = png::color(i->red, i->green, i->blue);
		if (i->alpha == 0) transparency.push_back(j);
	}
	useMask = srcPal->size() < 255; // only mask if enough room in the palette
	if (useMask) {
		// Increment any transparent indices because we're going to insert a new
		// colour for transparency
		for (png::tRNS::iterator i = transparency.begin(); i != transparency.end(); i++) {
			(*i)++;
		}
		// Make first colour transparent
		pal.insert(pal.begin(), png::color(255, 0, 192));
		transparency.insert(transparency.begin(), 0);
	}
	png.set_palette(pal);
	if (transparency.size() > 0) {
		png.set_tRNS(transparency);
	}

	unsigned int layerCount = map->getLayerCount();
	for (unsigned int layerIndex = 0; layerIndex < layerCount; layerIndex++) {
		gm::Map2D::LayerPtr layer = map->getLayer(layerIndex);

		// Figure out the layer size (in tiles) and the tile size
		unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
		getLayerDims(map, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

		// Prepare tileset
		std::vector<CachedTile> cache;

		// Run through all items in the layer and render them one by one
		const gm::Map2D::Layer::ItemPtrVectorPtr items = layer->getAllItems();
		for (gm::Map2D::Layer::ItemPtrVector::const_iterator t = items->begin();
			t != items->end(); t++
		) {
			CachedTile thisTile;
			unsigned int tileCode = (*t)->code;

			// Find the cached tile
			bool found = false;
			for (std::vector<CachedTile>::iterator ct = cache.begin();
				ct != cache.end(); ct++
			) {
				if (ct->code == tileCode) {
					thisTile = *ct;
					found = true;
					break;
				}
			}
			if (!found) {
				// Tile hasn't been cached yet, load it from the tileset
				gg::ImagePtr img;
				gm::Map2D::Layer::ImageType imgType;
				try {
					imgType = layer->imageFromCode(*t, allTilesets, &img);
				} catch (const std::exception& e) {
					std::cerr << "Error loading image: " << e.what() << std::endl;
					imgType = gm::Map2D::Layer::Unknown;
				}
				switch (imgType) {
					case gm::Map2D::Layer::Supplied:
						assert(img);
						thisTile.data = img->toStandard();
						thisTile.mask = img->toStandardMask();
						img->getDimensions(&thisTile.width, &thisTile.height);
						thisTile.code = tileCode;
						break;
					case gm::Map2D::Layer::Blank:
						thisTile.width = thisTile.height = 0;
						break;
					case gm::Map2D::Layer::Unknown:
					case gm::Map2D::Layer::Digit0:
					case gm::Map2D::Layer::Digit1:
					case gm::Map2D::Layer::Digit2:
					case gm::Map2D::Layer::Digit3:
					case gm::Map2D::Layer::Digit4:
					case gm::Map2D::Layer::Digit5:
					case gm::Map2D::Layer::Digit6:
					case gm::Map2D::Layer::Digit7:
					case gm::Map2D::Layer::Digit8:
					case gm::Map2D::Layer::Digit9:
					case gm::Map2D::Layer::DigitA:
					case gm::Map2D::Layer::DigitB:
					case gm::Map2D::Layer::DigitC:
					case gm::Map2D::Layer::DigitD:
					case gm::Map2D::Layer::DigitE:
					case gm::Map2D::Layer::DigitF:
					case gm::Map2D::Layer::Interactive:
						// Display nothing, but could be changed to a question mark
						thisTile.width = thisTile.height = 0;
						break;

					// Avoid compiler warning about unhandled enum
					case gm::Map2D::Layer::NumImageTypes:
						assert(imgType != gm::Map2D::Layer::NumImageTypes);
				}
				cache.push_back(thisTile);
			}

			if (!thisTile.data) continue; // no image

			// Draw tile onto png
			unsigned int offX = (*t)->x * tileWidth;
			unsigned int offY = (*t)->y * tileHeight;
			for (unsigned int tY = 0; tY < thisTile.height; tY++) {
				unsigned int pngY = offY+tY;
				if (pngY >= outHeight) break; // don't write past image edge
				for (unsigned int tX = 0; tX < thisTile.width; tX++) {
					unsigned int pngX = offX+tX;
					if (pngX >= outWidth) break; // don't write past image edge
					//png[offY + tY][offX + tX] = png::index_pixel(((*t)->code % 16) + 1);
					// Only write opaque pixels
					if (((thisTile.mask[tY*thisTile.width+tX] & 0x01) == 0) ||
						((!useMask) && (layerIndex == 0))
					) {
						png[pngY][pngX] =
							// +1 to the colour to skip over transparent (#0)
							png::index_pixel(thisTile.data[tY*thisTile.width+tX] + (useMask ? 1 : 0));
					} else {
						if (layerIndex == 0) {
							assert(useMask); // just to be sure my logic is right!
							png[pngY][pngX] = png::index_pixel(0);
						} // else let higher layers see through to lower ones
					}
				}
			}
		} // else layer is empty
	}

	png.write(destFile);
	return;
}

int main(int iArgC, char *cArgV[])
{
#ifdef __GLIBCXX__
	// Set a better exception handler
	std::set_terminate(__gnu_cxx::__verbose_terminate_handler);
#endif

	// Disable stdin/printf/etc. sync for a speed boost
	std::ios_base::sync_with_stdio(false);

	// Declare the supported options.
	po::options_description poActions("Actions");
	poActions.add_options()
		("info,i",
			"display information about the map")

		("print,p", po::value<int>(),
			"print the given layer in ASCII")

		("render,r", po::value<std::string>(),
			"render the map to the given .png file")
	;

	po::options_description poOptions("Options");
	poOptions.add_options()
		("type,t", po::value<std::string>(),
			"specify the map type (default is autodetect)")
		("graphics,g", po::value<std::string>(),
			"filename storing game graphics (required with --render)")
		("graphicstype,y", po::value<std::string>(),
			"specify format of file passed with --graphics")
		("script,s",
			"format output suitable for script parsing")
		("force,f",
			"force open even if the map is not in the given format")
		("list-types",
			"list supported file types")
	;

	po::options_description poHidden("Hidden parameters");
	poHidden.add_options()
		("map", "map file to manipulate")
		("help", "produce help message")
	;

	po::options_description poVisible("");
	poVisible.add(poActions).add(poOptions);

	po::options_description poComplete("Parameters");
	poComplete.add(poActions).add(poOptions).add(poHidden);
	po::variables_map mpArgs;

	std::string strFilename, strType;
	std::string strGraphics, strGraphicsType;

	// Get the format handler for this file format
	gm::ManagerPtr pManager(gm::getManager());

	bool bScript = false; // show output suitable for script parsing?
	bool bForceOpen = false; // open anyway even if map not in given format?
	int iRet = RET_OK;
	try {
		po::parsed_options pa = po::parse_command_line(iArgC, cArgV, poComplete);

		// Parse the global command line options
		for (std::vector<po::option>::iterator i = pa.options.begin(); i != pa.options.end(); i++) {
			if (i->string_key.empty()) {
				// If we've already got an map filename, complain that a second one
				// was given (probably a typo.)
				if (!strFilename.empty()) {
					std::cerr << "Error: unexpected extra parameter (multiple map "
						"filenames given?!)" << std::endl;
					return 1;
				}
				assert(i->value.size() > 0);  // can't have no values with no name!
				strFilename = i->value[0];
			} else if (i->string_key.compare("help") == 0) {
				std::cout <<
					"Copyright (C) 2010-2015 Adam Nielsen <malvineous@shikadi.net>\n"
					"This program comes with ABSOLUTELY NO WARRANTY.  This is free software,\n"
					"and you are welcome to change and redistribute it under certain conditions;\n"
					"see <http://www.gnu.org/licenses/> for details.\n"
					"\n"
					"Utility to manipulate map files used by games to store data files.\n"
					"Build date " __DATE__ " " __TIME__ << "\n"
					"\n"
					"Usage: gamemap <map> <action> [action...]\n" << poVisible << "\n"
					<< std::endl;
				return RET_OK;
			} else if (
				(i->string_key.compare("t") == 0) ||
				(i->string_key.compare("type") == 0)
			) {
				strType = i->value[0];
			} else if (
				(i->string_key.compare("g") == 0) ||
				(i->string_key.compare("graphics") == 0)
			) {
				strGraphics = i->value[0];
			} else if (
				(i->string_key.compare("y") == 0) ||
				(i->string_key.compare("graphicstype") == 0)
			) {
				strGraphicsType = i->value[0];
			} else if (
				(i->string_key.compare("s") == 0) ||
				(i->string_key.compare("script") == 0)
			) {
				bScript = true;
			} else if (
				(i->string_key.compare("f") == 0) ||
				(i->string_key.compare("force") == 0)
			) {
				bForceOpen = true;
			} else if (
				(i->string_key.compare("list-types") == 0)
			) {
				std::cout << "Map types: (--type)\n";
				unsigned int i = 0;
				{
					gm::MapTypePtr nextType;
					while ((nextType = pManager->getMapType(i++))) {
						std::string code = nextType->getMapCode();
						std::cout << "  " << code;
						int len = code.length();
						if (len < 20) std::cout << std::string(20-code.length(), ' ');
						std::cout << ' ' << nextType->getFriendlyName();
						std::vector<std::string> ext = nextType->getFileExtensions();
						if (ext.size()) {
							std::vector<std::string>::iterator i = ext.begin();
							std::cout << " (*." << *i;
							for (i++; i != ext.end(); i++) {
								std::cout << "; *." << *i;
							}
							std::cout << ")";
						}
						std::cout << '\n';
					}
				}

				std::cout << "\nMap tilesets: (--graphicstype)\n";
				i = 0;
				gg::ManagerPtr pManager(gg::getManager());
				{
					gg::TilesetTypePtr nextType;
					while ((nextType = pManager->getTilesetType(i++))) {
						std::string code = nextType->getCode();
						std::cout << "  " << code;
						int len = code.length();
						if (len < 20) std::cout << std::string(20-code.length(), ' ');
						std::cout << ' ' << nextType->getFriendlyName();
						std::vector<std::string> ext = nextType->getFileExtensions();
						if (ext.size()) {
							std::vector<std::string>::iterator i = ext.begin();
							std::cout << " (*." << *i;
							for (i++; i != ext.end(); i++) {
								std::cout << "; *." << *i;
							}
							std::cout << ")";
						}
						std::cout << '\n';
					}
				}
				return RET_OK;
			}
		}

		if (strFilename.empty()) {
			std::cerr << "Error: no game map filename given" << std::endl;
			return RET_BADARGS;
		}
		std::cout << "Opening " << strFilename << " as type "
			<< (strType.empty() ? "<autodetect>" : strType) << std::endl;

		stream::file_sptr psMap(new stream::file());
		try {
			psMap->open(strFilename.c_str());
		} catch (const stream::open_error& e) {
			std::cerr << "Error opening " << strFilename << ": " << e.what()
				<< std::endl;
			return RET_SHOWSTOPPER;
		}

		gm::MapTypePtr pMapType;
		if (strType.empty()) {
			// Need to autodetect the file format.
			gm::MapTypePtr pTestType;
			unsigned int i = 0;
			while ((pTestType = pManager->getMapType(i++))) {
				gm::MapType::Certainty cert = pTestType->isInstance(psMap);
				switch (cert) {
					case gm::MapType::DefinitelyNo:
						// Don't print anything (TODO: Maybe unless verbose?)
						break;
					case gm::MapType::Unsure:
						std::cout << "File could be a " << pTestType->getFriendlyName()
							<< " [" << pTestType->getMapCode() << "]" << std::endl;
						// If we haven't found a match already, use this one
						if (!pMapType) pMapType = pTestType;
						break;
					case gm::MapType::PossiblyYes:
						std::cout << "File is likely to be a " << pTestType->getFriendlyName()
							<< " [" << pTestType->getMapCode() << "]" << std::endl;
						// Take this one as it's better than an uncertain match
						pMapType = pTestType;
						break;
					case gm::MapType::DefinitelyYes:
						std::cout << "File is definitely a " << pTestType->getFriendlyName()
							<< " [" << pTestType->getMapCode() << "]" << std::endl;
						pMapType = pTestType;
						// Don't bother checking any other formats if we got a 100% match
						goto finishTesting;
				}
				if (cert != gm::MapType::DefinitelyNo) {
					// We got a possible match, see if it requires any suppdata
					camoto::SuppFilenames suppList = pTestType->getRequiredSupps(psMap,
						strFilename);
					if (suppList.size() > 0) {
						// It has suppdata, see if it's present
						std::cout << "  * This format requires supplemental files..." << std::endl;
						bool bSuppOK = true;
						for (camoto::SuppFilenames::iterator
							i = suppList.begin(); i != suppList.end(); i++
						) {
							try {
								stream::file_sptr suppStream(new stream::file());
								suppStream->open(i->second);
							} catch (const stream::open_error&) {
								bSuppOK = false;
								std::cout << "  * Could not find/open " << i->second
									<< ", map is probably not "
									<< pTestType->getMapCode() << std::endl;
								break;
							}
						}
						if (bSuppOK) {
							// All supp files opened ok
							std::cout << "  * All supp files present, map is likely "
								<< pTestType->getMapCode() << std::endl;
							// Set this as the most likely format
							pMapType = pTestType;
						}
					}
				}
			}
finishTesting:
			if (!pMapType) {
				std::cerr << "Unable to automatically determine the file type.  Use "
					"the --type option to manually specify the file format." << std::endl;
				return RET_BE_MORE_SPECIFIC;
			}
		} else {
			gm::MapTypePtr pTestType(pManager->getMapTypeByCode(strType));
			if (!pTestType) {
				std::cerr << "Unknown file type given to -t/--type: " << strType
					<< std::endl;
				return RET_BADARGS;
			}
			pMapType = pTestType;
		}

		assert(pMapType != NULL);

		// Check to see if the file is actually in this format
		if (!pMapType->isInstance(psMap)) {
			if (bForceOpen) {
				std::cerr << "Warning: " << strFilename << " is not a "
					<< pMapType->getFriendlyName() << ", open forced." << std::endl;
			} else {
				std::cerr << "Invalid format: " << strFilename << " is not a "
					<< pMapType->getFriendlyName() << "\n"
					<< "Use the -f option to try anyway." << std::endl;
				return RET_BE_MORE_SPECIFIC;
			}
		}

		// See if the format requires any supplemental files
		camoto::SuppFilenames suppList = pMapType->getRequiredSupps(psMap,
			strFilename);
		camoto::SuppData suppData;
		if (suppList.size() > 0) {
			for (camoto::SuppFilenames::iterator
				i = suppList.begin(); i != suppList.end(); i++
			) {
				try {
					std::cerr << "Opening supplemental file " << i->second << std::endl;
					stream::file_sptr suppStream(new stream::file());
					suppStream->open(i->second);
					suppData[i->first] = suppStream;
				} catch (const stream::open_error& e) {
					std::cerr << "Error opening supplemental file " << i->second << ": "
						<< e.what() << std::endl;
					return RET_SHOWSTOPPER;
				}
			}
		}

		// Open the map file
		//FN_TRUNCATE fnTruncate = boost::bind<void>(truncate, strFilename.c_str(), _1);
		gm::MapPtr pMap(pMapType->open(psMap, suppData));
		assert(pMap);

		// File type of inserted files defaults to empty, which means 'generic file'
		std::string strLastFiletype;

		// Run through the actions on the command line
		for (std::vector<po::option>::iterator i = pa.options.begin(); i != pa.options.end(); i++) {
			if (i->string_key.compare("info") == 0) {
				std::cout << (bScript ? "attribute_count=" : "Number of attributes: ")
					<< pMap->attributes.size() << "\n";
				int attrNum = 0;
				for (gm::Map::Attributes::const_iterator
					i = pMap->attributes.begin(); i != pMap->attributes.end(); i++
				) {
					const gm::Map::Attribute& a = *i;

					if (bScript) std::cout << "attribute" << attrNum << "_name=";
					else std::cout << "Attribute " << attrNum+1 << ": ";
					std::cout << a.name << "\n";

					if (bScript) std::cout << "attribute" << attrNum << "_desc=";
					else std::cout << "  Description: ";
					std::cout << a.desc << "\n";

					if (bScript) std::cout << "attribute" << attrNum << "_type=";
					else std::cout << "  Type: ";
					switch (a.type) {

						case gm::Map::Attribute::Integer: {
							std::cout << (bScript ? "int" : "Integer value") << "\n";

							if (bScript) std::cout << "attribute" << attrNum << "_value=";
							else std::cout << "  Current value: ";
							std::cout << a.integerValue << "\n";

							if (bScript) {
								std::cout << "attribute" << attrNum << "_min=" << a.integerMinValue
									<< "\nattribute" << attrNum << "_max=" << a.integerMaxValue;
							} else {
								std::cout << "  Range: ";
								if ((a.integerMinValue == 0) && (a.integerMaxValue == 0)) {
									std::cout << "[unlimited]";
								} else {
									std::cout << a.integerMinValue << " to " << a.integerMaxValue;
								}
							}
							std::cout << "\n";
							break;
						}

						case gm::Map::Attribute::Enum: {
							std::cout << (bScript ? "enum" : "Item from list") << "\n";

							if (bScript) std::cout << "attribute" << attrNum << "_value=";
							else std::cout << "  Current value: ";
							if (a.enumValue > a.enumValueNames.size()) {
								std::cout << (bScript ? "error" : "[out of range]");
							} else {
								if (bScript) std::cout << a.enumValue;
								else std::cout << "[" << a.enumValue << "] "
									<< a.enumValueNames[a.enumValue];
							}
							std::cout << "\n";

							if (bScript) std::cout << "attribute" << attrNum
								<< "_choice_count=" << a.enumValueNames.size() << "\n";

							int option = 0;
							for (std::vector<std::string>::const_iterator
								j = a.enumValueNames.begin(); j != a.enumValueNames.end(); j++
							) {
								if (bScript) {
									std::cout << "attribute" << attrNum << "_choice" << option
										<< "=";
								} else {
									std::cout << "  Allowed value " << option << ": ";
								}
								std::cout << *j << "\n";
								option++;
							}
							break;
						}

						case gm::Map::Attribute::Filename: {
							std::cout << (bScript ? "filename" : "Filename") << "\n";

							if (bScript) std::cout << "attribute" << attrNum << "_value=";
							else std::cout << "  Current value: ";
							std::cout << a.filenameValue << "\n";

							if (bScript) std::cout << "attribute" << attrNum
								<< "_filespec=";
							else std::cout << "  Valid files: ";
							std::cout << "*";
							if (!a.filenameValidExtension.empty()) {
								std::cout << '.' << a.filenameValidExtension;
							}
							std::cout << "\n";
							break;
						}

						default:
							std::cout << (bScript ? "unknown" : "Unknown type (fix this!)");
							break;
					}
					attrNum++;
				}

				std::cout << (bScript ? "gfx_filename_count=" : "Number of graphics filenames: ")
					<< pMap->graphicsFilenames.size() << "\n";
				int fileNum = 0;
				for (gm::Map::GraphicsFilenames::const_iterator
					i = pMap->graphicsFilenames.begin(); i != pMap->graphicsFilenames.end(); i++
				) {
					const gm::Map::GraphicsFilename& a = i->second;

					if (bScript) {
						std::cout << "gfx_file" << fileNum << "_name=" << a.filename << "\n";
						std::cout << "gfx_file" << fileNum << "_type=" << a.type << "\n";
						std::cout << "gfx_file" << fileNum << "_purpose=" << i->first << "\n";
					} else {
						std::cout << "Graphics file " << fileNum+1 << ": " << a.filename
							<< " [";
						switch (i->first) {
							case gm::GenericTileset1:    std::cout << "Generic tileset 1"; break;
							case gm::BackgroundImage:    std::cout << "Background image"; break;
							case gm::BackgroundTileset1: std::cout << "Background tileset 1"; break;
							case gm::BackgroundTileset2: std::cout << "Background tileset 2"; break;
							case gm::ForegroundTileset1: std::cout << "Foreground tileset 1"; break;
							case gm::ForegroundTileset2: std::cout << "Foreground tileset 2"; break;
							case gm::SpriteTileset1:     std::cout << "Sprite tileset 1"; break;
							case gm::FontTileset1:       std::cout << "Font tileset 1"; break;
							case gm::FontTileset2:       std::cout << "Font tileset 2"; break;
							default:
								std::cout << "Unknown purpose <fix this>";
								break;
						}
						std::cout << " of type " << a.type << "]\n";
					}
					fileNum++;
				}

				std::cout << (bScript ? "map_type=" : "Map type: ");
				gm::Map2DPtr map2d = boost::dynamic_pointer_cast<gm::Map2D>(pMap);
				if (map2d) {
					std::cout << (bScript ? "2d" : "2D grid-based") << "\n";
#define CAP(o, c, v)        " " __STRING(c) << ((v & o::c) ? '+' : '-')
#define MAP2D_CAP(c)        CAP(gm::Map2D,        c, mapCaps)
#define MAP2D_LAYER_CAP(c)  CAP(gm::Map2D::Layer, c, layerCaps)

					int mapCaps = map2d->caps;
					if (bScript) {
						std::cout << "map_caps=" << mapCaps << "\n";
					} else {
						std::cout << "Map capabilities:"
							<< MAP2D_CAP(CanResize)
							<< MAP2D_CAP(ChangeTileSize)
							<< MAP2D_CAP(HasViewport)
							<< MAP2D_CAP(HasPaths)
							<< MAP2D_CAP(FixedPathCount)
							<< "\n"
						;
					}
					unsigned int mapTileWidth, mapTileHeight;
					map2d->getTileSize(&mapTileWidth, &mapTileHeight);
					std::cout << (bScript ? "tile_width=" : "Tile size: ") << mapTileWidth
						<< (bScript ? "\ntile_height=" : "x") << mapTileHeight << "\n";

					unsigned int mapWidth, mapHeight;
					map2d->getMapSize(&mapWidth, &mapHeight);
					std::cout
						<< (bScript ? "map_width=" : "Map size: ") << mapWidth
						<< (bScript ? "\nmap_height=" : "x") << mapHeight
						<< (bScript ? "" : " tiles")
						<< "\n";

					if (mapCaps & gm::Map2D::HasViewport) {
						std::cout << (bScript ? "viewport_width=" : "Viewport size: ")
							<< map2d->viewportX
							<< (bScript ? "\nviewport_height=" : "x") << map2d->viewportY
							<< (bScript ? "" : " pixels") << "\n";
					}

					unsigned int layerCount = map2d->getLayerCount();
					std::cout << (bScript ? "layercount=" : "Layer count: ")
						<< layerCount << "\n";
					for (unsigned int i = 0; i < layerCount; i++) {
						gm::Map2D::LayerPtr layer = map2d->getLayer(i);
						std::string prefix;
						if (bScript) {
							std::stringstream ss;
							ss << "layer" << i << '_';
							prefix = ss.str();
							std::cout << prefix << "name=" << layer->getTitle() << "\n";
						} else {
							prefix = "  ";
							std::cout << "Layer " << i + 1 << ": \"" << layer->getTitle()
								<< "\"\n";
						}
						int layerCaps = layer->getCaps();
						if (bScript) std::cout << prefix << "caps=" << layerCaps << "\n";
						else std::cout << prefix << "Capabilities:"
							<< MAP2D_LAYER_CAP(HasOwnSize)
							<< MAP2D_LAYER_CAP(CanResize)
							<< MAP2D_LAYER_CAP(HasOwnTileSize)
							<< MAP2D_LAYER_CAP(ChangeTileSize)
							<< MAP2D_LAYER_CAP(HasPalette)
							<< MAP2D_LAYER_CAP(UseImageDims)
							<< "\n"
						;

						unsigned int layerTileWidth, layerTileHeight;
						bool layerTileSame;
						if (layerCaps & gm::Map2D::Layer::HasOwnTileSize) {
							layer->getTileSize(&layerTileWidth, &layerTileHeight);
							layerTileSame = false;
						} else {
							layerTileWidth = mapTileWidth;
							layerTileHeight = mapTileHeight;
							layerTileSame = true;
						}
						std::cout << prefix << (bScript ? "tile_width=" : "Tile size: ") << layerTileWidth;
						if (bScript) std::cout << "\n" << prefix << "tile_height=";
						else std::cout << "x";
						std::cout << layerTileHeight;
						if (layerTileSame && (!bScript)) {
							std::cout << " (same as map)";
						}
						std::cout << "\n";

						unsigned int layerWidth, layerHeight;
						bool layerSame;
						if (layerCaps & gm::Map2D::Layer::HasOwnSize) {
							layer->getLayerSize(&layerWidth, &layerHeight);
							layerSame = false;
						} else {
							// Convert from map tilesize to layer tilesize, leaving final
							// pixel dimensions unchanged
							layerWidth = mapWidth * mapTileWidth / layerTileWidth;
							layerHeight = mapHeight * mapTileHeight / layerTileHeight;
							layerSame = true;
						}
						std::cout << prefix << (bScript ? "width=" : "Layer size: ") << layerWidth;
						if (bScript) std::cout << "\n" << prefix << "height=";
						else std::cout << "x";
						std::cout << layerHeight;
						if (layerSame && (!bScript)) {
							std::cout << " (same as map)";
						}
						std::cout << "\n";
					}

				} else {
					std::cout << (bScript ? "unknown" : "Unknown!  Fix this!") << "\n";
				}

			} else if (i->string_key.compare("print") == 0) {
				gm::Map2DPtr map2d = boost::dynamic_pointer_cast<gm::Map2D>(pMap);
				if (map2d) {
					unsigned int targetLayer = strtoul(i->value[0].c_str(), NULL, 10);
					if (targetLayer == 0) {
						std::cerr << "Invalid layer index passed to --print.  Use --info "
							"to list layers in this map." << std::endl;
						iRet = RET_BADARGS;
						continue;
					}
					unsigned int layerCount = map2d->getLayerCount();
					if (targetLayer > layerCount) {
						std::cerr << "Invalid layer index passed to --print.  Use --info "
							"to list layers in this map." << std::endl;
						iRet = RET_BADARGS;
						continue;
					}

					gm::Map2D::LayerPtr layer = map2d->getLayer(targetLayer - 1);

					// Figure out the layer size
					unsigned int layerWidth, layerHeight, tileWidth, tileHeight;
					getLayerDims(map2d, layer, &layerWidth, &layerHeight, &tileWidth, &tileHeight);

					const gm::Map2D::Layer::ItemPtrVectorPtr items = layer->getAllItems();
					gm::Map2D::Layer::ItemPtrVector::const_iterator t = items->begin();
					unsigned int numItems = items->size();
					if (t != items->end()) {
						for (unsigned int y = 0; y < layerHeight; y++) {
							for (unsigned int x = 0; x < layerWidth; x++) {
								for (unsigned int i = 0; i < numItems; i++) {
									if (((*t)->x == x) && ((*t)->y == y)) break;
									t++;
									if (t == items->end()) t = items->begin();
								}
								if (((*t)->x != x) || ((*t)->y != y)) {
									// Grid position with no tile!
									std::cout << "     ";
								} else {
									std::cout << std::hex << std::setw(4)
										<< (unsigned int)(*t)->code << ' ';
								}
							}
							std::cout << "\n";
						}
					} else {
						std::cout << "Layer is empty!" << std::endl;
					}

				} else {
					std::cerr << "Support for printing this map type has not yet "
						"been implemented!" << std::endl;
				}

			} else if (i->string_key.compare("render") == 0) {
				if (strGraphics.empty()) {
					std::cerr << "You must use --graphics to specify a tileset."
						<< std::endl;
					iRet = RET_BADARGS;
					continue;
				}
				// Don't need to check i->value[0], program_options does that for us

				gm::Map2DPtr map2d = boost::dynamic_pointer_cast<gm::Map2D>(pMap);
				if (map2d) {
					gm::TilesetCollectionPtr allTilesets(new gm::TilesetCollection);
					/// @todo Load more than one tileset
					(*allTilesets)[gm::BackgroundTileset1] = openTileset(strGraphics, strGraphicsType);
					map2dToPng(map2d, allTilesets, i->value[0]);
				}

			// Ignore --type/-t
			} else if (i->string_key.compare("type") == 0) {
			} else if (i->string_key.compare("t") == 0) {
			// Ignore --script/-s
			} else if (i->string_key.compare("script") == 0) {
			} else if (i->string_key.compare("s") == 0) {
			// Ignore --force/-f
			} else if (i->string_key.compare("force") == 0) {
			} else if (i->string_key.compare("f") == 0) {

			}
		} // for (all command line elements)
		//pMap->flush();
	} catch (const po::error& e) {
		std::cerr << PROGNAME ": " << e.what()
			<< ".  Use --help for help." << std::endl;
		return RET_BADARGS;
	} catch (const stream::error& e) {
		std::cerr << PROGNAME ": " << e.what()
			<< ".  Use --help for help." << std::endl;
		return RET_SHOWSTOPPER;
	}

	return iRet;
}
