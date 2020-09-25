#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
int read_int(FILE* fptr) {
    int i32;
    int r = fscanf(fptr, "%d", &i32);
    return i32;
}
*/

int main(int argc, char** argv) {

    if(argc != 3) {
        fprintf(stderr, "usage:\n    ./compress <input .pil file> <output .bin file>\n\n");
        exit(1);
    }

    FILE* read_ptr = fopen(argv[1], "r");

    if(read_ptr == NULL) {
        fprintf(stderr, "unable to open input file '%s' for image compression\n", argv[1]);
        exit(1);
    }

    FILE* write_ptr = fopen(argv[2], "wb");

    if(write_ptr == NULL) {
        fprintf(stderr, "unable to open output file '%s' for image compression\n", argv[2]);
        exit(1);
    }

    int w, h;

    int tmp;

    tmp = fscanf(read_ptr, "w %d\n", &w);
    tmp = fscanf(read_ptr, "h %d\n", &h);
    //printf("w = %d\nh = %d\n\n", w, h);

    int x, y;
    int r, g, b; // rg triplets

    int intbuf[2];
    intbuf[0] = w;
    intbuf[1] = h;

    fwrite(intbuf, 8, 1, write_ptr);

    for(y = 0; y < h; y++) {
        for(x = 0; x < w; x++) {

            tmp = fscanf(read_ptr, "%d %d %d", &r, &g, &b);

            if(tmp != 3) {
                fprintf(stderr, "invalid format in file '%s'\n", argv[1]);
                fclose(read_ptr);
                fclose(write_ptr);
            }
            else {

                // write the binary data
                unsigned char buf[3];

                buf[0] = r & 0xFF;
                buf[1] = g & 0xFF;
                buf[2] = b & 0xFF;

                fwrite(buf, 3, 1, write_ptr);
            }

        }
    }

    fclose(read_ptr);
    fclose(write_ptr);

    return 0;
}


