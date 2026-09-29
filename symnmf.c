/* AI assistance was used in preparing this file. */
#include "symnmf.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AT(matrix, row, col) ((matrix)->data[(row) * (matrix)->cols + (col)])

Matrix *matrix_create(size_t rows, size_t cols)
{
    Matrix *matrix;

    if (rows == 0 || cols == 0 || rows > ((size_t)-1) / cols ||
        rows * cols > ((size_t)-1) / sizeof(double)) {
        return NULL;
    }
    matrix = (Matrix *)malloc(sizeof(Matrix));
    if (matrix == NULL) {
        return NULL;
    }
    matrix->data = (double *)calloc(rows * cols, sizeof(double));
    if (matrix->data == NULL) {
        free(matrix);
        return NULL;
    }
    matrix->rows = rows;
    matrix->cols = cols;
    return matrix;
}

void matrix_free(Matrix *matrix)
{
    if (matrix != NULL) {
        free(matrix->data);
        free(matrix);
    }
}

static char *skip_spaces(char *cursor)
{
    while (isspace((unsigned char)*cursor)) {
        cursor++;
    }
    return cursor;
}

static int parse_number(char **cursor, double *value)
{
    char *end;

    errno = 0;
    *value = strtod(*cursor, &end);
    if (end == *cursor || errno == ERANGE || *value != *value ||
        *value > DBL_MAX || *value < -DBL_MAX) {
        return 0;
    }
    *cursor = skip_spaces(end);
    return 1;
}

static int parse_row(char *line, double *values, size_t expected)
{
    char *cursor;
    size_t count;
    double value;

    cursor = skip_spaces(line);
    count = 0;
    while (*cursor != '\0') {
        if (!parse_number(&cursor, &value) ||
            (values != NULL && count >= expected)) {
            return -1;
        }
        if (values != NULL) {
            values[count] = value;
        }
        count++;
        if (*cursor == ',') {
            cursor = skip_spaces(cursor + 1);
            if (*cursor == '\0') {
                return -1;
            }
        } else if (*cursor != '\0') {
            return -1;
        }
    }
    return (int)count;
}

static int inspect_points(FILE *file, size_t *rows, size_t *cols)
{
    char line[65536];
    int count;

    *rows = 0;
    *cols = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strchr(line, '\n') == NULL && !feof(file)) {
            return 0;
        }
        count = parse_row(line, NULL, 0);
        if (count < 0 || (count > 0 && *cols != 0 && (size_t)count != *cols)) {
            return 0;
        }
        if (count > 0) {
            *cols = (size_t)count;
            (*rows)++;
        }
    }
    return !ferror(file) && *rows > 0 && *cols > 0 && fseek(file, 0L, SEEK_SET) == 0;
}

static int fill_points(FILE *file, Matrix *points)
{
    char line[65536];
    size_t row;
    int count;

    row = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        count = parse_row(line, NULL, 0);
        if (count < 0 || (count > 0 &&
            ((size_t)count != points->cols || row >= points->rows))) {
            return 0;
        }
        if (count > 0) {
            parse_row(line, points->data + row * points->cols, points->cols);
            row++;
        }
    }
    return !ferror(file) && row == points->rows;
}

