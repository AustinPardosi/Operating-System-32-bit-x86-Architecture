// File : user-shell.c
// Contains the implementation of functions needed to run shell program

#include "user-shell.h"
#include "cd.h"
#include "ls.h"
#include "mkdir.h"
#include "cat.h"
#include "cp.h"
#include "rm.h"
#include "mv.h"
#include "whereis.h"

uint32_t current_directory = ROOT_CLUSTER_NUMBER;
struct FAT32DirectoryTable dir_table;

/* ==================================================== SYSCALL INTERRUPT ==================================================== */

// Interrupt to main
void interrupt(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

/* ========================================================= PARSER ========================================================= */
int inputparse (char *args_val, int args_info[128][2]) {
    // Declare the vars
    int nums = 0;

    // Process to count the args, initialize 0
    int i = 0;
    int j = 0;
    int k = 0;

    bool endWord = TRUE;
    bool startWord = TRUE;
    int countchar = 0;

    // Iterate all over the chars
    // Ignore blanks at first
    while (args_val[i] == ' ' && args_val[i] != 0x0A) {
        i++;
    }

    // While belum eof
    while (args_val[i] != 0x0A) {
        // Ignore blanks
        while (args_val[i] == ' ' && args_val[i] != 0x0A) {
            if (!endWord) {
                k = 0;
                j++;
                endWord = TRUE;
            }
            startWord = TRUE;
            i++;
        }

        // Return the number of args
        if (args_val[i] == 0x0A) {
            return nums;
        }

        // Out then it is not the end of the word
        endWord = FALSE;

        // Process other chars
        if (startWord) {
            nums++;
            countchar = 0;
            args_info[j][k] = i;
            startWord = FALSE;
            k++;
        }

        countchar++;
        args_info[j][k] = countchar;
        i++; // Next char
    }

    return nums;
}

/* ========================================================= PRINTER ========================================================= */

// Wrapper for the base interrupt
void put(char* buf, uint8_t color) {
    interrupt(5, (uint32_t) buf, strlen(buf), color);
}

// Wrapper for the base interrupt, have additional argument n
void putn(char* buf, uint8_t color, int n) {
    interrupt(5, (uint32_t) buf, n, color);
}

// Print Current Working Directory
void printCWD(char* path_str, uint32_t current_dir) {
    // Intantiate vars and length vars
    int pathlen = 0;
    int nodecount = 0;
    char nodeIndex [10][64];

    // Biasakan untuk clear dulu
    clear(path_str, 128);
    for (int i = 0; i < 10; i++) {
        clear(nodeIndex[i], 64);
    }

    if (current_dir == ROOT_CLUSTER_NUMBER) {
        path_str[pathlen++] = '/';
        put (path_str, BIOS_LIGHT_BLUE);
        return;
    }
    
    // Loop sampe parentnya ROOT
    uint32_t parent = current_dir;
    path_str[pathlen++] = '/';
    while (parent != ROOT_CLUSTER_NUMBER) {
        // Isi dir_table dengan isi dari cluster sekarang
        updateDirectoryTable(parent);

        // Ambil parentnya
        parent = (uint32_t) ((dir_table.table[0].cluster_high << 16) | dir_table.table[0].cluster_low);
        
        // Masukin namanya ke list
        memcpy(nodeIndex[nodecount], dir_table.table[0].name, strlen(dir_table.table[0].name));
        nodecount++;
    }

    // Iterate back to get the full pathstr
    for (int i = nodecount - 1; i >= 0; i--) {
        for (size_t j = 0; j < strlen(nodeIndex[i]); j++) {
            path_str[pathlen++] = nodeIndex[i][j];
        } 
        
        if (i > 0) {
            path_str[pathlen++] = '/';
        }
    }

    put (path_str, BIOS_LIGHT_BLUE);
}

/* ======================================================== PATHING ======================================================== */

// Check if the path argument is an absolute path or not
bool isPathAbsolute(char* args_val, int (*args_info)[2], int args_pos) {
    return (memcmp(args_val + (*(args_info + args_pos))[0], "/", 1) == 0);
}

// Update the dir_table according to the cluster number
void updateDirectoryTable(uint32_t cluster_number) {
    interrupt(6, (uint32_t) &dir_table, cluster_number, 0x0);
}

// Find the name of directory in the dir_table and return its cluster number
int findEntryName(char* name) {
    int result = -1;

    int i = 1;
    bool found = FALSE;
    while (i < 64 && !found) {
        if (memcmp(dir_table.table[i].name, name, 8) == 0 && 
            dir_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
            result = i;
            found = TRUE;
        }
        else {
            i++;
        }
    }

    return result;
}

void initScreen() {
    put("\n\n\n\n\n", BIOS_LIGHT_GREEN);
    put("                          _____       _  __     ____   _____    ____   _____ \n", BIOS_LIGHT_GREEN);
    put("       /\\                / ____|     | |/ /    / __ \\ / ____|  / __ \\ / ____|\n", BIOS_LIGHT_GREEN);
    put("      /  \\   _ __   __ _| |  __  __ _| ' / ___| |  | | (___   | |  | | (___  \n", BIOS_LIGHT_GREEN);
    put("     / /\\ \\ | '_ \\ / _` | | |_ |/ _` |  < / _ \\ |  | |\\___ \\  | |  | |\\___ \\ \n", BIOS_LIGHT_GREEN);
    put("    / ____ \\| |_) | (_| | |__| | (_| | . \\  __/ |__| |____) | | |__| |____) |\n", BIOS_LIGHT_GREEN);
    put("   /_/    \\_\\ .__/ \\__,_|\\_____|\\__,_|_|\\_\\___|\\____/|_____/   \\____/|_____/ \n", BIOS_LIGHT_GREEN);
    put("            | |                                                              \n", BIOS_LIGHT_GREEN);
    put("            |_|                                                              \n", BIOS_LIGHT_GREEN);
    put("                          ApaGaKeOS - version 1.0.0\n", BIOS_LIGHT_GREEN);
    put("       github.com/AustinPardosi/Operating-System-32-bit-x86-Architecture\n\n", BIOS_LIGHT_GREEN);
    put("                                  WELCOME!\n", BIOS_LIGHT_GREEN);
    put("                      Press enter to get started\n\n", BIOS_LIGHT_GREEN);
}
/* ======================================================= MAIN FUNCTION ======================================================= */

// the main function where shell run
int main(void) {
    // The buffers
    char args_val[2048];
    int args_info[128][2];
    char path_str[2048];

    initScreen();
    interrupt (4, (uint32_t) args_val, 2048, 0x0);
    interrupt(7, 0, 0, 0);
    
    while (TRUE) {
        // Always start by clearing the buffer
        clear(args_val, 2048);
        for (int i = 0; i < 128; i++) {
            clear(args_info[i], 2);
        }
        clear(path_str, 2048);

        // Initialize
        put("ApaGaKeOS@OS-IF2230", BIOS_LIGHT_GREEN);
        put(":", BIOS_GREY);
        printCWD(path_str, current_directory);
        put("$ ", BIOS_GREY);
        
        // Asking for inputs
        interrupt (4, (uint32_t) args_val, 2048, 0x0);

        // Get the numbers of input args
        int args_count = inputparse (args_val, args_info);
        
        // processing the command
        if (args_count != 0) {
            if ((memcmp(args_val + *(args_info)[0], "cd", 2) == 0) && ((*(args_info))[1] == 2)) {
                cd(args_val, args_info, args_count);
            }
            else if ((memcmp(args_val + *(args_info)[0], "ls", 2) == 0) && ((*(args_info))[1] == 2)) {
                ls(args_val, args_info, args_count);
            }
            else if ((memcmp(args_val + *(args_info)[0], "mkdir", 5) == 0)&& ((*(args_info))[1] == 5)) {
                mkdir(args_val, args_info, args_count);
            }
            else if ((memcmp(args_val + *(args_info)[0], "cat", 3) == 0)&& ((*(args_info))[1] == 3)) {
                cat(args_val, args_info, args_count);
            }
            else if ((memcmp(args_val + *(args_info)[0], "cp", 2) == 0)&& ((*(args_info))[1] == 2)) {
                cp(args_val, args_info, args_count);
            }
            else if ((memcmp(args_val + *(args_info)[0], "rm", 2) == 0)&& ((*(args_info))[1] == 2)) {
                rm(args_val, args_info, args_count);
            }
            else if ((memcmp(args_val + *(args_info)[0], "mv", 2) == 0)&& ((*(args_info))[1] == 2)) {
                mv(args_val, args_info, args_count);
            }
            else if ((memcmp(args_val + *(args_info)[0], "whereis", 7) == 0)&& ((*(args_info))[1] == 7)) {
                whereis(args_val, args_info, args_count);
            }
            else if ((memcmp(args_val + *(args_info)[0], "clear", 5) == 0)&& ((*(args_info))[1] == 5)) {
                if (args_count > 1) {
                    put("clear: too many arguments\n", BIOS_RED);
                } else {
                    interrupt(7, 0, 0, 0);
                }
            }
            else {
                for (char i = 0; i < (*(args_info))[1]; i++) {
                    putn(args_val + (*(args_info))[0] + i, BIOS_RED, 1);
                }
                put(": command not found\n", BIOS_RED);
            }
        }
    }

    return 0;
}