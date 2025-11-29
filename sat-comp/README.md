# Blyat-Solver

## Overview
This sat solver is an implementation of the DPLL algorihtm for sat. This solver takes as input files in the DIMACS CNF format and returns either "SATISFIABLE" or "UNSATISFIABLE".

Before the solving occurs, the input is parsed into a struct of type sat_t. the struct contains the following members:
- clauses: the number of clauses that are currently remaining in the problem instance
- rows: the number of clauses in the full problem (before dpll starts). This value is used for iterating the 2d array
- columnms: the number of propositional atoms in the problem. This value is also used to iterate through the array
- assignemnt: array containing the current assignment for the problem instance. each variblae is assigned a 1 (true), -1 (false), or 0 (unassigned)
- content: an M x N array with the contents of the M claused and N literals of the sat problem instance. each entry contains a 1, -1, or 0 to signify if there is a positive atom, negitve atom, or no atom


My solver implement a DPLL method that is responsible for the backtracking part of the algorithm (exploring new branches) and there are various other methods responsible for the logic. 
At the start of the DPLL method we try to propagate first to see if the formula is sat or unsat before branching on different partial assignments. Unit propagation works by searching for unit clauses, and we delete atoms or clauses accordingly from the "content" matrix. Each time a clause is deleted, we decrement the "clauses" member. If this member reaches 0 we know the problem is SAT. On the other hand, if we delete an atom (but not a clause) and afterwards the row contains all zeros, then we know the problem is UNSAT. If neither of these outcomes are reached, we must branch and try partial assignments of the problem.
My solver branches in a very simple deterministic way. First we try assigning false to the smallest unassigned atom (p_1 at the start) then true. The DPLL tries all necessary assignments recursively. 
My solver also implements pure literal elimination to optimise to the DPLL algorithm.
To preserve problem instances between branches, we copy the problem instance into a new struct using a sat_copy method. 
I have also provided a print_sat funcition that prints the problem instance, i.e every member of the sat_t struct. This method is for debugging and not actually used in the solver.

For this project I used C because it has a fairly good compiler (gcc) that produces an efficient executable. I also wanted to practice programming in C so this language was a perfect choice for this. It was a challenge to write a lot of basic functionality that you get for free in other languages though. The source code for an equivalent Java or C++ program is probably half as long.

## Complexity
DPLL is known to have O(2^n) complexity where n is the number of atoms in the literal, since we might need to check every assignment in the worst case. My algorithm in particular will be (mn\*2^n) since each branch does a loop thorugh the whole matrix. The space complexity is O(mn) because we store at most 1 active copy of the sat instance at each level of branching. If we never freed memory in the program then the space complpeity woukld be O(mn\*2^n).

## Test results
The test results for each data set can all be found in the out_dir directory

## Compilation:
To compile the source file, use the command:

**gcc -o blyat-solver blyat-solver.c**

* Author: 
Marek Wicha-Frankiewicz


