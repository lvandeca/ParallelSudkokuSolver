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
 *              2021. Implementation of a parallel sudoku solver. 
 * Notes: 
 *       1. See header.h for all class methods and variables.
 *       2. All methods contain headers for individual usage and description.
 *
 */

#include "header.h"

// Constructor
Parallel::Parallel(vector<vector<Tile*>> in, int n)
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
        vector<Tile*> tmp = get_row(matrix, i);
        groups.push_back(tmp);

        tmp = get_col(matrix, i);
        groups.push_back(tmp);

        tmp = get_nonet(matrix, i);
        groups.push_back(tmp);

    }
}

// Destructor
Parallel::~Parallel()
{
    /* 
     * do nothing
     * vector will be cleaned up automatically by its destructor 
     */
}

/*
 *  solve()
 *          Description: Driver for a parallel sudoku solver. Uses a
 *                       combination of lone_ranger() and elimination() tactics
 *                       as well as a parallel guess and check algorithm.
 *          Input: None
 *          Output: None
 *          Calls: propagate(), is_complete(), is_valid(), and Tile class
 *                 methods. 
 */

void Parallel::solve()
{
    cout << "Parallel method...";

    // first see if we can solve without guess and check
    propagate(groups);
    if(is_complete())
        return;

    // set up stack and lock for parallel regions
    vector<vector<Tile>> stack;
    omp_lock_t stack_lock;
    omp_init_lock(&stack_lock);

    // determine the # of elements to initialize
    // our stack with based off of the # of threads
    int range;
    #pragma omp parallel
    {
        range = omp_get_num_threads() / 2;
    }

	if(range > dim)
		range = dim;
	else if(range < 1)
		range = 1;

    // initialize stack with some paths to start with
    // do this in serial since it is faster
    for(int i = 0; i < range; i++)
    {
        Tile* min_tile = min_choice_tile_row(i);
        for(unsigned int j = 0; j < min_tile->candidates.size(); j++)
        {
            vector<Tile> path;
            vector<int> empty;
            Tile tmp(min_tile->row, min_tile->col, min_tile->candidates[j]);
            tmp.candidates = empty;
            path.push_back(tmp);
            stack.push_back(path);
        }
    }

    // guess and check loop
    #pragma omp parallel
    {
        while (!stack.empty()) {
            // place cancellation point at beginning for
            // increased performance
            #pragma omp cancellation point parallel

            // make copy of groups for each thread to use
            vector<vector<Tile *>> tmp_matrix = copy_matrix(matrix);

            // get next path off stack
            vector<Tile> path;
            #pragma omp cancellation point parallel
            if(!stack.empty()) {
                omp_set_lock(&stack_lock);
                path = stack.back();
                stack.pop_back();
                omp_unset_lock(&stack_lock);
            }

            // create puzzle using path
            for (unsigned int i = 0; i < path.size(); i++) {
                int row = path[i].row;
                int col = path[i].col;
                tmp_matrix[row][col]->val = path[i].val;
                tmp_matrix[row][col]->candidates = path[i].candidates;
            }

            Sudoku thread_puzzle(tmp_matrix, dim);
            // solve as much as we can
            #pragma omp cancellation point parallel
            thread_puzzle.propagate();

            // check state of puzzle
            int status;
            if (!thread_puzzle.is_valid(status)) {
                // continue to next puzzle in the stack
                continue;
            } else {
                // puzzle is solved
                if (thread_puzzle.is_complete()) {
                    restore(thread_puzzle.groups);
                    # pragma omp cancel parallel

                // not solved, but valid, store new potential
                // guess puzzles, then move to next puzzle in stack
                } else {
                    Tile *min_tile = thread_puzzle.min_choice_tile();
                    for (unsigned int i = 0; i < min_tile->candidates.size(); i++) {
                        vector<int> empty;
                        Tile tmp(min_tile->row, min_tile->col, min_tile->candidates[i]);
                        tmp.candidates = empty;
                        path.push_back(tmp);

                        omp_set_lock(&stack_lock);
                        stack.push_back(path);
                        omp_unset_lock(&stack_lock);

                        path.pop_back();
                    }
                }
            }
            #pragma omp cancellation point parallel
        }
    }
    omp_destroy_lock(&stack_lock);
    cout << "done" << endl;
}

/*
 * print()
 *          Description: Prints current state of puzzle to stdout.
 *          Input: None
 *          Output: None
 *          Calls: None
 */

void Parallel::print()
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

int Parallel::is_valid(int &return_status)
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

int Parallel::is_complete()
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

