#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <systemd/sd-bus.h>

/* Callback function invoked when the client calls "Multiply" */
static int method_multiply(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int64_t a, b;
    int r;

    /* Read the two 64-bit integer arguments sent by the client */
    r = sd_bus_message_read(m, "xx", &a, &b);
    if (r < 0) {
        fprintf(stderr, "Failed to parse command line arguments: %s\n", strerror(-r));
        return r;
    }

    printf("Server received: a = %lld, b = %lld\n", (long long)a, (long long)b);

    /* Send the response back containing the multiplied result */
    return sd_bus_reply_method_return(m, "x", a * b);
}

/* Define the VTable for the D-Bus interface */
static const sd_bus_vtable my_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Multiply", "xx", "x", method_multiply, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

int main(int argc, char *argv[]) {
    sd_bus *bus = NULL;
    int r;

    /* Connect to the user bus */
    r = sd_bus_open_user(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to user bus: %s\n", strerror(-r));
        goto finish;
    }

    /* Install the object path and its associated VTable */
    r = sd_bus_add_object_vtable(bus,
                                 NULL,
                                 "/com/example/MySimpleCalculator",  /* Object Path */
                                 "com.example.MySimpleCalculator",   /* Interface Name */
                                 my_vtable,
                                 NULL);
    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
        goto finish;
    }

    /* Take a well-known service name so clients can find this server */
    r = sd_bus_request_name(bus, "com.example.MySimpleCalculator", 0);
    if (r < 0) {
        fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
        goto finish;
    }

    printf("Server is running and listening for D-Bus requests...\n");

    /* Enter the main event loop to process requests */
    for (;;) {
        r = sd_bus_process(bus, NULL);
        if (r < 0) {
            fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
            goto finish;
        }
        if (r > 0) /* If something was processed, cycle immediately */
            continue;

        /* Wait for next event if nothing was pending */
        r = sd_bus_wait(bus, (uint64_t) -1);
        if (r < 0) {
            fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
            goto finish;
        }
    }

finish:
    sd_bus_unref(bus);
    return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
