#ifndef __DBLP_Reader__TopKCalculator__
#define __DBLP_Reader__TopKCalculator__

#include <vector>
#include <map>
#include <set>
#include <string>
#include <queue>

#include "HIN_Graph.h"
#include "SimCalculator.h"
#define MAX_CANDIDATES_NUM 2
#define ALPHA 1
#define STRENGTH_ALPHA 0.5

using namespace std;

class MetaNode
{
public:
	int curr_edge_type_;
	vector<int> node_types_;
	map<int, set<int>> curr_nodes_with_parents_;// node id -> parent node ids
	double max_mni_;
	double strength_;
	double strength_times_penalty_;
	double partial_ub_score;
	int min_similar_nodes_num_;
	vector<int> meta_path_;
	MetaNode* parent_;
	MetaNode(int curr_edge_type, map<int, set<int>> & curr_nodes_with_parents, double max_mni, double strength, vector<int> meta_path, int min_similar_nodes_num, MetaNode* parent);
	~MetaNode();
	unsigned int size();
};

class MetaNodePointerCmp
{
public:
	bool operator () (MetaNode* & node_p1, MetaNode* & node_p2);
};



class TopKMetaPathsCmp
{
public:
	bool operator () (pair<vector<double>, vector<int>> & pair1, pair<vector<double>, vector<int>> & pair2);
};

class TopKCalculator
{
private:
	static int factorial(int n);
	static double min(double d1, double d2);
	static double getRarity(int src, int dst, MetaNode* src_middle_score_p, MetaNode* dst_middle_score_p, set<int> & srcSimilarNodes, set<int> & dstSimilarNodes, vector<int> meta_path_l, vector<int> meta_path_r, HIN_Graph & hin_graph_, bool direction_flag);
	static double getRarity(int src, int dst, set<int> & srcSimilarNodes, set<int> & dstSimilarNodes, vector<int>  meta_path, HIN_Graph & hin_graph_, bool direction_flag=true);
	static int getHit(set<int> & srcSimilarNodes, set<int> & dstSimilarNodes, vector<int> meta_path, HIN_Graph & hin_graph_);
	
	static void updateTopKMetaPaths(double score, double support, double rarity, vector<int> meta_path, int k,priority_queue<pair<vector<double>, vector<int>>, vector<pair<vector<double>, vector<int>>>, TopKMetaPathsCmp> & topKMetaPathsQ, set<string> & topKMetaPathsSet );
	static void updateMinSimilarNodesNum(map<int, map<int, set<int>>> & next_nodes_id_, map<int, int> & minSimilarNodesNum, HIN_Graph & hin_graph_);
 
	static vector<int> getMergedMetaPath(vector<int> meta_path_l, vector<int> meta_path_r, bool direction_flag);
	static vector<int> getReversedMetaPath(vector<int> meta_path);
	static double getMaxMeta(map<MetaNode*, map<int, set<int>>> & currMetaNodeListL, map<MetaNode*, map<int, set<int>>> & currMetaNodeListR, double maxRarity);
	static double getSupport(set<int> eids, int src, int dst, bool direction_flag, MetaNode* curr_score_node_pl, MetaNode* curr_score_node_pr, vector<int> meta_path_l, vector<int> meta_path_r, set<int> & srcNextNodes, set<int> & dstNextNodes, HIN_Graph & hin_graph_);
	static double getMetaPathStrength(vector<int> meta_path,  HIN_Graph & hin_graph_);
	static double getEdgeTypeStrength(int edge_type, HIN_Graph & hin_graph_);
	static pair<int, set<int>> getMNI(set<int> eids, int dst, MetaNode* curr_score_node_p, vector<int> meta_path, HIN_Graph & hin_graph_);
	static double getMaxMNI(double candidateMNI);
	static double getMetaPathScore(int src, int dst, vector<int> meta_path, int score_function, HIN_Graph & hin_graph_);
	static void expandTfIdFNodes(int src, set<int> srcSimilarNodes, int dst, set<int> dstSimilarNodes, int k, bool direction_flag, MetaNode* last_p, map<int, map<int, set<int>>> & next_nodes_id_, map<int, int> minSimilarNodes, double oppo_max_sl, map<int, set<int>> & edge_max_instances_num, map<int, map<int, vector<MetaNode*>>> & visitedNodes, map<int, map<int, vector<MetaNode*>>> & oppo_visitedNodes, priority_queue<MetaNode*, vector<MetaNode*>, MetaNodePointerCmp> & q, priority_queue<pair<vector<double>, vector<int>>, vector<pair<vector<double>, vector<int>>>, TopKMetaPathsCmp> & topKMetaPathsQ, set<string> & topKMetaPathsSet, set<MetaNode*> & sl_set, HIN_Graph & hin_graph_);
public:
	static int penalty_type_;// 1 -> beta^l; 2 -> factorial of l; 3 -> 1/l
	static double beta_;
	static int rarity_type_;// 1 -> true rarity; 0 -> 1(constant); 2 -> Strenth-based rarity
	static int support_type_;// 1 -> MNI; 2 -> PCRW; 0 -> 1(constant); 3 -> Strength-based MNI; 4 -> Strength
	static bool bi_directional_flag_;
	static int sample_size_;
	static int similarPairsSize;
	static double penalty(int length);
	static double getRarity(int similarPairsSize, int hit);
	static set<int> getSimilarNodes(int eid, map<int, HIN_Node> & hin_nodes_, bool sample_flag=false, bool output_flag=true);
	static void getDstEntities(int src, vector<int> meta_path, set<int> & dst_entities, HIN_Graph & hin_graph_);
	static double getBPCRW(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_);
	static double getPCRW(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_);
	static string metapathToString(vector<int> meta_path);
	static vector<int> stringToMetapath(string meta_path_str);
	static vector<pair<vector<double>, vector<int>>> getTopKMetaPath_TFIDF(int src, int dst, int k, HIN_Graph & hin_graph_);	
	static vector<pair<vector<double>, vector<int>>> getTopKMetaPath_Refiner(int src, int dst, int k, vector<vector<int>> & meta_paths, int score_function, HIN_Graph & hin_graph_);
	static void saveToFile(vector<vector<int>> topKMetaPaths, string file_name);
	static vector<vector<int>> loadMetaPaths(string file_name);
};

#endif
