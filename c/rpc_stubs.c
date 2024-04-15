#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

static const char *HOSTNAME = "127.0.0.1";
static const char *PORT_NUMBER = "5000";

typedef int (*open_t)(const char *, int, ...);
typedef int (*close_t)(int);
typedef ssize_t (*read_t)(int, void *, size_t);
typedef ssize_t (*write_t)(int, const void *, size_t);
typedef off_t (*lseek_t)(int, off_t, int);
typedef int (*stat_t)(const char *restrict, struct stat *restrict);
typedef int (*fstat_t)(int, struct stat *);
typedef int (*fsync_t)(int);

static open_t real_open = NULL;
static close_t real_close = NULL;
static read_t real_read = NULL;
static write_t real_write = NULL;
static lseek_t real_lseek = NULL;
static stat_t real_stat = NULL;
static fstat_t real_fstat = NULL;
static fsync_t real_fsync = NULL;

static const char *host_string = NULL;
static unsigned short port_number = 0;
static int sock;
struct sockaddr_in connection;

enum function {
    OPEN,
    CLOSE,
    READ,
    WRITE,
    LSEEK,
    STAT,
    FSTAT,
    FSYNC,
};

/**
 * @brief Connects to the RPC server
 *
 * @return 0 on success or -1 on error
 */
static int connect_to_server(void)
{
    int reuseaddr = 1;
    const char *port_string;

    if (host_string == NULL || port_number == 0) {
        host_string = getenv("RPC_HOST");
        if (host_string == NULL) {
            host_string = HOSTNAME;
        }
        port_string = getenv("RPC_PORT");
        if (port_string == NULL) {
            port_string = PORT_NUMBER;
        }
        port_number = (unsigned short) atoi(port_string);
    }

    if (real_close == NULL) {
        real_close = dlsym(RTLD_NEXT, "close");
    }

    // To use a Unix domain socket (local socket):
    // sock = socket(AF_UNIX, SOCK_STREAM, 0);
    // struct sockaddr_un local;
    // local.sun_family = AF_UNIX;
    // strncpy(local.sun_path, "/tmp/socket.sock", sizeof(local.sun_path) - 1);
    // connect(sock, (struct sockaddr *) &local, (socklen_t) sizeof(local));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket: %s\n", strerror(errno));
        return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt: %s\n", strerror(errno));
        real_close(sock);
        return -1;
    }
    connection.sin_family = AF_INET;
    connection.sin_port = htons(port_number);
    connection.sin_addr.s_addr = inet_addr(host_string);
    if (connect(sock, (struct sockaddr *) &connection, (socklen_t) sizeof(connection)) < 0) {
        fprintf(stderr, "connect: %s\n", strerror(errno));
        real_close(sock);
        return -1;
    }

    return 0;
}

/**
 * @brief Sends bytes to the RPC server.
 *
 * @param buf byte buffer
 * @param buf_size number of bytes to send
 * @return 0 on success or -1 on error
 */
static int send_data(unsigned char *buf, size_t buf_size)
{
    if (connect_to_server() < 0) {
        return -1;
    }
    if (send(sock, buf, buf_size, 0) < 0) {
        fprintf(stderr, "send: %s\n", strerror(errno));
        real_close(sock);
        return -1;
    }
    // not closing the socket on successful send until recv_data is called
    return 0;
}

/**
 * @brief Receives bytes from the RPC server.
 *
 * @param buf pointer to byte buffer
 * @param bytes_to_read number of bytes to receive
 * @return 0 on success or -1 on error
 */
