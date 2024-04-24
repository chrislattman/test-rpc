#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/un.h>

static const unsigned short PORT_NUMBER = 5000;
static int server_socket;

// thread arguments
typedef struct thread_arg {
    unsigned char *payload;
    size_t size;
    int client_socket;
} thread_arg;

// supported remote procedure calls
enum function {
    OPEN,
    CLOSE,
    READ,
    WRITE,
    LSEEK,
    STAT,
    FSTAT,
    FSYNC,
    RENAME,
    UNLINK,
};

/**
 * @brief Thread that handles each client connection.
 *
 * Payloads look like
 *
 * [function code (1 byte) || function arguments]
 *
 * @param arg pointer to sock_info struct
 * @return NULL
 */
static void *client_handler(void *arg)
{
    thread_arg *args;
    unsigned char *payload, function_code, *response_payload = NULL;
    const char *path, *old, *new;
    void *buf = NULL;
    size_t payload_size, path_size, nbyte, response_payload_size;
    int client_socket, oflag, fildes, whence, error_code;
    mode_t mode;
    off_t offset, resulting_offset;
    struct stat *statbuf = NULL;
    ssize_t io_retval;

    // extract thread arguments
    args = (thread_arg *) arg;
    payload = args->payload;
    payload_size = args->size;
    client_socket = args->client_socket;

    function_code = payload[0];
    switch(function_code) {
        case OPEN:
            // int open(const char *path, int oflag, ...);
            // extract path, oflag, and possibly mode from payload and call open
            path = (char *) (payload + sizeof(unsigned char));
            path_size = strlen(path) + 1;
            memcpy(&oflag, payload + sizeof(unsigned char) + path_size, sizeof(int));
            if (oflag == O_CREAT) {
                memcpy(&mode, payload + sizeof(unsigned char) + path_size + sizeof(int), sizeof(mode_t));
                fildes = INT_MAX - open(path, oflag, mode);
            } else {
                fildes = INT_MAX - open(path, oflag);
            }

            // create response payload
            response_payload_size = sizeof(int);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include fildes in response payload
            memcpy(response_payload, &fildes, sizeof(int));
            break;
        case CLOSE:
            // int close(int fildes);
            // extract fildes from payload
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));

            // call close
            error_code = close(INT_MAX - fildes);

            // create response payload
            response_payload_size = sizeof(int);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include error_code in response payload
            memcpy(response_payload, &error_code, sizeof(int));
            break;
        case READ:
            // ssize_t read(int fildes, void *buf, size_t nbyte);
            // extract fildes and nbyte from payload
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));
            memcpy(&nbyte, payload + sizeof(unsigned char) + sizeof(int), sizeof(size_t));

            // create buf and call read
            buf = malloc(nbyte);
            if (buf == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }
            io_retval = read(INT_MAX - fildes, buf, nbyte);

            // create response payload
            response_payload_size = sizeof(ssize_t) + nbyte;
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include io_retval and buf in response payload
            memcpy(response_payload, &io_retval, sizeof(ssize_t));
            memcpy(response_payload + sizeof(ssize_t), buf, nbyte);
            break;
        case WRITE:
            // ssize_t write(int fildes, const void *buf, size_t nbyte);
            // extract fildes, buf, and nbyte from payload
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));
            memcpy(&nbyte, payload + payload_size - sizeof(size_t), sizeof(size_t));
            buf = malloc(payload_size - sizeof(unsigned char) - sizeof(int) - sizeof(size_t));
            if (buf == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }
            memcpy(buf, payload + sizeof(unsigned char) + sizeof(int), nbyte);

            // call write
            io_retval = write(INT_MAX - fildes, buf, nbyte);

            // create response payload
            response_payload_size = sizeof(ssize_t);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include io_retval in response payload
            memcpy(response_payload, &io_retval, sizeof(ssize_t));
            break;
        case LSEEK:
            // off_t lseek(int fildes, off_t offset, int whence);
            // extract fildes, offset, and whence from payload
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));
            memcpy(&offset, payload + sizeof(unsigned char) + sizeof(int), sizeof(off_t));
            memcpy(&whence, payload + sizeof(unsigned char) + sizeof(int) + sizeof(off_t), sizeof(int));

            // call lseek
            resulting_offset = lseek(INT_MAX - fildes, offset, whence);

            // create response payload
            response_payload_size = sizeof(off_t);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include resulting_offset in response payload
            memcpy(response_payload, &resulting_offset, sizeof(off_t));
            break;
        case STAT:
            // int stat(const char *restrict path, struct stat *restrict statbuf);
            // extract path from payload
            path = (char *) (payload + sizeof(unsigned char));

            // create statbuf and call stat
            statbuf = malloc(sizeof(struct stat));
            if (statbuf == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }
            error_code = stat(path, statbuf);

            // create response payload
            response_payload_size = sizeof(int) + sizeof(struct stat);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include error_code and statbuf in response payload
            memcpy(response_payload, &error_code, sizeof(int));
            memcpy(response_payload + sizeof(int), statbuf, sizeof(struct stat));
            break;
        case FSTAT:
            // int fstat(int fildes, struct stat *statbuf);
            // extract fildes from payload
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));

            // create statbuf and call fstat
            statbuf = malloc(sizeof(struct stat));
            if (statbuf == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }
            error_code = fstat(INT_MAX - fildes, statbuf);

            // create response payload
            response_payload_size = sizeof(int) + sizeof(struct stat);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include error_code and statbuf in response payload
            memcpy(response_payload, &error_code, sizeof(int));
            memcpy(response_payload + sizeof(int), statbuf, sizeof(struct stat));
            break;
        case FSYNC:
            // int fsync(int fildes);
            // extract fildes from payload
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));

            // call fsync
            error_code = fsync(INT_MAX - fildes);

            // create response payload
            response_payload_size = sizeof(int);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include error_code in response payload
            memcpy(response_payload, &error_code, sizeof(int));
            break;
        case RENAME:
            // int rename(const char *old, const char *new);
            // extract old and new from payload
            old = (char *) (payload + sizeof(unsigned char));
            new = (char *) (payload + sizeof(unsigned char) + strlen(old) + 1);

            // call rename
            error_code = rename(old, new);

            // create response payload
            response_payload_size = sizeof(int);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include error_code in response payload
            memcpy(response_payload, &error_code, sizeof(int));
            break;
        case UNLINK:
            // int unlink(const char *path);
            // extract path from payload
            path = (char *) (payload + sizeof(unsigned char));

            // call unlink
            error_code = unlink(path);

            // create response payload
            response_payload_size = sizeof(int);
            response_payload = malloc(response_payload_size);
            if (response_payload == NULL) {
                fprintf(stderr, "malloc: %s\n", strerror(errno));
                goto cleanup;
            }

            // include error_code in response payload
            memcpy(response_payload, &error_code, sizeof(int));
            break;
        default:
            fprintf(stderr, "Unsupported function\n");
            goto cleanup;
    }

    // send response payload
    if (send(client_socket, response_payload, response_payload_size, 0) < 0) {
        fprintf(stderr, "send: %s\n", strerror(errno));
        goto cleanup;
    }
    if (shutdown(client_socket, SHUT_WR) < 0) {
        fprintf(stderr, "shutdown: %s\n", strerror(errno));
        goto cleanup;
    }
    if (close(client_socket) < 0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
        goto cleanup;
    }

