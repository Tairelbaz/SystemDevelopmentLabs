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
#include <stdint.h>

#define FILE_NAME_LEN 128
#define INPUT_LEN     256
#define MEM_BUF_SIZE  10000

/* ---- Global program state (names fixed by the assignment) ---- */
char debug_mode = 0;                  /* 0 = off, 1 = on */
char file_name[FILE_NAME_LEN] = "";   /* current target file */
int  unit_size = 1;                   /* 1, 2 or 4 bytes; default 1 */
unsigned char mem_buf[MEM_BUF_SIZE];  /* loaded file contents */
size_t mem_count = 0;                 /* bytes currently in mem_buf */
char display_mode = 0;                /* 0 = hexadecimal (default), 1 = decimal */

/* Unit-sized printf formats, indexed by unit_size-1 (sizes 1,2,4 -> 0,1,3). */
static char* hex_formats[] = {"%#hhx\n", "%#hx\n", "No such unit", "%#x\n"};
static char* dec_formats[] = {"%#hhd\n", "%#hd\n", "No such unit", "%#d\n"};

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

/* Task 1a: read length*unit_size bytes from file_name (at hex offset location)
 * into mem_buf. The file is opened and closed within this function. */
void load_into_memory() {
  char input[INPUT_LEN];
  unsigned int location;
  int length;
  FILE *f;
  size_t r;

  if (file_name[0] == '\0') {
    fprintf(stderr, "Error: file name not set\n");
    return;
  }
  f = fopen(file_name, "rb");
  if (f == NULL) {
    fprintf(stderr, "Error: cannot open '%s'\n", file_name);
    return;
  }
  printf("Please enter <location> <length>\n");
  if (read_line(input, sizeof(input)) == NULL) {
    fclose(f);
    return;
  }
  if (sscanf(input, "%x %d", &location, &length) != 2) {
    fprintf(stderr, "Error: invalid input\n");
    fclose(f);
    return;
  }
  if (debug_mode)
    fprintf(stderr, "Debug: file_name='%s', location=%#x, length=%d\n",
            file_name, location, length);
  if (length < 0 || (size_t)length * unit_size > MEM_BUF_SIZE) {
    fprintf(stderr, "Error: length exceeds buffer capacity\n");
    fclose(f);
    return;
  }
  fseek(f, location, SEEK_SET);
  r = fread(mem_buf, unit_size, length, f);
  mem_count = r * unit_size;
  fclose(f);
  printf("Loaded %zu units into memory\n", r);
}

/* Task 1b: switch between hexadecimal (default) and decimal display. */
void toggle_display_mode() {
  if (display_mode) {
    display_mode = 0;
    printf("Decimal display flag now off, hexadecimal representation\n");
  } else {
    display_mode = 1;
    printf("Decimal display flag now on, decimal representation\n");
  }
}

/* Task 1c: display u units of unit_size starting at addr (0 = start of mem_buf,
 * otherwise a virtual memory address), per the current display mode. */
void memory_display() {
  char input[INPUT_LEN];
  unsigned int addr;
  int u;
  unsigned char *p;
  char **formats;

  printf("Enter address and length\n");
  if (read_line(input, sizeof(input)) == NULL)
    return;
  if (sscanf(input, "%x %d", &addr, &u) != 2) {
    fprintf(stderr, "Error: invalid input\n");
    return;
  }

  p = (addr == 0) ? mem_buf : (unsigned char*)(uintptr_t)addr;
  formats = display_mode ? dec_formats : hex_formats;

  printf(display_mode ? "Decimal\n=======\n" : "Hexadecimal\n===========\n");
  for (int i = 0; i < u; i++) {
    unsigned int val = 0;
    memcpy(&val, p + (size_t)i * unit_size, unit_size);
    printf(formats[unit_size - 1], val);
  }
}

/* Task 1d: write length units from memory (source 0 = mem_buf, otherwise a
 * virtual address) to file_name at hex offset target. Opened "rb+" so the file
 * is updated in place, never truncated; opened and closed within the function. */
void save_into_file() {
  char input[INPUT_LEN];
  unsigned int source, target;
  int length;
  unsigned char *p;
  FILE *f;
  long size;

  if (file_name[0] == '\0') {
    fprintf(stderr, "Error: file name not set\n");
    return;
  }
  f = fopen(file_name, "rb+");
  if (f == NULL) {
    fprintf(stderr, "Error: cannot open '%s'\n", file_name);
    return;
  }
  printf("Please enter <source-address> <target-location> <length>\n");
  if (read_line(input, sizeof(input)) == NULL) {
    fclose(f);
    return;
  }
  if (sscanf(input, "%x %x %d", &source, &target, &length) != 3) {
    fprintf(stderr, "Error: invalid input\n");
    fclose(f);
    return;
  }
  if (debug_mode)
    fprintf(stderr, "Debug: file_name='%s', source=%#x, target=%#x, length=%d\n",
            file_name, source, target, length);

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  if ((long)target > size) {
    fprintf(stderr, "Error: target location %#x exceeds file size\n", target);
    fclose(f);
    return;
  }
  p = (source == 0) ? mem_buf : (unsigned char*)(uintptr_t)source;
  fseek(f, target, SEEK_SET);
  fwrite(p, unit_size, length, f);
  fclose(f);
}

/* Task 1e: overwrite the unit at byte offset location in mem_buf with val. */
void memory_modify() {
  char input[INPUT_LEN];
  unsigned int location, val;

  printf("Please enter <location> <val>\n");
  if (read_line(input, sizeof(input)) == NULL)
    return;
  if (sscanf(input, "%x %x", &location, &val) != 2) {
    fprintf(stderr, "Error: invalid input\n");
    return;
  }
  if (debug_mode)
    fprintf(stderr, "Debug: location=%#x, val=%#x\n", location, val);
  if ((size_t)location + unit_size > MEM_BUF_SIZE) {
    fprintf(stderr, "Error: location %#x out of range\n", location);
    return;
  }
  memcpy(mem_buf + location, &val, unit_size);
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
