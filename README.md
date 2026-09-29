# Final-Project
SymNMF Clustering in C and Python
An implementation of Symmetric Non-negative Matrix Factorization (SymNMF) for clustering, with a C computational core and Python bindings through the Python C API. Includes a comparison with K-means using silhouette scores and Adjusted Rand Index (ARI).
Developed as a two-person Software Project course assignment at Tel Aviv University.
Status: In development; correctness fixes, testing, and documentation are ongoing. AI tools assisted with the initial implementation.
Project Structure
File	Role
symnmf.c, symnmf.h	Matrix operations, SymNMF optimization, and C interface
symnmfmodule.c	Python bindings for the C core
symnmf.py	Python command-line interface and initialization
analysis.py	K-means and clustering evaluation
setup.py, Makefile	Build configuration


Build
Requires Python 3 with development headers, a C compiler, and Make.
python3 -m pip install numpy scikit-learn setuptools
python3 setup.py build_ext --inplace
make
Usage
Provide a headerless .txt file with comma-separated coordinates, one data point per line.
# Run SymNMF with 3 clusters and print the factor matrix H
python3 symnmf.py 3 symnmf points.txt

# Compute the normalized similarity matrix through the C interface
./symnmf norm points.txt

# Compare SymNMF and K-means with 3 clusters
python3 analysis.py 3 points.txt
Python syntax: python3 symnmf.py <k> <goal> <input_file>.
Available goals: sym (similarity), ddg (degree), norm (normalized similarity), and symnmf (factorization). The C interface supports the first three goals.
Matrix values and evaluation scores are printed to four decimal places.
