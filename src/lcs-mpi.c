#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>

#define MAX(x, y) ( ((x) > (y)) ? (x) : (y) )
#define MIN(x, y) ( ((x) > (y)) ? (y) : (x) )
#define EVEN(x) ( ((x) % 2) ? 0 : 1 )
#define ODD(x) ( ((x) % 2) ? 1 : 0 )

#define BLOCK_LOW(id,p,n) ((id)*(n)/(p))
#define BLOCK_HIGH(id,p,n) (BLOCK_LOW((id)+1,p,n)-1)
#define BLOCK_SIZE(id,p,n) (BLOCK_HIGH(id,p,n)-BLOCK_LOW(id,p,n)+1)
#define BLOCK_OWNER(index,p,n) (((p)*((index)+1)-1)/(n))


#define CHANGED 0;
#define NOT_CHANGED 1;

#define ROOT 0;


typedef struct {
	int height;
	int width;
	int matrixHeight;
	int maxLength_line;
	short CHANGE; 	//1 if it was not changed, otherwise 0
	char* vectorHeight;
	char* vectorWidth;
	short** matrix;
	short numProc;
	short rank;
}board_t;


typedef board_t* Board;

short cost(int x);
Board parseFile(char* fileName, int rank, int numProc);
void iterateBoard(board_t* board);
short processCell(short* vector1LineAbove, short* vector2LineAbove, int line, int globalIndex, Board board);
void printResults(board_t* board);
void cleanAll(board_t* board);
int getI(int line, int index, int height);
int getJ(int line, int index, int heigth);
int getLength_Line(int line, int height, int width);
void initMPI(int *argc, char ***argv, int *rank, int *numProc);
void processSubLine(short* vector1LineAbove, short* vector2LineAbove, short* vectorToProcess, int vectorToProcessLength, int line, Board board);

int main(int argc, char* argv[]){

	int rank, numProc;
	char* fileName = argv[1];
	//double start, end;
	//start = MPI_Wtime();
	initMPI(&argc, &argv, &rank, &numProc);
	Board board = parseFile(fileName, rank, numProc);
	iterateBoard(board);
	if(rank == 0){
		printResults(board);
	//	end = MPI_Wtime();
	//	printf("time: %f\n", end-start);
	}
	cleanAll(board);

	MPI_Finalize();
	return 0;
}

void initMPI(int *argc, char ***argv, int *rank, int *numProc){
  MPI_Init(argc, argv);
  MPI_Comm_rank(MPI_COMM_WORLD, rank);
  MPI_Comm_size(MPI_COMM_WORLD, numProc);
}

