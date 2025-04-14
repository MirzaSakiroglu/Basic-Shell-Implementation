#ifndef PROJECT_H
#define PROJECT_H

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stddef.h>
#include <unistd.h>

/* Sabit tanımlamalar */
#define BUF_SIZE 4096
#define SHARED_FILE_PATH "mymsgbuf"

/* Hata durumunda çıkış makrosu */
#define errExit(msg)        \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

/* Paylaşılan bellek yapısı */
typedef struct shmbuf {
    sem_t *sem;     /* Erişim kontrolü için semafor */
    size_t cnt;    /* msgbuf içinde kullanılan bayt sayısı */
    int fd;        /* Dosya tanımlayıcısı */
    char msgbuf[]; /* Aktarılacak mesaj verisi */
} ShmBuf;

/* Shell süreç bilgilerini tutan yapı */
typedef struct {
    pid_t pid;          /* Süreç ID'si */
    char command[256];  /* Çalıştırılan komut */
    int status;         /* Süreç durumu (çalışıyor/sonlandı) */
} ProcessInfo;

/* Model fonksiyon prototipleri */
ShmBuf* buf_init(void);
int execute_command(const char *command);
int model_send_message(ShmBuf *shmp, const char *msg);
int model_read_messages(ShmBuf *shmp, char *buffer, size_t bufsize);

/* Controller fonksiyon prototipleri */
void controller_parse_input(const char *input);
void controller_handle_command(const char *command);
void controller_handle_message(const char *message);
void controller_refresh_messages(void);

/* View (GUI) fonksiyon prototipleri */
void view_update_terminal(const char *output);
void view_display_message(const char *msg);
int view_main(int argc, char **argv);

#endif /* PROJECT_H */
