#include "TopKCalculator.h"
#include "AppUtils.h"
#include "CommonUtils.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <queue>
#include <fstream>
#include <ctime>

#define F1_BETA 1
#define METAPATHS_RESULT_FILE "./Exp/metapaths_result"

using namespace std;
void output_metapath(vector<int> curr_metapath, map<int,string> edge_name){
	for(int j = 0; j < curr_metapath.size(); j++){
		int curr_edge_type = curr_metapath[j];
		if(curr_edge_type < 0){
			curr_edge_type = -curr_edge_type;
			if(edge_name.find(curr_edge_type) != edge_name.end()){
				cout << "[" << edge_name[curr_edge_type] << "]^(-1)" << "\t";
			}else{
				cout << "NA" << "\t";
			}
		}else{
			if(edge_name.find(curr_edge_type) != edge_name.end()){
				cout << "[" << edge_name[curr_edge_type] << "]" << "\t";	
			}else{
				cout << "NA" << "\t";
			}
		}
	}	
	cout << endl;	



}

void output_metapath_result(map<string, double> metapaths_result, string filename){
	ofstream metapathsResultOut(filename.c_str(), ios::out);
	for(auto iter = metapaths_result.begin(); iter != metapaths_result.end(); iter++){
		metapathsResultOut << iter->first << ";" << iter->second << endl;
	}	
	metapathsResultOut.close();
}

void load_metapath_result(map<string, double> & metapaths_result, string filename) {
	ifstream metapathsResultIn(filename.c_str(), ios::in);
	if(!metapathsResultIn.good()){
		return;
	}

	string line;
	while(getline(metapathsResultIn, line)){
		vector<string> strs = split(line, ";");
		metapaths_result[strs[0]] = stod(strs[1]);
	}
}

 
void output(vector<double> result, string metric){
	cout << endl;
	if(result.size() > 0){
		cout << "Metrics" << "\t" << metric << "\t" << metric << "@k" << endl;
		cout.precision(6);
		cout << fixed;
		for(int i = 0; i < result.size(); i++){
			cout << i+1 << "\t" << result[i] << "\t";
			double sum = 0.0;
			for(int j = 0; j <= i; j++){
				sum += result[j]; 
			}
			cout << sum/(i + 1) << endl;	
		}

	}else{
		cerr << "No Meta Pah Found in the HIN Graph." << endl;
	}

}

void printUsage(const char* argv[]){
	cout << "Usage: " << endl;
	cout << argv[0] << " --classifier dataset (positive pairs file name) (negative pairs file name) k TF-IDF-type length-penalty (beta)" << endl;
	cout << argv[0] << " --refine_classifier dataset (positive pairs file name) (negative pairs file name) k score-function" << endl;
	cout << argv[0] << " --metapath_classifier dataset (positive pairs file name) (negative pairs file name) (meta paths file name) " << endl;
	cout << endl;

	cout << "\t TF-IDF-type:" << endl;
	cout << "\t\t MNIS -> Strength-based M-S" << endl;
	cout << "\t\t SLV1 -> Strength & Length based Version 1" << endl;
        cout << "\t\t SLV2 -> Strength & Length based Version 2" << endl;
	cout << "\t\t SMP -> Shortest Meta Path" << endl;
	
	cout << "classifier mode:" << endl;
	cout << "\t length-penalty(l is the meta-path's length): " << endl;
	cout << "\t\t 1 -> 1/beta^l" << endl;
	cout << "\t\t 2 -> 1/factorial(l)" << endl;
	cout << "\t\t 3 -> 1/l" << endl;
	cout << "\t\t 4 -> 1/e^l" << endl;

	cout << endl;


	cout << "refine_classifier mode:" << endl;
	cout << "\t refine k meta-paths from shortest k' meta-paths (default k' is 15)  " << endl;
	cout << "\t score-function: 1 -> PCRW, 2 -> BPCRW" << endl;
	cout << endl;
}

bool readClassifierSampleFile(string pos_file, string neg_file, vector<pair<int, int>> & pos_pairs, vector<pair<int, int>> & neg_pairs){
	pos_pairs.clear();	
	neg_pairs.clear();
	//cout << train_pos_file << endl;
	ifstream posSamplesIn(pos_file.c_str(), ios::in);
	string pos_line;
	while(getline(posSamplesIn, pos_line)){
		vector<string> strs = split(pos_line, "\t");
		if(strs.size() != 2){
			cerr << "Unsupported format for the pair: " << pos_line << endl;
		}else{
			int src = atoi(strs[0].c_str());
			int dst = atoi(strs[1].c_str());
			pos_pairs.push_back(make_pair(src, dst));

		}

	}
	posSamplesIn.close();

	ifstream negSamplesIn(neg_file.c_str(), ios::in);
	string neg_line;
	while(getline(negSamplesIn, neg_line)){
		vector<string> strs = split(neg_line, "\t");
		if(strs.size() != 2){
			cerr << "Unsupported format for the pair: " << neg_line << endl;
		}else{
			int src = atoi(strs[0].c_str());
			int dst = atoi(strs[1].c_str());
			neg_pairs.push_back(make_pair(src, dst));

		}

	}
	negSamplesIn.close();
	

	return true;
		
}


