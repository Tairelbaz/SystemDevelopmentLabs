#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct virus {
	unsigned short SigSize;
	unsigned char* VirusName;
	unsigned char* Sig;
} virus;

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
	fprintf(output, "Signature:\n");

	for (i = 0; i < v->SigSize; i++) {
		fprintf(output, "%02X ", v->Sig[i]);
	}
	fprintf(output, "\n\n");
}

int main(int argc, char** argv) {
	FILE* signatures;
	unsigned char magic[4];
	virus* v;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <signatures_file>\n", argv[0]);
		return 1;
	}

	signatures = fopen(argv[1], "rb");
	if (signatures == NULL) {
		fprintf(stderr, "Error: cannot open signatures file\n");
		return 1;
	}

	if (fread(magic, 1, 4, signatures) < 4) {
		fprintf(stderr, "Error: cannot read magic number\n");
		fclose(signatures);
		return 1;
	}

	if (memcmp(magic, "VIRL", 4) == 0) {
		g_is_big_endian = 0;
	} else if (memcmp(magic, "VIRB", 4) == 0) {
		g_is_big_endian = 1;
	} else {
		fprintf(stderr, "Error: invalid signatures file format\n");
		fclose(signatures);
		return 1;
	}

	while ((v = readVirus(signatures)) != NULL) {
		printVirus(v, stdout);
		freeVirus(v);
	}

	fclose(signatures);
	return 0;
}
