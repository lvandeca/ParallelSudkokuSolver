/*
 * Author: Luke Vandecasteele
 * 
 * Credits: Matthew Trappert, Dr. Jee W. Choi, Class notes, and other sources
 *			listed in final report.
 *
 * Date Created: 11/8/2021
 * Last Modified: 11/30/2021
 *
 * Description: Final project for CIS 431 at the University of Oregon Fall
 *				2021. Driver function for the implementation of a Parallel 
 *				Sudoku Solver using OpenMP. See project README.md for how to
 *				run the solver, as well as provide proper input.
 * Notes: 
 * 		 1. All results and data were collected on Talapas HPC system on a 
 *		    single node with varying number of threads.
 *		 2. For an in depth description of the parallel algorithm, please see
 * 			the presentation in the ProjectDocuments folder or the final report
 *		 3. Can currently solve 16x16 puzzles in under a second. Needs to be
 *			improved in order to solve large puzzles in a reasonable time. 
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <set>
#include <math.h>
#include <string.h>
#include <omp.h>
#include "common.h"
#include "header.h"


using namespace std;


/* Functions */
void usage(char *argv);
void read_info(char *file, vector<vector<Tile*>>& matrix, int& dim);

int main(int argc, char **argv)
{
	// check proper arguments
	if(argc > 3)
	{
		usage(argv[0]);
		return 0;
	}

	/* Note:
	 * Only one option can be given.
	 *
	 * To add additional solving methods (i.e., options) make sure to add 
	 * seperate boolean and flag for it. Also need to add additional if
	 * statement after the file has been read for the proper solving method.
	 * 
	 * If multiple flags are given, the first flag processed will be used.
	 */


	/* add boolean here */
	int serial = 0;
    int parallel = 0;

	int flag;
	opterr = 0;

	/* add flag to string */
	while((flag = getopt(argc, argv, "sp")) != -1)
	{
		switch(flag)
		{
			/* add new case for flag here */
			case 's':
				serial = 1;
				break;

            case 'p':
                parallel = 1;
                break;

			case '?':
				cerr << "Invalid option" << endl;
				usage(argv[0]);
				return 0;
		}
	}


	// read sudoku puzzle from file
	vector<vector<Tile*>> matrix;
	int dim;
	read_info(argv[2], matrix, dim);
  	double perfect_square = sqrt(dim) - (int)sqrt(dim);

	if(!dim || perfect_square)
	{
		usage(argv[0]);
		return 0;
	}


	// initialize timer
    InitTSC();
    unsigned long start = ReadTSC();


	/* add if statement for solving method here */
	int return_value;
	if(serial)
	{
		// init puzzle and print puzzle
        Sudoku puzzle(matrix, dim);
        puzzle.print();

        // check that input puzzle is vaild 
        cout << "Checking valid puzzle...";
        int valid = puzzle.is_valid(return_value);
        if(!valid)
            cout << "failure" << endl;
        else
            cout << "success" << endl;

        // solve
        start = ReadTSC();
		puzzle.solve();
        cout << "Time to solve: " << ElapsedTime(ReadTSC() - start) << endl;

        // print solved puzzle and check if valid
        puzzle.print();
        cout << "Checking valid puzzle...";
        valid = puzzle.is_valid(return_value);
        if(!valid)
            cout << "failure" << endl;
        else
            cout << "success" << endl;
	}
    else if(parallel)
    {
    	// init puzzle and print
        Parallel puzzle(matrix, dim);
        puzzle.print();

        // check that input is valid
        cout << "Checking valid puzzle...";
        int valid = puzzle.is_valid(return_value);
        if(!valid)
            cout << "failure" << endl;
        else
            cout << "success" << endl;

        // solve
        start = ReadTSC();
        puzzle.solve();
        cout << "Time to solve: " << ElapsedTime(ReadTSC() - start) << endl;

        // print solution and one final check that its valid
        puzzle.print();
        cout << "Checking valid puzzle...";
        puzzle.is_valid(return_value);
        valid = puzzle.is_valid(return_value);
        if(!valid)
            cout << "failure" << endl;
        else
            cout << "success" << endl;
    }

	return 1;
}

void usage(char *argv)
{
	// for proper usage of this file
	cout << "Usage: "
	     << argv
	     << " [OPTION]... [FILE]..."
	     << endl;
}

void read_info(char *file, vector<vector<Tile*>>& matrix, int& dim)
{
	dim = 0;
	FILE *fp = fopen(file, "r");
	if(fp != NULL)
	{
		// get dimension
		fscanf(fp, "%d\n", &dim);

		int line = 0;
		size_t len = 2048;
		char *buf = (char *)malloc(len);
		while(getline(&buf, &len, fp) != -1)
		{

			// confirm row length == dim and that
			// col length < dim
			if(strlen(buf) - 1 != (long unsigned)dim || line > dim)
			{
				dim = 0;
				break;
			}

			// get row values
			vector<Tile*> tmp;
			for(int i = 0; i < dim; i++)
            {
            	// create new tile, store values, put into puzzle
            	// different methods for different inputs depending
            	// upon if the puzzle is 9x9, 16x16 or 25x25
                Tile *nnew = new Tile();
				char num = tolower(buf[i]);
				if(num == 46)
				{
					nnew->row = line;
					nnew->col = i;
					nnew->val = -1;
				}
				else
				{
					nnew->row = line;
					nnew->col = i;
					if(dim > 16)
						nnew->val = (int)num - 96;
					else if(dim == 16)
					{
						if(num > 96)
							nnew->val = (int)(num - 86);
						else
							nnew->val = (int)(num - 47);
					}
					else
						nnew->val = (int)(num - '0');
				}
                vector<int>work;
                nnew->candidates = work;
				tmp.push_back(nnew);
			}
			matrix.push_back(tmp);
			line++;
		}
		free(buf);
		fclose(fp);
	}
}
