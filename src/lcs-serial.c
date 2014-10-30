#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX(x, y) ( ((x) > (y)) ? (x) : (y) )

typedef struct {
	int height;
	int width;
	char* vectorHeight;
	char* vectorWidth;
	short int** matrix;
}board_t;

typedef board_t* Board;

short cost(int x);
Board parseFile(char* fileName);
void processCell(int i, int j, Board board);
void printResults(board_t* board);
void iterateBoard(board_t* board);
void cleanAll(board_t* board);

int main(int argc, char* argv[]){

	char* fileName = argv[1];
	Board board = parseFile(fileName);
	iterateBoard(board);
	printResults(board);
	cleanAll(board);
	return 0;
}

void processCell(int i, int j, Board board){
	if (i == 0 || j == 0) {
		board->matrix[i][j] = 0;
	} else if (board->vectorHeight[i] == board->vectorWidth[j]) {
		board->matrix[i][j] = board->matrix[i - 1][j - 1] + cost(i+j);
	}else{
		board->matrix[i][j] = MAX(board->matrix[i - 1][j], board->matrix[i][j - 1]);
	}
}
void iterateBoard(board_t* board){
	int heightLength = board->height + 1;
	int widthLength = board->width + 1;
	size_t i, j;
	for (i = 0; i < heightLength; ++i) {
		for (j = 0; j < widthLength; ++j) {
			processCell(i, j, board);
		}
	}
}
Board parseFile(char* fileName){
	FILE* file;
	int height;
	int width;
	int c;
	char* vectorHeight;
	char* vectorWeidth;
	short int** matrix;
	size_t n = 1;


	if (!(file = fopen(fileName,"r"))){
	    printf("Error opening file \"%s\"\n", fileName);
	    exit(2);
	}
	if(fscanf(file, "%d %d\n", &height, &width) == 0){
	    printf("Could not read height and width");
	    exit(3);
	}
	if(!(vectorHeight = (char*) malloc(sizeof(char)*(height + 1)))||
	   !(vectorWeidth = (char*) malloc(sizeof(char)*(width + 1)))){
		    printf("Error allocating row pointers for board.\n");
		    exit(4);
	}

	while((c = fgetc(file)) != '\n'){
		 vectorHeight[n++] = (char)c;
	}
	vectorHeight[0] = '/';

	n = 1;
	while((c = fgetc(file)) != '\n'){
		vectorWeidth[n++] = (char)c;
	}
	vectorWeidth[0] = '/';

	matrix = (short int**)malloc(sizeof(short int*) * (height + 1));
	for (n = 0; n < height + 1; ++n) {
		matrix[n] = (short int*)malloc(sizeof(short int) * (width + 1));
	}
	fclose(file);
	file = NULL;

	Board result = (Board)malloc(sizeof(board_t));

	result->height =  height;
	result->width = width;
	result->matrix = matrix;
	result->vectorHeight = vectorHeight;
	result->vectorWidth = vectorWeidth;

	return result;

}
void printResults(board_t* board){
	int heightLength = board->height;
	int widthLength = board->width;
	size_t i, j;
	int aux, finalSize = board->matrix[heightLength][widthLength];
	char* subsequence = (char*) malloc(sizeof(char) * finalSize);
	short int** matrix = board->matrix;

	/* just for testing 
	printf("    (/)");
	for(i=0;i<=board->width;i++){
		printf("(%c)",board->vectorWidth[i]);
	}
	printf("\n");

	for (i = 0; i <= board->height; ++i) {
		printf("i:%d (%c)",i,board->vectorHeight[i]);
		for (j = 0; j <= board->width; ++j) {
			printf("|%d|", board->matrix[i][j]);
		}
		printf("\n");
	}
	/* just for testing */

	aux = finalSize;
	while(aux > 0){
		if((matrix[heightLength-1][widthLength] != aux) &&
		   (matrix[heightLength][widthLength-1] != aux)){
			subsequence[aux -1] = board->vectorHeight[heightLength];
			heightLength--;
			widthLength--;
			aux--;
		}else if (board->vectorHeight[heightLength] == board->vectorWidth[widthLength]) {
			subsequence[aux -1] = board->vectorHeight[heightLength];
			heightLength--;
			widthLength--;
			aux--;
		}else if (matrix[heightLength][widthLength-1] == aux) {
			widthLength--;
		}else if (matrix[heightLength-1][widthLength] == aux) {
			heightLength--;
		}
	}
	printf("%d\n", finalSize);
	for (aux = 0; aux < finalSize; ++aux) {
		printf("%c", subsequence[aux]);
	}
	printf("\n");

	free(subsequence);
	matrix = NULL;


}
short cost(int x){
	int i, n_iter = 20;
	double dcost = 0;
	for(i = 0; i < n_iter; i++)
		dcost += pow(sin((double) x),2) + pow(cos((double) x),2);
	return (short) (dcost / n_iter + 0.1);
}
void cleanAll(board_t* board){
	size_t n;

	free(board->vectorHeight);
	free(board->vectorWidth);

	for(n = 0; n < board->height; n++){
		free(board->matrix[n]);
	}
	free(board->matrix);

	free(board);
}

