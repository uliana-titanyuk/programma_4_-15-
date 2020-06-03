#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

struct cell_t {
    struct cell_t* cdr;
    size_t col;
    size_t row;
    float  value;
};

struct matrix_t {
    size_t cols;
    size_t rows;
    float  data[0];
};

inline void matrix_put(struct matrix_t* m, size_t c, size_t r, float v) {
    assert(m);
    m->data[r * m->cols + c] = v;
}

inline float matrix_get(struct matrix_t* m, size_t c, size_t r) {
    assert(m);
    return m->data[r * m->cols + c];
}

struct matrix_t* matrix_new_empty(size_t r, size_t c) {
    size_t msize = sizeof(struct matrix_t) + c * r * sizeof(float);
    struct matrix_t* m = (struct matrix_t*)malloc(msize);
    assert(m);
    memset(m, 0, msize);
    m->cols = c;
    m->rows = r;
    return m;
}

inline size_t matrix_elems(struct matrix_t* m) {
    assert(m);
    return m->cols * m->rows;
}

bool read_token(FILE* f, float* r) {
    char token[256] = { 0 };
    char* pe = token + sizeof(token);
    char* p = token;
    int c = 0;
    while ((c = fgetc(f)) != EOF) {
        if (!isspace(c) && p < pe) {
            *p++ = c;
        }
        else {
            ungetc(c, f); 
            break;
        }
    }
    char* endp = token;
    *r = strtof(token, &endp);
    return endp > token;
}


struct matrix_t* from_list(struct cell_t* root) {
}

struct matrix_t* parse_array(const char* fname) {
    FILE* f = fopen(fname, "r");
    if (!f) {
        printf("can not open file");
        return 0;
    }

    struct cell_t* root = 0;

    float num = 0;

    size_t max_col = 0;
    size_t max_row = 0;
    size_t col = 0;
    size_t row = 0;
    int    c = 0;

    while ((c = fgetc(f)) != EOF) {
        if (c == '.' || isdigit(c)) {
            ungetc(c, f); 
            if (read_token(f, &num)) {
                struct cell_t* cell = (struct cell_t*)malloc(sizeof(struct cell_t));
                assert(cell);

                cell->col = col;
                cell->row = row;
                cell->value = num;
                cell->cdr = root;
                root = cell;

                col++;
            }
        }
        else if (c == '\n') {
            row++;
            max_col = col > max_col ? col : max_col;
            col = 0;
        }
    }

    max_row = row;

    size_t msize = sizeof(struct matrix_t) + max_row * max_col * sizeof(float);
    struct matrix_t* m = matrix_new_empty(max_row, max_col);
    assert(m);

    struct cell_t* cell = root;
    for (; cell; ) {
        matrix_put(m, cell->col, cell->row, cell->value);
        void* wipe = cell;
        cell = cell->cdr;
        free(wipe);
    }

    fclose(f);
    return m;
}

float matrix_avg_nn(struct matrix_t* t) {
    size_t i = 0;
    const size_t len = matrix_elems(t);

    if (!len) {
        return 0;
    }

    float sum = 0;
    size_t k = 0;
    for (; i < len; i++) {
        sum += t->data[i];
        k += t->data[i] != 0 ? 1 : 0;
    }

    return sum / k;
}

int main(int argc, char** argv) {

    const char* fn = argc > 1 ? argv[1] : "data.dat";

    struct matrix_t* m = parse_array(fn);

    assert(m);

    printf("(%d,%d)\n", m->rows, m->cols);

    if (!matrix_elems(m)) {
        fprintf(stderr, "matrix should have dim > (0,0)\n");
        exit(-1);
    }

    float avg = matrix_avg_nn(m);
    printf("matrix avg: (%d) %f\n", matrix_elems(m), avg);

    size_t has_avg = 0;
    size_t* has_avg_p = 0;

    size_t i = 0, j = 0;
    for (; j < m->rows; j++) {
        for (i = 0; i < m->cols; i++) {
            float f = matrix_get(m, i, j);
            if (avg == f) {
                has_avg = i;
                has_avg_p = &has_avg;
                break;
            }
        }
    }

    if (has_avg_p) {
        printf("found avg at col %d\n", *has_avg_p);
    }

    struct matrix_t* m1 = matrix_new_empty(m->rows, m->cols - 1);

    assert(m1);

    for (j = 0; j < m->rows; j++) {
        for (i = 0; i < m->cols; i++) {
            if (!has_avg_p) {
                matrix_put(m1, i, j, matrix_get(m, i, j));
            }
            else {
                if (i < *has_avg_p) {
                    matrix_put(m1, i, j, matrix_get(m, i, j));
                }
                else if (i == *has_avg_p) {
                    continue;
                }
                else {
                    matrix_put(m1, i - 1, j, matrix_get(m, i, j));
                }
            }
        }
    }


    FILE* out = fopen("data.res", "w");

    if (!out) {
        fprintf(stderr, "can't open output file\n");
        exit(-1);
    }

    for (j = 0; j < m1->rows; j++) {
        for (i = 0; i < m1->cols; i++) {
            printf("%.2f ", matrix_get(m1, i, j));
            fprintf(out, "%.2f ", matrix_get(m1, i, j));
        }
        printf("\n");
        fprintf(out, "\n");
    }

    fclose(out);
    free(m);
    free(m1);

    return 0;
}