static int recv_data(unsigned char **buf, size_t bytes_to_read)
{
    // we only call recv_data after calling send_data, so it is safe to assume
    // that we are already connected to the server
    if (recv(sock, *buf, bytes_to_read, 0) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        real_close(sock);
        return -1;
    }
    if (real_close(sock) < 0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * @brief Receives bytes from the RPC server into a stack buffer.
 *
 * @param buf stack buffer
 * @param bytes_to_read number of bytes to receive
 * @return 0 on success or -1 on error
 */
static int recv_data_buf(unsigned char buf[], size_t bytes_to_read)
{
    // we only call recv_data after calling send_data, so it is safe to assume
    // that we are already connected to the server
    if (recv(sock, buf, bytes_to_read, 0) < 0) {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        real_close(sock);
        return -1;
    }
    if (real_close(sock) < 0) {
        fprintf(stderr, "close: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int open(const char *path, int oflag, ...)
{
    va_list args;
    mode_t mode;
    size_t request_payload_size, path_size, response_payload_size;
    unsigned char *request_buf = NULL;
    unsigned char response_payload[sizeof(int)];
    int status, fildes = -1;

    if (real_open == NULL) {
        real_open = dlsym(RTLD_NEXT, "open");
    }

    va_start(args, oflag);
    if (oflag == O_CREAT) {
        mode = va_arg(args, mode_t);
    }
    va_end(args);

    if (path[0] == '/' && path[1] == '/') { // calling open remotely
        path_size = strlen(path) - 1; // minus first 2 forward slashes plus null terminator
        // create request buffer, including request payload
        if (oflag == O_CREAT) {
            request_payload_size = sizeof(unsigned char) + path_size + sizeof(int) + sizeof(mode_t);
        } else {
            request_payload_size = sizeof(unsigned char) + path_size + sizeof(int);
        }
        request_buf = malloc(sizeof(size_t) + request_payload_size);
        if (request_buf == NULL) {
            fprintf(stderr, "malloc: %s\n", strerror(errno));
            goto cleanup;
        }

        // prepend payload size and open function code
        memcpy(request_buf, &request_payload_size, sizeof(size_t));
        request_buf[sizeof(size_t)] = OPEN;

        // send path, oflag, and possibly mode
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char), path + 2, path_size);
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char) + path_size, &oflag, sizeof(int));
        if (oflag == O_CREAT) {
            memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char) + path_size + sizeof(int), &mode, sizeof(mode_t));
        }
        status = send_data(request_buf, sizeof(size_t) + request_payload_size);
        if (status < 0) {
            goto cleanup;
        }

        // receive response payload
        response_payload_size = sizeof(response_payload);
        status = recv_data_buf(response_payload, response_payload_size);
        if (status < 0) {
            goto cleanup;
        }

        // extract fildes from response payload
        memcpy(&fildes, response_payload, sizeof(int));
    } else { // calling open locally
        if (oflag == O_CREAT) {
            return real_open(path, oflag, mode);
        } else {
            return real_open(path, oflag);
        }
    }

cleanup:
    free(request_buf);
    return fildes;
}

int close(int fildes)
{
    size_t request_payload_size, response_payload_size;
    unsigned char request_buf[sizeof(size_t) + sizeof(unsigned char) + sizeof(int)];
    unsigned char response_payload[sizeof(int)];
    int status, error_code = -1;

    if (real_close == NULL) {
        real_close = dlsym(RTLD_NEXT, "close");
    }

    if (fildes > INT_MAX / 2) { // calling close remotely
        request_payload_size = sizeof(request_buf) - sizeof(size_t);

        // prepend payload size and close function code
        memcpy(request_buf, &request_payload_size, sizeof(size_t));
        request_buf[sizeof(size_t)] = CLOSE;

        // send fildes
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char), &fildes, sizeof(int));
        status = send_data(request_buf, sizeof(size_t) + request_payload_size);
        if (status < 0) {
            return error_code;
        }

        // receive response payload
        response_payload_size = sizeof(response_payload);
        status = recv_data_buf(response_payload, response_payload_size);
        if (status < 0) {
            return error_code;
        }

        // extract error_code from response payload
        memcpy(&error_code, response_payload, sizeof(int));
    } else { // calling close locally
        return real_close(fildes);
    }

    return error_code;
}

