/* Multithreaded stress test for IOTrace: 4 threads x 200000 fwrite() calls. */
#include <pthread.h>
#include <stdio.h>

enum { THREADS = 4, CALLS = 200000 };

static void *worker(void *arg)
{
    FILE *f = arg;
    for (int i = 0; i < CALLS; ++i)
        fwrite("x", 1, 1, f);
    return NULL;
}

int main(void)
{
    FILE *f = fopen("/dev/null", "w");
    if (!f) return 1;
    pthread_t t[THREADS];
    for (int i = 0; i < THREADS; ++i) pthread_create(&t[i], NULL, worker, f);
    for (int i = 0; i < THREADS; ++i) pthread_join(t[i], NULL);
    fclose(f);
    return 0;
}
