#include "esa.h"

/**********************************************************************/

int open_input(t_TEXT * Text, const char * c_dir, const char * c_file, IntText n)
{ //ok
	char c_aux[500];
	sprintf(c_aux, "%s%s\0", c_dir, c_file);

	printf("input = %s\n", c_aux);

	#if !INPUT_CAT
	FILE* f_aux = fopen(c_aux, "r");
	if(!f_aux) 
		perror("open_input");
	fseek(f_aux, 0, SEEK_SET);
	#endif

	IntText i = 0;
	for(; i < n; i++) { //file_name \t size
		#if !INPUT_CAT

		Text[i].c_file = (char*)malloc(200*sizeof(char));

		fscanf(f_aux, "%s\n", c_aux);
		sprintf(Text[i].c_file, "%s%s\0", c_dir, c_aux);

			#if DEBUG
		printf("T_%d = %s\n", i, Text[i].c_file);
			#endif
		#endif
	}

	#if !INPUT_CAT
	fclose(f_aux);
	#endif

	printf("\n");

	return 0;
}

/**********************************************************************/

int open_fasta(t_TEXT * Text, char * c_file, size_t begin)
{ //ok
#if DEBUG
	printf("opening: %s\n", c_file);
#endif
	Text->f_in = fopen(c_file, "r");
	if(!Text->f_in) perror("open_fasta");
#if INPUT_CAT
	Text->c_buffer = (int8*)malloc((MAX_FASTA)*sizeof(int8));            //10MB
#else
	fseek(Text->f_in, 0, SEEK_END);
	Text->length = ftell(Text->f_in);
	Text->c_buffer = (int8*)malloc((Text->length) * sizeof(int8));
	#endif
	fseek(Text->f_in, begin, SEEK_SET);
	return 0;
}

int load_fasta(t_TEXT * Text) 
{
	char c_aux[201];
	fgets(c_aux, 200, Text->f_in); //ignores first line
	Text->length = 0;
	size_t j = 0;
	size_t end = 0;
	//read the sequence
	while(fgets(c_aux, 200, Text->f_in)) {
		#if INPUT_CAT
		if(c_aux[0] == '>') {
			Text->length = j;
			fclose(Text->f_in);
			free(Text->c_file);

			return end;
		}
		#endif

		size_t i = 0;
		for(; i < strlen(c_aux) - 1; i++) {
			#if DNA
			switch(c_aux[i]) {
				case 65: Text->c_buffer[j++] = 1; break; //A
				case 67: Text->c_buffer[j++] = 2; break; //C
				case 71: Text->c_buffer[j++] = 3; break; //G
				case 84: Text->c_buffer[j++] = 4; break; //T
				default: break; //N
			}
			#elif PROTEIN

			//Text->c_buffer[j++] = c_aux[i];
			switch(c_aux[i]) {
				case 65: Text->c_buffer[j++] = 1; break; //A
				//B
				case 67: Text->c_buffer[j++] = 2; break; //C
				case 68: Text->c_buffer[j++] = 3; break; //D

				case 69: Text->c_buffer[j++] = 4; break; //E
				case 70: Text->c_buffer[j++] = 5; break; //F
				case 71: Text->c_buffer[j++] = 6; break; //G

				case 72: Text->c_buffer[j++] = 7; break; //H
				case 73: Text->c_buffer[j++] = 8; break; //I
				//J
				case 75: Text->c_buffer[j++] = 9; break; //K

				case 76: Text->c_buffer[j++] = 10; break; //L
				case 77: Text->c_buffer[j++] = 11; break; //M
				case 78: Text->c_buffer[j++] = 12; break; //N
				//O
				case 80: Text->c_buffer[j++] = 13; break; //P
				case 81: Text->c_buffer[j++] = 14; break; //Q
				case 82: Text->c_buffer[j++] = 15; break; //R

				case 83: Text->c_buffer[j++] = 16; break; //S
				case 84: Text->c_buffer[j++] = 17; break; //T
				//U
				case 86: Text->c_buffer[j++] = 18; break; //V

				case 87: Text->c_buffer[j++] = 19; break; //W
				case 88: Text->c_buffer[j++] = 20; break; //X
				case 89: Text->c_buffer[j++] = 21; break; //Y
				//Z

				default: break; //N
			}

			#endif
		}
		#if INPUT_CAT
		end = ftell(Text->f_in);
		#endif
	}

	Text->length = j;
	fclose(Text->f_in);

	return 0;
}

