#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX(x, y) ( ((x) > (y)) ? (x) : (y) )

//Board is a matrix MxN, where M is height and N is width.
//The horizotal string will be in horizontalString
//Vertical string in verticalString

typedef struct {
	int height;
	int width;
	char* horizontalString;
	char* verticalString;
	int** matrix;
}board_t;

typedef board_t* Board;

short cost(int x);
Board parseFile(char* fileName);
void processCell(int i, int j, Board board);
void printResults(Board board);
void iterateBoard(Board board);
void cleanAll(Board board);

int main(int argc, char* argv[]){

	char* fileName = argv[1];
	Board board = parseFile(fileName);
	iterateBoard(board);
	printResults(board);
	cleanAll(board);
	return 0;
}

//Function intended to build the board, given a file
//Arguments: fileName, the name of the file to be read
Board parseFile(char* fileName){
	FILE* file;
	int height;
	int width;
	int c;
	char* horizontalString;
	char* verticalString;
	int** matrix;
	size_t n = 1;


	if (!(file = fopen(fileName,"r"))){
	    printf("Error opening file \"%s\"\n", fileName);
	    exit(2);
	}
	if(fscanf(file, "%d %d\n", &height, &width) == 0){
	    printf("Could not read height and width");
	    exit(3);
	}
	if(!(horizontalString = (char*) malloc(sizeof(char)*(height + 1)))||
	   !(verticalString = (char*) malloc(sizeof(char)*(width + 1)))){
		    printf("Error allocating row pointers for board.\n");
		    exit(4);
	}

	while((c = fgetc(file)) != '\n'){
		 verticalString[n++] = (char)c;
	}
	verticalString[0]='/';

	n = 1;
	while((c = fgetc(file)) != '\n'){
		horizontalString[n++] = (char)c;
	}
	horizontalString[0] = '/';


	matrix = (int**)malloc(sizeof(int*) * (height + 1));
	for (n = 0; n < height + 1; ++n) {
		matrix[n] = (int*)malloc(sizeof(int) * (width + 1));
	}
	fclose(file);
	file = NULL;

	Board result = (Board)malloc(sizeof(board_t));

	result->height =  height;
	result->width = width;
	result->matrix = matrix;
	result->horizontalString = horizontalString;
	result->verticalString = verticalString;

	return result;

}


//This function iterates through the cells of the given board
//Arguments: board, the current board.
void iterateBoard(Board board){
	int verticalStringLength = board->height + 1;
	int horizontalStringLength = board->width + 1;
	size_t i, j;
	for (i = 0; i < verticalStringLength; ++i) {
		for (j = 0; j < horizontalStringLength; ++j) {
			processCell(i, j, board);                       //for each cell, process it.
		}
	}
}


//This function applies the algorithm to the given cell on the board
//Arguments: i, the column of the cell. j, the row of the cell. board, the current board.
void processCell(int i, int j, Board board){

	if (i == 0 || j == 0) {                                                             //First it checks if either i=0 or j=0, if so, we are in the first line/column so it must be filled with 0's
		board->matrix[i][j] = 0;
	} else if (board->horizontalString[j] == board->verticalString[i]) {                //Else, if there is a match, the current cell will be given the value of the diagonally left cell + the cost function (that is always 1)
		board->matrix[i][j] = board->matrix[i - 1][j - 1] + cost(i+j);
	}else{                                                                              //Else, means no match, the cell will be given the value of the highest of either the left cell or the top cell
		board->matrix[i][j] = MAX(board->matrix[i - 1][j], board->matrix[i][j - 1]);
	}
}


//This function performs the backtrack on the matrix, while filling the Longest Common Subsequence. Also prints the supposed output
//Arguments: board, the current board
void printResults(Board board){
	int verticalStringLength = board->height;
	int horizontalStringLength = board->width;
	size_t i, j;
	int aux, finalSize = board->matrix[verticalStringLength][horizontalStringLength];           //finalsize, the last cell of the board, containing the size of the substring
	char* subsequence = (char*) malloc(sizeof(char) * finalSize);
	int** matrix = board->matrix;
	char* horizontalString = board->horizontalString;
	char* vectorWidth =board->verticalString;

	/* just for testing
	printf("(/)");
	for(i=0;i<=horizontalStringLength;i++){
		printf("(%c)",board->horizontalString[i]);
	}
	printf("\n");

	for (i = 0; i <= verticalStringLength; ++i) {
		printf("(%c)",board->verticalString[i]);
		for (j = 0; j <= horizontalStringLength; ++j) {
			printf("|%d|", board->matrix[i][j]);
		}
		printf("\n");
	}
	 just for testing */

	aux = finalSize;
	while(aux > 0){
		if((matrix[verticalStringLength-1][horizontalStringLength] != aux) &&
		   (matrix[verticalStringLength][horizontalStringLength-1] != aux)){            //if the left value, or the top value are lower: match
			subsequence[aux -1] = board->horizontalString[horizontalStringLength];      //add string to the subsequence
			verticalStringLength--;
			horizontalStringLength--;
			aux--;
		}else if (horizontalString[horizontalStringLength] == vectorWidth[verticalStringLength]) {  //if there is a match, move diagonally
			subsequence[aux -1] = board->horizontalString[horizontalStringLength];                   //add the value to the subsequence
			verticalStringLength--;
			horizontalStringLength--;
			aux--;
		}else if (matrix[verticalStringLength][horizontalStringLength-1] == aux) {
			horizontalStringLength--;
		}else if (matrix[verticalStringLength-1][horizontalStringLength] == aux) {
			verticalStringLength--;
		}
	}
	printf("%d\n", finalSize);                              //print the final output
	for (aux = 0; aux < finalSize; ++aux) {
		printf("%c", subsequence[aux]);
	}
	printf("\n");

	free(subsequence);
	matrix = NULL;


}

//Function to add computation so speedups can be seen in parallel mode. Always returns 1
short cost(int x){
	int i, n_iter = 20;
	double dcost = 0;
	for(i = 0; i < n_iter; i++)
		dcost += pow(sin((double) x),2) + pow(cos((double) x),2);
	return (short) (dcost / n_iter + 0.1);
}

//Function to clear the resources
void cleanAll(Board board){
	size_t n;

	free(board->horizontalString);
	free(board->verticalString);

	for(n = 0; n <= board->height; n++){
		free(board->matrix[n]);
	}
	free(board->matrix);

	free(board);
}
