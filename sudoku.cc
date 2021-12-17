/*
 * Author: Luke Vandecasteele
 * 
 * Credits: Matthew Trappert, Dr. Jee W. Choi, Class notes, and other sources
 *          listed in final report.
 *
 * Date Created: 11/8/2021
 * Last Modified: 12/3/2021
 *
 * Description: Final project for CIS 431 at the University of Oregon Fall
 *              2021. Implementation of a serial sudoku solver. 
 * Notes: 
 *       1. See header.h for all class methods and variables.
 *       2. All methods contain headers for individual usage and description.
 *
 */

#include "header.h"

// Constructor
Sudoku::Sudoku(vector<vector<Tile*>> in, int n)
{
	matrix = in;
	dim = n;
	nonet = sqrt(n);


	// set possible candidates for the puzzle
	for(int i = 1; i <= dim; i++) choices.push_back(i);

	// generate candidates for each tile
    for(int i = 0; i < dim; i++)
	{
        for(int j = 0; j < dim; j++)
		{
			matrix[i][j]->gen_candidates(choices);
		}
	}

    // generate groups
    for(int i = 0; i < dim; i++)
    {
        vector<Tile*> tmp = get_row(i);
        groups.push_back(tmp);

        tmp = get_col(i);
        groups.push_back(tmp);

        tmp = get_nonet(i);
        groups.push_back(tmp);

    }
}

// Destructor
Sudoku::~Sudoku()
{
    /* do nothing */
    /* vector will be cleaned up automatically by its destructor */
}

/*
 *  solve()
 *          Description: Driver for a serial sudoku solver. Uses a
 *                       combination of lone_ranger() and elimination() tactics
 *                       as well as a recursive guess and check algorithm.
 *          Input: None
 *          Output: None
 *          Calls: propagate(), is_complete(), is_valid(), and Tile class
 *                 methods. 
 */

int Sudoku::solve()
{
    // apply tactics
    int return_status;
    propagate();

    // check if our puzzle is valid
    // if not, we return 0 and make another guess
    if(!(is_valid(return_status)))
        return 0;

    // if complete, we return 1 and the solve()
    // recursive stack ends
    else if(is_complete())
        return 1;

    // puzzle is valid, but not solved yet
    // start another recursive guess and check
    else
    {
        // save the current state of the puzzle
        vector<vector<int>> save = as_list();
        vector<vector<vector<int>>> save_candidates = candidates_list();

        // find the tile with the least amount of candidates
        // recursively call solve() for each guess
        Tile *min_tile = min_choice_tile();
        for(unsigned int i = 0; i < min_tile->candidates.size(); i++)
        {
            vector<int> tmp;
            min_tile->val = min_tile->candidates[i];
            min_tile->candidates = tmp;

            // puzzle was solved, return up the recursive stack
            if(solve())
                return 1;
            // restore the state of the puzzle, make another guess
            else
            {
                restore_values(save);
                restore_candidates(save_candidates);
            }
        }
        return 0;
    }
}

/*
 * propagate()
 *              Description: Driver function for tactics to propagate values
 *                           into each tile in the puzzle. 
 *              Input: None
 *              Ouput: None
 *              Calls: elimination() lone_ranger()
 */

void Sudoku::propagate()
{
    // apply tactics until we no longer make any changes
    int cont = 1;
    while(cont)
    {
        cont = elimination();
        lone_ranger();
    }
}

/*
 * elimination()
 *              Description: This function
 *                           checks for each group (row, column, or nonet)
 *                           the possible candidates for that group, and then
 *                           for each tile in the group, then eliminates 
 *                           candidates for that particular tile depending upon
 *                           which values are available in that group. If there
 *                           is only one available candidate left for a Tile,
 *                           then the Tile's value is set to that candidate.
 *              Input: None
 *              Output: returns 1 if a change was made, and 0 if a change was
 *                      not made to any Tile's candidates in the puzzle
 *              Calls: Tile.remove_candidates()
 */

int Sudoku::elimination()
{
    // for each group in the puzzle
	int return_value = 0;
	for(unsigned int i = 0; i < groups.size(); i++)
	{
        // check which values have been used
		set<int> used_values;
		for(unsigned int j = 0; j < groups[i].size(); j++)
		{
			if(in(groups[i][j]->val, choices))
				used_values.insert(groups[i][j]->val);
		}

        // for each tile in the group, remove_candidates()
        // depending upon which values have been used in the group
        for(unsigned int j = 0; j < groups[i].size(); j++)
        {
        	vector<int> tmp{used_values.begin(), used_values.end()};
            int change_made = groups[i][j]->remove_candidates(tmp);

            // a change was made to a Tile's candidates
            // set return value
            if(change_made)
                return_value = 1;
        }
	}
    return return_value;

}

