#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

int main(int argc, char *argv[]) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    sd_bus *bus = NULL;
    int64_t result;
    int r;

    /* Connect to the user bus */
    r = sd_bus_open_user(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to user bus: %s\n", strerror(-r));
        goto finish;
    }

    /* Invoke the remote function "Multiply" synchronously */
    r = sd_bus_call_method(bus,
                           "com.example.MySimpleCalculator",    /* Service Name */
                           "/com/example/MySimpleCalculator",   /* Object Path */
                           "com.example.MySimpleCalculator",    /* Interface Name */
                           "Multiply",                     /* Method Name */
                           &error,                         /* Error return object */
                           &m,                             /* Response message object */
                           "xx",                           /* Input argument types (2 int64) */
                           (int64_t) 6, (int64_t) 7);      /* Values to send */
    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
        goto finish;
    }

    /* Parse the single 64-bit integer response from the server */
    r = sd_bus_message_read(m, "x", &result);
    if (r < 0) {
        fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
        goto finish;
    }

    printf("Client received result from server: %lld\n", (long long)result);

finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    sd_bus_unref(bus);

    return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
