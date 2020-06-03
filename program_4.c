#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

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

static inline void matrix_put(struct matrix_t* m, size_t c, size_t r, float v) {
    assert(m);
    assert(r < m->rows&& c < m->cols);
    m->data[r * m->cols + c] = v;
}

static inline float matrix_get(struct matrix_t* m, size_t c, size_t r) {
    assert(m);
    assert(r < m->rows&& c < m->cols);
    return m->data[r * m->cols + c];
}

static inline size_t matrix_elems(struct matrix_t* m) {
    assert(m);
    return m->cols * m->rows;
}

struct matrix_t* matrix_new_empty(size_t r, size_t c) {
    size_t msize = sizeof(struct matrix_t) + c * r * sizeof(float);
    struct matrix_t* m = (struct matrix_t*)malloc(msize);
    assert(m);
    m->cols = c;
    m->rows = r;
    const size_t e = matrix_elems(m);
    size_t i = 0;
    for (; i < e; i++) {
        m->data[i] = NAN;
    }
    return m;
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
            ungetc(c, f); // let's hope for the best
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
        if (c == '.' || c == '-' || isdigit(c)) {
            ungetc(c, f); // let's hope for the best
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
        if (!isnan(t->data[i])) {
            sum += t->data[i];
            k++;
        }
    }

    return sum / k;
}

int main(int argc, char** argv) {

    const char* fn = argc > 1 ? argv[1] : "data.dat";

    struct matrix_t* m = parse_array(fn);

    if (!m) {
        fprintf(stderr, "could not read input file\n");
        exit(-1);
    }

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

    struct matrix_t* mout = m;
    struct matrix_t* m1 = 0;

    if (has_avg_p) {
        m1 = matrix_new_empty(m->rows, has_avg_p ? m->cols - 1 : m->cols);
        assert(m1);
        mout = m1;
        for (j = 0; j < m->rows; j++) {
            for (i = 0; i < m->cols; i++) {
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

    for (j = 0; j < mout->rows; j++) {
        for (i = 0; i < mout->cols; i++) {
            printf("%.2f ", matrix_get(mout, i, j));
            fprintf(out, "%.2f ", matrix_get(mout, i, j));
        }
        printf("\n");
        fprintf(out, "\n");
    }

    fclose(out);
    free(m);
    if (m1) free(m1);

    return 0;
}