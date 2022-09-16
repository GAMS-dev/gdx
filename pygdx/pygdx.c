#include <Python.h>
#include <stddef.h>
#include <structmember.h>
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
    void *pgx;
} GDXObject;

static PyObject *GDXObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    GDXObject *self = (GDXObject *)type->tp_alloc(type, 0);
    if(self) {
        self->pgx = NULL;
    }
    return (PyObject *)self;
}

static int GDXObject_init(GDXObject *self, PyObject *args, PyObject *kwds) {
    return 0;
}

static void GDXObject_dealloc(GDXObject *self) {
    //if(self->pgx) free(self->pgx);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMemberDef GDXObject_members[] = {
        {"pgx", T_OBJECT, offsetof(GDXObject, pgx), 0, "pointer to internal GDX object"},
        {NULL} // sentinel
};

static PyObject *dumpfile(GDXObject *self, PyObject *Py_UNUSED(ignored)) {
    create_gdx_file("calleddump.gdx");
    return PyUnicode_FromString("Wrote gdx file!");
}

static PyMethodDef GDXObject_methods[] = {
        {"dumpfile", (PyCFunction)dumpfile, METH_NOARGS, "Dump some gdx file"},
        {NULL} // sentinel
};

static PyTypeObject GDXDataStorage = {
        PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "pygdx.GDXDataStorage",
        .tp_doc = PyDoc_STR("A GDX object"),
        .tp_basicsize = sizeof(GDXObject),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = GDXObject_new,
        .tp_init = (initproc)GDXObject_init,
        .tp_dealloc = (destructor)GDXObject_dealloc,
        .tp_members = GDXObject_members,
        .tp_methods = GDXObject_methods
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
