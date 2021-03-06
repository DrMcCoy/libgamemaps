/**
 * @file   test-map-ccaves.cpp
 * @brief  Test code for Crystal Caves maps.
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

#include "test-map2d.hpp"

// copied from fmt-map-ccaves.cpp
/// Create a tile number from a tileset number and an index into the tileset.
#define MAKE_TILE(tileset, tile) (((tileset) << 8) | (tile))

class test_map_ccaves: public test_map2d
{
	public:
		test_map_ccaves()
		{
			this->type = "map-ccaves";
			this->pxWidth = 40 * 16;
			this->pxHeight = 17 * 16;
			this->numLayers = 2;
			this->mapCode[0].x = 33;
			this->mapCode[0].y = 0;
			this->mapCode[0].code = MAKE_TILE(13,  0);
			this->mapCode[1].x = 32;
			this->mapCode[1].y = 3;
			this->mapCode[1].code = MAKE_TILE(12, 36);
			this->outputWidth = 41;
		}

		void addTests()
		{
			this->test_map2d::addTests();

			// c00: Initial state
			this->isInstance(MapType::DefinitelyYes, this->initialstate());

			// c01: Too small
			this->isInstance(MapType::DefinitelyNo, STRING_WITH_NULLS(
				"\x28" "\x01\x02\x03\x04\x05\x06\x07\x08\x09"
			));

			// c02: Wrong row length
			this->isInstance(MapType::DefinitelyNo, STRING_WITH_NULLS(
					"\x29" "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B"
				) + std::string(30, '\0')
				+ STRING_WITH_NULLS("\x28") + std::string(40, '\x20')
				+ STRING_WITH_NULLS("\x28") + std::string(40, '\x20')
			);

			// c03: Incomplete row
			this->isInstance(MapType::DefinitelyNo,
				STRING_WITH_NULLS("\x28") + std::string(40, '\x20')
				+ STRING_WITH_NULLS("\x28") + std::string(40, '\x20')
				+ STRING_WITH_NULLS(
					"\x28" "\x01\x02\x03\x04\x05\x06\x07\x08\x09"
				) + std::string(30, '\0')
			);

			// c04: Invalid tile code
			this->isInstance(MapType::DefinitelyNo,
				STRING_WITH_NULLS("\x28") + std::string(40, '\x20')
				+ STRING_WITH_NULLS("\x28") + std::string(40, '\x20')
				+ STRING_WITH_NULLS(
					"\x28" "\x01\xFF\x03\x04\x05\x06\x07\x08\x09\x0A"
				) + std::string(30, '\0')
			);

			// c05: Map too tall
			this->isInstance(MapType::DefinitelyNo, std::string(40 * 101, '\x20'));

			// 01: Vine transformation
			this->conversion(STRING_WITH_NULLS(
				"\x28" "\x6E\x6E\x6E\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				"\x28" "\x6E\x86\x88\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				"\x28" "\x87\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				), STRING_WITH_NULLS(
				"\x28" "\x87\x86\x88\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				"\x28" "\x87\x86\x88\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				"\x28" "\x87\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
			));

			// 02: Misc transformation
			this->conversion(STRING_WITH_NULLS(
				"\x28" "\xFD\xFE\x4B\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				), STRING_WITH_NULLS(
				"\x28" "\x91\x92\x43\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
			));
		}

		virtual std::string initialstate()
		{
			return STRING_WITH_NULLS(
				/* 1x1 codes, 0x20 used for invalid codes */
				"\x28" "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x21\x22\x23\x20\x25\x26\x20"
				"\x28" "\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x20\x20\x20\x34\x35\x36\x20\x38\x39\x3A\x20\x20\x3D\x20\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x20\x4C\x4D\x4E\x20"
				"\x28" "\x20\x20\x52\x53\x54\x20\x56\x20\x20\x59\x5A\x20\x20\x5D\x5E\x5F\x20\x61\x62\x63\x64\x20\x66\x67\x68\x20\x6A\x6B\x6C\x6D\x20\x6F\x70\x71\x72\x73\x74\x75\x76\x77"
				"\x28" "\x78\x79\x7A\x20\x7C\x20\x7E\x20\x20\x81\x82\x83\x84\x20\x20\x20\x20\x89\x8A\x8B\x8C\x20\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x20\x20\x20\x20\x9F"
				"\x28" "\xA0\xA1\xA2\x20\x20\x20\xA6\xA7\xA8\xA9\xAA\xAB\xAC\x20\x20\x20\xB0\xB1\xB2\xB3\x20\x20\x20\x20\x20\x20\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\x20\xC5\xC6\xC7"
				"\x28" "\xC8\x20\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\x20\x20\x20\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xE7\xE8\xE9\xEA\xEB\xEC\xED\x20\x20"
				"\x28" "\xF0\x20\xF2\xF3\xF4\xF5\xF6\xF7\x20\xF9\xFA\xFB\xFC\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				/* 2x2 and above codes */
				"\x28" "\x24\x55\x6E\x57\x4C\x57\x52\x58\x6E\x69\x80\x6E\x85\x86\x87\x88\x94\x95\x95\x6E\xA3\xA4\xA5\xC3\xE0\x6E\xF8\x6E\x5B\x34\x6E\xF0\x6E\x20\x75\x6E\x20\x20\x20\x20"
				"\x28" "\x6E\x6E\x6E\x20\x20\x20\x20\x6E\x6E\x6E\x6E\x6E\x85\x86\x87\x88\x96\x97\x97\x6E\x6E\x6E\x6E\xC4\x6E\x6E\x6E\x6E\x20\x20\x20\xB3\x6E\x20\x8E\x6E\x20\x20\x20\x20"
				"\x28" "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x6E\x20\x20\x20\x20\x20\x20\x20\x20\x20\x8F\x6E\x6E\x6E\x20\x20\x20"
				/* Signs part 1 */
				"\x28" "\x5B\x23\x5B\x2A\x5B\x2D\x5B\x31\x5B\x32\x5B\x33\x5B\x34\x5B\x35\x5B\x3A\x5B\x3B\x5B\x3D\x5B\x41\x5B\x42\x5B\x44\x5B\x45\x6E\x5B\x4F\x5B\x50\x6E\x6E\x5B\x51\x20"
				"\x28" "\x6E\x6E\x6E\x6E\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x6E\x6E\x6E\x6E\x20\x20\x20\x20\x20\x6E\x6E\x20\x20\x20\x20\x6E\x6E\x20"
				"\x28" "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x6E\x6E\x20"
				"\x28" "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x6E\x6E\x20"
				/* Signs part 2 */
				"\x28" "\x5B\x54\x6E\x5B\x5D\x5B\x5E\x5B\x62\x6E\x6E\x5B\x63\x5B\x64\x5B\x66\x5B\x67\x6E\x5B\x6D\x5B\x72\x5B\x78\x5B\x79\x5B\x7C\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				"\x28" "\x20\x6E\x20\x6E\x6E\x20\x20\x6E\x6E\x6E\x6E\x6E\x6E\x20\x20\x20\x20\x6E\x6E\x6E\x20\x20\x20\x20\x6E\x6E\x6E\x6E\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
				/* Various continuations */
				"\x28" "\x70\x6E\x20\x70\x6A\x20\x70\x6A\x6E\x20\x44\x6E\x20\x44\x64\x6E\x20\x98\x6E\x20\x98\x99\x6E\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20"
			);
		}
};

IMPLEMENT_TESTS(map_ccaves);
