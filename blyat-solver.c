#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    int clauses;    //n.o clauses not yet satisfied
    int rows;       //n.o clauses  
    int columns;    //n.o atoms
    int *assignment; //complete or incomplete assignment for the boolean variables
    int **content;  //2d array of clauses
                    //format of 2d array:
                    // - each row is a clause
                    // - each entry is an atom
                    //     - 0 means no atom present
                    //     - 1 means the atom p_i is present
                    //     - -1 mean the atom ¬p_i is present
} sat_t;

int init_sat_arr(sat_t *sat) {
    //allocate memory and initialise every entry to 0
    sat->assignment = calloc(sat->columns, sizeof(int));
    sat->content = calloc(sat->rows, sizeof(int *));
    if (!sat->content) {
        return -1;
    }

    for (int i = 0; i < sat->rows; i++) {
        sat->content[i] = calloc(sat->columns, sizeof(int));
        if (!sat->content[i]) {
            for (int j = 0; j < i; j++) {
                free(sat->content[j]);
            }
            free(sat->content);
            return -1;
        }
    }

    return 0;
}

void free_sat(sat_t *sat) {
    if (!sat) {
        return;
    }
    if (sat->content) {
        for (int i = 0; i < sat->rows; i++) {
            free(sat->content[i]);
        }
        free(sat->content);
        sat->content = NULL;
    }
    if (sat->assignment) {
        free(sat->assignment);
        sat->assignment = NULL;
    }
    free(sat);
}

void print_sat(sat_t *sat) {
    printf("rows: %d\n", sat->rows);
    printf("columns: %d\n", sat->columns);
    printf("current clauses: %d\n", sat->clauses);
    printf("current assignment: ");
    for (int i = 0; i < sat->columns; i++) {
        printf("%d ", sat->assignment[i]);
    }
    printf("\nproblem:\n");
    for (int i = 0; i < sat->rows; i++) {
        for (int j = 0; j < sat->columns; j++) {
            printf("%d ", sat->content[i][j]);
        }
        printf("\n");
    }
}

bool char_is_digit(char c) {
    if (c >= '0' && c <= '9'){
        return true;
    }
    return false;
}

void remove_clause(sat_t *sat, int clear_row) {
    if (!sat) {
        return;
    }
    bool had_literal = false;
    for (int i = 0; i < sat->columns; i++) {
        if (sat->content[clear_row][i] != 0) had_literal = true;
        sat->content[clear_row][i] = 0;
    }
    if (had_literal) {
        sat->clauses--;
    }
}