void parseMetaPaths(vector<vector<int>> & metapaths, string metapaths_file){
	for(int i = 0; i < metapaths.size(); i++){
		metapaths[i].clear();
	}
	metapaths.clear();
	ifstream metapathsIn(metapaths_file.c_str(), ios::in);
	if(!metapathsIn.good()){
		cerr << "Error when reading positive pairs files" << endl;
	}
	string line;
	while(getline(metapathsIn, line)){
		vector<string> strs = split(line, "\t");	
		vector<int> tmp_metapath;
		tmp_metapath.clear();
		for(int i = 0; i < strs.size(); i++){
			tmp_metapath.push_back(atoi(strs[i].c_str()));
		}
		metapaths.push_back(tmp_metapath);
	}
	metapathsIn.close();
	

}

double getClassifierAccuracy(vector<int> meta_path, vector<pair<int, int>> pos_pairs, vector<pair<int, int>> neg_pairs, HIN_Graph & hin_graph_){

	int pos_hit_count = 0; 
	int neg_hit_count = 0;
	for(vector<pair<int, int>>::iterator iter = pos_pairs.begin(); iter != pos_pairs.end(); iter++){
		if(Bi_BFS_SimCalculator::isConnected(iter->first, iter->second, meta_path, hin_graph_)){
			pos_hit_count++;	
		}		
	}
	for(vector<pair<int, int>>::iterator iter = neg_pairs.begin(); iter != neg_pairs.end(); iter++){
		if(Bi_BFS_SimCalculator::isConnected(iter->first, iter->second, meta_path, hin_graph_)){
			neg_hit_count++;	
		}		
	}
	double accuracy = pos_hit_count*1.0/(pos_hit_count + neg_hit_count);
	return accuracy;
}


class MetaPathsCmp
{
public:
        bool operator () (pair<vector<int>, double> & p1,  pair<vector<int>, double> & p2);
};
	
bool MetaPathsCmp::operator () (pair<vector<int>, double> & p1,  pair<vector<int>, double> & p2){
	return p1.second < p2.second;
}