/*
 * lone_ranger()
 *              Description: This function
 *                           checks for each group (row, column, or nonet)
 *                           if there is an empty Tile such that one of the
 *                           candidates for that empty Tile does not appear
 *                           as one of the candidates for any other Tile in the
 *                           same group. If this is true, then it sets that 
 *                           particular candidate as the value for the Tile.
 *              Input: None
 *              Output: returns 1 if a change was made to the state of the 
 *                      puzzle, 0 otherwise.
 *              Calls: None
 */

int Sudoku::lone_ranger()
{
    // for each group in the puzzle
    int return_value = 0;
    for(unsigned int i = 0; i < groups.size(); i++)
    {
        // check which values are available to be used for
        // this particular group
        set<int> leftovers{choices.begin(), choices.end()};
        for(unsigned int j = 0; j < groups[i].size(); j++)
        {
            if(in(groups[i][j]->val, choices))
                leftovers.erase(groups[i][j]->val);
        }

        // for each available candidate
        vector<int> replace{leftovers.begin(), leftovers.end()};
        for(unsigned int j = 0; j < replace.size(); j++)
        {
            int num_leftover = 0;
            int place_holder;

            // for each Tile in the group
            // count how many Tile's have each available candidate
            // in their own candidates list
            for(unsigned int k = 0; k < groups[i].size(); k++)
            {
                if(in(replace[j], groups[i][k]->candidates))
                {
                    num_leftover++;
                    place_holder = k;
                }
            }

            // only one Tile has this available candidate as a
            // candidate: then this value must be the value of the Tile 
            if(num_leftover == 1)
            {
                groups[i][place_holder]->val = replace[j];
                groups[i][place_holder]->candidates.clear();
                return_value = 1;
            }
        }
    }
    return return_value;
}

/*
 *  in()
 *          Description: Simple helper function for elimination() and
 *                       lone_ranger(). Checks if a given value is in a vector
 *          Input: val -> particular value to check if it is in the list
 *                 list -> list to be scanned for value
 *          Output: returns 1 if the value is in the list, 0 otherwise.
 *          Calls: None
 */

int Sudoku::in(int val, vector<int> list)
{
    // iterate through list checking if it contains
    // the value
	int return_value = 0;
	for(unsigned int i = 0; i < list.size(); i++)
	{
		if(list[i] == val)
		{
			return_value = 1;
			break;
		}
	}
	return return_value;
}

/*
 * min_choice_tile()
 *                  Description: Finds the Tile in the puzzle with the least
 *                               amount of candidates.
 *                  Input: None
 *                  Output: a Tile pointer
 *                  Calls: None
 */

Tile* Sudoku::min_choice_tile()
{
    Tile *choice = NULL;
    unsigned int min_tile_num = dim;
    for(unsigned int i = 0; i < matrix.size(); i++)
    {
        for(unsigned int j = 0; j < matrix[i].size(); j++)
        {
            if(matrix[i][j]->val == -1)
            {
                if(matrix[i][j]->candidates.size() < min_tile_num)
                {
                    choice = matrix[i][j];
                    min_tile_num = matrix[i][j]->candidates.size();
                }
            }
        }
    }
    return choice;
}

/*
 * as_list()
 *              Description: Saves the current state of puzzles via the current
 *                           puzzles values.
 *              Input: None
 *              Output: A 2d vector of values representing the current state of
 *                      the values in the puzzle
 *              Calls: None
 */
vector<vector<int>> Sudoku::as_list()
{
    vector<vector<int>> value_list(dim);
    for(unsigned int i = 0; i < matrix.size(); i++)
    {
        for(unsigned int j = 0; j < matrix[i].size(); j++)
        {
            value_list[i].push_back(matrix[i][j]->val);
        }
    }
    return value_list;
}

/*
 * candidates_list()
 *                      Description: Saves the current state of the puzzles by
 *                                   storing each Tile's list of candidates.
 *                      Input: None
 *                      Output: 2d grid of the puzzles candidates
 *                      Calls: None
 */

vector<vector<vector<int>>> Sudoku::candidates_list()
{
    vector<vector<vector<int>>> candidates_list(dim);
    for(unsigned int i = 0; i < matrix.size(); i++)
    {
        for(unsigned int j = 0; j < matrix[i].size(); j++)
        {
            candidates_list[i].push_back(matrix[i][j]->candidates);
        }
    }
    return candidates_list;
}

/*
 * restore_values()
 *                      Description: For a given 2d grid of values, stores the
 *                                   values back into the puzzle.
 *                      Input: Saved values for the puzzle
 *                      Output: None
 *                      Calls: None
 */

void Sudoku::restore_values(vector<vector<int>> value_list)
{
    for(unsigned int i = 0; i < matrix.size(); i++)
    {
        for(unsigned int j = 0; j < matrix[i].size(); j++)
        {
            matrix[i][j]->val = value_list[i][j];
        }
    }
}