int parse_file_and_init_sat(char *input_file, sat_t *sat) {
    FILE *fp = fopen(input_file, "r");
    if (!fp) return -1;

    size_t capacity = 4096;
    size_t size = 0;
    char *buffer = malloc(capacity);
    if (!buffer) { 
        fclose(fp);
        return -1;
    }

    size_t bytes_read;
    while ((bytes_read = fread(buffer + size, 1, capacity - size, fp)) > 0) {
        size += bytes_read;
        if (size >= capacity - 1) {
            capacity *= 2;
            char *new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                free(buffer);
                fclose(fp);
                return -1;
            }
            buffer = new_buffer;
        }
    }
    if (ferror(fp)) {
        free(buffer);
        fclose(fp);
        return -1;
    }

    buffer[size] = '\0';
    fclose(fp);

    sat->rows = -1;
    sat->columns = -1;
    bool comment = false;
    bool p = false;
    bool sat_initialised = false;
    int first_clause_pos = -1;

    // determine rows and columns in the matrix
    for (int i = 0; i < (int)size; i++) {
        if (buffer[i] == 'c') {
            comment = true;
        }
        if (buffer[i] == '\n') {
            comment = false;
            p = false;
        }
        if (comment) { 
            continue;
        }
        if (buffer[i] == 'p' && !sat_initialised) {
            p = true;
        }
        if (p && !sat_initialised) {
            char num_buffer[64];
            int nb_pos = 0;
            while (i < (int)size && !char_is_digit(buffer[i])) {
                i++;
            }
            while (i < (int)size && char_is_digit(buffer[i]) && nb_pos < (int)sizeof(num_buffer)-1) {
                num_buffer[nb_pos++] = buffer[i++];
            }
            num_buffer[nb_pos] = '\0';
            if (nb_pos == 0) return -1;
            sat->columns = atoi(num_buffer); 

            nb_pos = 0;
            while (i < (int)size && !char_is_digit(buffer[i])) {
                i++;
            }
            while (i < (int)size && char_is_digit(buffer[i]) && nb_pos < (int)sizeof(num_buffer)-1) {
                num_buffer[nb_pos++] = buffer[i++];
            }
            num_buffer[nb_pos] = '\0';
            if (nb_pos == 0) return -1;
            sat->rows = atoi(num_buffer); 
            sat->clauses = sat->rows;

            if (sat->rows <= 0 || sat->columns <= 0 || sat->clauses <= 0) {
                return -1;
            }

            if (init_sat_arr(sat) != 0) {
                return -1;
            }
            sat_initialised = true;

            while (i < (int)size && buffer[i] != '\n') {
                i++;
            }
            if (i < (int)size) {
                first_clause_pos = i + 1;
            }
            else {
                first_clause_pos = i;
            }
        }
        if (sat_initialised) {
            break;
        }
    }

    if (!sat_initialised || first_clause_pos < 0) {
        free(buffer);
        return -1;
    }

    // fill the clauses: each clause line has integers terminating with 0
    int row = 0;
    bool tautology = false;
    for (int i = first_clause_pos; i < (int)size && row < sat->rows; i++) {
        while (i < (int)size && buffer[i] == ' ') {
            i++;
        }
        while (i < (int)size) {
            while (i < (int)size && !char_is_digit(buffer[i]) && buffer[i] != '-' && buffer[i] != '0') {
                if (buffer[i] == '\n') {
                    break;
                }
                i++;
            }
            if (i >= (int)size) {
                break;
            }
            if (buffer[i] == '\n') {
                break;
            }
            if (buffer[i] == '0') {
                i++;
                break;
            }

            char num_buffer[64];
            int nb_pos = 0;
            if (buffer[i] == '-') {
                num_buffer[nb_pos++] = buffer[i++];
            }
            while (i < (int)size && char_is_digit(buffer[i]) && nb_pos < (int)sizeof(num_buffer)-1) {
                num_buffer[nb_pos++] = buffer[i++];
            }
            num_buffer[nb_pos] = '\0';
            if (nb_pos == 0) {
                i++;
                continue;
            }
            int value = atoi(num_buffer);

            // add atom to clause
            // remove tautology if detected
            if (value < 0) {
                if (sat->content[row][abs(value) - 1] == 1) {
                    remove_clause(sat, row);
                    tautology = true;
                } else if (!tautology) {
                    sat->content[row][abs(value) - 1] = -1;
                }
            } 
            else {
                if (sat->content[row][abs(value) - 1] == -1) {
                    remove_clause(sat, row);
                    tautology = true;
                } else if (!tautology) {
                    sat->content[row][abs(value) - 1] = 1;
                }
            }

            while (i < (int)size && (buffer[i] == ' ' || buffer[i] == '\t')) {
                i++;
            }
        }
        while (i < (int)size && buffer[i] != '\n') {
            i++;
        }
        row++;
        tautology = false;
    }

    free(buffer);
    return 0;
}

bool is_pure_literal(int atom, sat_t *sat) {
    int polarity = 0;
    for (int i = 0; i < sat->rows; i++) {
        if (sat->content[i][atom] == 0) {
            continue;
        }
        else {
            if (polarity == 0) {
                polarity = sat->content[i][atom];
            }
            else if (polarity == -sat->content[i][atom]) {
                return false;
            }
        }
    }
    if (polarity == 0) {
        return false;
    }
    return true;
}

void eliminate_clauses_with_atom(int atom, sat_t *sat) {
    for (int i = 0; i < sat->rows; i++) {
        if (sat->content[i][atom] != 0) {
            for (int j = 0; j < sat->columns; j++) {
                sat->content[i][j] = 0;
            }
            sat->clauses--;
        }
    }
}

void pure_literal_elimination(sat_t *sat) {
    for (int i = 0; i < sat->columns; i++) {
        if (is_pure_literal(i, sat)) {
            eliminate_clauses_with_atom(i, sat);
        }
    }
}

bool is_unit_clause(sat_t *sat, int row, int *unit) {
    int found = 0;
    int lit = 0;
    for (int j = 0; j < sat->columns; j++) {
        if (sat->content[row][j] != 0) {
            if (found) {
                return false;
            }
            found = 1;
            lit = sat->content[row][j] * (j + 1);
        }
    }

    if (found) {
        *unit = lit;
        return true;
    }
    return false;
}

bool clause_empty(sat_t *sat, int row) {
    for (int i = 0; i < sat->columns; i++) {
        if (sat->content[row][i] != 0) {
            return false;
        }
    }
    return true;
}

int propagate(sat_t *sat, int unit) {
    if (!sat) {
        return -1;
    }
    int sign = (unit > 0 ? 1 : -1);
    int var = abs(unit) - 1;
    if (var < 0 || var >= sat->columns) return -1;

    if (sat->assignment[var] != 0 && sat->assignment[var] != sign) {
        return -1;
    }
    sat->assignment[var] = sign;

    for (int i = 0; i < sat->rows; i++) {
        int lit = sat->content[i][var]; 
        if (lit == sign) {
            remove_clause(sat, i);
            if (sat->clauses == 0) return 1;
        } else if (lit == -sign) {
            sat->content[i][var] = 0;
            if (clause_empty(sat, i)) {
                return -1; 
            }
        }
    }
    return 0;
}

