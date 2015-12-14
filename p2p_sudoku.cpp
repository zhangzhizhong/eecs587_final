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
#include <assert.h>

using namespace std;

// N is used for size of Sudoku grid. Size will be NxN
#define N 16

int min(int a, int b){
  if (a < b) return a;
  return b;
}

struct Node{
bool col[N+1][N+1],row[N+1][N+1],f[N+1][N+1];
int board[N][N];
int x,y;
};

int mpi_size;
vector<int> peers;



void printBoard(Node* start);


/* 
  Tag 1: request channel
  Tag 2: reply channel
  Tag 3: teminate
*/
void solveSudoku(Node* root){

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  cout << "worker hello world(" << rank << ")" << endl;

  vector<Node*> local_stack;
  Node* start;
  bool idle_flag = false;

  if (rank == 0){
    Node* start = new Node;
    *start = *root;
    local_stack.push_back(start);
  }

  int i,j;
  int probe_flag;
  MPI_Status status;
  char *buffer = new char[sizeof(Node)];

  long long run_time_counter = 0;
  int stack_size;
  int num_push_back;
  int peer_index = 0;
  int pull_start;
  int terminate_flag = 0;


  long long local_cnt = 0;
  long long load_balance_cnt = 0;

  while(1){

    if (local_stack.empty()){
      // Send request and turn idle
      if (!idle_flag){
        idle_flag = true;
        // Tag 1: request channel
        int pull_flag = 1;
        if (++peer_index == peers.size()){
          peer_index = 0;
        }
        MPI_Send(&pull_flag, 1, MPI_INT, peers[peer_index], 1, MPI_COMM_WORLD);
        //cout << "idle worker request(" << rank << ") to "<< peers[peer_index] <<" at time "<< local_cnt<< endl;
        continue;
      }

      probe_flag = 0;
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &probe_flag, &status);
      if (probe_flag){
        

        // Tag 1: request channel
        if (status.MPI_TAG == 1) {
          // Reject peer requst
          int pull_flag = 0;
          MPI_Recv (&pull_flag, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
          pull_start = 0;
          MPI_Send (&pull_start, 1, MPI_INT, status.MPI_SOURCE, 2, MPI_COMM_WORLD);
          continue;
        }

        // Tag 2: reply channel
        if (status.MPI_TAG == 2){
          assert(status.MPI_SOURCE == peers[peer_index]);
          // Pull start
          pull_start = 0;
          MPI_Recv (&pull_start, 1, MPI_INT, status.MPI_SOURCE, 2, MPI_COMM_WORLD, &status);

          // Reset idle_flag, try other peer
          idle_flag = false;
          if (!pull_start){
            //cout << "idle worker detect null reply(" << rank << ")"<<endl;
            continue;
          }
          //cout << "idle worker detect reply(" << rank << ") from" << status.MPI_SOURCE <<" flag = "<< status.MPI_TAG << "at time:" << local_cnt << endl;
          // Receive node
          start = new Node;
          MPI_Recv(start, sizeof(Node), MPI_CHAR, peers[peer_index], 2, MPI_COMM_WORLD, &status);
          local_stack.push_back(start);
          //printBoard(start);
        }

        // Tag 3: teminate
        if (status.MPI_TAG == 3) {
          // Send termination signal to all peers, then exit
          terminate_flag = 1;
          for (int notify_id:peers){
            MPI_Send (&terminate_flag, 1, MPI_INT, notify_id, 3, MPI_COMM_WORLD);
          }
          break;
        }
      }
      continue;
    } // End if local_stack.empty()

    if (terminate_flag){
      break;
    }

    local_cnt++;

    // Runtime Load balancing

    if (++run_time_counter == 300000){
      load_balance_cnt++;
      run_time_counter = 0;
      int loop_cnt = 0;
      while(1){
        // Too much message
        if (++loop_cnt == mpi_size){
          break;
        }
        probe_flag = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &probe_flag, &status);
        if (probe_flag){
          //cout << "idle worker detect reply(" << rank << ") flag = "<< status.MPI_TAG << "at time:" << local_cnt << endl;
         //cout<<"Iprobe cpu: ("<<rank<<") of tag: "<<status.MPI_TAG<<endl;
          // Tag 1: request channel
          if (status.MPI_TAG == 1) {
            // reply peer requst
            int pull_flag = 0;
            MPI_Recv (&pull_flag, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
            pull_start = 0; 
            if (local_stack.size() >= 2){
              pull_start = 1;
            }
            MPI_Send (&pull_start, 1, MPI_INT, status.MPI_SOURCE, 2, MPI_COMM_WORLD);
            if (local_stack.size() >= 2){
              // Send one Node
              start = local_stack[0];
              local_stack.erase(local_stack.begin());
              buffer = static_cast<char*>(static_cast<void*>(start));
              MPI_Send (buffer, sizeof(Node), MPI_CHAR, status.MPI_SOURCE, 2, MPI_COMM_WORLD);
              delete start;
            }
            continue;
          }

          // Tag 2: receive channel
          if (status.MPI_TAG == 2){
            cout<<"[Error:] recieve Node at Runtime! rank="<<rank<<endl;
            break;
          }

          // Tag 3: teminate
          if (status.MPI_TAG == 3) {
            // Send termination signal to all peers, then exit
            terminate_flag = 1;
            for (int notify_id:peers){
              MPI_Send (&terminate_flag, 1, MPI_INT, notify_id, 3, MPI_COMM_WORLD);
            }
            break;
          }
        } else{
          break;
        }
      } // End while 1
    }
    

    // Start local search
    start = local_stack.back();
    local_stack.pop_back();

    i = start->x;
    j = start->y;

    // Terminate with an answer
    if(i >= N){
      printBoard(start);
      int terminate_flag = 1;
      for (int notify_id:peers){
        MPI_Send (&terminate_flag, 1, MPI_INT, notify_id, 3, MPI_COMM_WORLD);
      }
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
  cout << "worker terminate(" << rank << ") with work:"<< local_cnt <<" and comm:"<<load_balance_cnt<< endl;
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

  // Calculate peers id vector
  cout<< rank<<" has peers:";
  int i=1;
  while (i<mpi_size){
    int peer_id = rank ^ i;
    if (peer_id < mpi_size){
      peers.push_back(peer_id);
    }
    i = i<<1;
    cout<<peer_id<<" ";
  }
  cout<<endl;


  // 65 secs
  int grid[N][N] = {{ 7, 14, 6, 9, 8, 5, 1, 0, 3, 11, 0, 0, 0, 0, 0, 0 },
                    { 12, 8, 0, 0, 0, 15, 14, 0, 4, 0, 9, 0, 11, 0, 16, 2 },
                    { 0, 15, 10, 2, 13, 0, 0, 0, 0, 7, 0, 5, 8, 0, 3, 0 },
                    { 1, 3, 11, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    { 0, 0, 1, 0, 0, 2, 0, 0, 15, 0, 0, 0, 5, 0, 0, 11 },
                    { 15, 0, 0, 3, 0, 0, 0, 0, 0, 0, 7, 14, 6, 0, 1, 0 },
                    { 14, 0, 16, 0, 0, 0, 0, 0, 0, 5, 6, 0, 10, 2, 0, 0 },
                    { 0, 0, 12, 0, 0, 0, 0, 8, 9, 1, 10, 13, 16, 0, 0, 3 },
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0, 9, 8, 0, 0 },
                    { 0, 6, 4, 0, 0, 10, 0, 0, 7, 0, 14, 0, 0, 0, 11, 0 },
                    { 0, 16, 0, 12, 0, 3, 9, 0, 10, 0, 0, 8, 0, 0, 5, 0 },
                    { 0, 0, 0, 10, 15, 11, 8, 1, 6, 13, 0, 4, 0, 0, 0, 16 },
                    { 16, 12, 0, 6, 11, 14, 15, 13, 5, 10, 0, 7, 1, 0, 2, 9 },
                    { 9, 2, 0, 0, 0, 8, 12, 4, 13, 16, 15, 1, 3, 11, 10, 0 },
                    { 5, 13, 8, 4, 3, 1, 10, 2, 12, 9, 11, 6, 14, 0, 15, 7 },
                    { 10, 1, 15, 11, 9, 7, 5, 16, 14, 3, 4, 0, 13, 6, 0, 8 },};




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
    solveSudoku(&start);
  } else{
    solveSudoku(nullptr);
  }
  if (rank == 0){
    double endtime   = MPI_Wtime();
    printf("That took %f seconds\n",endtime-starttime);
    cout<<endl<<endl;
  }
  MPI_Barrier(MPI_COMM_WORLD);
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