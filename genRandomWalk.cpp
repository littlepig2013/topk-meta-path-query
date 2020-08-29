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

#define METAPATHS_DIR "./mp2vec-Clf/topKMPs/"
#define ONE_HOP_NEIGHBORS_DIR "./mp2vec-Clf/one_hop_neighbors/"
#define RANDOMWALKS_DIR "./mp2vec-Clf/RandomWalks/"
using namespace std;

int** Mat;
int** tmpMat;
map<int, int> eid2idx;
map<int, int> nextEid2idx;
vector<int> nextEids;
map<string, map<int, vector<pair<int, int> > > > MP2OneHopNeighbors;
vector<thread> threads_pool;
class ScoreMinPairDoubleStringCmp
{
public:
	bool operator () (pair<double, string> & scorePair1, pair<double, string> & scorePair2){
		return scorePair1.first >= scorePair2.first;
	}

};



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

void readTestingEntities(string filename, vector<int>& test_entities){
	test_entities.clear();
	ifstream fin(filename.c_str(), ios::in);
	if(!fin.good()){
		cerr << "Error when reading testing entities" << endl;
	}
	string line;
	while(getline(fin, line)){
		vector<string> strs = split(line, "\t");
		int src = atoi(strs[0].c_str());
		test_entities.push_back(src);
	}
	fin.close();
}

void getTopKMetaPath(string filename,int src, int dst, int top_k, HIN_Graph & hin_graph_, vector<pair<vector<double>, vector<int>>> & topKMetaPaths ){
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
		int line_idx = 0;
		string line;
		while(getline(fin, line) && line_idx < top_k){
			vector<string> tmp = split(line, ":");
			double score = stof(tmp[0]);
			vector<int> meta_path = TopKCalculator::stringToMetapath(tmp[1]);
			topKMetaPaths.push_back(make_pair(vector<double> (1, score), meta_path));	
			line_idx++;
		}	
		fin.close();
	}
}


void readFixedMPs(string filename, vector<string> & fix_mps){
	fix_mps.clear();
	ifstream fin(filename.c_str(), ios::in);
	if(!fin.good()){
		cerr << "Error when reading fixed meta paths" << endl;
	}
	string line;
	while(getline(fin, line)){
		fix_mps.push_back(line);
	}
	fin.close();
}

void recTopKMPs(map<string, double> metapath2Score, vector< pair<double, string> > & mps, int k){
	
	priority_queue<pair<double, string>, vector<pair<double, string>>, ScoreMinPairDoubleStringCmp> q;	
	for(auto iter = metapath2Score.begin(); iter != metapath2Score.end(); iter++){
		if(q.size() == k){
			double minScore = q.top().first;
			if(minScore > iter->second){
				continue;
			}else if(minScore == iter->second){
				double randomNumber = (double)rand()/RAND_MAX;
				if(randomNumber <= 0.5) continue;
			}
			q.pop();
		}
		q.push(make_pair(iter->second, iter->first));
	}
	mps.clear();
	int i = k - 1;
	double weight = 0.0; // normalize
	while(!q.empty()){
		mps.push_back(q.top());
		weight += q.top().first;
		q.pop();
		i--;
	}
	for(int i = 0; i < k; i++){
		mps[i] = make_pair(mps[i].first*1.0/weight, mps[i].second);
	}
}

inline int getNextNumNodes(int edge_type, HIN_Graph & hin_graph_){
	int total = 0;
	set<int> node_types;
	int abs_edge_type = abs(edge_type);
	if(edge_type > 0){
		node_types = hin_graph_.edge_type_node_types_[abs_edge_type].second;
	}else{
		node_types = hin_graph_.edge_type_node_types_[abs_edge_type].first;
	}
	for(auto type:node_types){
		total += hin_graph_.node_type_num_[type];
	}	
	return total;
}

