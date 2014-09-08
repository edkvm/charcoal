#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h> 
#include "../include/charcoal_msg.h"

char * client_list_files_in_dir(char* dir_name){
    
    // A pointer to a directory
    DIR *d;
    
    // Represents an entry
    struct dirent *dir;
    
    // Open the dir
    d = opendir(dir_name);
    
    //
    char *buffer = (char *)malloc(sizeof(char) * 1024);
    int buffer_mul = 1;
    if(d){
        // Read dir content
        while((dir = readdir(d)) != NULL){
            // Do not record sub directories
            if(dir->d_type != DT_DIR){
                if((strlen(buffer) + 128) > 1024){
                    buffer_mul++;
                    realloc(buffer, sizeof(char) * buffer_mul * 1024);
                }
                strcat(buffer, dir->d_name);
                strcat(buffer, "\n");
            }
        }
        
        // Close dir after reading all file names
        closedir(d);
    } else {
    	perror("opendir");
    }
    
    return buffer;
}

int main(void){
	
	message_t *msg = message_new_text("ABCDEFGHIJK");

	char *buffer = message_encode(msg);	

	printf("Result: %s\n", buffer);

	message_t *r_msg = message_decode(buffer);	

	if(message_get_type(r_msg) == CHARC_MSG_TEXT){
		char *text = message_get_body(r_msg);
		printf("decoded echo message: %s\n", text);
	} else {
		printf("error decoding message %s\n",buffer);
	}

	printf("dir:%s\n",client_list_files_in_dir("/Users/admin/Desktop/charcoal/charcoal"));
}