/**********************************************************************/

#if INPUT_CAT

int write_file(t_TEXT * Text, char * c_dir, char* c_file, int n) { //ok
	char c_aux[200] = "";
	sprintf(c_aux, "%s%s.seq\0", c_dir, c_file);

	FILE * f_aux = fopen(c_aux, "w");
	if(!f_aux) perror("write_file");

	int i = 0;
	for(; i < n; i++) {
		fprintf(f_aux, "%zu %zu\n", Text[i].begin, Text[i].length);
	}

	fclose(f_aux);

	return 0;
}

int read_file(t_TEXT * Text, char * c_dir, char* c_file, IntText n) { //ok
	char c_aux[200];
	sprintf(c_aux, "%s%s.seq\0", c_dir, c_file);

	FILE * f_aux = fopen(c_aux, "r");
	if(!f_aux) perror("read_file");

	IntText i = 0;
	for(; i < n; i++) {
		fscanf(f_aux, "%zu %zu\n", &Text[i].begin, &Text[i].length);
	}

	fclose(f_aux);

	return 0;
}

#endif
/**********************************************************************/

int open_sequence(t_TEXT * Text, const char * c_dir, const char * c_file)
{
	char c_aux[200];
#if INPUT_CAT
	sprintf(c_aux, "%s%s.bin\0", c_dir, c_file);
#else
	sprintf(c_aux, "%s.bin\0", Text->c_file);
#endif
	Text->f_in = fopen(c_aux, "rb");
	if(!Text->f_in) 
		perror("open_sequence");
	fseek(Text->f_in, 0, SEEK_END);
#if !INPUT_CAT
	Text->length = ftell(Text->f_in);
#endif
	Text->length--;
	return 0;
}

void seek_sequence(FILE * File, size_t pos)
{
	fseek(File, pos, SEEK_SET);
}

int8 read_sequence(t_TEXT * Text)
{
	int8 aux = -1;
	fread(&aux, sizeof(int8), 1, Text->f_in);
	return aux;
}

void load_sequence(t_TEXT * Text) 
{ //load .bin file
	Text->c_buffer = (int8*)malloc((Text->length + 1) * sizeof(int8));
	if(!Text->c_buffer) perror("load_sequence");

	fread(Text->c_buffer, sizeof(int8), Text->length, Text->f_in);

	size_t i = Text->length;
	for(; i < Text->length+ 1; i++)
		Text->c_buffer[i] = 0;

	fclose(Text->f_in);
}

int write_sequence(t_TEXT * Text, const char * c_dir, const char * c_file) 
{
	char c_aux[200] = "";
#if INPUT_CAT
	sprintf(c_aux, "%s%s.bin\0", c_dir, c_file);
#else
	sprintf(c_aux, "%s.bin\0", Text->c_file);
#endif
	FILE * f_out;
#if INPUT_CAT
	f_out = fopen(c_aux, "ab");
#else
	f_out = fopen(c_aux, "wb");
#endif
	if(!f_out) 
		perror("write_sequence");
	Text->c_buffer[Text->length] = 0; //adiciona 0 no final de cada cadeia
	fwrite(Text->c_buffer, sizeof(int8), Text->length+1, f_out);
	int8 c_last = SIGMA+1;
	fwrite(&c_last, sizeof(int8), 1, f_out);
	fclose(f_out);
	return 0;
}

/**********************************************************************/

void remove_file(const char * c_file)
{
	printf("removing: %s\n", c_file);
	char c_aux[500];
	strcpy(c_aux, "rm ");
	//strcat (c_aux, c_dir);
	strcat(c_aux, c_file);
	system(c_aux); //remove .bin
}

/**********************************************************************/

inline void mkdir(char* c_file){
	printf("mkdir: %s\n", c_file);

	char c_aux[500];

	strcpy(c_aux, "mkdir ");
	//strcat (c_aux, c_dir);
	strcat(c_aux, c_file);

	system(c_aux); //remove .bin
}