vector<Tile*> Parallel::get_row(vector<vector<Tile*>> input, int row)
{
    vector<Tile*> tmp;
    for(int i = 0; i < dim; i++)
    {
        tmp.push_back(input[row][i]);
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

vector<Tile*> Parallel::get_col(vector<vector<Tile*>> input, int col)
{
    vector<Tile*> tmp;
    for(int i = 0; i < dim; i++)
    {
        tmp.push_back(input[i][col]);
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

vector<Tile*> Parallel::get_nonet(vector<vector<Tile*>> input, int non)
{
    vector<Tile*> tmp;

    // calculate the row and col offset for the top right index into the puzzle
    // for a specific nonet
    int row = (non / nonet) * nonet;
    int col = (non % nonet) * nonet;
    int saved_col = col;
    for(int i = 0; i < dim; i++)
    {
        tmp.push_back(input[row][col++]);

        // every "nonet" # values,
        // reset col index to start at the far left of the nonet
        // and move to next row in the nonet
        if((i + 1) % nonet == 0)
        {
            row++;
            col = saved_col;
        }
    }
    return tmp;
}

/*
 * copy_matrix()
 *                  Description: Creates an identical copy of a 2d matrix
 *                               of Tile pointers. Used in solve().
 *                  Input: 2d source matrix
 *                  Output: copy of the input matrix
 *                  Calls: None
 */

vector<vector<Tile*>> Parallel::copy_matrix(vector<vector<Tile*>> input)
{
    vector<vector<Tile*>> output;
    for(unsigned int i = 0; i < input.size(); i++)
    {
        vector<Tile*> tmp;
        for(unsigned int j = 0; j < input[i].size(); j++)
        {
            // allocate memory for pointer to Tile object
            Tile *new_tile = new Tile();
            new_tile->row = input[i][j]->row;
            new_tile->col = input[i][j]->col;
            new_tile->val = input[i][j]->val;
            new_tile->candidates = input[i][j]->candidates;
            tmp.push_back(new_tile);
        }
        output.push_back(tmp);
    }
    return output;
}

/*
 * group_create()
 *                  Description: For a given matrix, generates an array of row,
 *                               column, and nonet arrays from the matrix 
 *                               representation. This way, when checking for
 *                               candidate values for a square, we do not have
 *                               to manually check the row, column and nonet 
 *                               for each square. Instead, each of these are
 *                               stored in the group format.
 *                  Input: 2d matrix of Tiles
 *                  Output: row, col, and nonet groups of Tiles of the matrix
 *                  Calls: get_row(), get_col(), get_nonet()
 */

vector<vector<Tile*>> Parallel::group_create(vector<vector<Tile*>> cpy_matrix)
{
    vector<vector<Tile*>> output;
    for(unsigned int i = 0; i < cpy_matrix.size(); i++)
    {
        vector<Tile*> tmp = get_row(cpy_matrix, i);
        output.push_back(tmp);

        tmp = get_col(cpy_matrix, i);
        output.push_back(tmp);

        tmp = get_nonet(cpy_matrix, i);
        output.push_back(tmp);
    }
    return output;
}

/*
 * restore()
 *              Description: Given a solved puzzle (in group form), store the
 *                           puzzles state in the objects matrix variable.
 *              Input: group format of a solved puzzle
 *              Output: None
 *              Calls: None
 */

void Parallel::restore(const vector<vector<Tile*>>& solved)
{
    for(unsigned int i = 0, j = 0; i < solved.size(); i += 3, j++)
    {
        matrix[j] = solved[i];
    }
}

/*
 * min_choice_tile()
 *                  Description: For a given puzzle in group format, this
 *                               function finds the Tile with the least amount
 *                               of candidates.
 *                  Input: a puzzle in group format
 *                  Output: a Tile pointer
 *                  Calls: None
 */

Tile* Parallel::min_choice_tile(vector<vector<Tile*>> puzzle)
{
    /* puzzle will be group format */
    Tile *choice = NULL;
    unsigned int min_tile_num = dim;
    // only iterate through the row group in the puzzle
    for(unsigned int i = 0; i < puzzle.size(); i += 3)
    {
        for(unsigned int j = 0; j < puzzle[i].size(); j++)
        {
            if(puzzle[i][j]->val == -1)
            {
                if(puzzle[i][j]->candidates.size() < min_tile_num)
                {
                    choice = puzzle[i][j];
                    min_tile_num = puzzle[i][j]->candidates.size();
                }
            }
        }
    }
    return choice;
}

/*
 * min_choice_tile_row()
 *                      Description: See min_choice_tile(). Operates the same
 *                                   except it finds the tile with the least 
 *                                   amount of candidates in specific row, and
 *                                   it operates over the objects matrix form 
 *                                   of the puzzle. Used only for initiating 
 *                                   the stack in solve().
 *                      Input: A given row to search
 *                      Output: A Tile pointer to the minimum tile in that row
 *                      Calls: None
 */

Tile* Parallel::min_choice_tile_row(int row)
{
    Tile *choice = NULL;
    unsigned int min_tile_num = dim;
    for(unsigned int j = 0; j < matrix[row].size(); j++)
    {
        if(matrix[row][j]->val == -1)
        {
            if(matrix[row][j]->candidates.size() < min_tile_num)
            {
                choice = matrix[row][j];
                min_tile_num = matrix[row][j]->candidates.size();
            }
        }
    }
    return choice;
}

/*
 * propagate()
 *              Description: Driver function for tactics to propagate values
 *                           into each tile in the puzzle. Input puzzle is in
 *                           group format.
 *              Input: 2d grid of Tile's to apply tacitcs to
 *              Ouput: None
 *              Calls: elimination() lone_ranger()
 */

void Parallel::propagate(vector<vector<Tile*>> &input)
{
    // keep applying tactics until we no longer
    // are making changes to the candidates of the puzzle
    int cont = 1;
    while(cont)
    {
        cont = elimination(input);
        lone_ranger(input);
    }
}

/*
 * elimination()
 *              Description: For a given puzzle in group form, this function
 *                           checks for each group (row, column, or nonet)
 *                           the possible candidates for that group, and then
 *                           for each tile in the group, then eliminates 
 *                           candidates for that particular tile depending upon
 *                           which values are available in that group. If there
 *                           is only one available candidate left for a Tile,
 *                           then the Tile's value is set to that candidate.
 *              Input: 2d grid of Tile's
 *              Output: returns 1 if a change was made, and 0 if a change was
 *                      not made to any Tile's candidates in the puzzle
 *              Calls: Tile.remove_candidates()
 */

int Parallel::elimination(vector<vector<Tile*>> &input)
{
    // loop through each group of the puzzle
    int return_value = 0;
    for(unsigned int i = 0; i < input.size(); i++)
    {
        // determine which values are already used
        // in the group (i.e. cannot be candidates)
        set<int> used_values;
        for(unsigned int j = 0; j < input[i].size(); j++)
        {
            if(in(input[i][j]->val, choices))
                used_values.insert(input[i][j]->val);
        }

        // for each tile, eliminate candidates for that tile
        for(unsigned int j = 0; j < input[i].size(); j++)
        {
            vector<int> tmp{used_values.begin(), used_values.end()};
            int change_made = input[i][j]->remove_candidates(tmp);

            // set return value if a change was made to the
            // candidates of the tile
            if(change_made)
                return_value = 1;
        }
    }
    return return_value;

}

/*
 * lone_ranger()
 *              Description: For a given puzzle in group for, this function
 *                           checks for each group (row, column, or nonet)
 *                           if there is an empty Tile such that one of the
 *                           candidates for that empty Tile does not appear
 *                           as one of the candidates for any other Tile in the
 *                           same group. If this is true, then it sets that 
 *                           particular candidate as the value for the Tile.
 *              Input: 2d puzzle of Tile's
 *              Output: returns 1 if a change was made to the state of the 
 *                      puzzle, 0 otherwise.
 *              Calls: None
 */

int Parallel::lone_ranger(vector<vector<Tile*>> &input)
{
    // loop through groups of the puzzle
    int return_value = 0;
    for (unsigned int i = 0; i < input.size(); i++) {

        // find which values have not been used for the group
        set<int> leftovers{choices.begin(), choices.end()};
        for (unsigned int j = 0; j < input[i].size(); j++) {
            if (in(input[i][j]->val, choices))
                leftovers.erase(groups[i][j]->val);
        }

        // for each available value of the group
        vector<int> replace{leftovers.begin(), leftovers.end()};
        for (unsigned int j = 0; j < replace.size(); j++) {

            // iterate through each tile in the group, counting
            // how many empty tiles have available
            // value as a candidate
            int num_leftover = 0;
            int place_holder;
            for (unsigned int k = 0; k < input[i].size(); k++) {
                if (in(replace[j], input[i][k]->candidates)) {
                    num_leftover++;
                    place_holder = k;
                }
            }

            // only one empty tile has available value as a candidate
            // must be the value for the Tile
            if (num_leftover == 1) {
                input[i][place_holder]->val = replace[j];
                input[i][place_holder]->candidates.clear();
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

int Parallel::in(int val, vector<int> list)
{

    // loop through vector checking for value
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
