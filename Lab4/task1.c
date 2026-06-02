/* Lab 4 - hexeditplus
 * Authors: Ofek Hamdi, Tair Elbaz
 *
 * Menu-driven tool for inspecting and patching binary files.
 * Task 0b implements the menu framework plus Toggle Debug, Set File Name,
 * Set Unit Size and Quit. The remaining options are stubs that are filled
 * in during Task 1.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_NAME_LEN 128
#define INPUT_LEN     256
#define MEM_BUF_SIZE  10000

/* ---- Global program state (names fixed by the assignment) ---- */
char debug_mode = 0;                  /* 0 = off, 1 = on */
char file_name[FILE_NAME_LEN] = "";   /* current target file */
int  unit_size = 1;                   /* 1, 2 or 4 bytes; default 1 */
unsigned char mem_buf[MEM_BUF_SIZE];  /* loaded file contents */
size_t mem_count = 0;                 /* bytes currently in mem_buf */

/* Read one line from stdin into buf (size bytes), stripping the trailing
 * newline. Returns buf, or NULL on EOF. */
char* read_line(char *buf, int size) {
  if (fgets(buf, size, stdin) == NULL)
    return NULL;
  buf[strcspn(buf, "\n")] = '\0';
  return buf;
}

/* ---- Menu actions ---- */

void toggle_debug() {
  if (debug_mode) {
    debug_mode = 0;
    fprintf(stderr, "Debug flag now off\n");
  } else {
    debug_mode = 1;
    fprintf(stderr, "Debug flag now on\n");
  }
}

void set_file_name() {
  char input[INPUT_LEN];
  printf("Enter file name:\n");
  if (read_line(input, sizeof(input)) == NULL)
    return;
  strcpy(file_name, input);   /* file name assumed <= 100 chars */
  if (debug_mode)
    fprintf(stderr, "Debug: file name set to '%s'\n", file_name);
}

void set_unit_size() {
  char input[INPUT_LEN];
  int size;
  printf("Enter unit size (1, 2 or 4):\n");
  if (read_line(input, sizeof(input)) == NULL)
    return;
  if (sscanf(input, "%d", &size) != 1) {
    fprintf(stderr, "Invalid unit size\n");
    return;
  }
  if (size == 1 || size == 2 || size == 4) {
    unit_size = size;
    if (debug_mode)
      fprintf(stderr, "Debug: set size to %d\n", unit_size);
  } else {
    fprintf(stderr, "Invalid unit size: %d (must be 1, 2 or 4)\n", size);
  }
}

/* ---- Stubs to be implemented during Task 1 ---- */

void load_into_memory() {
  printf("Not implemented yet\n");
}

void toggle_display_mode() {
  printf("Not implemented yet\n");
}

void memory_display() {
  printf("Not implemented yet\n");
}

void save_into_file() {
  printf("Not implemented yet\n");
}

void memory_modify() {
  printf("Not implemented yet\n");
}

void quit() {
  if (debug_mode)
    fprintf(stderr, "quitting\n");
  exit(0);
}

/* ---- Menu table (same scheme as lab 1, NULL-terminated) ---- */

struct fun_desc {
  char *name;
  char index;
  void (*fun)(void);
};

struct fun_desc menu[] = {
  { "Toggle <D>ebug Mode",   'D', toggle_debug        },
  { "Set <F>ile Name",       'F', set_file_name       },
  { "Set <U>nit Size",       'U', set_unit_size       },
  { "<L>oad Into Memory",    'L', load_into_memory    },
  { "<T>oggle Display Mode", 'T', toggle_display_mode },
  { "<M>emory Display",      'M', memory_display      },
  { "<S>ave Into File",      'S', save_into_file      },
  { "Memory Modif<y>",       'y', memory_modify       },
  { "<Q>uit",                'Q', quit                },
  { NULL, 0, NULL }
};

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  char input[INPUT_LEN];

  while (1) {
    if (debug_mode)
      fprintf(stderr, "unit_size = %d, file_name = '%s', mem_count = %zu\n",
              unit_size, file_name, mem_count);

    printf("Choose action:\n");
    for (int i = 0; menu[i].name != NULL; i++)
      printf("%s\n", menu[i].name);

    if (fgets(input, sizeof(input), stdin) == NULL)
      break;

    char chosen = input[0];

    int found = -1;
    for (int i = 0; menu[i].name != NULL; i++) {
      if (menu[i].index == chosen) {
        found = i;
        break;
      }
    }

    if (found == -1) {
      printf("Not a valid option\n");
      continue;
    }

    menu[found].fun();
  }

  return 0;
}
