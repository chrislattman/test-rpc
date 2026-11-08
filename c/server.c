#define _GNU_SOURCE // needed for truncate and ftruncate
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

static const unsigned short PORT_NUMBER = 5000;
// Upper bound on request payloads and read() sizes, so a bogus size from a
// client can't make the server allocate gigabytes
static const size_t MAX_PAYLOAD_SIZE = 64 * 1024 * 1024;
static int server_socket;

// supported remote procedure calls
enum function {
    OPEN,
    CLOSE,
    READ,
    WRITE,
    LSEEK,
    TRUNCATE,
    FTRUNCATE,
    STAT,
    FSTAT,
    FSYNC,
    RENAME,
    UNLINK,
};

/**
 * @brief Receives exactly n bytes. A single recv() can return fewer bytes
 * than requested, so keep calling it until everything has arrived.
 *
 * @param fd socket
 * @param buf destination buffer
 * @param n number of bytes to receive
 * @return 0 on success, or -1 on error or if the peer closed the connection early
 */
static int recv_all(int fd, void *buf, size_t n)
{
    unsigned char *p = buf;
    ssize_t received;

    while (n > 0) {
        if ((received = recv(fd, p, n, 0)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "recv: %s\n", strerror(errno));
            return -1;
        }
        if (received == 0) {
            fprintf(stderr, "recv: connection closed before full message arrived\n");
            return -1;
        }
        p += received;
        n -= (size_t) received;
    }
    return 0;
}

/**
 * @brief Sends exactly n bytes. A single send() can send fewer bytes than
 * requested, so keep calling it until everything has been sent.
 *
 * @param fd socket
 * @param buf source buffer
 * @param n number of bytes to send
 * @return 0 on success or -1 on error
 */
static int send_all(int fd, const void *buf, size_t n)
{
    const unsigned char *p = buf;
    ssize_t sent;

    while (n > 0) {
        // MSG_NOSIGNAL: a client that already disconnected shouldn't kill the
        // whole server with SIGPIPE
        if ((sent = send(fd, p, n, MSG_NOSIGNAL)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "send: %s\n", strerror(errno));
            return -1;
        }
        p += sent;
        n -= (size_t) sent;
    }
    return 0;
}

/**
 * @brief Checks that the payload contains n bytes starting at offset.
 *
 * Every field read out of a payload must be checked first: the payload comes
 * from the network, so its contents can't be trusted.
 */
static int has_bytes(size_t payload_size, size_t offset, size_t n)
{
    return offset <= payload_size && n <= payload_size - offset;
}

/**
 * @brief Thread that handles each client connection.
 *
 * Request buffers look like
 *
 * [payload size (8 bytes) || payload (payload size bytes)]
 *
 * Payloads look like
 *
 * [function code (1 byte) || function arguments]
 *
 * @param arg client socket, passed by value (cast to a pointer)
 * @return NULL
 */
