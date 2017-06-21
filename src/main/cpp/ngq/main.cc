#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define FIMPORT __declspec(dllimport)
#else
#define FIMPORT __attribute__((visibility("default")))
#endif

extern "C" {

FIMPORT void
kenlm_misc();

FIMPORT void *
kenlm_init(size_t size, void *data, size_t ex_msg_size, char *ex_msg);

FIMPORT void
kenlm_clean(void *pHandle);

FIMPORT float
kenlm_query(void *pHandle, const char *pTag);

}

int
main(void) {
    char file_name [] = "tag.lm.bin"; // "tag.lm.bin" "build.gradle"
    FILE *f = fopen(file_name, "rb");

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);

    void *data = malloc((size + 1)*sizeof(char));
    fread(data, size, 1, f);
    fclose(f);

    size_t ex_msg_size = 2048;
    char *ex_msg = reinterpret_cast<char *>(malloc((ex_msg_size)*sizeof(char)));

    void *pHandle = kenlm_init(size, data, ex_msg_size, ex_msg);
    free(data);

    if (!pHandle) {
        std::cout << "kenlm_init exception found." << std::endl;
        std::cout << ex_msg << std::endl;
    } else {
        {
            const char *tag = "V";
            float total = kenlm_query(pHandle, tag);
            std::cout << "tags: " << tag << " total: " << total << std::endl;
        }
        {
            const char *tag = "P";
            float total = kenlm_query(pHandle, tag);
            std::cout << "tags: " << tag << " total: " << total << std::endl;
        }
        {
            const char *tag = "V P";
            float total = kenlm_query(pHandle, tag);
            std::cout << "tags: " << tag << " total: " << total << std::endl;
        }
        {
            const char *tag = "P V";
            float total = kenlm_query(pHandle, tag);
            std::cout << "tags: " << tag << " total: " << total << std::endl;
        }
        kenlm_clean(pHandle);
    }

    free(ex_msg);

    kenlm_misc();

    return 0;
}
