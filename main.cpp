#include "HIN_Graph.h"
#include "SimCalculator.h"
#include "yagoReader.h"
#include "TopKCalculator.h"
#include "AppUtils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>
#include <vector>

using namespace std;

void output(vector<pair<vector<double>, vector<int>>> topKMetaPaths, map<int, string> & edge_name, string output_file_name, bool refine_flag, int output_type) {
	if (topKMetaPaths.size() > 0) {
		if (output_type == 1 || output_type == 3) {
			bool invalid_edge_type_flag = false;
			if (refine_flag) {
				cout << "Score" << "\t\t";
			}
			else {
				cout << "Importance" << "\t\t" << "Support" << "\t\t" << "Rarity" << "\t";
			}
			cout << "length" << "\t" << "Meta Path" << endl;

			for (vector<pair<vector<double>, vector<int>>>::iterator iter = topKMetaPaths.begin(); iter != topKMetaPaths.end(); iter++) {
				vector<double> score_info = iter->first;
				cout.precision(2);
				cout << scientific;

				if (refine_flag) {
					cout << score_info[0] << "\t";
				}
				else {
					cout << score_info[0] << "\t";
					cout << score_info[1] << "\t";
					cout.precision(4);
					cout << fixed;
					cout << score_info[2] << "\t";
				}
				cout << iter->second.size() << "\t";

				vector<int> temp_meta_path = iter->second;
				for (vector<int>::iterator iter = temp_meta_path.begin(); iter != temp_meta_path.end(); iter++) {
					if (edge_name.find(*iter) == edge_name.end() && edge_name.find(-(*iter)) == edge_name.end()) {
						invalid_edge_type_flag = true;
						break;
					}
				}

				if (!invalid_edge_type_flag) {
					for (vector<int>::iterator iter = temp_meta_path.begin(); iter != temp_meta_path.end() - 1; iter++) {
						int curr_edge_type = *iter;
						if (curr_edge_type > 0) {
							cout << edge_name[curr_edge_type];
						}
						else {
							cout << "[" << edge_name[-curr_edge_type] << "]^(-1)";
						}
						cout << " -> ";
					}
					int curr_edge_type = temp_meta_path.back();
					if (curr_edge_type > 0) {
						cout << edge_name[curr_edge_type];
					}
					else {
						cout << "[" << edge_name[-curr_edge_type] << "]^(-1)";
					}
					cout << endl;
				}
				else {
					cout << "Found some Invalid Edge Type" << endl;
				}
			}
		}

		if (output_type == 2 || output_type == 3) {
			vector<vector<int>> meta_paths;
			for (vector<pair<vector<double>, vector<int>>>::iterator iter = topKMetaPaths.begin(); iter != topKMetaPaths.end(); iter++) {
				meta_paths.push_back(iter->second);
			}
			TopKCalculator::saveToFile(meta_paths, output_file_name);

		}

	}
	else {
		cerr << "No Meta Path Found in the HIN Graph." << endl;
	}
}


void printUsage(const char* argv[]) {
	cout << "Usage: " << endl;
	cout << argv[0] << " --default dataset entityId1 entityId2 k" << endl;
	cout << argv[0] << " --advance dataset entityId1 entityId2 k output-type Importance-Type length-penalty (beta)" << endl;
	cout << argv[0] << " --fast_advance dataset entityId1 entityId2 k output-type Importance-Type length-penalty (beta)" << endl;
	cout << argv[0] << " --refine dataset entityId1 entityId2 k score-function" << endl;
	cout << argv[0] << " --train dataset" << endl;
	cout << endl;

	cout << "--advance/fast_advance mode:" << endl;
	
	cout << "\t output-type:" << endl;
	cout << "\t\t 1 -> typing ranking details in std::cout" << endl;
	cout << "\t\t 2 -> saving to a file" << endl;
	cout << "\t\t 3 -> both 1 and 2" << endl;	

	cout << "\t TF-IDF-type:" << endl;
	cout << "\t\t MNIS -> ours" << endl;
	cout << "\t\t SLV1 -> Strength & Length based Version 1" << endl;
	cout << "\t\t SLV2 -> Strength & Length based Version 2" << endl;
	cout << "\t\t SMP -> Shortest Meta Path" << endl;

	cout << "\t length-penalty(l is the meta-path's length): " << endl;
	cout << "\t\t 1 -> beta^l (beta < 1)" << endl;
	cout << "\t\t 2 -> 1/factorial(l)" << endl;	
	cout << "\t\t 3 -> 1/l" << endl;
	cout << "\t\t 4 -> 1/e^l" << endl;
	cout << endl;


	cout << "--default mode:" << endl;
	cout << "\t output-type -> 1" << endl;
	cout << "\t TF-IDF-type -> MNIS" << endl;
	cout << "\t length-penalty -> 1" << endl;
	cout << "\t beta -> 0.2" << endl;
	cout << endl;


	cout << "--refine mode:" << endl;
	cout << "\t refine k meta-paths from previous generated meta-paths" << endl;
	cout << "\t default meta-paths file name: dataset_entityId1_entityId2.txt" << endl;
	cout << "\t score-function: 1 -> BPCRW" << endl;
	cout << "\t output-type -> 1" << endl;
	cout << endl;

	cout << "--train mode:" << endl;
	cout << "\t get statistics of different node types and edge types(including the number of each node type, each edge type and the weight of each edge type" << endl;
	cout << endl; 
}

