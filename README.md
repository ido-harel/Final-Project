# SymNMF Clustering — Python & C

Implementation of **Symmetric Non-negative Matrix Factorization (SymNMF)** for clustering, with a **C computational core** integrated into Python through a custom extension module.

The project combines Python's data handling and evaluation tools with numerical computation in C, and compares SymNMF with **K-means** using silhouette scores and Adjusted Rand Index (ARI).

## Features

* Implements similarity, diagonal degree, and normalized similarity matrices in C.
* Implements iterative SymNMF optimization in C with initialization in Python.
* Exposes the computational core through the Python C API.
* Provides Python and standalone C command-line interfaces.
* Compares SymNMF and K-means clustering results.

## Project Structure

```text
symnmf-python-c/
├── symnmf.py           # Python interface and factor initialization
├── symnmf.c            # Numerical algorithms and standalone C interface
├── symnmf.h            # Matrix structure and C function declarations
├── symnmfmodule.c      # Python bindings for the C implementation
├── analysis.py         # K-means and clustering evaluation
├── setup.py            # Build configuration for the C extension
├── Makefile            # Build configuration for the C program
└── README.md
```

## Architecture

The project is divided into three main components:

**Python layer — `symnmf.py` and `analysis.py`**

Handles input data, initializes the factor matrix, and evaluates clustering results.

**C core — `symnmf.c` and `symnmf.h`**

Implements matrix operations, memory management, and SymNMF optimization. The same core also serves the standalone C program.

**Integration layer — `symnmfmodule.c`**

Converts data between Python and C and exposes the numerical functions as an importable Python module.

## Technologies

* Python and C
* Python C API
* NumPy
* scikit-learn evaluation metrics
* Setuptools and Make

## What I Practiced

* Implementing numerical and clustering algorithms.
* Integrating C code with Python through a native extension.
* Managing dynamic memory and matrix data structures in C.
* Evaluating clustering quality and agreement between algorithms.
* Structuring and debugging a project across multiple programming languages.

## Background

Developed with a partner as part of the Software Project course at Tel Aviv University.

The project is under active review and testing. AI tools assisted with the initial implementation.