// !!! ONLY SUPPORT META PATH WITH LENGTH LONGER THAN 1
void getAdjMat(vector<int> & total_entities, vector<int> meta_path, HIN_Graph & hin_graph_){

	int num_total_entities = total_entities.size();
	int num_next_nodes = getNextNumNodes(meta_path[0], hin_graph_);	
	int num_next_next_nodes = getNextNumNodes(meta_path[1], hin_graph_);	

	if(tmpMat == NULL) tmpMat = new int*[num_total_entities];
	for(int i = 0; i < num_total_entities; i++){
		Mat[i] = new int[num_next_nodes];
		tmpMat[i] = new int[num_next_next_nodes];
		memset(Mat[i], 0, num_next_nodes*4);
		memset(tmpMat[i], 0, num_next_next_nodes*4);
	}

	map<int,vector<int> > neighbors;
	int next_eid_idx = 0;
	for(int i = 0; i < total_entities.size(); i++){
		
		set<int> tmp_next_entities;
		SimCalculator::getNextEntities(total_entities[i], meta_path[0], tmp_next_entities, hin_graph_);
		neighbors[i] = vector<int> ();
		int idx;
		for(auto eid:tmp_next_entities){
			if(nextEid2idx.find(eid) == nextEid2idx.end()){
				nextEid2idx[eid] = next_eid_idx;
				nextEids.push_back(eid);
				next_eid_idx++;
			}
			idx = nextEid2idx[eid];
			neighbors[i].push_back(idx);
			Mat[i][idx]++;
		}
	}

	int edge_idx = 1;
	/*
	if(meta_path[1] + meta_path[0] == 0){
		for(int i = 0; i < num_total_entities; i++){
			for(int j = i; j < num_total_entities; j++){
				for(auto idx:neighbors[i]){
					tmpMat[i][j] += Mat[j][idx];
				}
				tmpMat[j][i] = tmpMat[i][j];
			}
		}
		edge_idx++;
		for(int i = 0; i < num_total_entities; i++){

			delete[] Mat[i];
			Mat[i] = NULL;
		}
		delete[] Mat;
		Mat = NULL;
		Mat = tmpMat;
	}*/

	map<int,vector<int> > tmp_neighbors;
	map<int, int> tmp_nextEid2idx;
	vector<int> tmp_nextEids;
	int tmp_next_eid_idx ;
	for(;edge_idx < meta_path.size(); edge_idx++){
		tmp_next_eid_idx = 0;
		tmp_nextEid2idx.clear();
		tmp_nextEids.clear();
		for(auto iter = tmp_neighbors.begin(); iter != tmp_neighbors.end(); iter++){
			iter->second.clear();
		}
		tmp_neighbors.clear();

		
		num_next_next_nodes = getNextNumNodes(meta_path[edge_idx], hin_graph_);

		for(auto iter = nextEid2idx.begin(); iter != nextEid2idx.end(); iter++){
			set<int> tmp_next_entities;
			SimCalculator::getNextEntities(iter->first, meta_path[edge_idx], tmp_next_entities, hin_graph_);
			tmp_neighbors[iter->second] = vector<int> ();
			int idx;
			for(auto eid:tmp_next_entities){
				if(tmp_nextEid2idx.find(eid) == tmp_nextEid2idx.end()){
					tmp_nextEids.push_back(eid);
					tmp_nextEid2idx[eid] = tmp_next_eid_idx;
					tmp_next_eid_idx++;
				}
				idx = tmp_nextEid2idx[eid];
				tmp_neighbors[iter->second].push_back(idx);
			}
		}

		tmpMat = new int*[num_total_entities];			
		for(int i = 0; i < num_total_entities; i++){
			tmpMat[i] = new int[num_next_next_nodes];
			memset(tmpMat[i], 0, num_next_next_nodes*4);

			for(int j = 0; j < num_next_nodes; j++){
				if(Mat[i][j] > 0){
					if(tmp_neighbors.find(j) != tmp_neighbors.end()){
						for(auto idx:tmp_neighbors[j]){	
							tmpMat[i][idx] += Mat[i][j];
						}	
					}
				}	
			}
		}

		for(int i = 0; i < num_total_entities; i++){
			delete[] Mat[i];
			Mat[i] = NULL;
		}
		delete[] Mat;
		Mat = NULL;
		Mat = tmpMat;
		tmpMat = NULL;
		num_next_nodes = num_next_next_nodes;

		nextEid2idx.clear();
	        nextEid2idx = tmp_nextEid2idx;	
		nextEids = tmp_nextEids;
	}

}

