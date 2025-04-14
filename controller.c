#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "../include/project.h"

extern ShmBuf *g_shm;

static gboolean refresh_messages_callback(gpointer data) {
    (void)data;
    controller_refresh_messages();
    return G_SOURCE_CONTINUE;
}

void controller_init(void) {
    g_timeout_add(1500, refresh_messages_callback, NULL);
}

void controller_parse_input(const char *input) {
    if (!input) return;

    char formatted_msg[BUF_SIZE];
    pid_t pid = getpid();

    if (strncmp(input, "@msg", 4) == 0) {
        const char *message = input + 4;
        while (*message == ' ') message++;

        snprintf(formatted_msg, sizeof(formatted_msg), "Terminal %d: %s", pid, message);
        if (model_send_message(g_shm, formatted_msg) == 0) {
            view_display_message(formatted_msg);
            printf("Sent message: %s\n", formatted_msg); // Gönderme kontrolü
        } else {
            fprintf(stderr, "Error while sending message\n");
        }
    } else {
        controller_handle_command(input);
    }
}

void controller_refresh_messages(void) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    int bytesRead = model_read_messages(g_shm, buffer, sizeof(buffer));
    if (bytesRead > 0) {
        pid_t my_pid = getpid();
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "Terminal %d: ", my_pid);
        
        if (strncmp(buffer, pid_str, strlen(pid_str)) != 0) {
            view_display_message(buffer);
            // Mesaj görüntülendi, şimdi temizle
            sem_wait(g_shm->sem);
            memset(g_shm->msgbuf, 0, BUF_SIZE - sizeof(ShmBuf));
            g_shm->cnt = 0;
            sem_post(g_shm->sem);
        }
    } else if (bytesRead < 0) {
        fprintf(stderr, "Mesaj okuma hatası meydana geldi.\n");
    }
}
// controller_handle_command ve controller_handle_message aynı kalabilir

void controller_handle_command(const char *command) {
    if (!command) {
        fprintf(stderr, "Need a valid command\n");
        return;
    }
    int status = execute_command(command);
    char resultMsg[256];
    snprintf(resultMsg, sizeof(resultMsg), "Running command '%s' with %d status code", command, status);
    view_update_terminal(resultMsg);
}
void controller_handle_message(const char *message){
    if(!message){
        fprintf(stderr,"Write a valid message\n");
        return;
    }
    if(model_send_message(g_shm,message)==0){
        view_display_message(message);
    }else{
        fprintf(stderr,"Error while sending the message");
    }
}