int main(int argc, const char * argv[]) {
	string dataset;
	int src, dst, k;
		

	if (argc == 3){
		if(strcmp(argv[1], "--train") == 0 || strcmp(argv[1], "-t")){
			getMetaInfo(argv[2]);
			return 0;
		}
	}else if (argc > 5) {
		dataset = argv[2];
		src = atoi(argv[3]);
		dst = atoi(argv[4]);
		k = atoi(argv[5]);

		int penalty_type = DEFAULT_PENALTY_TYPE;
		double beta = DEFAULT_BETA;
		string score_type = DEFAULT_SCORE_TYPE;
		bool refine_flag = DEFAULT_REFINE_FLAG;
		int output_type = DEFAULT_OUTPUT_TYPE;
		int score_function = DEFAULT_SCORE_FUNCTION;
		bool fast_flag = DEFAULT_FAST_FLAG;

		if (strcmp(argv[1], "--default") == 0 || strcmp(argv[1], "-d") == 0) {
			scoreSetup(score_type.c_str(), penalty_type, beta, fast_flag);
			//cout << argv[1] << " " << argc << " " << penalty_type << " " << score_type << endl;
		}
		else if (strcmp(argv[1], "--advance") == 0 || strcmp(argv[1], "-a") == 0 || strcmp(argv[1], "--fast_advance") == 0 || strcmp(argv[1], "-fa") == 0) {
			
			output_type = atoi(argv[6]);
			score_type = argv[7];
			fast_flag = false;
			if (argc > 8) {
				penalty_type = atoi(argv[8]);
				if(penalty_type != 1 && penalty_type != 2){
					cerr << "The penalty_type parameter must be 1 or 2" << endl;
					return -1;
				}
			}
			if(penalty_type == 1 && argc > 9){
				beta = atof(argv[9]);
				if(beta <= 0 || beta >= 1){
					cerr << "The beta parameter must be greater than 0 and less than 1" << endl;
					return -1;
				}
			}
			if(strcmp(argv[1], "--fast_advance") == 0 || strcmp(argv[1], "-fa") == 0) fast_flag = true;
			scoreSetup(score_type.c_str(), penalty_type, beta, fast_flag);
			//cout << argv[1] << " " << argc << " " << penalty_type << " " << score_type << endl;
		}
		else if (strcmp(argv[1], "--refine") == 0 || strcmp(argv[1], "-r") == 0) {
			output_type = 1;
			if (argc > 6) {
				score_function = atoi(argv[6]);
				if(score_function == 1){
					score_type = "BPCRW";
				}			}
			refine_flag = true;
		}
		else {
			printUsage(argv);
			return -1;
		}

		clock_t t1, t2;

		HIN_Graph hin_graph_;

		map<int, vector<Edge>> adj;
		map<int, string> node_name;
		map<int, string> node_type_name;
		map<int, int> node_type_num;
		map<int, vector<int>> node_id_to_type;
		map<int, string> edge_name;

		hin_graph_ = loadHinGraph(dataset.c_str(), node_name, adj, node_type_name, node_type_num, node_id_to_type, edge_name);
		if(strcmp(score_type.c_str(), "MNIS") == 0 || strcmp(score_type.c_str(), "SLV1") == 0 || strcmp(score_type.c_str(), "SLV2") == 0 ){
			loadMetaInfo(dataset, hin_graph_);
		}


		string file_name = getFileName(src, dst, score_type, k, dataset, beta);

		vector<pair<vector<double>, vector<int>>> topKMetaPaths;

		t1 = clock();
		if (!refine_flag) {
			topKMetaPaths = TopKCalculator::getTopKMetaPath_TFIDF(src, dst, k, hin_graph_);
		}
		else {
			clock_t t3;
			int refinedK = 15;
			score_type = "SMP";
			string tmp_file_name = getFileName(src, dst, score_type, refinedK, dataset, beta);
			vector<vector<int>> meta_paths = TopKCalculator::loadMetaPaths(tmp_file_name);
			cerr << "Take " << (0.0 + t3 - t1) / CLOCKS_PER_SEC << "s to load candidate meta-paths" << endl;
			t3 = clock();
			topKMetaPaths = TopKCalculator::getTopKMetaPath_Refiner(src, dst, k, meta_paths, score_function, hin_graph_);
			t1 = t3;
		}
		t2 = clock();

		output(topKMetaPaths, edge_name, file_name, refine_flag, output_type);
		cerr << "Take " << (0.0 + t2 - t1) / CLOCKS_PER_SEC << "s to calculate top k meta-paths" << endl;

	}
	else {
		printUsage(argv);
		return -1;
	}



	return 0;
}