void getWeightedDstEntities(int src, vector<int> meta_path, map<int, double> & dst_entities, double alpha, HIN_Graph & hin_graph_){
	dst_entities.clear();
	map<int, double> curr_entities;
	map<int, double> next_entities;

	curr_entities[src] = 1.0;
	for(int i = 0; i < meta_path.size(); i++){
		for(auto iter = curr_entities.begin(); iter != curr_entities.end(); iter++){
			set<int> tmp_next_entities;
			SimCalculator::getNextEntities(iter->first, meta_path[i], tmp_next_entities, hin_graph_);
			double weight = 1.0/pow(tmp_next_entities.size(), alpha);
			for(set<int>::iterator iter_n = tmp_next_entities.begin(); iter_n != tmp_next_entities.end(); iter_n++){
				int eid = *iter_n;
				if(next_entities.find(eid) == next_entities.end()){
					next_entities[eid] = 0.0;
				}
				next_entities[eid] += weight*iter->second;
			}
		}

		
		curr_entities = next_entities;
		next_entities.clear();
	}
		
	dst_entities = curr_entities;
	
}

void loadMP2OneHopNeighbors(string filename, set<string> metapaths, vector<int> & total_entities, HIN_Graph & hin_graph_){
	if(MP2OneHopNeighbors.size() == 0){
		ifstream fin(filename.c_str());
		if(fin.good()){
			string line;
			int line_number = 0;
			string curr_meta_path = "";
			while(getline(fin, line)){
				if(line_number%1000==0) cout << line_number << endl;
				line_number++;
				vector<string> tmp = split(line,":");
				if(tmp[0].compare("meta path") == 0){
					curr_meta_path = tmp[1];
					MP2OneHopNeighbors[curr_meta_path] = map<int, vector<pair<int, int> > > ();
				}else{
					if(curr_meta_path.compare("") != 0){
						int src = atoi(tmp[0].c_str());
						MP2OneHopNeighbors[curr_meta_path][src] = vector<pair<int, int> > ();
						vector<string> dst_entities_str = split(tmp[1],",");
						int acc = 0;
						for(string dst_weight:dst_entities_str){
							vector<string> dst_weight_vec = split(dst_weight, "-");
							int dst = atoi(dst_weight_vec[0].c_str());
							int num = atoi(dst_weight_vec[1].c_str());
							acc += num;
							MP2OneHopNeighbors[curr_meta_path][src].push_back(make_pair(dst, acc));
								
						}
					}
				}
				
			}
		}
		fin.close();
	}
	int num_total_entities = total_entities.size();
	for(auto iter = metapaths.begin(); iter != metapaths.end(); iter++){
		vector<int> meta_path = TopKCalculator::stringToMetapath(*iter);
		if(MP2OneHopNeighbors.find(*iter) == MP2OneHopNeighbors.end()){
			cout << "Generating one-hop neighbors for: " << *iter << endl;
			MP2OneHopNeighbors[*iter] = map<int, vector<pair<int, int> > > ();
			if(meta_path.size() >= 2){
				getAdjMat(total_entities, meta_path, hin_graph_);
				for(int i = 0; i < total_entities.size(); i++){
					MP2OneHopNeighbors[*iter][total_entities[i]] = vector<pair<int, int> > ();

					int acc = 0;
					for(int  j = 0; j < total_entities.size(); j++){
						if(Mat[i][j]){
							acc += Mat[i][j];
							MP2OneHopNeighbors[*iter][total_entities[i]].push_back(make_pair(nextEids[j], acc));
						}
					}	
					delete[] Mat[i];
					Mat[i] = NULL;
				}
				tmpMat = NULL;
			}else{
				for(int entity:total_entities){

					map<int, double> dst_entities;
					getWeightedDstEntities(entity, meta_path, dst_entities,0, hin_graph_);
					MP2OneHopNeighbors[*iter][entity] = vector<pair<int, int> >();
					int acc = 0;
					for(auto iter_dst = dst_entities.begin(); iter_dst != dst_entities.end(); iter_dst++){
						acc += iter_dst->second;
						MP2OneHopNeighbors[*iter][entity].push_back(make_pair(iter_dst->first,acc));
					}
					
				}
			}
		
			ofstream fout(filename.c_str(),ofstream::out | ofstream::app);	
			fout << "meta path" << ":" << *iter << endl;	
			map<int, vector<pair<int, int> > > oneHopNeighbors = MP2OneHopNeighbors[*iter];
			for(auto iter_src = oneHopNeighbors.begin(); iter_src != oneHopNeighbors.end(); iter_src++){
			
				vector<pair<int, int> > dsts = iter_src->second;
				if(dsts.size() == 0) continue;

				fout << iter_src->first << ":";
				fout << dsts[0].first << "-" << dsts[0].second;
				int prev = dsts[0].second;
				for(int i = 1; i < dsts.size(); i++){
					fout << "," << dsts[i].first << "-" << dsts[i].second - prev;
					prev = dsts[i].second;
					
				}
				fout << endl;

			}
			oneHopNeighbors.clear();
			
			fout.close();
		}
		/*
		if(meta_path.size() > 5){
			MP2OneHopNeighbors[*iter].clear();
			MP2OneHopNeighbors.erase(*iter);
		}*/
	}
	
	
	
}

