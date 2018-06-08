#include <stdio.h>
#include <sys/time.h>

double get_delta_timeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

void print_result(struct timeval *begin, struct timeval *end, int size, int count) {

    double delta_time, data_size_in_mb;

    delta_time = get_delta_timeofday(begin, end);

    data_size_in_mb = 1.0 * size * count / 1e6;

    printf("Data size: %.3f megabytes.\nChunk size: %d bytes.\nTime cost: %.2f seconds.\n",
        data_size_in_mb,
        size,
        delta_time
    );

}