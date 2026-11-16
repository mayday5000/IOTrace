/* Small test program for IOTrace: exercises the traced stdio calls. */
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *path = "fileio_demo.txt";
    char line[64];
    char buf[16];

    FILE *out = fopen(path, "w");
    if (!out) { perror("fopen"); return 1; }
    for (int i = 0; i < 3; ++i) {
        snprintf(line, sizeof line, "record %d\n", i);
        fwrite(line, 1, strlen(line), out);
    }
    fclose(out);

    FILE *in = fopen(path, "r");
    if (!in) { perror("fopen"); return 1; }
    while (fgets(line, sizeof line, in))
        printf("read: %s", line);
    fseek(in, 0, SEEK_SET);
    size_t n = fread(buf, 1, sizeof buf - 1, in);
    buf[n] = '\0';
    printf("fread %zu bytes\n", n);
    fclose(in);
    remove(path);
    return 0;
}