inline int binarySearch(int target, vector<pair<int, int> > & vec){

	int start = 0;
	int end = vec.size() - 1;
	int mid = (start + end)/2;
	while(end - start > 1){
		if(target < vec[mid].second){
			if(mid == 0){
				return vec[0].first;
			}else if(target >= vec[mid-1].second){
				return vec[mid].first;
			}else{
				end = mid;
				mid = (start + end)/2;
			}
		}else{
			if(mid == vec.size() - 1){
				return vec.back().first;
			}else if(target < vec[mid + 1].second){
				return vec[mid].first;
			}else{
				start = mid;
				mid = (start + end)/2;
			}
		}
		
	}
	if(target < vec[start].second){
		return vec[start].first;
	}else{
		return vec[end].first;
	}
}

void genRandomWalks(int src, vector<pair<string, double> >* weighted_mps, int num_walks, int walk_length, string filename){
	ofstream fout(filename.c_str(), ofstream::out|ofstream::app);
	vector<string> segmented_mps;
	vector<double> segmented_points;
	double weight = 0;
	int clf_k_mps = weighted_mps->size();
	for(int i = 0; i < clf_k_mps; i++){
		weight += weighted_mps->at(i).second;	
		segmented_points.push_back(weight);
		segmented_mps.push_back(weighted_mps->at(i).first);
	}
	double x;
	int mp_i;
	string metapath;
	vector<pair<int, int> > neighbors;
	int curr_node, next_node, last_node;

	for(int i = 0; i < num_walks; i++){

		// choose meta path
		x = (double) rand()/RAND_MAX;
		mp_i = 0;
		for(; mp_i < clf_k_mps; mp_i++){
			if(x < segmented_points[mp_i]){
				break;
			}
		}
		metapath = segmented_mps[mp_i];
	

		// generate one random walk	
		curr_node = src;
		last_node = src;
		if(MP2OneHopNeighbors[metapath].find(src) == MP2OneHopNeighbors[metapath].end()){
			continue;
		}
		if(MP2OneHopNeighbors[metapath][src].size() == 0){
			break;
		}
		fout << "a" << src;
		for(int j = 0; j < walk_length; j++){
			x = (double) rand()/RAND_MAX;
			neighbors = MP2OneHopNeighbors[metapath][curr_node];
			if(neighbors.size()== 0){
				next_node = last_node;
			}else{
				next_node = binarySearch((int) round(x*neighbors.back().second), neighbors);
			}
			fout << " a" << next_node;
			last_node = curr_node;
			curr_node = next_node;
		}
		fout << endl;
	}
	fout.close();
}

