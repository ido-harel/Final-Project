/* AI assistance was used in preparing this file. */
#ifndef SYMNMF_H
#define SYMNMF_H

#include <stddef.h>

typedef struct {
    size_t rows;
    size_t cols;
    double *data;
} Matrix;

Matrix *matrix_create(size_t rows, size_t cols);
void matrix_free(Matrix *matrix);
Matrix *read_points(const char *path);
Matrix *similarity_matrix(const Matrix *points);
Matrix *degree_matrix(const Matrix *points);
Matrix *normalized_matrix(const Matrix *points);
Matrix *matrix_product(const Matrix *left, const Matrix *right);
int symnmf_optimize(Matrix *h, const Matrix *w, int max_iter, double epsilon);
void matrix_print_csv(const Matrix *matrix);

#endif