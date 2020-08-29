#include "TopKCalculator.h"
#include "AppUtils.h"
#include "CommonUtils.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <set>
#include <cmath>
#include <algorithm>
#include <queue>
#include <fstream>
#include <ctime>
#include <thread>
#include <atomic>
#define METAPATHS_DIR "./mp2vec-Clf/topKMPs/"
using namespace std;

atomic<int> atomic_counter (0);
HIN_Graph hin_graph_;
vector<thread> threads_pool;

void readTrainingPairs(string filename, vector<pair<int, vector<int> > >& pairs){
	pairs.clear();
	ifstream pairsIn(filename.c_str(), ios::in);
	if(!pairsIn.good()){
		cerr << "Error when reading training pairs" << endl;
	}
	string line;
	while(getline(pairsIn, line)){
		vector<string> strs = split(line, "\t");
		if(strs.size() < 2){
			cerr << "Unsupported format for this  pairs: " << line << endl;
		}else{
			int src = atoi(strs[0].c_str());
			vector<string> str_dsts = split(strs[1],",");
			vector<int> dsts;
			for(int i = 0;i < str_dsts.size(); i++){
				dsts.push_back(atoi(str_dsts[i].c_str()));	
			}	
			pairs.push_back(make_pair(src,dsts));
		}
	}
	pairsIn.close();
}

void getTopKMetaPath(string filename,int src, int dst, int top_k){
	vector<pair<vector<double>, vector<int>>> topKMetaPaths;
	ifstream fin(filename.c_str());
	if(!fin.good()){
		fin.close();
		topKMetaPaths = TopKCalculator::getTopKMetaPath_TFIDF(src, dst, top_k, hin_graph_);
		ofstream fout(filename.c_str());
		for(int k = 0; k < topKMetaPaths.size(); k++){
			fout << topKMetaPaths[k].first[0]  << ":" << TopKCalculator::metapathToString(topKMetaPaths[k].second) << endl;;
		}
		fout.close();
	}else{
		topKMetaPaths.clear();
		string line;
		while(getline(fin, line)){
			vector<string> tmp = split(line, ":");
			double score = stof(tmp[0]);
			vector<int> meta_path = TopKCalculator::stringToMetapath(tmp[1]);
			topKMetaPaths.push_back(make_pair(vector<double> (1, score), meta_path));	
		}	
		fin.close();
	}
}

int main(int argc, const char * argv[]) {

	map<int, vector<Edge>> adj;
	map<int, string> node_name;
	map<int, string> node_type_name;
	map<int, int> node_type_num;
	map<int, vector<int>> node_id_to_type;
	map<int, string> edge_name;

	string dataset;
	int num_threads = 1;
	int walk_length = 100;
	if(argc == 1){
		cout << "Usage: ./genRandomWalk dataset [#threads=1]" << endl;
	}
	if(argc >= 2){
		dataset = argv[1];	
		if(dataset.compare("DBLP") != 0 && dataset.compare("ACM") != 0){
			cout << " Unsupported dataset" << endl;
			return 0;
		}
	}
	if(argc >= 3){
		num_threads = atoi(argv[2]);
		if(num_threads < 1){
			cout << "#threads has to be larger than 0" << endl;
			return 0;
		}
		cout << "Running with " << num_threads << " threads.. " << endl;
	}

	hin_graph_ = loadHinGraph(dataset.c_str(), node_name, adj, node_type_name, node_type_num, node_id_to_type, edge_name);
	loadMetaInfo(dataset.c_str(), hin_graph_);

	int top_k = 5;
	int clf_k_mps = 3;
	double beta = 0.1;
	int penalty_type = 1;

	vector<pair<int, vector<int>>> training_pairs;	
	vector<int> test_entities;
	readTrainingPairs(dataset+"/train_pairs.txt", training_pairs);	



	vector<string> score_types;
	score_types.push_back("MNIS");
	score_types.push_back("SLV1");
	score_types.push_back("SLV2");
	score_types.push_back("SMP");

	int training_pairs_size = training_pairs.size();
        for(int j = 0; j < score_types.size(); j++){

			string score_type = score_types[j];
			scoreSetup(score_type.c_str(), penalty_type, beta, true);
		for(int i = training_pairs_size - 1; i >= 0; i--){
			int src = training_pairs[i].first;
			vector<int> dsts = training_pairs[i].second;

		

			for(int dst_i = 0; dst_i < dsts.size(); dst_i++){
				int dst = dsts[dst_i];
				string filename(METAPATHS_DIR);
				filename += "/" + dataset + "_" + to_string(src)+"_"+to_string(dst)+"_"+to_string(top_k)+"_"+score_type+"_"+to_string(beta)+".txt";
				int counter = atomic_counter++;
				int thd_idx = counter%num_threads;
				if(thd_idx >= threads_pool.size()){
					threads_pool.push_back(thread(getTopKMetaPath, filename, src, dst, top_k));
				}else{
					threads_pool[thd_idx].join();
					threads_pool[thd_idx] = thread(getTopKMetaPath, filename, src, dst, top_k);
			        }
				
			}

			
		}
		for(auto &t:threads_pool){
			t.join();
		}
			
		atomic_counter = 0;

		
	}

	
	return 0;
}