int main(int argc, const char * argv[]) {
	HIN_Graph hin_graph_;

	map<int, vector<Edge>> adj;
	map<int, string> node_name;
	map<int, string> node_type_name;
	map<int, int> node_type_num;
	map<int, vector<int>> node_id_to_type;
	map<int, string> edge_name;

	string dataset;
	int num_walks = 100;
	int walk_length = 100;
	bool fix_mp_switcher = false;
	if(argc == 1){
		cout << "Usage: ./genRandomWalk dataset [num_walks] [walk_length] [fix_mp_switcher]" << endl;
	}
	if(argc >= 2){
		dataset = argv[1];	
		if(dataset.compare("DBLP") != 0 && dataset.compare("ACM") != 0){
			cout << " Unsupported dataset" << endl;
			return 0;
		}
	}
	if(argc >= 3){
		num_walks = atoi(argv[2]);
	}
	if(argc >= 4){
		walk_length = atoi(argv[3]);
	}
	if(argc >= 5){
		                fix_mp_switcher = atoi(argv[4]) == 1 ? true : false;
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
	readTestingEntities(dataset+"/test_entities.txt", test_entities);	
	set<int> similarNodes = TopKCalculator::getSimilarNodes(training_pairs[0].first, hin_graph_.nodes_, false, false);
	vector<int> total_entities (similarNodes.begin(), similarNodes.end());

	set<string> metapathsForRandomWalks;

	int num_total_entities = total_entities.size();
	Mat = new int*[num_total_entities];
	tmpMat = new int*[num_total_entities];

	vector<string> fix_mps;
	if(fix_mp_switcher){
		readFixedMPs(dataset+"/fix_mps.txt",fix_mps);
		for(string mp:fix_mps){
			metapathsForRandomWalks.insert(mp);
		}
		for(string metapath:metapathsForRandomWalks){
			pair<string, double> mp_pair = make_pair(metapath, 1.0);
			vector<pair<string, double> > mp_pair_vec;
			mp_pair_vec.push_back(mp_pair);
			string filename(RANDOMWALKS_DIR);
			filename += "/FIX_MP_" + dataset;
			vector<int> meta_path = TopKCalculator::stringToMetapath(metapath);
			cout << "Generating one-hop neighbors for: " << metapath << endl;
			MP2OneHopNeighbors[metapath] = map<int, vector<pair<int, int> > > ();
			if(meta_path.size() >= 2){
				getAdjMat(total_entities, meta_path, hin_graph_);
				for(int i = 0; i < total_entities.size(); i++){
					MP2OneHopNeighbors[metapath][total_entities[i]] = vector<pair<int, int> > ();

					int acc = 0;
					for(int  j = 0; j < total_entities.size(); j++){
						if(Mat[i][j]){
							acc += Mat[i][j];
							MP2OneHopNeighbors[metapath][total_entities[i]].push_back(make_pair(nextEids[j], acc));
						}
					}	
					delete[] Mat[i];
					Mat[i] = NULL;
				}
				tmpMat = NULL;
			}else{
				for(int entity:total_entities){

					map<int, double> dst_entities;
					getWeightedDstEntities(entity, meta_path, dst_entities,0, hin_graph_);
					MP2OneHopNeighbors[metapath][entity] = vector<pair<int, int> >();
					int acc = 0;
					for(auto iter_dst = dst_entities.begin(); iter_dst != dst_entities.end(); iter_dst++){
						acc += iter_dst->second;
						MP2OneHopNeighbors[metapath][entity].push_back(make_pair(iter_dst->first,acc));
					}
					
				}
			}

			for(int i = 0; i < meta_path.size(); i++){
				filename += "-" + to_string(meta_path[i]);
			}
			filename += ".txt";
			//ofstream tmp_fout(filename.c_str(), ofstream::out|ofstream::app);
			for(int i = 0; i < training_pairs.size(); i++){
				int entity = training_pairs[i].first;
				genRandomWalks(entity,&mp_pair_vec, num_walks, walk_length, filename);
			}

		}
		return 0;
	}
	for(int i = 0; i < total_entities.size(); i++){
		eid2idx[total_entities[i]] = i;
		//Mat[i] = new int[num_total_entities];
		//tmpMat[i] = new int[num_total_entities];
	}

	vector<string> score_types;
	score_types.push_back("MNIS");
	score_types.push_back("SLV1");
	score_types.push_back("SLV2");
	score_types.push_back("SMP");

	int training_pairs_size = training_pairs.size();
	bool thread_init_flag = true;
	for(int i = 0; i < training_pairs_size; i++){
		int src = training_pairs[i].first;
		vector<int> dsts = training_pairs[i].second;


		map<string, vector<pair<string, double> > > type2TopKMPs;
		map<string, set<int> > MP2DstEntities;

		clock_t t1, t2;
		t1 = clock();
		for(int j = 0; j < score_types.size(); j++){

			string score_type = score_types[j];
			type2TopKMPs[score_type] = vector<pair<string, double> > ();
			map<string, double> currExploredMPs;
			scoreSetup(score_type.c_str(), penalty_type, beta, true);

			for(int dst_i = 0; dst_i < dsts.size(); dst_i++){
				int dst = dsts[dst_i];
				string filename(METAPATHS_DIR);
				filename += "/" + dataset + "_" + to_string(src)+"_"+to_string(dst)+"_"+to_string(top_k)+"_"+score_type+"_"+to_string(beta)+".txt";
				vector<pair<vector<double>, vector<int>>> topKMetaPaths;
				getTopKMetaPath(filename, src, dst, top_k, hin_graph_, topKMetaPaths);

				for(int k = 0; k < top_k; k++){
					vector<int> meta_path = topKMetaPaths[k].second;
					string meta_path_str = TopKCalculator::metapathToString(meta_path);
					if(currExploredMPs.find(meta_path_str) == currExploredMPs.end()){

						currExploredMPs[meta_path_str] =0.0;
					}
					currExploredMPs[meta_path_str] += topKMetaPaths[k].first[0];
					//currExploredMPs[meta_path_str] += 1.0;
				}
			}
			vector< pair<double, string> > recMPs;
			recTopKMPs(currExploredMPs, recMPs, clf_k_mps);	
			for(int mp_i = 0; mp_i < recMPs.size(); mp_i++){
				//cout << "mp i : " << mp_i << "\t" << meta_path_str << endl;
				type2TopKMPs[score_type].push_back(make_pair(recMPs[mp_i].second, recMPs[mp_i].first));
				metapathsForRandomWalks.insert(recMPs[mp_i].second);
			}
		}

		
		t2 = clock();
		
		cout << "Take " << (0.0 + t2 - t1) / CLOCKS_PER_SEC << "s to query top-k meta-paths" << endl;

		// generate random walks
		string one_hop_neighbors_filename(ONE_HOP_NEIGHBORS_DIR) ;
		one_hop_neighbors_filename += "/" + dataset + ".txt";
		loadMP2OneHopNeighbors(one_hop_neighbors_filename, metapathsForRandomWalks, total_entities, hin_graph_);
		if(thread_init_flag){
			for(int j = 0; j < score_types.size(); j++){
				string score_type = score_types[j];
				string filename(RANDOMWALKS_DIR);
				filename += "/"+dataset + "_" + score_type+"_"+to_string(clf_k_mps)+".txt";
				threads_pool.push_back(thread(genRandomWalks, src, &(type2TopKMPs[score_types[j]]), num_walks, walk_length, filename));
			}
			thread_init_flag = false;
		}else{
			for(int j = 0; j < score_types.size(); j++){
				string score_type = score_types[j];
				string filename(RANDOMWALKS_DIR);
				filename += "/"+dataset + "_" + score_type+"_"+to_string(clf_k_mps)+".txt";
				threads_pool[j] = thread(genRandomWalks, src,&(type2TopKMPs[score_types[j]]), num_walks, walk_length, filename);
			}
                }
		
		for(auto &t: threads_pool){
			t.join();
		}
	}

	threads_pool.clear();
	
	return 0;
}
