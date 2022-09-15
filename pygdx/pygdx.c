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

typedef struct {
    PyObject_HEAD;
    //void *pgx;
} GDXObject;

static PyTypeObject GDXDataStorage = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "pygdx.GDXDataStorage",
        .tp_doc = PyDoc_STR("A GDX object"),
        .tp_basicsize = sizeof(GDXObject),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyType_GenericNew
};

static struct PyModuleDef pygdxmodule = {
        PyModuleDef_HEAD_INIT,
        "pygdx",
        "Python interface for simple GDX library usage example",
        -1,
        methods
};

PyMODINIT_FUNC PyInit_pygdx(void) {
    PyObject *po;
    if(PyType_Ready(&GDXDataStorage) < 0) return NULL;

    po = PyModule_Create(&pygdxmodule);
    if(!po) return NULL;

    Py_INCREF(&GDXDataStorage);
    if(PyModule_AddObject(po, "GDXDataStorage", (PyObject *)&GDXDataStorage) < 0) {
        Py_DECREF(&GDXDataStorage);
        Py_DECREF(po);
        return NULL;
    }

    return po;
}