static void *client_handler(void *arg)
{
    unsigned char *payload = NULL, function_code, *response_payload = NULL;
    unsigned char payload_size_buf[8];
    const char *path, *old, *new;
    void *buf = NULL;
    size_t payload_size, path_size, nbyte, response_payload_size;
    int client_socket, oflag, fildes, whence, error_code;
    mode_t mode;
    off_t offset, resulting_offset, length;
    struct stat *statbuf = NULL;
    ssize_t io_retval;

    // The socket is passed by value, so the main loop can't overwrite it
    // with the next connection's socket before this thread reads it
    client_socket = (int) (intptr_t) arg;

    // Reading the request happens here rather than in main() so that a slow
    // or silent client only holds up its own thread, not the accept loop

    // extract payload size
    if (recv_all(client_socket, payload_size_buf, sizeof(payload_size_buf)) < 0) {
        goto cleanup;
    }
    memcpy(&payload_size, payload_size_buf, sizeof(payload_size_buf));
    if (payload_size < sizeof(unsigned char) || payload_size > MAX_PAYLOAD_SIZE) {
        fprintf(stderr, "Invalid payload size %zu\n", payload_size);
        goto cleanup;
    }

    // create payload buffer, with one extra byte that's always '\0' so that
    // strlen() on a path from the payload can never run past the buffer
    payload = malloc(payload_size + 1);
    if (payload == NULL) {
        fprintf(stderr, "malloc: %s\n", strerror(errno));
        goto cleanup;
    }
    payload[payload_size] = '\0';

    // extract payload
    if (recv_all(client_socket, payload, payload_size) < 0) {
        goto cleanup;
    }

    function_code = payload[0];
    switch(function_code) {
        case OPEN:
            // int open(const char *path, int oflag, ...);
            // extract path, oflag, and possibly mode from payload and call open
            path = (char *) (payload + sizeof(unsigned char));
            path_size = strlen(path) + 1;
            if (!has_bytes(payload_size, sizeof(unsigned char) + path_size, sizeof(int))) {
                goto bad_payload;
            }
            memcpy(&oflag, payload + sizeof(unsigned char) + path_size, sizeof(int));
            if (oflag == O_CREAT) {
                if (!has_bytes(payload_size, sizeof(unsigned char) + path_size + sizeof(int), sizeof(mode_t))) {
                    goto bad_payload;
                }
                memcpy(&mode, payload + sizeof(unsigned char) + path_size + sizeof(int), sizeof(mode_t));
                fildes = open(path, oflag, mode);
            } else {
                fildes = open(path, oflag);
            }
            if (fildes >= 0) {
                fildes = INT_MAX - fildes;
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
            if (!has_bytes(payload_size, sizeof(unsigned char), sizeof(int))) {
                goto bad_payload;
            }
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
            if (!has_bytes(payload_size, sizeof(unsigned char), sizeof(int))) {
                goto bad_payload;
            }
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));
            if (!has_bytes(payload_size, sizeof(unsigned char) + sizeof(int), sizeof(size_t))) {
                goto bad_payload;
            }
            memcpy(&nbyte, payload + sizeof(unsigned char) + sizeof(int), sizeof(size_t));
            if (nbyte > MAX_PAYLOAD_SIZE) {
                goto bad_payload;
            }

            // create buf and call read (calloc: the whole buffer is sent back,
            // so bytes past what read() filled in must not be leftover heap data)
            buf = calloc(1, nbyte > 0 ? nbyte : 1);
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
            if (!has_bytes(payload_size, sizeof(unsigned char), sizeof(int))) {
                goto bad_payload;
            }
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));
            if (!has_bytes(payload_size, sizeof(unsigned char) + sizeof(int), sizeof(size_t))) {
                goto bad_payload;
            }
            memcpy(&nbyte, payload + payload_size - sizeof(size_t), sizeof(size_t));
            // buf sits between fildes and nbyte, so nbyte can't exceed that gap
            if (nbyte > payload_size - sizeof(unsigned char) - sizeof(int) - sizeof(size_t)) {
                goto bad_payload;
            }
            buf = malloc(nbyte > 0 ? nbyte : 1);
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
            if (!has_bytes(payload_size, sizeof(unsigned char), sizeof(int))) {
                goto bad_payload;
            }
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));
            if (!has_bytes(payload_size, sizeof(unsigned char) + sizeof(int), sizeof(off_t) + sizeof(int))) {
                goto bad_payload;
            }
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
        case TRUNCATE:
            // int truncate(const char *path, off_t length);
            // extract path and length from payload
            path = (char *) (payload + sizeof(unsigned char));
            path_size = strlen(path) + 1;
            if (!has_bytes(payload_size, sizeof(unsigned char) + path_size, sizeof(off_t))) {
                goto bad_payload;
            }
            memcpy(&length, payload + sizeof(unsigned char) + path_size, sizeof(off_t));

            // call truncate
            error_code = truncate(path, length);

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
        case FTRUNCATE:
            // int ftruncate(int fildes, off_t length);
            // extract fildes and length from payload
            if (!has_bytes(payload_size, sizeof(unsigned char), sizeof(int))) {
                goto bad_payload;
            }
            memcpy(&fildes, payload + sizeof(unsigned char), sizeof(int));
            if (!has_bytes(payload_size, sizeof(unsigned char) + sizeof(int), sizeof(off_t))) {
                goto bad_payload;
            }
            memcpy(&length, payload + sizeof(unsigned char) + sizeof(int), sizeof(off_t));

            // call ftruncate
            error_code = ftruncate(INT_MAX - fildes, length);

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
            if (!has_bytes(payload_size, sizeof(unsigned char), sizeof(int))) {
                goto bad_payload;
            }
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
            if (!has_bytes(payload_size, sizeof(unsigned char), sizeof(int))) {
                goto bad_payload;
            }
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
            // new must start inside the payload (it can then be at most the
            // terminating '\0' that was added after the payload)
            if (!has_bytes(payload_size, sizeof(unsigned char) + strlen(old) + 1, 0)) {
                goto bad_payload;
            }
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
    send_all(client_socket, response_payload, response_payload_size);
    goto cleanup;

bad_payload:
    fprintf(stderr, "Malformed payload for function %u\n", function_code);

cleanup:
    // Always close the socket, even after an error, so file descriptors don't
    // leak and the client isn't left waiting for a response forever
    if (shutdown(client_socket, SHUT_WR) < 0 && errno != ENOTCONN) {
        fprintf(stderr, "shutdown: %s\n", strerror(errno));
    }
    if (close(client_socket) < 0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
    }
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
 * @return 0
 */
int main(void)
{
    char *port_string;
    unsigned short port_number;
    int client_socket, err, reuseaddr = 1;
    struct sockaddr_in server_connection = {0};
    pthread_t thread_id;
    struct timeval recv_timeout = { .tv_sec = 10, .tv_usec = 0 };

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

    while (1) {
        if ((client_socket = accept(server_socket, NULL, NULL)) < 0) {
            // Errors like ECONNABORTED (client gave up) or EMFILE (out of
            // file descriptors) only affect one connection; keep serving
            fprintf(stderr, "accept: %s\n", strerror(errno));
            continue;
        }

        // Bound how long an idle client can tie up its thread
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) < 0) {
            fprintf(stderr, "setsockopt: %s\n", strerror(errno));
        }

        // call client_handler in new thread
        if ((err = pthread_create(&thread_id, NULL, client_handler,
                (void *) (intptr_t) client_socket)) != 0) {
            // pthread_create returns the error code rather than setting errno
            fprintf(stderr, "pthread_create: %s\n", strerror(err));
            close(client_socket);
            continue;
        }
        // Detached threads clean up after themselves. Joining them in batches
        // would make the accept loop wait on the slowest client in the batch.
        if ((err = pthread_detach(thread_id)) != 0) {
            fprintf(stderr, "pthread_detach: %s\n", strerror(err));
        }
    }

cleanup:
    if (close(server_socket) < 0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
    }
    return 0;
}