Board parseFile(char* fileName, int rank, int numProc){
	FILE* file;
	int height;
	int width;
	int c;
	char* vectorHeight;
	char* vectorWidth;
	char* vectorAux;
	short** matrix;
	int maxlenght_line, aux, matrixHeight;
	int n = 1;
	short CHANGE = NOT_CHANGED;

	if(rank == 0){
		if (!(file = fopen(fileName,"r"))){
			printf("Error opening file \"%s\"\n", fileName);
			exit(2);
		}
		if(fscanf(file, "%d %d\n", &height, &width) == 0){
			printf("Could not read height and width");
			exit(3);
		}
		if(!(vectorHeight = (char*) malloc(sizeof(char)*(height + 1)))||
		   !(vectorWidth = (char*) malloc(sizeof(char)*(width + 1)))){
				printf("Error allocating row pointers for board.\n");
				exit(4);
		}

		matrixHeight = width + height + 1;

		while((c = fgetc(file)) != '\n'){
			 vectorHeight[n++] = (char)c;
		}
		vectorHeight[0] = '/';

		n = 1;
		while((c = fgetc(file)) != '\n'){
			vectorWidth[n++] = (char)c;
		}
		vectorWidth[0] = '/';

		if(height < width){
			c = height;
			height = width;
			width = c;
			vectorAux = vectorHeight;
			vectorHeight = vectorWidth;
			vectorWidth = vectorAux;
			CHANGE = CHANGED;
		}
		fclose(file);
		file = NULL;
	}
	MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(rank != 0){
		vectorHeight = (char*)malloc(sizeof(char)*(height + 1));
		vectorWidth = (char*)malloc(sizeof(char)*(width + 1));
	}
	MPI_Bcast(vectorHeight, height + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(vectorWidth, width + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(&matrixHeight, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&CHANGE, 1, MPI_SHORT, 0, MPI_COMM_WORLD);


	maxlenght_line = width + 1;
	aux = EVEN(matrixHeight) ? matrixHeight / 2 : (matrixHeight / 2) + 1;
	matrix = (short**)malloc(sizeof(short*) * matrixHeight);
	c = 1;
	for(n = 0; n < aux; n++){
		matrix[n] = (short*)malloc(sizeof(short) * c);
		if(n != (matrixHeight - 1 - n)) matrix[matrixHeight - 1 - n] = (short*)malloc(sizeof(short) * c);
		c = (c < maxlenght_line) ? c + 1 : c;
	}
	Board result = (Board)malloc(sizeof(board_t));

	result->height =  height;
	result->width = width;
	result->matrix = matrix;
	result->vectorHeight = vectorHeight;
	result->vectorWidth = vectorWidth;
	result->matrixHeight = matrixHeight;
	result->maxLength_line = maxlenght_line;
	result->CHANGE = CHANGE;
	result->rank = rank;
	result->numProc = numProc;
	return result;

}

void iterateBoard(board_t* board){
	int i,j, matrixHeight = board->matrixHeight;
	size_t line, index;
	int rank = board->rank;
	int numProc = board->numProc;
	int length_line;
	int height = board->height;
	int width = board->width;
	short* vectorToProcess;
	short* vector2LinesAbove;
	short* vector1LineAbove;
	int counts[numProc];
	int displs[numProc];

	for (line = 0; line < matrixHeight; line++) {
		length_line = getLength_Line(line,height, width);
		if(length_line >= numProc && line > 1){
			for(i = 0; i < numProc; i++){
				displs[i] = (int)BLOCK_LOW(i, numProc, length_line);
				counts[i] = (int)BLOCK_SIZE(i, numProc,length_line);
			}

			vectorToProcess = (short*)malloc(sizeof(short) * BLOCK_SIZE(rank, numProc,length_line));
			if(rank == 0){
				vector2LinesAbove = board->matrix[line-2];
				vector1LineAbove = board->matrix[line-1];
			}
			else{
				vector2LinesAbove = (short*)malloc(sizeof(short) * getLength_Line(line -2, height, width));
				vector1LineAbove = 	(short*)malloc(sizeof(short) * getLength_Line(line -1, height, width));
			}

			MPI_Bcast(vector1LineAbove, getLength_Line(line -1, height, width),MPI_SHORT,0,MPI_COMM_WORLD);
			MPI_Bcast(vector2LinesAbove, getLength_Line(line -2, height, width),MPI_SHORT,0,MPI_COMM_WORLD);

			processSubLine(vector1LineAbove, vector2LinesAbove, vectorToProcess, BLOCK_SIZE(rank, numProc,length_line), line, board);

			MPI_Gatherv(vectorToProcess,BLOCK_SIZE(rank, numProc,length_line),MPI_SHORT,board->matrix[line],counts, displs,MPI_SHORT,0,MPI_COMM_WORLD);

			if(rank != 0){
				free(vector1LineAbove);
				free(vector2LinesAbove);
			}
			free(vectorToProcess);
		}else if(rank == 0){
			if(line < 2)
				processSubLine(vector1LineAbove, vector2LinesAbove, board->matrix[line], getLength_Line(line,height, width), line, board);
			else
				processSubLine(board->matrix[line-1], board->matrix[line-2], board->matrix[line], getLength_Line(line,height, width), line, board);

		}
	}
}

void processSubLine(short* vector1LineAbove, short* vector2LineAbove, short* vectorToProcess, int vectorToProcessLength, int line, Board board){
	int globalIndex, localIndex;
	int rank = board->rank;
	int numProc = board->numProc;
	int height = board->height;
	int width = board->width;

#pragma omp parallel for shared(vectorToProcess) private(localIndex)
	for(localIndex = 0; localIndex < vectorToProcessLength; localIndex++){
		globalIndex = localIndex + BLOCK_LOW(rank, numProc, getLength_Line(line,height, width));
		vectorToProcess[localIndex] = processCell(vector1LineAbove, vector2LineAbove, line, globalIndex, board);
	}
}
short processCell(short* vector1LineAbove, short* vector2LineAbove, int line, int globalIndex, Board board){
	int i = getI(line, globalIndex, board->height);
	int j = getJ(line, globalIndex, board->height);
	int length_line = getLength_Line(line,board->height, board->width);
	int maxLength_line = board->maxLength_line;


	int matrixHeight = board->matrixHeight;
	if (i == 0 || j == 0) {
		return 0;
	} else if (board->vectorHeight[i] == board->vectorWidth[j]) {
		if((length_line == maxLength_line - 1) && (line > matrixHeight/2)){
			return vector2LineAbove[globalIndex] + cost(i+j);
		}else if((length_line < maxLength_line - 1) && (line > matrixHeight/2)){
			return vector2LineAbove[globalIndex+1] + cost(i+j);
		}else{
			return vector2LineAbove[globalIndex-1] + cost(i+j);
		}
	}else{
		if((length_line <= maxLength_line - 1) && (line > matrixHeight/2)){
			return MAX(vector1LineAbove[globalIndex+1],vector1LineAbove[globalIndex]);
		}else{
			return MAX(vector1LineAbove[globalIndex-1],vector1LineAbove[globalIndex]);
		}
	}
}
short cost(int x){
	int i, n_iter = 20;
	double dcost = 0;
	for(i = 0; i < n_iter; i++)
		dcost += pow(sin((double) x),2) + pow(cos((double) x),2);
	return (short) (dcost / n_iter + 0.1);
}

void printResults(board_t* board){
	int line = board->matrixHeight - 1;
	int index = 0;
	int i,j;
	int maxLength_line = board->maxLength_line;
	int length_line;
	int matrixHeight = board->matrixHeight;
	short aux, finalSize = board->matrix[line][0];
	char* subsequence = (char*) malloc(sizeof(char) * finalSize);
	char* vectorHeight = board->vectorHeight;
	char* vectorWidth = board->vectorWidth;
	short** matrix = board->matrix;

	/* just for testing*
	printf("height: %d, width: %d\n",board->height, board->width);
	for(i = 0; i < board->matrixHeight; i++){
		//printf("line:%d ",i);
		for(j = 0; j < getLength_Line(i, board->height, board->width); j++){
			printf("|%d,%d:%d|", getI(i, j, board->height), getJ(i, j, board->height), board->matrix[i][j]);
			//printf("|%d,%d|",board->matrix[i][j].i, board->matrix[i][j].j);
			//printf("|%d|",board->matrix[i][j].value);
			//printf("|%d|",board->matrix[i][j].length_Line);
		}
		printf("\n");
	}
	/* just for testing */


	aux = finalSize;
	while(aux > 0){
		i = getI(line, index, board->height);
		j = getJ(line, index, board->height);
		length_line = getLength_Line(line, board->height, board->width);
		if(vectorHeight[i] == vectorWidth[j]){
			if((length_line == maxLength_line - 1) && (line > matrixHeight/2)){
				line -=2;
				subsequence[aux - 1] = vectorWidth[j];
				aux--;
			}else if((length_line < maxLength_line - 1) && (line > matrixHeight/2)){
				line -= 2;
				index += 1;
				subsequence[aux - 1] = vectorHeight[i];
				aux--;
			}else{
				line -= 2;
				index -= 1;
				subsequence[aux - 1] = vectorHeight[i];
				aux--;
			}
		}else{
			if((length_line <= maxLength_line - 1) && (line > matrixHeight/2)){
				if(board->CHANGE){
					if(matrix[line-1][index] == aux){
						line -= 1;
					}else{
						index += 1;
						line -= 1;
					}
				}else{
					if(matrix[line-1][index+1] == aux){
						line -= 1;
						index += 1;
					}else{
						line -= 1;
					}
				}
			}
			else{
				if(board->CHANGE){
					if(matrix[line-1][index-1] == aux){
						index -= 1;
						line -= 1;
					}else{
						line -= 1;
					}
				}else{
					if(matrix[line-1][index] == aux){
						line -= 1;
					}else{
						index -= 1;
						line -= 1;
					}
				}
			}
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

void cleanAll(board_t* board){
	size_t n;

	free(board->vectorHeight);
	free(board->vectorWidth);

	for(n = 0; n < board->matrixHeight; n++){
		free(board->matrix[n]);
	}
	free(board->matrix);

	free(board);
}

int getI(int line, int index, int height){
	if(line <= height){
		 return line - index;
	}else{
		return height - index;
	}
}
int getJ(int line, int index, int height){
	if(line <= height){
		return index;
	}else{
		return (line - height) + index;
	}

}
int getLength_Line(int line, int height, int width){
	if(line <= width){
		return line +1;
	}else if(line > height){
		return (width + 1) -(line - height);
	}else{
		return (width + 1);
	}
}