int main(int argc, const char * argv[]) {

	if(argc > 5){
		
		int penalty_type = DEFAULT_PENALTY_TYPE;
		double beta = DEFAULT_BETA;
		string score_type = DEFAULT_SCORE_TYPE;
		bool refine_flag = DEFAULT_REFINE_FLAG;
		int score_function = DEFAULT_SCORE_FUNCTION;
		bool fast_flag = DEFAULT_FAST_FLAG;
		string test_type;
		vector<vector<int>> metapaths;
			if(strcmp(argv[1], "--classifier") == 0 || strcmp(argv[1], "-c") == 0){
					test_type = "classifier";
					
					score_type = argv[6];
					if(argc > 7){
						penalty_type = atoi(argv[7]);
						if(penalty_type != 1 && penalty_type != 2){
							cerr << "The penalty_type parameter must be 1 or 2" << endl;
							return -1;
                                		}
					}
						
					if(argc > 8 && penalty_type == 1){
						beta = atof(argv[8]);
						if(beta <= 0 || beta >= 1){ 
							cerr << "The beta parameter must be greater than 0 and less than 1" << endl; 
							return -1;
						}							
					}
					scoreSetup(score_type.c_str(), penalty_type, beta, fast_flag);
			
				}else if(strcmp(argv[1], "--refine_classifier") == 0 || strcmp(argv[1], "-rc") == 0){
					score_type = "SP";
						if(argc > 6){
								score_function = atoi(argv[6]);
						}
						refine_flag = true;
					test_type = "classifier";
				}else if(strcmp(argv[1], "--metapath_classifier") == 0 || strcmp(argv[1], "-mc") == 0){
					string metapath_file_name = argv[5];
					test_type = "test_classifier";
					parseMetaPaths(metapaths, metapath_file_name);
									
				}else{
						printUsage(argv);
						return -1;
				}
		
		string dataset, pos_pairs_file_name, train_pos_pairs_file_name, train_neg_pairs_file_name;
			int k;
			dataset = argv[2];
			pos_pairs_file_name = argv[3];

		HIN_Graph hin_graph_;

			map<int, vector<Edge>> adj;
			map<int,string> node_name;
			map<int,string> node_type_name;
			map<int,int> node_type_num;
			map<int,vector<int>> node_id_to_type;
			map<int,string> edge_name;

			hin_graph_ = loadHinGraph(dataset.c_str(), node_name, adj, node_type_name, node_type_num, node_id_to_type, edge_name);

			 if(strcmp(score_type.c_str(), "MNIS") == 0 || strcmp(score_type.c_str(), "SLV1") == 0 || strcmp(score_type.c_str(), "SLV2") == 0 ){	
				loadMetaInfo(dataset, hin_graph_);
			}
		vector<pair<int, int>> pos_pairs;
		vector<pair<int, int>> neg_pairs;


			if(strcmp(test_type.c_str(), "test_classifier") == 0 && readClassifierSampleFile(pos_pairs_file_name, argv[4], pos_pairs, neg_pairs)){
				int metapath_num = metapaths.size();
					vector<double> accuracy_result (metapath_num, 0.0);
					for(int i = 0; i < metapath_num; i++){
						double accuracy = getClassifierAccuracy(metapaths[i], pos_pairs, neg_pairs, hin_graph_);	
						accuracy_result[i] = accuracy;
						output_metapath(metapaths[i], edge_name);	
						
					}
					output(accuracy_result, "Accuracy");
					return 0;
					

			}

			set<int> candidate_entities;
			
			k = atoi(argv[5]);
			vector<pair<int, int>> train_pos_pairs;
			vector<pair<int, int>> train_neg_pairs;
		
			if(strcmp(test_type.c_str(), "classifier") == 0 && !(readClassifierSampleFile(pos_pairs_file_name, argv[4], pos_pairs, neg_pairs) && readClassifierSampleFile(train_pos_pairs_file_name, train_neg_pairs_file_name, train_pos_pairs, train_neg_pairs))){
					return -1;
			}

		
		int sample_size = pos_pairs.size();
		vector<double> accuracy_result (k, 0.0);

		double time_cost = 0.0;
		map<string, double> metapaths_result;
		string metapath_output_file = METAPATHS_RESULT_FILE;
		metapath_output_file += "_" + dataset + ".txt";
		load_metapath_result(metapaths_result, metapath_output_file);

		for(int i = 0; i < pos_pairs.size(); i++){
					int src = pos_pairs[i].first;
					int dst = pos_pairs[i].second;
			
			vector<pair<vector<double>, vector<int>>> topKMetaPaths;
			clock_t t2, t1;
			t1 = clock();
					if(!refine_flag){
							topKMetaPaths = TopKCalculator::getTopKMetaPath_TFIDF(src, dst, k, hin_graph_);
					}else{
							int refineK = 15;
							topKMetaPaths = TopKCalculator::getTopKMetaPath_TFIDF(src, dst, refineK, hin_graph_);
							vector<vector<int>> meta_paths;
							for(int i = 0; i < refineK; i++){
								meta_paths.push_back(topKMetaPaths[i].second);
							}
							topKMetaPaths = TopKCalculator::getTopKMetaPath_Refiner(src, dst, k, meta_paths, score_function, hin_graph_);
					}
			t2 = clock();
			double curr_time_cost = (double) ((0.0 + t2 - t1)/CLOCKS_PER_SEC);
			cout << "Time cost for this pair is " << curr_time_cost << " seconds" << endl;
			time_cost += curr_time_cost;

			double total = 0.0;
			for(int i = 0; i < topKMetaPaths.size(); i++){
				vector<int> curr_meta_path = topKMetaPaths[i].second;
				if(strcmp(test_type.c_str(), "classifier") == 0){
					string meta_path_str = TopKCalculator::metapathToString(curr_meta_path);
					double accuracy;
					if(metapaths_result.find(meta_path_str) != metapaths_result.end()){
						accuracy = metapaths_result[meta_path_str];
					}else{
						accuracy = getClassifierAccuracy(curr_meta_path, pos_pairs, neg_pairs, hin_graph_);	
						metapaths_result[meta_path_str] = accuracy;
					}
					accuracy_result[i] += accuracy;
				}

			}
						
		}
		output_metapath_result(metapaths_result, metapath_output_file);
				//get top k meta paths for the training set
		for(int i = 0; i < k; i++){
			
				accuracy_result[i] /= pos_pairs.size();	
				
		}
		cout << "Average time cost is " << time_cost/sample_size << " seconds" << endl;


		output(accuracy_result, "Accuracy");

		
	}else{
		printUsage(argv);
		return -1;
	}		

	
	return 0;	

}
