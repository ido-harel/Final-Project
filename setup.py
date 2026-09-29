# AI assistance was used in preparing this file.
from setuptools import Extension, setup


extension = Extension(
    "symnmfmodule",
    sources=["symnmfmodule.c", "symnmf.c"],
    libraries=["m"],
)

setup(name="symnmfmodule", version="1.0.0", ext_modules=[extension])