/*
 * restore_values()
 *                      Description: For a given 2d grid of candidates, stores 
 *                                   the candidates back into the puzzle.
 *                      Input: Saved candidates for the puzzle
 *                      Output: None
 *                      Calls: None
 */

void Sudoku::restore_candidates(vector<vector<vector<int>>> candidate_list)
{
    for(unsigned int i = 0; i < matrix.size(); i++)
    {
        for(unsigned int j = 0; j < matrix[i].size(); j++)
        {
            matrix[i][j]->candidates = candidate_list[i][j];
        }
    }
}

/*
 * print()
 *          Description: Prints current state of puzzle to stdout.
 *          Input: None
 *          Output: None
 *          Calls: None
 */

void Sudoku::print()
{
	cout << "Dimension: " << dim << endl;
	cout << "Puzzle: " << endl;

	int out = (int)matrix.size();
	for(int i = 0; i < out; i++)
	{
		cout << "\t";
		int in = (int)matrix[i].size();
		for(int j = 0; j < in; j++)
		{
			if(matrix[i][j]->val == -1)
				cout << ".";
			else
			{
				if(dim > 16)
				{
					int num = matrix[i][j]->val + 96;
					printf("%c", (char)num);
				}
				else if(dim == 16)
				{
					printf("%x", matrix[i][j]->val - 1);
				}
				else
					printf("%x", matrix[i][j]->val);
			}
			cout << " ";
		}
		cout << endl;
	}
}

/*
 * is_valid()
 *              Description: Checks if the current state of the puzzle is
 *                           valid. Returns 1 if valid and 0 otherwise. Stores
 *                           type of error (if any) in return_status.
 *              Input: int &return_status -> error message
 *              Output: int, a boolean whether the puzzle is valid 
 *              Calls: None
 */

int Sudoku::is_valid(int &return_status)
{
	int success = 1;
	return_status = 0;
	for(unsigned int i = 0; i < groups.size(); i++)
	{
		vector<Tile*> group = groups[i];
		int stop = 0;

		// check to make sure each one is valid
		for(int j = 0; j < dim; j++)
		{
			for(int k = j + 1; k < dim; k++)
			{
				if(group[j]->val == group[k]->val && group[j]->val != -1)
				{
					success = 0;
					stop = (i % 3) + 1;
					return_status = (stop * 10000) + 
									(group[j]->row + 1) * 1000 + (group[j]->col * 100) +
									(group[k]->row * 10) + group[k]->col;
					break;
				}
			}

			if(stop)
				break;
		}
		if(!success)
			break;
	}

	return success;
}

/*
 * is_complete()
 *                  Description: Checks if puzzle is entirely filled with
 *                               values. Does not check if it is valid.
 *                  Input: None
 *                  Output: None
 *                  Calls: None
 */

int Sudoku::is_complete()
{
    for(int i = 0; i < dim; i++)
    {
        for(int j = 0; j < dim; j++)
        {
            if(matrix[i][j]->val == -1)
                return 0;
        }
    }
    return 1;
}

/*
 * get_row()
 *              Description: Returns a certain row from a 2d grid of vectors
 *              Input: input -> 2d grid of vectors from which to get a row
 *                     row -> index of the row to get from input
 *              Output: a row from the matrix
 *              Calls: None
 */
vector<Tile*> Sudoku::get_row(int row)
{
	vector<Tile*> tmp;
	for(int i = 0; i < dim; i++)
	{
		tmp.push_back(matrix[row][i]);
	}
	return tmp;

}
/*
 * get_col()
 *              Description: Returns a certain col from a 2d grid of vectors
 *              Input: input -> 2d grid of vectors from which to get a col
 *                     col -> index of the col to get from input
 *              Output: a col from the matrix
 *              Calls: None
 */

vector<Tile*> Sudoku::get_col(int col)
{
	vector<Tile*> tmp;
	for(int i = 0; i < dim; i++)
	{
		tmp.push_back(matrix[i][col]);
	}
	return tmp;
	
}

/*
 * get_nonet()
 *              Description: Returns a certain nonet from a 2d grid of vectors.
 *                           The ordering of nonets is as follows:
 *                           0 1 2
 *                           3 4 5
 *                           6 7 8
 *              Input: input -> 2d grid of vectors from which to get a nonet
 *                     nonet -> index of the nonet to get from input
 *              Output: a nonet from the matrix
 *              Calls: None
 */

vector<Tile*> Sudoku::get_nonet(int non)
{
	vector<Tile*> tmp;
	int row = (non / nonet) * nonet;
    int col = (non % nonet) * nonet;
    int saved_col = col;
	for(int i = 0; i < dim; i++)
	{
		tmp.push_back(matrix[row][col++]);
        if((i + 1) % nonet == 0)
        {
            row++;
            col = saved_col;
        }
	}
	return tmp;
}
