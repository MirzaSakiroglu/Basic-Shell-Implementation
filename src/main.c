#include "../include/project.h"
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <sys/mman.h>

ShmBuf *g_shm;

int main(int argc, char *argv[]) {
    g_shm = buf_init(); // Paylaşımlı belleği başlat
    int status = view_main(argc, argv);
    shm_unlink(SHARED_FILE_PATH); // Program bittiğinde belleği temizle
    return status;
}
