#include <iostream>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <functional>
#include <vector>

using namespace std;

int **A;

void WarmUp(int A[], int len){
	for(int i = 0; i < len; i++)
		A[i] = 0;
}

typedef void(*Work)(int A[], int len, int itr);

//Continous accessing data in array
void LinearAccessPattern(int A[], int len, int itr){
	for(int i = 0; i < itr; i++)
		for(int j = 0; j < len; j++)
			A[j]++;
}

void StripLinePattern(int A[], int len, int itr){
	// 64/4=16
	for(int k = 0; k < 16; k++)
		for(int i = 0; i < itr; i++)
			for(int j = 0; j < len; j += 16)
				A[j]++;
}

void StripPagePattern(int A[], int len, int itr){
	// 4096/4=1024
	for(int k = 0; k < 1024; k++)
		for(int i = 0; i < itr; i++)
			for(int j = 0; j < len; j += 1024)
				A[j]++;
}

void MyProfile(int A[], int len, int ITR, Work w){
	chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();

	w(A, len, ITR);

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cout << "elapsed time: " << elapsed_seconds.count() << "s" << endl;
}

void verify(int **A, int C, int ITR){
	for(int i = 0; i < C; i++)
		if(A[i][0] != ITR){
			cerr << "ERROR Result" << endl;
			return;
		}
}

int64_t CanonicalSize(int64_t size, char &c){
	c = 'B';
	if((size >> 30)){
		size >>= 30;
		c = 'G';
	}
	else if(size >> 20){
		size >>= 20;
		c = 'M';
	}
	else if(size >> 10){
		size >>= 10;
		c = 'K';
	}
	return size;
}

string PrintCanonicalSize(int64_t size){
	char c = 'a';
	size = CanonicalSize(size, c);
	return to_string(size) + c;
}

void PrintSetup(int C, int64_t LEN, int ITR){
	int64_t work = LEN * ITR;
	int64_t workTotal = work * C;

	cout << "Number of Threads : " << C << endl;
	cout << "Array Size : " << PrintCanonicalSize(LEN << 2) << endl;
	cout << "Number of Iterations : " << PrintCanonicalSize(ITR) << endl;
	cout << "Access Count Per Thread: " << PrintCanonicalSize(LEN * ITR) << endl;
	cout << "Sum of Access : " << PrintCanonicalSize(C * LEN * ITR) << endl;
}

void RunTest(int C, int LEN, int ITR, Work w[]){
	LEN = 1 << LEN;
	ITR = 1 << ITR;

	PrintSetup(C, LEN, ITR);
	A = new int* [C];
	for(int i = 0; i < C; i++){
		A[i] = new int[LEN];
		WarmUp(A[i], LEN);
	}

	thread **T = new thread*[C];
	for(int i = 0; i < C; i++)
		T[i] = new thread(MyProfile, A[i], LEN, ITR, w[i]);
	for(int i = 0; i < C; i++)
		T[i]->join();
	//verify(A, C, ITR);

	for(int i = 0; i < C; i++)
		delete A[i];
}

void tiny1_1(int C){
	// Access 1G per thread, all threads are StripLinePattern,
	int work = 30;
	Work w[C];;
	fill(w, w+C, StripLinePattern);

	for(int LEN = 10; LEN <= min(27, work); LEN++){
		int ITR = work-LEN;
		RunTest(C, LEN, ITR, w);
		cout << endl;
	}
}

void tiny1_2(int C){
	//Same as tiny1_1, LinearAccessPattern
	int work = 30;
	Work w[C];;
	fill(w, w+C, LinearAccessPattern);

	for(int LEN = 10; LEN <= min(27, work); LEN++){
		int ITR = work-LEN;
		RunTest(C, LEN, ITR, w);
		cout << endl;
	}
}

void tiny1_3(int C){
	//Same as tiny1_1, StripPagePattern
	int work = 30;
	Work w[C];;
	fill(w, w+C, StripPagePattern);

	for(int LEN = 10; LEN <= min(27, work); LEN++){
		int ITR = work-LEN;
		RunTest(C, LEN, ITR, w);
		cout << endl;
	}
}

void tiny1_4(int C){
	int work = 30;
	Work w[C];;
	for(int i = 0; i < C; i++){
		if(i & 1)
			w[i] = LinearAccessPattern;
		else
			w[i] = StripLinePattern;
	}

	for(int LEN = 10; LEN <= min(27, work); LEN++){
		int ITR = work-LEN;
		RunTest(C, LEN, ITR, w);
		cout << endl;
	}
}

void tiny1_5(int C){
	int work = 30;
	Work w[C];;
	for(int i = 0; i < C; i++){
		if(i & 1)
			w[i] = LinearAccessPattern;
		else
			w[i] = StripPagePattern;
	}

	for(int LEN = 10; LEN <= min(27, work); LEN++){
		int ITR = work-LEN;
		RunTest(C, LEN, ITR, w);
		cout << endl;
	}
}

void PrintHelpInfo(){
	cout << "arguments:" << endl;
	cout << "\tc : number of thraeds" << endl;
	cout << "\tl : length of array" << endl;
	cout << "\ti : iteration" << endl;
	cout << "\tp : run benchmark configuration" << endl;
}

vector< function<void(int)> > BenchRegister;

void BenchRegist(){
	BenchRegister.push_back(function<void(int)>(tiny1_1));
	BenchRegister.push_back(function<void(int)>(tiny1_2));
	BenchRegister.push_back(function<void(int)>(tiny1_3));
	BenchRegister.push_back(function<void(int)>(tiny1_4));
	BenchRegister.push_back(function<void(int)>(tiny1_5));
}

int main(int argc, char** argv){
	int ch;

	int C = 1;
	int LEN = 10;
	int ITR = 10;
	int P = 0;
	while((ch=getopt(argc, argv, "c:l:i:p:h")) !=-1 ){
		switch(ch){
			case 'c':
				C = atoi(optarg);
				break;
			case 'l':
				LEN = atoi(optarg);
				break;
			case 'i':
				ITR = atoi(optarg);
				break;
			case 'p':
				P = atoi(optarg);
				break;
			case 'h':
				PrintHelpInfo();
				break;
			default:
				cout << "Unrecongnized arguments : " << (char)(ch) << endl;

		}
	}
	//int LEN = atoi(argv[2]);
	//int ITR = atoi(argv[3]);

	//RunTest(C, LEN, ITR);
	//TestIdenticalWork(30, C);

	--P;
	BenchRegist();
	if(P < BenchRegister.size() && P >= 0)
		BenchRegister[P](C);
	else
		cerr << "NO Bench Select" << endl;

	return 0;
}
