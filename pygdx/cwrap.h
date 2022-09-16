#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    int create_gdx_file(const char *filename);

    void *gdx_create(char *errBuf, int bufSize);
    void gdx_destroy(void **pgx);

    int gdx_open_write(void *pgx, const char *filename, int *ec);
    int gdx_open_read(void *pgx, const char *filename, int *ec);
    void gdx_close(void *pgx);

    int gdx_set1d(void *pgx, const char *name, const char **elems);
#ifdef __cplusplus
}
#endif
