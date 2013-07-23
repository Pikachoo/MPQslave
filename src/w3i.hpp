#ifndef W3I_HPP_
#define W3I_HPP_

#include <string>

#include <stdint.h>

#include "extends/readable.hpp"

/* ======================== *\
 * File format version = 18
\* ======================== */

struct W3I_18
{
		/* ===================================================================== *\
		 * It contains some of the info displayed when you start a game.
		 *
		 * Format:
		 *	int: file format version = 18
		 *	int: number of saves (map version)
		 *	int: editor version (little endian)
		 *	String: map name
		 *	String: map author
		 *	String: map description
		 *	String: players recommended
		 *	float[8]: "Camera Bounds" as defined in the JASS file
		 *	int[4]: camera bounds complements* (see note 1) (ints A, B, C and D)
		 *	int: map playable area width E* (see note 1)
		 *	int: map playable area height F* (see note 1)
		 *
		 *	*note 1:
		 *	map width = A + E + B
		 *	map height = C + F + D
		 *
		 *	...
		 *
		\* ===================================================================== */

		int FILE_FORMAT;
		int NUM_OF_SAVES;
		int EDITOR_VERSION;

		std::string MAP_NAME;
		std::string MAP_AUTHOR;
		std::string MAP_DESCRIPTION;
		std::string MAP_PLAYERS_RECOMMENDED;

		double CAMERA_BOUNDS_JASS;

		int32_t CAMERA_BOUNDS_COMPLEMENTS;

		int MAP_WIDTH;
		int MAP_HEIGHT;


		/* =============================================================================== *\
		 *
		 * ...
		 *
		 * int: flags
		 *	0x0001: 1=hide minimap in preview screens
		 *	0x0002: 1=modify ally priorities
		 *	0x0004: 1=melee map
		 *	0x0008: 1=playable map size was large and has never been reduced to medium (?)
		 *	0x0010: 1=masked area are partially visible
		 *	0x0020: 1=fixed player setting for custom forces
		 *	0x0040: 1=use custom forces
		 *	0x0080: 1=use custom techtree
		 *	0x0100: 1=use custom abilities
		 *	0x0200: 1=use custom upgrades
		 *	0x0400: 1=map properties menu opened at least once since map creation (?)
		 *	0x0800: 1=show water waves on cliff shores
		 *	0x1000: 1=show water waves on rolling shores
		 *
		 *	...
		 *
		\* =============================================================================== */

		int FLAGS;


		/* ==================================================================================================== *\
		 *
		 * ...
		 *
		 * char: map main ground type (Example: 'A'= Ashenvale, 'X' = City Dalaran)
		 * int: Campaign background number (-1 = none)
		 * String: Map loading screen text
		 * String: Map loading screen title
		 * String: Map loading screen subtitle
		 * int: Map loading screen number (-1 = none)
		 * String: Prologue screen text
		 * String: Prologue screen title
		 * String: Prologue screen subtitle
		 * int: max number "MAXPL" of players
		 * array of structures: then, there is MAXPL times a player data like described below.
		 * int: max number "MAXFC" of forces
		 * array of structures: then, there is MAXFC times a force data like described below.
		 * int: number "UCOUNT" of upgrade availability changes
		 * array of structures: then, there is UCOUNT times a upgrade availability change like described below.
		 * int: number "TCOUNT" of tech availability changes (units, items, abilities)
		 * array of structures: then, there is TCOUNT times a tech availability change like described below
		 * int: number "UTCOUNT" of random unit tables
		 * array of structures: then, there is UTCOUNT times a unit table like described below
		\* ==================================================================================================== */

		char GROUND_TYPE;

		int CAMPAIGN_BACKGROUND_NUMBER;

		std::string MAP_LOADING_SCREEN_TEXT;
		std::string MAP_LOADING_SCREEN_TITLE;
		std::string MAP_LOADING_SCREEN_SUBTITLE;

		int MAP_LOADING_SCREEN_NUMBER;

		std::string PROLOGUE_SCREEN_TEXT;
		std::string PROLOGUE_SCREEN_TITLE;
		std::string PROLOGUE_SCREEN_SUBTITLE;

		int MAX_NUMBER_OF_PLAYERS;
		int MAX_NUMBER_OF_FORCES;
		int NUMBER_OF_UPGRADE_AVALIBLE_CHANGES;
		int NUMBER_OF_TECH_AVALIBLE_CHANGES;
		int NUMBER_OF_RANDOM_UNIT_TABLES;
};

namespace MPQs
{
	class w3i : public MPQs::readable
	{
		private:

		public:
			W3I_18 read_it(); // TODO: rename/delete
	};
}


#endif /* W3I_HPP_ */
