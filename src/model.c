#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glib.h>
#include "../include/project.h"

#define BUF_SIZE 4096
#define MAX_ARGS 64
#define MAX_PIPE_CMDS 16  // Desteklenecek en fazla pipe segmenti


ShmBuf* buf_init(void) {
    ShmBuf *shmp;

    // Önceki paylaşılan belleği temizle
    shm_unlink(SHARED_FILE_PATH); // Bu hata verirse yok sayılır, önemli değil

    int fd = shm_open(SHARED_FILE_PATH, O_CREAT | O_RDWR, 0600);
    if (fd < 0) {
        perror("could not open shared file");
        exit(EXIT_FAILURE);
    }

    printf("BUF_SIZE: %ld\n", (long)BUF_SIZE);
    if (ftruncate(fd, BUF_SIZE) == -1) {
        perror("ftruncate error");
        exit(EXIT_FAILURE);
    }

    shmp = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED) {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    shmp->fd = fd;
    shmp->sem = sem_open("/mysem", O_CREAT, 0600, 1);
    if (shmp->sem == SEM_FAILED) {
        perror("sem_open error");
        exit(EXIT_FAILURE);
    }

    return shmp;
}
int model_send_message(ShmBuf *shmp, const char *msg) {
    if (shmp == NULL || msg == NULL) return -1;

    sem_wait(shmp->sem); // & kaldırıldı
    strncpy(shmp->msgbuf, msg, BUF_SIZE - sizeof(ShmBuf));
    shmp->msgbuf[BUF_SIZE - sizeof(ShmBuf) - 1] = '\0';
    shmp->cnt = strlen(shmp->msgbuf);
    sem_post(shmp->sem); // & kaldırıldı
    return 0;
}

int model_read_messages(ShmBuf *shmp, char *buffer, size_t bufsize) {
    if (shmp == NULL || buffer == NULL) return -1;

    sem_wait(shmp->sem); // & kaldırıldı
    size_t count = shmp->cnt;
    if (count >= bufsize) {
        count = bufsize - 1;
    }
    strncpy(buffer, shmp->msgbuf, count);
    buffer[count] = '\0';
    sem_post(shmp->sem); // & kaldırıldı
    return count;
}
static void trim_whitespace(char **str) {
    // Başındaki boşlukları atla:
    while (**str == ' ') (*str)++;
    // Sondaki boşluklar için; basitçe strlen kullanıp, sondan geriye doğru kontrol edebilirsiniz.
    char *end = *str + strlen(*str) - 1;
    while (end > *str && (*end == ' ' || *end == '\n')) {
        *end = '\0';
        end--;
    }
}

/*
 * execute_pipeline: Komut satırında '|' operatörüne göre ayrılmış segmentleri zincir halinde çalıştırır.
 *
 * Örnek: "ls -l | grep .c | wc -l" şeklindeki komutları destekler.
 */
