#include <iostream>   
#include <fstream>
using namespace std;

#define N 16

int main () {
  std::ifstream ifs;
  int a;

  ifs.open ("plain_sudoku.in", std::ifstream::in);
  cout<<"{ ";
  for (int i = 0; i<N ; i++){
    cout<<"{ ";
    for (int j = 0; j<N; j++){
        ifs >> a;
        std::cout << a;
        if (j != N-1) cout<<", ";
    }
    cout<<" },"<<endl;
  }
  cout<<" }";

  ifs.close();

  return 0;
}