cleanup:
    free(response_payload);
    free(buf);
    free(statbuf);
    free(payload);
    return NULL;
}

/**
 * @brief Signal handler for Ctrl + C (SIGINT).
 *
 * @param signum unused
 */
static void signal_handler(__attribute__((unused)) int signum)
{
    if (close(server_socket) < 0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
    }
    exit(0);
}

/**
 * @brief Main server loop for the RPC server.
 *
 * Request buffers look like
 *
 * [payload size (8 bytes) || payload (payload size bytes)]
 *
 * @return 0
 */
int main(void)
{
    char *port_string;
    unsigned short port_number;
    int client_socket, thread_index, reuseaddr = 1;
    struct sockaddr_in server_connection;
    pthread_t thread_id[60];
    unsigned char payload_size_buf[8], *payload;
    size_t payload_size;
    thread_arg argument;

    port_string = getenv("RPC_PORT");
    if (port_string != NULL) {
        port_number = (unsigned short) atoi(port_string);
    } else {
        port_number = PORT_NUMBER;
    }

    // To use a Unix domain socket (local socket):
    // server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    // struct sockaddr_un local;
    // local.sun_family = AF_UNIX;
    // strncpy(local.sun_path, "/tmp/socket.sock", sizeof(local.sun_path) - 1);
    // bind(server_socket, (struct sockaddr *) &local, (socklen_t) sizeof(local));
    // listen(server_socket, INT_MAX);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket: %s\n", strerror(errno));
        exit(0);
    }
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt: %s\n", strerror(errno));
        exit(0);
    }

    server_connection.sin_family = AF_INET;
    server_connection.sin_port = htons(port_number);
    server_connection.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr *) &server_connection,
            (socklen_t) sizeof(server_connection)) < 0) {
        fprintf(stderr, "bind: %s\n", strerror(errno));
        goto cleanup;
    }
    signal(SIGINT, signal_handler);

    if (listen(server_socket, INT_MAX) < 0) {
        fprintf(stderr, "listen: %s\n", strerror(errno));
        goto cleanup;
    }

    thread_index = 0;
    while (1) {
        if ((client_socket = accept(server_socket, NULL, NULL)) < 0) {
            fprintf(stderr, "accept: %s\n", strerror(errno));
            goto cleanup;
        }

        // extract payload size
        if (recv(client_socket, payload_size_buf, sizeof(payload_size_buf), 0) < 0) {
            fprintf(stderr, "recv: %s\n", strerror(errno));
            goto cleanup;
        }
        memcpy(&payload_size, payload_size_buf, sizeof(payload_size_buf));

        // create payload buffer (gets freed in client_handler)
        payload = malloc(payload_size);
        if (payload == NULL) {
            fprintf(stderr, "malloc: %s\n", strerror(errno));
            goto cleanup;
        }

        // extract payload
        if (recv(client_socket, payload, payload_size, 0) < 0) {
            fprintf(stderr, "recv: %s\n", strerror(errno));
            goto cleanup;
        }

        // call client_handler in new thread
        argument = (thread_arg) {
            .payload = payload,
            .size = payload_size,
            .client_socket = client_socket,
        };
        if (pthread_create(&thread_id[thread_index++], NULL, client_handler,
                &argument) != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(errno));
            goto cleanup;
        }

        if (thread_index >= 50) {
            thread_index = 0;
            while (thread_index < 50) {
                if (pthread_join(thread_id[thread_index++], NULL) != 0) {
                    fprintf(stderr, "pthread_join: %s\n", strerror(errno));
                    goto cleanup;
                }
            }
            thread_index = 0;
        }
    }

cleanup:
    if (close(server_socket) < 0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
    }
    return 0;
}
