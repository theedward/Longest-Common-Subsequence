#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>


#define MAX(x, y) ( ((x) > (y)) ? (x) : (y) )

typedef struct {
	int height;
	int width;
	char* vectorHeight;
	char* vectorWidth;
	short int** matrix;
	omp_lock_t* locks;
}board_t;

typedef board_t* Board;

short cost(int x);
Board parseFile(char* fileName);
void processCell(int i, int j, Board board);
void printResults(board_t* board);
void iterateBoard(board_t* board);
void cleanAll(board_t* board);
void lockCell(int i, int j, Board board);
void unlockCell(int i, int j, Board board);
void lockLine(int i, Board board);


int main(int argc, char* argv[]){

	char* fileName = argv[1];
	Board board = parseFile(fileName);
	iterateBoard(board);
	printResults(board);
	cleanAll(board);
	return 0;
}


/* Function that applies the given algorithm.
 * If the line or column are 0, fill the line or column with 0's
 * If there is a match grab the value from the top left cell and add it cost
 * Else grab the Max value from the left cell or top cell
 * i, the line; j, the column; board, the current board
 */
void processCell(int i, int j, Board board){
	if (i == 0 || j == 0) {
		board->matrix[i][j] = 0;
	} else if (board->vectorHeight[i] == board->vectorWidth[j]) {
		lockCell(i-1, j-1, board);
		board->matrix[i][j] = board->matrix[i - 1][j - 1] + cost(i+j);
		unlockCell(i-1, j-1, board);
	}else{
		lockCell(i-1, j, board);
		board->matrix[i][j] = MAX(board->matrix[i - 1][j], board->matrix[i][j - 1]);
		unlockCell(i-1, j, board);
	}
	unlockCell(i, j, board);
}

/* Function that iterates through the matrix
 * Each thread locks the lines it's going to process
 * Calls the processCell funtion on each cell
 * board, the current board
 */
void iterateBoard(board_t* board){
	int heightLength = board->height + 1;
	int widthLength = board->width + 1;
	size_t i, j;
#pragma omp parallel private(i,j) shared(board)
	{
#pragma omp for schedule(static, 1)

	for (i = 0; i < heightLength; ++i) {
		lockLine(i, board);
	}
#pragma omp barrier

#pragma omp for schedule(static, 1)
	for (i = 0; i < heightLength; ++i) {
		for (j = 0; j < widthLength; ++j) {
			processCell(i, j, board);
		}
	}
	}
}


/* Function that reads the file and builds the board
 * Builds the vertical and horizontal string, and allocs for matrix postions
 * filename, the name of the file to be read
 * returns the board (matrix)
 */
Board parseFile(char* fileName){
	FILE* file;
	int height;
	int width;
	int c;
	char* vectorHeight;
	char* vectorWeidth;
	short int** matrix;
	size_t n = 1;
	omp_lock_t* locks;


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
	

	n = 1;
	while((c = fgetc(file)) != '\n'){
		vectorWeidth[n++] = (char)c;
	}
	
	Board result = (Board)malloc(sizeof(board_t));
	locks = (omp_lock_t*)malloc(sizeof(omp_lock_t)*(height+1)*(width+1));



	#pragma omp parallel 
	{
	#pragma omp sections 
	{
	#pragma omp section
	vectorHeight[0] = '/';
	#pragma omp section
	vectorWeidth[0] = '/';
	#pragma omp section
	{
	fclose(file);
	file = NULL;
	}
	#pragma omp section
	matrix = (short int**)malloc(sizeof(short int*) * (height + 1));
	}
	#pragma omp for private (n)
	for (n = 0; n < height + 1; ++n) {
		matrix[n] = (short int*)malloc(sizeof(short int) * (width + 1));
	}
	
		
	#pragma omp for private (n)
	for(n = 0; n < ((height+1) * (width +1)); n++){
		omp_init_lock(&(locks[n]));
	}

	#pragma omp sections
	{
	#pragma omp section	
	result->height =  height;
	#pragma omp section
	result->width = width;
	#pragma omp section
	result->matrix = matrix;
	#pragma omp section
	result->vectorHeight = vectorHeight;
	#pragma omp section
	result->vectorWidth = vectorWeidth;
	#pragma omp section
	result->locks = locks;
	}
}
	return result;

}

/* Function that backtracks the matrix and fills the subsequence
 * Starts from the bottom of the matrix and applies the backtracking rules
 * If the letters match, move diagonally, else go left
 * Prints out the final output
 * board, the current board 
 */
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

/* Function to add computation time
 * returns 1 always
 */
short cost(int x){
	int i, n_iter = 20;
	double dcost = 0;
	for(i = 0; i < n_iter; i++)
		dcost += pow(sin((double) x),2) + pow(cos((double) x),2);
	return (short) (dcost / n_iter + 0.1);
}

/* 
 * Clears the resourses used
 */
void cleanAll(board_t* board){
	size_t n;
	
	#pragma omp parallel 
	{
	#pragma omp sections
	{
	#pragma omp section
	free(board->vectorHeight);
	#pragma omp section
	free(board->vectorWidth);	
	}
	#pragma omp for private (n)
	for(n = 0; n < board->height; n++){
		free(board->matrix[n]);
	}

	#pragma omp for private (n)
	for(n = 0; n < ((board->height +1) * (board->width +1)); n++){
		omp_destroy_lock(&(board->locks[n]));
	}
	
	#pragma omp sections
	{
	#pragma omp section
	free(board->locks);
	#pragma omp section
	free(board->matrix);
	}
}
	free(board);
}


/* Locks a single cell
 * i, j the coordinates of the cell; board, the current board
 */
void lockCell(int i, int j, Board board){

	int width = board->width + 1;
	int offset = i * width + j;

	omp_set_lock(&(board->locks[offset]));
}

/* Unlocks a single cell
 * i, j the coordinates of the cell; board, the current board
 */
void unlockCell(int i, int j, Board board){

	int width = board->width + 1;
	int offset = i * width + j;

	omp_unset_lock(&(board->locks[offset]));
}

/* Locks the whole line
 * i, the coordinate of the line; board, the current board
 */
void lockLine(int i, Board board){

	int width = board->width + 1;
	int offset = i * width;
	int n;
	for(n = 0; n < width; n++){
		omp_set_lock(&(board->locks[n + offset]));
	}
}
