// char *filename

    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    if (size == 0) {
        return 0;
    }
    char *io_buf = malloc((size+1)*sizeof(char));
    fread(io_buf, size, 1, f);
    fclose(f);
    *(io_buf+size) = '\0';

// io_buf, size