ssize_t read(int fildes, void *buf, size_t nbyte)
{
    size_t request_payload_size, response_payload_size;
    unsigned char request_buf[sizeof(size_t) + sizeof(unsigned char) + sizeof(int) + sizeof(size_t)];
    unsigned char *response_payload = NULL;
    int status;
    ssize_t io_retval = -1;

    if (real_read == NULL) {
        real_read = dlsym(RTLD_NEXT, "read");
    }

    if (fildes > INT_MAX / 2) { // calling read remotely
        request_payload_size = sizeof(request_buf) - sizeof(size_t);

        // prepend payload size and read function code
        memcpy(request_buf, &request_payload_size, sizeof(size_t));
        request_buf[sizeof(size_t)] = READ;

        // send fildes and nbyte
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char), &fildes, sizeof(int));
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char) + sizeof(int), &nbyte, sizeof(size_t));
        status = send_data(request_buf, sizeof(size_t) + request_payload_size);
        if (status < 0) {
            return io_retval;
        }

        // receive response payload
        response_payload_size = sizeof(ssize_t) + nbyte;
        response_payload = malloc(response_payload_size);
        if (response_payload == NULL) {
            fprintf(stderr, "malloc: %s\n", strerror(errno));
            goto cleanup;
        }
        status = recv_data(&response_payload, response_payload_size);
        if (status < 0) {
            goto cleanup;
        }

        // extract io_retval and buf from response payload
        memcpy(&io_retval, response_payload, sizeof(ssize_t));
        memcpy(buf, response_payload + sizeof(ssize_t), nbyte);
    } else { // calling read locally
        return real_read(fildes, buf, nbyte);
    }

cleanup:
    free(response_payload);
    return io_retval;
}

ssize_t write(int fildes, const void *buf, size_t nbyte)
{
    size_t request_payload_size, response_payload_size;
    unsigned char *request_buf = NULL;
    unsigned char response_payload[sizeof(ssize_t)];
    int status;
    ssize_t io_retval = -1;

    if (real_write == NULL) {
        real_write = dlsym(RTLD_NEXT, "write");
    }

    if (fildes > INT_MAX / 2) { // calling write remotely
        // create request buffer, including request payload
        request_payload_size = sizeof(unsigned char) + sizeof(int) + nbyte + sizeof(size_t);
        request_buf = malloc(sizeof(size_t) + request_payload_size);
        if (request_buf == NULL) {
            fprintf(stderr, "malloc: %s\n", strerror(errno));
            goto cleanup;
        }

        // prepend payload size and write function code
        memcpy(request_buf, &request_payload_size, sizeof(size_t));
        request_buf[sizeof(size_t)] = WRITE;

        // send fildes, buf, and nbyte
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char), &fildes, sizeof(int));
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char) + sizeof(int), buf, nbyte);
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char) + sizeof(int) + nbyte, &nbyte, sizeof(size_t));
        status = send_data(request_buf, sizeof(size_t) + request_payload_size);
        if (status < 0) {
            goto cleanup;
        }

        // receive response payload
        response_payload_size = sizeof(response_payload);
        status = recv_data_buf(response_payload, response_payload_size);
        if (status < 0) {
            goto cleanup;
        }

        // extract io_retval from response payload
        memcpy(&io_retval, response_payload, sizeof(ssize_t));
    } else { // calling write locally
        return real_write(fildes, buf, nbyte);
    }

cleanup:
    free(request_buf);
    return io_retval;
}

off_t lseek(int fildes, off_t offset, int whence)
{
    size_t request_payload_size, response_payload_size;
    unsigned char request_buf[sizeof(size_t) + sizeof(unsigned char) + sizeof(int) + sizeof(off_t) + sizeof(int)];
    unsigned char response_payload[sizeof(off_t)];
    int status;
    off_t resulting_offset = -1;

    if (real_lseek == NULL) {
        real_lseek = dlsym(RTLD_NEXT, "lseek");
    }

    if (fildes > INT_MAX / 2) { // calling lseek remotely
        request_payload_size = sizeof(request_buf) - sizeof(size_t);

        // prepend payload size and lseek function code
        memcpy(request_buf, &request_payload_size, sizeof(size_t));
        request_buf[sizeof(size_t)] = LSEEK;

        // send fildes, offset, and whence
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char), &fildes, sizeof(int));
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char) + sizeof(int), &offset, sizeof(off_t));
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char) + sizeof(int) + sizeof(off_t), &whence, sizeof(int));
        status = send_data(request_buf, sizeof(size_t) + request_payload_size);
        if (status < 0) {
            return resulting_offset;
        }

        // receive response payload
        response_payload_size = sizeof(response_payload);
        status = recv_data_buf(response_payload, response_payload_size);
        if (status < 0) {
            return resulting_offset;
        }

        // extract resulting_offset from response payload
        memcpy(&resulting_offset, response_payload, sizeof(off_t));
    } else { // calling lseek locally
        return real_lseek(fildes, offset, whence);
    }

    return resulting_offset;
}