Matrix *read_points(const char *path)
{
    FILE *file;
    size_t rows;
    size_t cols;
    Matrix *points;

    file = fopen(path, "r");
    if (file == NULL) {
        return NULL;
    }
    if (!inspect_points(file, &rows, &cols)) {
        fclose(file);
        return NULL;
    }
    points = matrix_create(rows, cols);
    if (points == NULL || !fill_points(file, points)) {
        matrix_free(points);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return points;
}

Matrix *similarity_matrix(const Matrix *points)
{
    Matrix *similarity;
    size_t i;
    size_t j;
    size_t dimension;
    double difference;
    double distance;

    similarity = matrix_create(points->rows, points->rows);
    if (similarity == NULL) {
        return NULL;
    }
    for (i = 0; i < points->rows; i++) {
        for (j = 0; j < points->rows; j++) {
            if (i == j) {
                continue;
            }
            distance = 0.0;
            for (dimension = 0; dimension < points->cols; dimension++) {
                difference = AT(points, i, dimension) - AT(points, j, dimension);
                distance += difference * difference;
            }
            AT(similarity, i, j) = exp(-distance / 2.0);
        }
    }
    return similarity;
}

static Matrix *degree_from_similarity(const Matrix *similarity)
{
    Matrix *degree;
    size_t i;
    size_t j;

    degree = matrix_create(similarity->rows, similarity->rows);
    if (degree == NULL) {
        return NULL;
    }
    for (i = 0; i < similarity->rows; i++) {
        for (j = 0; j < similarity->cols; j++) {
            AT(degree, i, i) += AT(similarity, i, j);
        }
    }
    return degree;
}

Matrix *degree_matrix(const Matrix *points)
{
    Matrix *similarity;
    Matrix *degree;

    similarity = similarity_matrix(points);
    if (similarity == NULL) {
        return NULL;
    }
    degree = degree_from_similarity(similarity);
    matrix_free(similarity);
    return degree;
}

Matrix *normalized_matrix(const Matrix *points)
{
    Matrix *similarity;
    Matrix *degree;
    Matrix *normalized;
    size_t i;
    size_t j;

    similarity = similarity_matrix(points);
    if (similarity == NULL) {
        return NULL;
    }
    degree = degree_from_similarity(similarity);
    normalized = matrix_create(points->rows, points->rows);
    if (degree == NULL || normalized == NULL) {
        matrix_free(similarity);
        matrix_free(degree);
        matrix_free(normalized);
        return NULL;
    }
    for (i = 0; i < points->rows; i++) {
        for (j = 0; j < points->rows; j++) {
            if (AT(degree, i, i) > 0.0 && AT(degree, j, j) > 0.0) {
                AT(normalized, i, j) = (AT(similarity, i, j) /
                    sqrt(AT(degree, i, i))) / sqrt(AT(degree, j, j));
            }
        }
    }
    matrix_free(similarity);
    matrix_free(degree);
    return normalized;
}

Matrix *matrix_product(const Matrix *left, const Matrix *right)
{
    Matrix *product;
    size_t i;
    size_t j;
    size_t inner;

    if (left->cols != right->rows) {
        return NULL;
    }
    product = matrix_create(left->rows, right->cols);
    if (product == NULL) {
        return NULL;
    }
    for (i = 0; i < left->rows; i++) {
        for (inner = 0; inner < left->cols; inner++) {
            for (j = 0; j < right->cols; j++) {
                AT(product, i, j) += AT(left, i, inner) * AT(right, inner, j);
            }
        }
    }
    return product;
}

static double h_denominator(const Matrix *h, size_t i, size_t j)
{
    size_t row;
    size_t inner;
    double denominator;

    denominator = 0.0;
    for (row = 0; row < h->rows; row++) {
        for (inner = 0; inner < h->cols; inner++) {
            denominator += AT(h, i, inner) * AT(h, row, inner) * AT(h, row, j);
        }
    }
    return denominator;
}

static int update_h(const Matrix *h, const Matrix *w, Matrix *next,
                    double *squared_change)
{
    Matrix *wh;
    size_t i;
    size_t j;
    double denominator;
    double difference;

    wh = matrix_product(w, h);
    if (wh == NULL) {
        return 0;
    }
    *squared_change = 0.0;
    for (i = 0; i < h->rows; i++) {
        for (j = 0; j < h->cols; j++) {
            denominator = h_denominator(h, i, j);
            next->data[i * h->cols + j] = denominator > DBL_MIN ?
                AT(h, i, j) * (0.5 + 0.5 * AT(wh, i, j) / denominator) : 0.0;
            difference = next->data[i * h->cols + j] - AT(h, i, j);
            *squared_change += difference * difference;
        }
    }
    matrix_free(wh);
    return 1;
}

int symnmf_optimize(Matrix *h, const Matrix *w, int max_iter, double epsilon)
{
    Matrix *next;
    double squared_change;

    if (h->rows != w->rows || h->rows != w->cols || max_iter <= 0 || epsilon < 0.0) {
        return -1;
    }
    next = matrix_create(h->rows, h->cols);
    if (next == NULL) {
        return -2;
    }
    while (max_iter-- > 0) {
        if (!update_h(h, w, next, &squared_change)) {
            matrix_free(next);
            return -2;
        }
        memcpy(h->data, next->data, h->rows * h->cols * sizeof(double));
        if (squared_change < epsilon) {
            break;
        }
    }
    matrix_free(next);
    return 0;
}

void matrix_print_csv(const Matrix *matrix)
{
    size_t i;
    size_t j;

    for (i = 0; i < matrix->rows; i++) {
        for (j = 0; j < matrix->cols; j++) {
            if (j > 0) {
                putchar(',');
            }
            printf("%.4f", AT(matrix, i, j));
        }
        putchar('\n');
    }
}

#ifdef SYMNMF_STANDALONE
int main(int argc, char **argv)
{
    Matrix *points;
    Matrix *result;

    if (argc != 3) {
        printf("An Error Has Occurred\n");
        return 1;
    }
    points = read_points(argv[2]);
    if (points == NULL) {
        printf("An Error Has Occurred\n");
        return 1;
    }
    result = NULL;
    if (strcmp(argv[1], "sym") == 0) {
        result = similarity_matrix(points);
    } else if (strcmp(argv[1], "ddg") == 0) {
        result = degree_matrix(points);
    } else if (strcmp(argv[1], "norm") == 0) {
        result = normalized_matrix(points);
    } else {
        matrix_free(points);
        printf("An Error Has Occurred\n");
        return 1;
    }
    if (result == NULL) {
        matrix_free(points);
        printf("An Error Has Occurred\n");
        return 1;
    }
    matrix_print_csv(result);
    matrix_free(result);
    matrix_free(points);
    return 0;
}
#endif