#include <Python.h>
#include "cwrap.h"

static PyObject *method_create_gdx_file(PyObject *self, PyObject *args) {
    char *filename = NULL;
    int rc = 0;
    if(!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }
    rc = create_gdx_file(filename);
    return PyLong_FromLong(rc);
}

static PyMethodDef methods[] = {
        {"create_gdx_file", method_create_gdx_file, METH_VARARGS, "Create a GDX file with one set and given name."},
        {NULL, NULL, 0, NULL}
};


static struct PyModuleDef pygdxmodule = {
        PyModuleDef_HEAD_INIT,
        "pygdx",
        "Python interface for simple GDX library usage example",
        -1,
        methods
};

PyMODINIT_FUNC PyInit_pygdx(void) {
    return PyModule_Create(&pygdxmodule);
}