int stat(const char *restrict path, struct stat *restrict statbuf)
{
    size_t request_payload_size, path_size, response_payload_size;
    unsigned char *request_buf = NULL;
    unsigned char response_payload[sizeof(int) + sizeof(struct stat)];
    int status, error_code = -1;

    if (real_stat == NULL) {
        real_stat = dlsym(RTLD_NEXT, "stat");
    }

    if (path[0] == '/' && path[1] == '/') { // calling stat remotely
        // create request buffer, including request payload
        path_size = strlen(path) - 1; // minus first 2 forward slashes plus null terminator
        request_payload_size = sizeof(unsigned char) + path_size;
        request_buf = malloc(sizeof(size_t) + request_payload_size);
        if (request_buf == NULL) {
            fprintf(stderr, "malloc: %s\n", strerror(errno));
            goto cleanup;
        }

        // prepend payload size and stat function code
        memcpy(request_buf, &request_payload_size, sizeof(size_t));
        request_buf[sizeof(size_t)] = STAT;

        // send path
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char), path + 2, path_size);
        status = send_data(request_buf, sizeof(size_t) + request_payload_size);
        if (status < 0) {
            goto cleanup;
        }

        // receive response payload
        response_payload_size = sizeof(response_payload);
        status = recv_data_buf(response_payload, response_payload_size);
        if (status < 0) {
            goto cleanup;
        }

        // extract error_code and statbuf from response payload
        memcpy(&error_code, response_payload, sizeof(int));
        memcpy(statbuf, response_payload + sizeof(int), sizeof(struct stat));
    } else { // calling stat locally
        return real_stat(path, statbuf);
    }

cleanup:
    free(request_buf);
    return error_code;
}

int fstat(int fildes, struct stat *statbuf)
{
    size_t request_payload_size, response_payload_size;
    unsigned char request_buf[sizeof(size_t) + sizeof(unsigned char) + sizeof(int)];
    unsigned char response_payload[sizeof(int) + sizeof(struct stat)];
    int status, error_code = -1;

    if (real_fstat == NULL) {
        real_fstat = dlsym(RTLD_NEXT, "fstat");
    }

    if (fildes > INT_MAX / 2) { // calling fstat remotely
        request_payload_size = sizeof(request_buf) - sizeof(size_t);

        // prepend payload size and fstat function code
        memcpy(request_buf, &request_payload_size, sizeof(size_t));
        request_buf[sizeof(size_t)] = FSTAT;

        // send fildes
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char), &fildes, sizeof(int));
        status = send_data(request_buf, sizeof(size_t) + request_payload_size);
        if (status < 0) {
            return error_code;
        }

        // receive response payload
        response_payload_size = sizeof(response_payload);
        status = recv_data_buf(response_payload, response_payload_size);
        if (status < 0) {
            return error_code;
        }

        // extract error_code and statbuf from response payload
        memcpy(&error_code, response_payload, sizeof(int));
        memcpy(statbuf, response_payload + sizeof(int), sizeof(struct stat));
    } else { // calling fstat locally
        return real_fstat(fildes, statbuf);
    }

    return error_code;
}

int fsync(int fildes)
{
    size_t request_payload_size, response_payload_size;
    unsigned char request_buf[sizeof(size_t) + sizeof(unsigned char) + sizeof(int)];
    unsigned char response_payload[sizeof(int)];
    int status, error_code = -1;

    if (real_fsync == NULL) {
        real_fsync = dlsym(RTLD_NEXT, "fsync");
    }

    if (fildes > INT_MAX / 2) { // calling fsync remotely
        request_payload_size = sizeof(request_buf) - sizeof(size_t);

        // prepend payload size and fsync function code
        memcpy(request_buf, &request_payload_size, sizeof(size_t));
        request_buf[sizeof(size_t)] = FSYNC;

        // send fildes
        memcpy(request_buf + sizeof(size_t) + sizeof(unsigned char), &fildes, sizeof(int));
        status = send_data(request_buf, sizeof(size_t) + request_payload_size);
        if (status < 0) {
            return error_code;
        }

        // receive response payload
        response_payload_size = sizeof(response_payload);
        status = recv_data_buf(response_payload, response_payload_size);
        if (status < 0) {
            return error_code;
        }

        // extract error_code from response payload
        memcpy(&error_code, response_payload, sizeof(int));
    } else { // calling fsync locally
        return real_fsync(fildes);
    }

    return error_code;
}
