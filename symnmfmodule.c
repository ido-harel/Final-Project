/* AI assistance was used in preparing this file. */
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "symnmf.h"

#include <float.h>
#include <math.h>

static PyObject *sequence_rows(PyObject *object, Py_ssize_t *row_count,
                              Py_ssize_t *col_count)
{
    PyObject *rows;
    PyObject *row;

    rows = PySequence_Fast(object, "expected a two-dimensional sequence");
    if (rows == NULL) {
        return NULL;
    }
    *row_count = PySequence_Fast_GET_SIZE(rows);
    if (*row_count == 0) {
        Py_DECREF(rows);
        PyErr_SetString(PyExc_ValueError, "matrix must not be empty");
        return NULL;
    }
    row = PySequence_Fast(PySequence_Fast_GET_ITEM(rows, 0), "expected matrix rows");
    if (row == NULL) {
        Py_DECREF(rows);
        return NULL;
    }
    *col_count = PySequence_Fast_GET_SIZE(row);
    Py_DECREF(row);
    if (*col_count == 0) {
        Py_DECREF(rows);
        PyErr_SetString(PyExc_ValueError, "matrix rows must not be empty");
        return NULL;
    }
    return rows;
}

static int copy_sequence_row(PyObject *object, Matrix *matrix, Py_ssize_t index)
{
    PyObject *row;
    Py_ssize_t col;
    double value;

    row = PySequence_Fast(object, "expected matrix rows");
    if (row == NULL) {
        return 0;
    }
    if (PySequence_Fast_GET_SIZE(row) != (Py_ssize_t)matrix->cols) {
        Py_DECREF(row);
        PyErr_SetString(PyExc_ValueError, "matrix rows must have equal length");
        return 0;
    }
    for (col = 0; col < (Py_ssize_t)matrix->cols; col++) {
        value = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(row, col));
        if (PyErr_Occurred() || value != value || value > DBL_MAX || value < -DBL_MAX) {
            Py_DECREF(row);
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError, "matrix values must be finite numbers");
            }
            return 0;
        }
        matrix->data[(size_t)index * matrix->cols + (size_t)col] = value;
    }
    Py_DECREF(row);
    return 1;
}

static int matrix_from_sequence(PyObject *object, Matrix **result)
{
    PyObject *rows;
    Py_ssize_t row_count;
    Py_ssize_t col_count;
    Py_ssize_t i;
    Matrix *matrix;

    rows = sequence_rows(object, &row_count, &col_count);
    if (rows == NULL) {
        return 0;
    }
    matrix = matrix_create((size_t)row_count, (size_t)col_count);
    if (matrix == NULL) {
        Py_DECREF(rows);
        PyErr_NoMemory();
        return 0;
    }
    for (i = 0; i < row_count; i++) {
        if (!copy_sequence_row(PySequence_Fast_GET_ITEM(rows, i), matrix, i)) {
            matrix_free(matrix);
            Py_DECREF(rows);
            return 0;
        }
    }
    Py_DECREF(rows);
    *result = matrix;
    return 1;
}

static PyObject *matrix_to_list(const Matrix *matrix)
{
    PyObject *outer;
    PyObject *row;
    size_t i;
    size_t j;

    outer = PyList_New((Py_ssize_t)matrix->rows);
    if (outer == NULL) {
        return NULL;
    }
    for (i = 0; i < matrix->rows; i++) {
        row = PyList_New((Py_ssize_t)matrix->cols);
        if (row == NULL) {
            Py_DECREF(outer);
            return NULL;
        }
        for (j = 0; j < matrix->cols; j++) {
            PyObject *value;

            value = PyFloat_FromDouble(matrix->data[i * matrix->cols + j]);
            if (value == NULL) {
                Py_DECREF(row);
                Py_DECREF(outer);
                return NULL;
            }
            PyList_SET_ITEM(row, (Py_ssize_t)j, value);
        }
        PyList_SET_ITEM(outer, (Py_ssize_t)i, row);
    }
    return outer;
}

typedef Matrix *(*PointOperation)(const Matrix *);

static PyObject *call_point_operation(PyObject *args, PointOperation operation)
{
    PyObject *object;
    Matrix *points;
    Matrix *result;
    PyObject *output;

    if (!PyArg_ParseTuple(args, "O", &object)) {
        return NULL;
    }
    if (!matrix_from_sequence(object, &points)) {
        return NULL;
    }
    result = operation(points);
    matrix_free(points);
    if (result == NULL) {
        return PyErr_NoMemory();
    }
    output = matrix_to_list(result);
    matrix_free(result);
    return output;
}

static PyObject *py_sym(PyObject *self, PyObject *args)
{
    (void)self;
    return call_point_operation(args, similarity_matrix);
}

static PyObject *py_ddg(PyObject *self, PyObject *args)
{
    (void)self;
    return call_point_operation(args, degree_matrix);
}

static PyObject *py_norm(PyObject *self, PyObject *args)
{
    (void)self;
    return call_point_operation(args, normalized_matrix);
}

static PyObject *py_symnmf(PyObject *self, PyObject *args)
{
    PyObject *h_object;
    PyObject *w_object;
    Matrix *h;
    Matrix *w;
    int max_iter;
    int status;
    double epsilon;
    PyObject *output;

    (void)self;
    max_iter = 300;
    epsilon = 1e-4;
    if (!PyArg_ParseTuple(args, "OO|id", &h_object, &w_object, &max_iter, &epsilon)) {
        return NULL;
    }
    if (!matrix_from_sequence(h_object, &h)) {
        return NULL;
    }
    if (!matrix_from_sequence(w_object, &w)) {
        matrix_free(h);
        return NULL;
    }
    status = symnmf_optimize(h, w, max_iter, epsilon);
    if (status != 0) {
        matrix_free(h);
        matrix_free(w);
        if (status == -2) {
            return PyErr_NoMemory();
        }
        PyErr_SetString(PyExc_ValueError, "incompatible matrices or invalid parameters");
        return NULL;
    }
    output = matrix_to_list(h);
    matrix_free(h);
    matrix_free(w);
    return output;
}

static PyMethodDef methods[] = {
    {"symnmf", py_symnmf, METH_VARARGS, "Optimize a nonnegative factor matrix."},
    {"sym", py_sym, METH_VARARGS, "Compute the similarity matrix."},
    {"ddg", py_ddg, METH_VARARGS, "Compute the diagonal degree matrix."},
    {"norm", py_norm, METH_VARARGS, "Compute the normalized similarity matrix."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "symnmfmodule",
    "C implementation of the symNMF matrix operations.",
    -1,
    methods
};

PyMODINIT_FUNC PyInit_symnmfmodule(void)
{
    return PyModule_Create(&module);
}