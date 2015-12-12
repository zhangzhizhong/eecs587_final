// parallel forward checking sudoku solver
#include <stdio.h>
#include <cmath>
#include <vector>
#include <queue>
#include <unordered_set>
#include <iostream>
#include <string.h>
#include <time.h>
#include <mpi.h>

using namespace std;

// N is used for size of Sudoku grid. Size will be NxN
#define N 16

struct Node{
bool col[N+1][N+1],row[N+1][N+1],f[N+1][N+1];
int board[N][N];
int x,y;
};

int mpi_size; 



void printBoard(Node* start);

void solveSudoku(){
  vector<Node*> local_stack;
  Node* start;
  bool idle_flag = false;

  int i,j;
  int probe_flag;
  MPI_Status status;
  char *buffer = new char[sizeof(Node)];

  while(1){

    if (local_stack.empty()){

      if (!idle_flag){
        idle_flag = true;
        // Tag 1: pull job
        int pull_flag = 1;
        MPI_Send (&pull_flag, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        continue;
      }

      probe_flag = 0;
      MPI_Iprobe (0, MPI_ANY_TAG, MPI_COMM_WORLD, &probe_flag, &status);
      if (probe_flag){
        // Tag 1: pull job
        if (status.MPI_TAG = 1) {
          MPI_Recv (buffer, sizeof(Node), MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status);
          start = new Node;
          *start = *(static_cast<Node *>(static_cast<void*>(buffer)));
          local_stack.push_back(start);
          idle_flag = false;
        }

        // Tag 3: teminate
        if (status.MPI_TAG = 3) {
          break;
        }
      }

      continue;
    }


    start = local_stack.back();
    local_stack.pop_back();

    i = start->x;
    j = start->y;



    // Terminate with an answer
    if(i >= N){
      printBoard(start);
      int terminate_flag = 1;
      MPI_Send (&terminate_flag, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
      break;
    }

    // Find next empty cell
    if(start->board[i][j] != 0){
      if(j < N-1)  {
        start->y = j+1;
        local_stack.push_back(start);
        continue;
      }
      else{
        start->y = 0;
        (start->x) ++;
        local_stack.push_back(start);
        continue;
      }
    }

    // Expand
    int t = sqrt(N);
    int temp = t*(i/t)+j/t;
    for(int n = 1; n <= N; n++){
      if(!start->col[j][n] && !start->row[i][n] && !start->f[temp][n]){
        // Create new sub job
        Node* next_node = new Node;
        *next_node = *start;
        next_node->board[i][j] = n;
        next_node->col[j][n] = next_node->row[i][n] = next_node->f[temp][n] = true;
        if(j < N-1){
          next_node->y = j+1;
          local_stack.push_back(next_node);
          
        } else{
          next_node->y = 0;
          (next_node->x) ++;
          local_stack.push_back(next_node);
        }
      }
    }
    delete start;

  // End while
  }
  delete buffer;
}

/* 
  Tag 1: pull job
  Tag 2: push job
  Tag 3: teminate
*/
void Master(Node* root){
  struct compare {
    bool operator () (Node* left_node, Node* right_node) {
      return left_node->x * N + left_node->y < right_node->x * N + right_node->y;
    }
  }; 
  priority_queue<Node*, vector<Node*>, compare> global_queue;
  Node* start = new Node;
  *start = *root;
  global_queue.push(start);
  int probe_flag;
  MPI_Status status;
  char *buffer;

  while(1){
    for (int pid = 1; pid < mpi_size; pid++){
      probe_flag = 0;
      MPI_Iprobe (pid, MPI_ANY_TAG, MPI_COMM_WORLD, &probe_flag, &status);

      // Processing message
      if (probe_flag){
        // Tag 1: pull job
        if (status.MPI_TAG = 1) {
          int pull_flag = 0;
          MPI_Recv (&pull_flag, 1, MPI_INT, pid, 1, MPI_COMM_WORLD, &status);
          if (!global_queue.empty()){
            start = global_queue.top();
            global_queue.pop();
            buffer = static_cast<char*>(static_cast<void*>(start));
            MPI_Send (&buffer, sizeof(Node), MPI_CHAR, pid, 1, MPI_COMM_WORLD);
          }
        }

        // Tag 2: push job
        if (status.MPI_TAG = 2) {
          int stack_size = 0;
          MPI_Recv (&stack_size, 1, MPI_INT, pid, 2, MPI_COMM_WORLD, &status);

        }

        // Tag 3: teminate
        if (status.MPI_TAG = 3) {
          int ter_flag = 0;
          MPI_Recv (&ter_flag, 1, MPI_INT, pid, 3, MPI_COMM_WORLD, &status);
          for (int notify_id = 1; notify_id < mpi_size; notify_id++){
            int terminate_flag = 0;
            MPI_Send (&terminate_flag, 1, MPI_INT, notify_id, 3, MPI_COMM_WORLD);
            break;
          }
        }
      } // End processing MPI_Iprobe
    }
  } // End while

}


/* Driver Program to test above functions */
int main(int argc, char** argv){
  int rank;
  int size;
  double starttime;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  mpi_size = size;
  starttime = MPI_Wtime();
  cout << "hello world(" << rank << ", " << size << ")" << endl;


  int grid[N][N] = {{ 7, 0, 0, 0, 0, 5, 1, 0, 3, 11, 0, 0, 0, 0, 0, 0 },
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
                    { 3, 9, 2, 10, 15, 11, 8, 1, 6, 13, 0, 4, 0, 14, 0, 16 },
                    { 16, 12, 3, 6, 11, 14, 15, 13, 5, 10, 8, 7, 1, 4, 2, 9 },
                    { 9, 2, 7, 14, 6, 8, 12, 4, 13, 16, 15, 1, 3, 11, 10, 5 },
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

  if (rank == 0){
    Master(&start);
  } else{
    solveSudoku();
  }
  if (rank == 0){
    double endtime   = MPI_Wtime();
    printf("That took %f seconds\n",endtime-starttime);
    cout<<endl<<endl;
  }
  MPI_Finalize();
  return 0;
}

void printBoard(Node* start) {
  cout<< start->x<<' '<<start->y<<endl;
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++)
      cout << start->board[i][j] << " ";
    cout << endl;
  }
  cout<<endl;
  return;
}