int unit_propagation(sat_t *sat) {
    if (!sat) {
        return -1;
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < sat->rows; i++) {
            int unit;
            if (is_unit_clause(sat, i, &unit)) {
                int res = propagate(sat, unit);
                if (res != 0) return res; 
                changed = true;
                break; 
            }
        }
    }
    return 0;
}


sat_t *sat_copy(sat_t *sat){
    if (!sat) {
        return NULL;
    }

    sat_t *sat_copy = malloc(sizeof(*sat_copy));
    if (!sat_copy) {
        return NULL;
    }
    
    sat_copy->rows = sat->rows;
    sat_copy->columns = sat->columns;
    sat_copy->clauses = sat->clauses;

    sat_copy->assignment = malloc(sat_copy->columns * sizeof(int));
    if (!sat_copy->assignment) {
        free(sat_copy);
        return NULL;
    }
    for (int i = 0; i < sat_copy->columns; i++) {
        sat_copy->assignment[i] = sat->assignment[i];
    }


    sat_copy->content = malloc(sat_copy->rows * sizeof(int *));
    if (!sat_copy->content) {
        free(sat_copy);
        return NULL;
    }

    for (int i = 0; i < sat_copy->rows; i++) {
        sat_copy->content[i] = malloc(sat_copy->columns * sizeof(int));
        if (!sat_copy->content[i]) {
            for (int j = 0; j < i; j++) {
                free(sat_copy->content[j]);
            }
            free(sat_copy->content);
            free(sat_copy);
            return NULL;
        }
        memcpy(sat_copy->content[i], sat->content[i], sat_copy->columns * sizeof(int));
    }
    return sat_copy;
}

int choose_unassigned_var(sat_t *sat) {
    if (!sat) {
        return -1;
    }
    for (int i = 0; i < sat->columns; i++) {
        if (sat->assignment[i] == 0) return i;
    }
    return -1; 
}

int dpll(sat_t *sat) {
    if (!sat) return -1;

    int up = unit_propagation(sat);
    if (up == 1) {
        return 1; 
    }
    if (up == -1) {
        return -1;
    } 

    if (sat->clauses == 0) {
        return 1;
    }

    int var = choose_unassigned_var(sat);
    if (var == -1) {
        return (sat->clauses == 0) ? 1 : -1;
    }

    // true branch
    sat_t *branch_true = sat_copy(sat);
    if (!branch_true) {
        return -1; 
    }
    if (branch_true->assignment[var] == 0) {
        branch_true->assignment[var] = 1;
    }
    int r = propagate(branch_true, var + 1);
    if (r == 1) { 
        for (int i = 0; i < sat->columns; i++) {
            if (sat->assignment[i] == 0 && branch_true->assignment[i] != 0) {
                sat->assignment[i] = branch_true->assignment[i];
            }
        }
        free_sat(branch_true);
        return 1;
    } else if (r == 0) {
        int res = dpll(branch_true);
        if (res == 1) {
            for (int i = 0; i < sat->columns; i++) {
                if (sat->assignment[i] == 0 && branch_true->assignment[i] != 0) {
                    sat->assignment[i] = branch_true->assignment[i];
                }
            }
            free_sat(branch_true);
            return 1;
        }
    }
    free_sat(branch_true);

    // false branch
    sat_t *branch_false = sat_copy(sat);
    if (!branch_false) {
        return -1;
    }
    if (branch_false->assignment[var] == 0) {
        branch_false->assignment[var] = -1;
    }
    r = propagate(branch_false, -(var + 1));
    if (r == 1) {
        for (int i = 0; i < sat->columns; i++) {
            if (sat->assignment[i] == 0 && branch_false->assignment[i] != 0) {
                sat->assignment[i] = branch_false->assignment[i];
            }
        }
        free_sat(branch_false);
        return 1;
    } else if (r == 0) {
        int res = dpll(branch_false);
        if (res == 1) {
            for (int i = 0; i < sat->columns; i++) {
                if (sat->assignment[i] == 0 && branch_false->assignment[i] != 0) {
                    sat->assignment[i] = branch_false->assignment[i];
                }
            }
            free_sat(branch_false);
            return 1;
        }
    }
    free_sat(branch_false);

    return -1;
}

int main() {
    sat_t sat;
    sat_t *sat_p = &sat;
    parse_file_and_init_sat("sat-comp/sat_tests/simple/prop_rnd_6136_v_6_c_25_vic_1_4.cnf", sat_p);
    //parse_file_and_init_sat("sat-comp/sat_tests/simple/prop_rnd_8342_v_3_c_12_vic_2_4.cnf", sat_p);
    print_sat(sat_p);
    
    if (dpll(sat_p) == 1) {
        printf("sat\n");
    }
    else {
        printf("unsat\n");
    }
    return 0;
}