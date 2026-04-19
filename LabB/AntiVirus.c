#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct virus {
	unsigned short SigSize;
	unsigned char* VirusName;
	unsigned char* Sig;
} virus;

typedef struct link link;

struct link {
	link *nextVirus;
	virus *vir;
};

static int g_is_big_endian = 0;

static void freeVirus(virus* v) {
	if (v == NULL) {
		return;
	}
	free(v->VirusName);
	free(v->Sig);
	free(v);
}

virus* readVirus(FILE* input) {
	unsigned char size_bytes[2];
	virus* v;
	size_t bytes_read;

	bytes_read = fread(size_bytes, 1, 2, input);
	if (bytes_read == 0) {
		return NULL;
	}
	if (bytes_read < 2) {
		return NULL;
	}

	v = (virus*)malloc(sizeof(virus));
	if (v == NULL) {
		return NULL;
	}
	memset(v, 0, sizeof(virus));

	if (g_is_big_endian) {
		v->SigSize = ((unsigned short)size_bytes[0] << 8) | size_bytes[1];
	} else {
		v->SigSize = ((unsigned short)size_bytes[1] << 8) | size_bytes[0];
	}

	v->VirusName = (unsigned char*)malloc(16);
	if (v->VirusName == NULL) {
		freeVirus(v);
		return NULL;
	}

	if (fread(v->VirusName, 1, 16, input) < 16) {
		freeVirus(v);
		return NULL;
	}

	if (v->SigSize > 0) {
		v->Sig = (unsigned char*)malloc(v->SigSize);
		if (v->Sig == NULL) {
			freeVirus(v);
			return NULL;
		}

		if (fread(v->Sig, 1, v->SigSize, input) < v->SigSize) {
			freeVirus(v);
			return NULL;
		}
	} else {
		v->Sig = NULL;
	}

	return v;
}

void printVirus(virus* v, FILE* output) {
	int i;
	unsigned char name[17];

	memcpy(name, v->VirusName, 16);
	name[16] = '\0';

	fprintf(output, "Virus name: %s\n", name);
	fprintf(output, "Virus size: %u\n", v->SigSize);
	fprintf(output, "signature:\n");

	for (i = 0; i < v->SigSize; i++) {
		fprintf(output, "%02X ", v->Sig[i]);
	}
	fprintf(output, "\n\n");
}

void list_print(link *virus_list, FILE* output) {
	link *curr = virus_list;

	while (curr != NULL) {
		printVirus(curr->vir, output);
		curr = curr->nextVirus;
	}
}

link* list_append(link* virus_list, virus* data) {
	link *new_link;
	link *curr;

	new_link = (link*)malloc(sizeof(link));
	if (new_link == NULL) {
		return virus_list;
	}

	new_link->vir = data;
	new_link->nextVirus = NULL;

	if (virus_list == NULL) {
		return new_link;
	}

	curr = virus_list;
	while (curr->nextVirus != NULL) {
		curr = curr->nextVirus;
	}
	curr->nextVirus = new_link;

	return virus_list;
}

void list_free(link *virus_list) {
	link *curr = virus_list;
	link *next;

	while (curr != NULL) {
		next = curr->nextVirus;
		freeVirus(curr->vir);
		free(curr);
		curr = next;
	}
}

#define BUFFER_SIZE 10240

void detect_virus(char *buffer, unsigned int size, link *virus_list) {
	link *curr = virus_list;
	unsigned int i;

	while (curr != NULL) {
		unsigned short sig_size = curr->vir->SigSize;
		unsigned char *sig = curr->vir->Sig;
		unsigned char name[17];
		memcpy(name, curr->vir->VirusName, 16);
		name[16] = '\0';

		for (i = 0; i + sig_size <= size; i++) {
			if (memcmp(buffer + i, sig, sig_size) == 0) {
				printf("Starting byte: %u\n", i);
				printf("Virus name: %s\n", name);
				printf("Signature size: %u\n\n", sig_size);
			}
		}
		curr = curr->nextVirus;
	}
}

static link* load_signatures(const char *filename) {
	FILE* signatures;
	unsigned char magic[4];
	virus* v;
	link* virus_list = NULL;

	signatures = fopen(filename, "rb");
	if (signatures == NULL) {
		fprintf(stderr, "Error: cannot open signatures file\n");
		return NULL;
	}

	if (fread(magic, 1, 4, signatures) < 4) {
		fprintf(stderr, "Error: cannot read magic number\n");
		fclose(signatures);
		return NULL;
	}

	if (memcmp(magic, "VIRL", 4) == 0) {
		g_is_big_endian = 0;
	} else if (memcmp(magic, "VIRB", 4) == 0) {
		g_is_big_endian = 1;
	} else {
		fprintf(stderr, "Error: invalid signatures file format\n");
		fclose(signatures);
		return NULL;
	}

	while ((v = readVirus(signatures)) != NULL) {
		virus_list = list_append(virus_list, v);
	}

	fclose(signatures);
	return virus_list;
}

int main(int argc, char** argv) {
	link* virus_list = NULL;
	char line[256];
	char selected_file[256] = "";
	char filename[256];
	char option;

	(void)argc;
	(void)argv;

	while (1) {
		printf("\n<L>oad signatures\n");
		printf("<P>rint signatures\n");
		printf("<S>elect file to inspect\n");
		printf("<D>etect viruses\n");
		printf("<F>ix file\n");
		printf("<Q>uit\n");
		printf("Option: ");

		if (fgets(line, sizeof(line), stdin) == NULL) {
			break;
		}

		if (sscanf(line, " %c", &option) != 1) {
			continue;
		}

		switch (option) {
			case 'L':
			case 'l':
				printf("Enter signatures file name: ");
				if (fgets(line, sizeof(line), stdin) == NULL) {
					break;
				}
				if (sscanf(line, "%255s", filename) == 1) {
					link* new_list = load_signatures(filename);
					if (new_list != NULL) {
						list_free(virus_list);
						virus_list = new_list;
					}
				}
				break;

			case 'P':
			case 'p':
				list_print(virus_list, stdout);
				break;

			case 'S':
			case 's':
				printf("Enter file name to inspect: ");
				if (fgets(line, sizeof(line), stdin) == NULL) {
					break;
				}
				if (sscanf(line, "%255s", selected_file) == 1) {
					printf("Selected file: %s\n", selected_file);
				}
				break;

			case 'D':
			case 'd':
				if (selected_file[0] == '\0') {
					fprintf(stderr, "Error: no file selected\n");
				} else {
					FILE *f = fopen(selected_file, "rb");
					if (f == NULL) {
						fprintf(stderr, "Error: cannot open file %s\n", selected_file);
					} else {
						char buffer[BUFFER_SIZE];
						unsigned int bytes_read = fread(buffer, 1, BUFFER_SIZE, f);
						fclose(f);
						detect_virus(buffer, bytes_read, virus_list);
					}
				}
				break;

			case 'F':
			case 'f':
				printf("Not implemented\n");
				break;

			case 'Q':
			case 'q':
				list_free(virus_list);
				return 0;

			default:
				printf("Unknown option\n");
				break;
		}
	}

	list_free(virus_list);
	return 0;
}