int execute_pipeline(const char *command) {
    char *cmd_copy = strdup(command);
    if (!cmd_copy) {
        perror("strdup error");
        return -1;
    }

    // Komutları "|" operatörüne göre bölüyoruz.
    char *commands[MAX_PIPE_CMDS];
    int cmd_count = 0;
    char *segment = strtok(cmd_copy, "|");
    while (segment && cmd_count < MAX_PIPE_CMDS) {
        trim_whitespace(&segment);
        commands[cmd_count++] = segment;
        segment = strtok(NULL, "|");
    }
    commands[cmd_count] = NULL;  // Sonlandırma

    int i, status = 0;
    int prev_fd = -1;   // Önceki pipe'ın okuma ucu
    pid_t pid;
    int pipefd[2];      // Ara segmentler için geçici pipe
    int final_pipefd[2]; // Son segment için final pipe

    // Son segmentin çıktısını yakalamak için final pipe oluşturuyoruz.
    if (pipe(final_pipefd) == -1) {
        perror("final pipe error");
        free(cmd_copy);
        return -1;
    }

    // Her segment için döngü
    for (i = 0; i < cmd_count; i++) {
        // Son segment değilse, geçici bir pipe oluştur.
        if (i < cmd_count - 1) {
            if (pipe(pipefd) == -1) {
                perror("pipe error");
                free(cmd_copy);
                return -1;
            }
        }

        pid = fork();
        if (pid < 0) {
            perror("fork error");
            free(cmd_copy);
            return -1;
        }
        if (pid == 0) {  // Çocuk süreç
            // İlk segment değilse, önceki pipe'dan gelen veriyi STDIN'e yönlendir.
            if (i > 0) {
                if (dup2(prev_fd, STDIN_FILENO) < 0) {
                    perror("dup2 error");
                    exit(EXIT_FAILURE);
                }
                close(prev_fd);
            }
            // Eğer bu segment son segmentse, çıktıyı final_pipefd'nin yazma ucuna yönlendir.
            if (i == cmd_count - 1) {
                close(final_pipefd[0]);  // Okuma ucunu kapat
                if (dup2(final_pipefd[1], STDOUT_FILENO) < 0) {
                    perror("dup2 error");
                    exit(EXIT_FAILURE);
                }
                if (dup2(final_pipefd[1], STDERR_FILENO) < 0) {
                    perror("dup2 error");
                    exit(EXIT_FAILURE);
                }
                close(final_pipefd[1]);
            }
            // Son segment değilse, çıktıyı geçici pipe'ın yazma ucuna yönlendir.
            else {
                close(pipefd[0]);
                if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                    perror("dup2 error");
                    exit(EXIT_FAILURE);
                }
                close(pipefd[1]);
            }
            // Komut segmentini tokenize et ve execvp ile çalıştır.
            char *args[MAX_ARGS];
            int j = 0;
            char *token = strtok(commands[i], " ");
            while (token != NULL && j < MAX_ARGS - 1) {
                args[j++] = token;
                token = strtok(NULL, " ");
            }
            args[j] = NULL;

            execvp(args[0], args);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } else {  // Ebeveyn süreç
            if (i > 0)
                close(prev_fd);
            if (i < cmd_count - 1) {
                // Geçici pipe kullanılıyorsa, okunacak ucu sakla.
                prev_fd = pipefd[0];
                close(pipefd[1]);
            }
        }
    }

    // Tüm çocuk süreçlerini bekle.
    for (i = 0; i < cmd_count; i++) {
        wait(&status);
    }

    // Final pipe'dan çıktıyı okuyup view_update_terminal ile gönder.
    close(final_pipefd[1]);  // Yazma ucunu kapat
    char buffer[BUF_SIZE];
    int nbytes;
    while ((nbytes = read(final_pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[nbytes] = '\0';
        view_update_terminal(buffer);
    }
    if (nbytes < 0) {
        perror("read error");
    }
    close(final_pipefd[0]);

    free(cmd_copy);
    return status;
}

/*
 * execute_command: Komut satırındaki redirection veya pipe operatörüne göre komutu çalıştırır.
 *
 * Eğer komut satırında "|" karakteri varsa, execute_pipeline() çağrılır.
 * Aksi halde, eğer ">" operatörü varsa çıkış redirection yapılır.
 */
int execute_command(const char *command) {
    if (strchr(command, '|') != NULL) {
        return execute_pipeline(command);
    }

    int pipefd[2];
    char buffer[BUF_SIZE];
    pid_t pid;

    char *cmd_copy = strdup(command);
    if (!cmd_copy) {
        perror("strdup error");
        return -1;
    }

    // Redirection kontrolleri
    int redirect_out = 0, redirect_append = 0, redirect_in = 0;
    char *outfile = NULL;
    char *infile = NULL;

    // Output append (>>)
    char *append_ptr = strstr(cmd_copy, ">>");
    if (append_ptr) {
        redirect_append = 1;
        *append_ptr = '\0';
        append_ptr += 2;
        while (*append_ptr == ' ') append_ptr++;
        outfile = append_ptr;
        trim_whitespace(&outfile);
    } else {
        // Output overwrite (>)
        char *redir_ptr = strstr(cmd_copy, ">");
        if (redir_ptr) {
            redirect_out = 1;
            *redir_ptr = '\0';
            redir_ptr++;
            while (*redir_ptr == ' ') redir_ptr++;
            outfile = redir_ptr;
            trim_whitespace(&outfile);
        }
    }

    // Input redirection (<)
    char *in_ptr = strstr(cmd_copy, "<");
    if (in_ptr) {
        redirect_in = 1;
        *in_ptr = '\0';
        in_ptr++;
        while (*in_ptr == ' ') in_ptr++;
        infile = in_ptr;
        trim_whitespace(&infile);
    }

    trim_whitespace(&cmd_copy);

    // Tokenize
    char *args[MAX_ARGS];
    int i = 0;
    char *token = strtok(cmd_copy, " ");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (!redirect_out && !redirect_append && !redirect_in) {
        if (pipe(pipefd) == -1) {
            perror("pipe error");
            free(cmd_copy);
            return -1;
        }
    }

    pid = fork();
    if (pid < 0) {
        perror("fork error");
        free(cmd_copy);
        return -1;
    } else if (pid == 0) {
        // Çocuk süreç: redirection işlemleri
        if (redirect_in) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) {
                perror("open infile error");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2 infile error");
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);
        }

        if (redirect_out || redirect_append) {
            int flags = O_WRONLY | O_CREAT;
            if (redirect_append) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }

            int fd = open(outfile, flags, 0666);
            if (fd < 0) {
                perror("open outfile error");
                exit(EXIT_FAILURE);
            }

            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2 outfile error");
                close(fd);
                exit(EXIT_FAILURE);
            }

            close(fd);
        } else {
            // Pipe'dan çıktı al
            close(pipefd[0]);
            if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                perror("dup2 stdout error");
                exit(EXIT_FAILURE);
            }
            if (dup2(pipefd[1], STDERR_FILENO) < 0) {
                perror("dup2 stderr error");
                exit(EXIT_FAILURE);
            }
            close(pipefd[1]);
        }

        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        // Ebeveyn süreç
        int status;
        waitpid(pid, &status, 0);

        if (!redirect_out && !redirect_append) {
            close(pipefd[1]);
            int nbytes = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (nbytes < 0) {
                perror("read error");
            } else {
                buffer[nbytes] = '\0';
                view_update_terminal(buffer);
            }
            close(pipefd[0]);
        }

        free(cmd_copy);
        return status;
    }
}
