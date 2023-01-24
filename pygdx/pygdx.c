#include <Python.h>
#include <stddef.h>
#include <structmember.h>

#define PYGDX_EXPERIMENT
#include "../cwrap/cwrap.h"

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
    TGXFileRec_t *pgx;
} GDXObject;

static PyObject *GDXObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    GDXObject *self = (GDXObject *)type->tp_alloc(type, 0);
    if(self) {
        const int bufSize = 256;
        char buf[bufSize];
        int rc = gdxCreate(&self->pgx, buf, bufSize);
        if(buf[0] != '\0') {
            // TODO: Throw python runtime error/exception here!
        }
    }
    return (PyObject *)self;
}

static int GDXObject_init(GDXObject *self, PyObject *args, PyObject *kwds) {
    // FIXME: This function does nothing really!
    return 0;
}

static void GDXObject_dealloc(GDXObject *self) {
    if(self->pgx) gdxDestroy(&self->pgx);
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

// TODO: Add error handling -> throw Python exception on not-ok error/return codes
static PyObject *open_write(GDXObject *self, PyObject *args, PyObject *kwds) {
    int ec, rc;
    const char *filename;
    if(!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
    rc = gdxOpenWrite(self->pgx, filename, "pygdxnative", &ec);
    return PyLong_FromLong(rc);
}

static PyObject *GDXError;

static PyObject *open_read(GDXObject *self, PyObject *args, PyObject *kwds) {
    int ec, rc;
    const char *filename;
    if(!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
    rc = gdxOpenRead(self->pgx, filename, &ec);
    if(!rc) {
        GDXError = PyErr_NewException("gdx.error", NULL, NULL);
        PyErr_SetString(GDXError, "Unable to open file!");
        return NULL;
    }
    return PyLong_FromLong(rc);
}

static PyObject *close_file(GDXObject *self, PyObject *args, PyObject *kwds) {
    gdxClose(self->pgx);
    return Py_None;
}

static PyObject *create_set1d(GDXObject *self, PyObject *args, PyObject *kwds) {
    const char *name;
    PyObject *elems = NULL;
    if(!PyArg_ParseTuple(args, "sO", &name, &elems))
        return NULL;
    const long numElems = PyList_Size(elems);
    printf("Got list with %ld elements...\n", numElems);
    char **elemBuffers = (char **)malloc((numElems+1) * sizeof(char *));
    elemBuffers[numElems] = NULL;
    for(int i=0; i<numElems; i++) {
        PyObject *elem = PyList_GetItem(elems, i);
        const char *s = PyUnicode_AsUTF8(elem);
        printf("Element no. %d is %s\n", i+1, s);
        const unsigned long len = strlen(s);
        char *elemBuffer = (char *)malloc(sizeof(char)*(len+1));
        memcpy(elemBuffer, s, sizeof(char)*(len+1));
        elemBuffers[i] = elemBuffer;
    }
    int num_elems_added = gdx_set1d(self->pgx, name, (const char **)elemBuffers);
    for(int i=0; i<numElems; i++)
        free(elemBuffers[i]);
    free(elemBuffers);
    return PyLong_FromLong(num_elems_added);
}

static PyMethodDef GDXObject_methods[] = {
        //{"dumpfile", (PyCFunction)dumpfile, METH_NOARGS, "Dump some gdx file"},
        {"open_write", (PyCFunction)open_write, METH_VARARGS, "Open new gdx file for writing"},
        {"open_read", (PyCFunction)open_read, METH_VARARGS, "Open new gdx file for reading"},
        {"set1D", (PyCFunction)create_set1d, METH_VARARGS, "Create a one dimensional set by supplying element UELs as list"},
        {"close", (PyCFunction)close_file, METH_VARARGS, "Close gdx file"},
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

    GDXError = PyErr_NewException("gdx.error", NULL, NULL);
    Py_XINCREF(GDXError);
    if(PyModule_AddObject(po, "error", GDXError) < 0) {
        Py_XDECREF(GDXError);
        Py_CLEAR(GDXError);
        Py_DECREF(po);
        return NULL;
    }

    Py_INCREF(&GDXDataStorage);
    if(PyModule_AddObject(po, "GDXDataStorage", (PyObject *)&GDXDataStorage) < 0) {
        Py_DECREF(&GDXDataStorage);
        Py_DECREF(po);
        return NULL;
    }

    return po;
}
