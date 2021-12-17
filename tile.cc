/*
 * Author: Luke Vandecasteele
 * 
 * Credits: Matthew Trappert, Dr. Jee W. Choi, Class notes, and other sources
 *          listed in final report.
 *
 * Date Created: 11/8/2021
 * Last Modified: 12/2/2021
 *
 * Description: Final project for CIS 431 at the University of Oregon Fall
 *              2021. Implementation of Tile class for storing additional
 *              information about the sqaures of the puzzle. 
 * Notes: 
 *       1. See header.h for all class methods and variables.
 *       2. All methods contain headers for individual usage and description.
 *
 */

#include "header.h"

// default constructor
Tile::Tile()
{
	row = -1;
	col = -1;
	val = 0;
}

// constructor
Tile::Tile(int r, int c, int v)
{
	row = r;
	col = c;
	val = v;
}

// destructor
Tile::~Tile()
{
	/* Do nothing */
}

void Tile::print()
{
	cout << "Tile: {";
	cout << "(" << row;
	cout << ", " << col << ") ";
	cout << "val: }" << val;
	cout << endl;
}

// sets the candidates of the Tile
// called at puzzle initialization
void Tile::gen_candidates(vector<int> choices)
{
	if(val == -1)
		candidates = choices;
}

/*
 * remove_candidates()
 *											Description: Called by elimination(). For a given array
 *																	 of values that have been used in this
 *																	 Tile's group, this function removes values
 *																	 from the Tile's candidates list that can 
 *																	 no longer be used.
 *											Input: array of values that are no longer available for
 *														 the Tile's candidates.
 *											Output: returns 1 if the Tile's candidates list was
 *															made smaller, and 0 otherwise.
 *											Calls: None
 */

int Tile::remove_candidates(vector<int> used_values)
{
	// sort lists so we can use set_difference() function
	sort(candidates.begin(), candidates.end());
	sort(used_values.begin(), used_values.end());

	// find values that are different between the used_values in the
	// group, and the available candidates for the Tile (i.e. check
	// which values can still be candidates for the Tile)
	vector<int> new_candidates(26);
  vector<int>::iterator begin;
	begin = set_difference(candidates.begin(), candidates.end(),
		 						 				 used_values.begin(), used_values.end(), 
								 				 new_candidates.begin());
  new_candidates.resize(begin-new_candidates.begin());

  // if we did not remove any candidates, return
	if (new_candidates.size() == candidates.size())
	{
		return 0;
	}

	// otherwise, set our new candidates
	// if the number of candidates is 1, then we set the value of
	// the Tile to that leftover candidate
	candidates = new_candidates;
	if(candidates.size() == 1)
	{
		val = candidates[0];
		candidates.pop_back();
	}
	return 1;
}
