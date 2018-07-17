#include <string.h>

#include "utils.h"
#include "network.h"

int get_addrfamily(char *addr) {
    int family = 0;
    struct addrinfo hint, *res = NULL;
    memset(&hint, 0, sizeof(hint));

    hint.ai_family = PF_UNSPEC;
    hint.ai_flags  = AI_NUMERICHOST;

    int ret = getaddrinfo(addr, NULL, &hint, &res);
    if (ret) {
        puts("Invalid address");
        puts(gai_strerror(ret));
        return 1;
    }
    family = res->ai_family;
    freeaddrinfo(res);
    return family;
}

void * xmalloc(size_t size) {
	void * result = malloc(size);
	if (result == NULL)
		fatal("[-] malloc error");

	return result;
}

void * xmalloc_0(size_t size) {
    void * result = xmalloc(size);
	memset(result, 0, size);
	return result;
}

bool is_negative_int(char *str) {
    char *ptr;
    long long ret = strtoll(str, &ptr, 10);
    
    return ret < 0;
}

bool is_file(char *str) {
    /* TODO: implement this function */
    return false;
}

int read_file(const char *filename, char **content, size_t *content_size) {
    if (filename == NULL)     return null_filename;
    if (*content == NULL)     return null_content;
    if (content_size == NULL) return null_content_size;
    
    char ch        = '\x00';
    char *rcontent = *content;
    size_t index   = 0;
    FILE *fp       = fopen(filename, "r");

    if (fp == NULL) return open_file_error;

    while ((ch = fgetc(fp)) != EOF) {
        if (index < *content_size) rcontent[index++] = ch;
        else {
            char *buf = realloc(rcontent, (*content_size * 2) * sizeof(char));
            if (buf == NULL) {
                free(rcontent);
                return realloc_failed;
            }
            rcontent = buf;
            *content_size = (*content_size) * 2;
            rcontent[index++] = ch;
        }
    }
    rcontent[index] = '\x00';
    *content = rcontent;
    
    fclose(fp);
    return index-1;
}

void log_info(char *msg) {
    fprintf(stderr, "%s\n", msg);
}

void fatal(char *errormsg) {
    fprintf(stderr, "%s\n", errormsg);
    exit(1);
}