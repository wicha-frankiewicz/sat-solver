#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    int rows;       //n.o atoms  
    int columns;    //n.o clauses
    int **content;  //2d array of clauses
                    //format of 2d array:
                    // - each row is a clause
                    // - each entry is an atom
                    //     - 0 means no atom
                    //     - i means the atom p_i
                    //     - -i mean the atom ¬p_i
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

int parse_file_and_init_sat(char *input_file, sat_t *sat) {
    //read into buffer
    FILE *fp = fopen(input_file, "r");
    if (!fp) {
        return -1;
    }

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

    if (ferror(fp)) {  // Check for read errors
        free(buffer);
        fclose(fp);
        return -1;
    }

    buffer[size] = '\0';
    fclose(fp);

    /*    for (int i = 0; i < size; i++) {
        printf("%c", buffer[i]);
    }
    printf("\n");
    */

    //parse buffer into 2d array
    sat->rows = -1;
    sat->columns = -1;
    bool comment = false;
    bool p = false;
    bool sat_initialised = false;

    for (int i = 0; i < size; i++) {
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
        //initialise sat struct
        if (buffer[i] == 'p' && !sat_initialised) {
            p = true;
        }
        if (p && !sat_initialised) {
            char num_buffer[50];
            int nb_pos = 0;
            while (!char_is_digit(buffer[i])) {
                i++;
            }
            while (char_is_digit(buffer[i])) {
                num_buffer[nb_pos] = buffer[i];
                i++;
                nb_pos++;
            }
            num_buffer[nb_pos+1] = '\0';
            sat->columns = atoi(num_buffer);
            nb_pos = 0;
            while (!char_is_digit(buffer[i])) {
                i++;
            }
            while (char_is_digit(buffer[i])) {
                num_buffer[nb_pos] = buffer[i];
                i++;
                nb_pos++;
            }
            num_buffer[nb_pos+1] = '\0';
            sat->rows = atoi(num_buffer);

            init_sat_arr(sat);
            sat_initialised = true;
            //at this point we have allocated a 2d array initialised with 0s

            while (buffer[i] != '\n') {
                i++;
            }
        }

        //fill contents of the array with the sat problem


    }
}

int main() {
    sat_t sat;
    parse_file_and_init_sat("sat-comp/sat_tests/simple/prop_rnd_6136_v_6_c_25_vic_1_4.cnf", &sat);
    print_sat(&sat);
    return 0;
}