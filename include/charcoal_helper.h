#ifndef __CHARCOAL_HELPER_H__
#define __CHARCOAL_HELPER_H__

extern char _readline_input[2048];

char* helper_readline(char* prompt){
    fputs(prompt, stdout);
    fgets(_readline_input, 2048, stdin);
    char *cpy = malloc(strlen(_readline_input)+1);
    strcpy(cpy, _readline_input);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

#endif