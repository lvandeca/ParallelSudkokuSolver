/*
 * Author: Luke Vandecasteele
 * 
 * Credits: Matthew Trappert, Dr. Jee W. Choi, Class notes, and other sources
 *          listed in final report.
 *
 * Date Created: 11/8/2021
 * Last Modified: 12/1/2021
 *
 * Description: Final project for CIS 431 at the University of Oregon Fall
 *              2021. Header file for Tile, Sudoku, and Parallel classes. 
 *              Sudoku class contains the serial implementation of the solver
 *              while the Parallel class is clearlly the parallel version. 
 * Notes: 
 *       1. The classes, Sudoku and Parallel, contain a 2x2 grid of Tile class
 *          objects.
 *       2. All attributes of the classes are public for simplicity and ease of
 *          use.
 *
 */

// forward declarations
class Tile;
class Sudoku;


#ifndef HEADER_H_
#define HEADER_H_

#include <iostream>
#include <vector>
#include <math.h>
#include <set>
#include <algorithm>
#include <unistd.h>
#include <omp.h>
#include <stack>


using namespace std;


class Tile
{
public:

    // basic attributes
	int row;
	int col;
	int val;
	vector<int> candidates;

	// constructors
	Tile();
	Tile(int r, int c, int v);

	//destructor
	~Tile();

	// helper functions
	void print();
	void gen_candidates(vector<int> choices);
    int remove_candidates(vector<int> used_values);

};


class Sudoku
{
public:

	// variables 
	vector<vector<Tile*>> matrix;
	int dim;
	int nonet;
	vector<vector<Tile*>> groups;
	vector<int> choices;

	// Constructor
	Sudoku(vector<vector<Tile*>> in, int n);
	// Destructor
	~Sudoku();

    // driver for solver
    // uses recursive guess and check and tactics
    int solve();

    // propagate is the driver function for our two 
    // tactics, lone_ranger and elimination
    void propagate();
    int elimination();
    int lone_ranger();


    // find optimal tile to make guesses for
    Tile* min_choice_tile();

    // for saving and restore state of the puzzle
    // when incorrect guesses are made during
    // the recursive guess and check
    vector<vector<int>> as_list();
    vector<vector<vector<int>>> candidates_list();
    void restore_values(vector<vector<int>> value_list);
    void restore_candidates(vector<vector<vector<int>>> candidate_list);

    // helper functions for initiating the puzzle
	vector<Tile*> get_row(int row);
	vector<Tile*> get_col(int col);
	vector<Tile*> get_nonet(int nonet);

    // ease of use helper functions
    void print();
    int in(int val, vector<int> list);
    int is_valid(int &return_status);
    int is_complete();
};

class Parallel
{
public:
    //variables
    vector<vector<Tile*>> matrix;
    int dim;
    int nonet;
    vector<vector<Tile*>> groups;
    vector<int> choices;

    // Constructor
    Parallel(vector<vector<Tile*>> in, int n);
    // Destructor
    ~Parallel();

    // driver function for solving the puzzle
    // contains algorithm for parallel smart
    // guess and check
    void solve();

    // used to create copy of the puzzle for threads to use
    vector<vector<Tile*>> copy_matrix(vector<vector<Tile*>> input);
    vector<vector<Tile*>> group_create(vector<vector<Tile*>> cpy_matrix);

    // to store solved puzzle in class variables
    void restore(const vector<vector<Tile*>>& solved);

    // to find optimal tile to make guesses for
    Tile* min_choice_tile(vector<vector<Tile*>> puzzle);
    Tile* min_choice_tile_row(int row);

    // propagate is the driver function for our two 
    // tactics, lone_ranger and elimination
    void propagate(vector<vector<Tile*>> &input);
    int lone_ranger(vector<vector<Tile*>> &input);
    int elimination(vector<vector<Tile*>> &input);

    // functions for puzzle initialization
    vector<Tile*> get_row(vector<vector<Tile*>> input, int row);
    vector<Tile*> get_col(vector<vector<Tile*>> input, int col);
    vector<Tile*> get_nonet(vector<vector<Tile*>> input, int nonet);

    // ease of use helper functions
    void print();
    int in(int val, vector<int> list);
    int is_valid(int &return_status);
    int is_complete();
};

#endif