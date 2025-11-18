#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    int rows;       //n.o atoms  
    int columns;    //n.o clauses
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

void print_sat(sat_t *sat) {
    printf("rows: %d\n", sat->rows);
    printf("columns: %d\n", sat->columns);
    printf("problem:\n");
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
    for (int i = 0; i < sat->columns; i++) {
        sat->content[clear_row][i] = 0;
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

            if (sat->rows <= 0 || sat->columns <= 0) {
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
    for (int i = 1; i < sat->rows; i++) {
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
    return true;
}

void eliminate_clauses_with_atom(int atom, sat_t *sat) {
    for (int i = 1; i < sat->rows; i++) {
        if (sat->content[i][atom] != 0) {
            for (int j = 0; j < sat->columns; j++) {
                sat->content[i][j] = 0;
            }
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

void propagate(sat_t *sat, int unit) {
    int sign = (unit > 0 ? 1 : -1);
    for (int i = 0; i < sat->rows; i++) {
        if (sat->content[i][abs(unit) - 1] == sign) {
            for (int j = 0; j < sat->columns; j++) {
                sat->content[i][j] = 0;
            }
        } 
        else if (sat->content[i][abs(unit) - 1] == -sign) {
            sat->content[i][abs(unit) - 1] = 0;
        }
    }
}

void unit_propagation(sat_t *sat) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < sat->rows; i++) {
            int unit;
            if (is_unit_clause(sat, i, &unit)) {
                propagate(sat, unit);
                changed = true;
            }
        }
    }
}


sat_t *sat_copy(sat_t *sat){
    if (!sat) {
        return NULL;
    }

    sat_t *sat_copy = malloc(sizeof(sat));
    if (!sat_copy) {
        return NULL;
    }
    
    sat_copy->rows = sat->rows;
    sat_copy->columns = sat->columns;

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

int main() {
    sat_t sat;
    sat_t *sat_p = &sat;
    parse_file_and_init_sat("sat-comp/sat_tests/simple/prop_rnd_6136_v_6_c_25_vic_1_4.cnf", sat_p);
    print_sat(sat_p);
    sat_t *sat2_p = sat_copy(sat_p);

    pure_literal_elimination(sat2_p);
    unit_propagation(sat2_p);
    printf("\n");

    print_sat(sat2_p);
    return 0;
}