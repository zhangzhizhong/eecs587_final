// A Backtracking program  in C++ to solve Sudoku problem
#include <stdio.h>
#include <cmath>
#include <vector>
#include <queue>
#include <unordered_set>
#include <iostream>
#include <string.h>
#include <time.h>

using namespace std;

// UNASSIGNED is used for empty cells in sudoku grid
#define UNASSIGNED 0

// N is used for size of Sudoku grid. Size will be NxN
#define N 16


struct Node{
bool col[N+1][N+1],row[N+1][N+1],f[N+1][N+1];
int board[N][N];
int x,y;
};




void printBoard(Node* start) {
  cout<< start->x<<' '<<start->y<<endl;
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++)
      cout << start->board[i][j] << " ";
    cout << endl;
  }
  cout<<endl;

  for (int i = 0; i <= N; i++) {
    for (int j = 0; j <= N; j++)
      cout << start->row[i][j] << " ";
    cout << endl;
  }
  cout<<endl;

  for (int i = 0; i <= N; i++) {
    for (int j = 0; j <= N; j++)
      cout << start->col[i][j] << " ";
    cout << endl;
  }
  cout<<endl;

  return;
}

void solveSudoku(Node* start);

/* Driver Program to test above functions */
int main()
{


  int grid[N][N] = {{7, 0, 0, 0, 0, 5, 1, 0, 3, 11, 0, 0, 0, 0, 0, 0 },
                    { 12, 8, 0, 0, 0, 15, 14, 0, 4, 0, 9, 0, 11, 0, 16, 2 },
                    { 0, 15, 10, 2, 13, 0, 0, 0, 0, 7, 0, 5, 8, 0, 3, 0 },
                    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    { 0, 0, 1, 0, 0, 2, 0, 0, 15, 0, 0, 0, 5, 0, 0, 11 },
                    { 15, 0, 0, 3, 0, 0, 0, 0, 0, 0, 7, 14, 6, 0, 1, 0 },
                    { 14, 0, 16, 0, 0, 0, 0, 0, 0, 5, 6, 0, 10, 2, 0, 0 },
                    { 0, 0, 12, 0, 0, 0, 0, 8, 9, 1, 10, 13, 16, 0, 0, 3 },
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 9, 8, 0, 0 },
                    { 0, 6, 4, 0, 0, 10, 0, 0, 7, 0, 14, 0, 0, 0, 11, 0 },
                    { 0, 16, 0, 12, 0, 3, 9, 0, 10, 0, 0, 8, 0, 0, 5, 0 },
                    { 3, 0, 0, 0, 15, 0, 0, 0, 0, 13, 0, 4, 0, 14, 0, 16 },
                    { 16, 12, 3, 6, 11, 0, 15, 0, 5, 10, 8, 7, 1, 4, 2, 9 },
                    { 9, 2, 7, 14, 6, 8, 12, 4, 0, 16, 15, 1, 3, 11, 10, 5 },
                    { 5, 13, 8, 4, 3, 1, 10, 2, 12, 9, 11, 6, 14, 16, 15, 7 },
                    { 10, 1, 15, 11, 9, 7, 5, 16, 14, 3, 4, 2, 13, 6, 12, 8 },};
  Node start;
  clock_t t = clock();
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++){
      start.board[i][j] = grid[i][j];
    }
  }

  memset(start.col,false,sizeof(start.col));
  memset(start.row,false,sizeof(start.row));
  memset(start.f,false,sizeof(start.f));

  
  for(int i = 0; i < N;i++){
      for(int j = 0; j < N;j++){
          if(start.board[i][j] == 0)   continue;
          int t = sqrt(N);
          int block_number = t*(i/t)+j/t;
          int v = start.board[i][j];
          start.col[j][v] = start.row[i][v] = start.f[block_number][v] = true;
      }
  }
  start.x = start.y = 0;

  solveSudoku(&start);

  t = clock() - t;
  printf ("It took me %d clicks (%f seconds).\n",t,((float)t)/CLOCKS_PER_SEC);
  return 0;
}

void solveSudoku(Node* start) {
  vector<Node*> local_stack;
  Node* curr_node = new Node;
  *curr_node = *start;
  local_stack.push_back(curr_node);
  int local_counter = 0;
  while(1){
    if (local_stack.empty()){
      break;
    }
    curr_node = local_stack.back();
    local_stack.pop_back();

    local_counter++;
    // printBoard(curr_node);
    
    // if (local_counter == 15){
    //   break;
    // }

    int x = curr_node->x;
    int y = curr_node->y;
    int t = sqrt(N);
    int block_number = t*(x/t)+y/t;

    // Find next cell 
    while(curr_node->board[x][y] != 0 && x < N){
        if (y < N-1){
          y++;
        } else{
          x++; y=0;
        }
    }
    curr_node->x = x;
    curr_node->y = y;

    // Termination
    if (x >= N){
      printBoard(curr_node);
      break;
    }


    for(int v = N; v >= 1; v--){
      if(!curr_node->col[y][v] && !curr_node->row[x][v]
         && !curr_node->f[block_number][v]){
        //Expand
        Node* next_node = new Node;
        *next_node = *curr_node;
        next_node->board[x][y] = v;
        next_node->col[y][v] = next_node->row[x][v] = next_node->f[block_number][v] = true;
        // if(y < N-1){
        //   (next_node->y)++;
        // }
        // else{
        //   (next_node->x)++;
        //   next_node->y = 0;
        // }
        local_stack.push_back(next_node);
      }
    }
    delete curr_node;
  }
  cout<<"cnt:"<<local_counter<<endl;
}


