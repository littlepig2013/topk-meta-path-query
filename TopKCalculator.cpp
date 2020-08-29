#include "TopKCalculator.h"
#include "CommonUtils.h"

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <vector>
#include <iterator>
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>

using namespace std;

int TopKCalculator::penalty_type_ = 1;
double TopKCalculator::beta_ = 0.3;
int TopKCalculator::rarity_type_ = 1;
int TopKCalculator::support_type_ = 1;
bool TopKCalculator::bi_directional_flag_ = true;
int TopKCalculator::sample_size_ = 100;
int TopKCalculator::similarPairsSize = 1;

unsigned int MetaNode::size(){
	return this->curr_nodes_with_parents_.size();
}

MetaNode::MetaNode(int curr_edge_type, map<int, set<int>> & curr_nodes_with_parents, double max_mni, double strength, vector<int> meta_path, int min_similar_nodes_num, MetaNode* parent):curr_edge_type_(curr_edge_type), curr_nodes_with_parents_(curr_nodes_with_parents), max_mni_(max_mni), strength_(strength), meta_path_(meta_path), min_similar_nodes_num_(min_similar_nodes_num), parent_(parent){
	if(TopKCalculator::support_type_ == 4 && TopKCalculator::bi_directional_flag_){
		this->strength_times_penalty_ = strength*pow(0.5*(TopKCalculator::penalty(meta_path_.size())), 0.5);
	}else{
		this->strength_times_penalty_ = strength*(TopKCalculator::penalty(meta_path_.size()));
	}
	this->partial_ub_score = this->max_mni_*(TopKCalculator::getRarity(TopKCalculator::similarPairsSize, this->min_similar_nodes_num_))*this->strength_times_penalty_;
}

MetaNode::~MetaNode(){
	for(map<int, set<int>>::iterator iter = this->curr_nodes_with_parents_.begin(); iter != this->curr_nodes_with_parents_.end(); iter++){
		iter->second.clear();
	}
	this->curr_nodes_with_parents_.clear();
	this->meta_path_.clear(); 
	this->parent_ = NULL;
}

bool MetaNodePointerCmp::operator () (MetaNode* & node_p1, MetaNode* & node_p2)
{
	if(node_p1->max_mni_ == 0 && node_p2->max_mni_ == 0){
		return node_p1->meta_path_.size() > node_p2->meta_path_.size();
	}else{
		double tmp1 = node_p1->partial_ub_score;
		double tmp2 = node_p2->partial_ub_score;
		if(tmp1 < tmp2){
			return true;
		}else if(tmp1 > tmp2){
			return false;
		}else{
			double randomNumber = (double)rand()/RAND_MAX;
			return randomNumber <= 0.5;
		}
	}
}


class ScorePairCmp
{
public:
	bool operator () (pair<double, int> & scorePair1, pair<double, int> & scorePair2){
		if(scorePair1.first == scorePair2.first){
			return scorePair1.second > scorePair2.second;
		}else{
			return scorePair1.first < scorePair2.first;
		}
	}
};


bool TopKMetaPathsCmp::operator () (pair<vector<double>, vector<int>> & pair1, pair<vector<double>, vector<int>> & pair2){
	if(pair1.first[0] > pair2.first[0]){
		return true;
	}else if(pair1.first[0] < pair2.first[0]){
		return false;
	}else{
		double randomNumber = (double)rand()/RAND_MAX;
		return randomNumber <= 0.5;
	}
}

int TopKCalculator::factorial(int n){
	return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

double TopKCalculator::min(double d1, double d2){
	return d1 > d2 ? d2 : d1;
}

double TopKCalculator::getMetaPathScore(int src, int dst, vector<int> meta_path, int score_function, HIN_Graph & hin_graph_){
	if(score_function == 1){
		double pcrw = getPCRW(src, dst, meta_path, hin_graph_);
		return pcrw;
	}else if(score_function == 2){

		return getBPCRW(src, dst, meta_path, hin_graph_);
	}

	return 2.0;	
}

double TopKCalculator::penalty(int length)
{
	if(penalty_type_ == 0){
		return 1.0;
	}else if(penalty_type_ == 1){
		return pow(beta_, length);
	}else if(penalty_type_ == 2){
		return 1.0/factorial(length);
	}else if(penalty_type_ == 3){
		return 1.0/length;
	}else if(penalty_type_ == 4){
		return 1/exp(length);
	}else{
		return 1.0;
	}
}

set<int> TopKCalculator::getSimilarNodes(int eid, map<int, HIN_Node> & hin_nodes_, bool sample_flag, bool output_flag)
{
	set<int> similarNodes;

	if(hin_nodes_.find(eid) == hin_nodes_.end()){
		cout << eid << " Not Found" << endl;
		return similarNodes;
	}


	vector<int> curr_node_types_id_ = hin_nodes_[eid].types_id_;
	// cout << curr_node_types_id_.size() << " " << curr_node_types_id_[0] << endl;
	for(map<int, HIN_Node>::iterator i = hin_nodes_.begin(); i != hin_nodes_.end(); i++){
		int temp_node_id = i->first;
		if(temp_node_id == eid){
			continue;
		}
		HIN_Node tempNode = i->second;
		vector<int> tempNode_types_id = tempNode.types_id_;
		for(vector<int>::iterator j = tempNode_types_id.begin(); j != tempNode_types_id.end(); j++){
			vector<int>::iterator iter = find(curr_node_types_id_.begin(), curr_node_types_id_.end(), *j);
			if(iter != curr_node_types_id_.end()){
				similarNodes.insert(temp_node_id);
				break;
			}
		}
	}
	if(output_flag){
		cerr << eid << "\t";
		if(strcmp(hin_nodes_[eid].key_.c_str(), "") == 0){
			cerr << "NA" << "\t";
		}else{
			cerr << hin_nodes_[eid].key_ << "\t";
		} 

	}	
	
	similarNodes.insert(eid);

	if(sample_flag){
		set<int> samples;
		for(int i = 0; i < sample_size_; i++){
			int offset = (int) (rand() % similarNodes.size());
			set<int>::iterator iter(similarNodes.begin());
			advance(iter, offset);
			samples.insert(*iter);
			similarNodes.erase(*iter);	
		}
		similarNodes = samples;	
	}
	if(output_flag)
		cout << "candidate size: " << similarNodes.size() << endl;
	return similarNodes;
}

int TopKCalculator::getHit(set<int> & srcSimilarNodes, set<int> & dstSimilarNodes, vector<int> meta_path, HIN_Graph & hin_graph_) {

	map<int, vector<HIN_Edge> > hin_edges_src_ = hin_graph_.edges_src_;
	map<int, vector<HIN_Edge> > hin_edges_dst_ = hin_graph_.edges_dst_;

	set<int> currNodes = srcSimilarNodes;
	set<int> nextNodes;	

	for(int i = 0; i < meta_path.size(); i++){
		int curr_edge_type = meta_path[i];
		map<int, vector<HIN_Edge> > temp_hin_edges;
		if (curr_edge_type > 0) {
			temp_hin_edges = hin_edges_src_;
		}
		else {
			temp_hin_edges = hin_edges_dst_;
		}

		for (auto j = currNodes.begin(); j != currNodes.end(); j++) {
			int currNode = *j;
			if (temp_hin_edges.find(currNode) != temp_hin_edges.end()) {
				vector<HIN_Edge> tempEdges = temp_hin_edges[currNode];
				for (vector<HIN_Edge>::iterator e = tempEdges.begin(); e != tempEdges.end(); e++) {
					if (curr_edge_type > 0) {
						if (e->edge_type_ == curr_edge_type) {
							nextNodes.insert(e->dst_);
								//break;
						}
					}
					else {
						if (e->edge_type_ == -curr_edge_type) {
							nextNodes.insert(e->src_);
								//break;
						}
					}
				}
			}
		}
		
		currNodes = nextNodes;
		nextNodes.clear();

	}

	for(auto iter = hin_edges_src_.begin(); iter != hin_edges_src_.end(); iter++){
		iter->second.clear();
	}
	hin_edges_src_.clear();
	
	for(auto iter = hin_edges_dst_.begin(); iter != hin_edges_dst_.end(); iter++){
		iter->second.clear();
	}
	hin_edges_dst_.clear();
	
	set<int> intersect;
	set_intersection(currNodes.begin(), currNodes.end(), dstSimilarNodes.begin(), dstSimilarNodes.end(), inserter(intersect, intersect.begin()));

	return intersect.size();
}

double TopKCalculator::getRarity(int similarPairsSize, int hit){
	if(TopKCalculator::rarity_type_ == 0){
		return 1.0;
	}else{
		return log(1+similarPairsSize*1.0/hit);
	}
}

// This is a light weight version of getRarity where we consider a smaller set of D
// D' = {(src, v)|v \in dstSimilarNodes} \cup {(u, dst)|u \in srcSimilarNodes}
double TopKCalculator::getRarity(int src, int dst, set<int> & srcSimilarNodes, set<int> & dstSimilarNodes, vector<int> meta_path, HIN_Graph & hin_graph_, bool direction_flag) {
	if (rarity_type_ == 0) {
		return 1.0;
	}

	vector<int> tmp_meta_path;
	if(!direction_flag){
		for(int i = meta_path.size() - 1; i >= 0 ; i--){
			tmp_meta_path.push_back(-meta_path[i]);
		}	
	}else{
		tmp_meta_path = meta_path;
	}

	//cout << "Running with a light weight version of getRarity" << endl;

	// (src, dst) are counted twice, hence we have to substract 1
	similarPairsSize = srcSimilarNodes.size() + dstSimilarNodes.size() - 1;

	set<int> tempNode;
	int hit = 0;

	tempNode.clear();tempNode.insert(src);dstSimilarNodes.erase(dst);
	hit += getHit(tempNode, dstSimilarNodes, tmp_meta_path, hin_graph_);
	dstSimilarNodes.insert(dst);

	tempNode.clear();tempNode.insert(dst);srcSimilarNodes.erase(src);
	hit += getHit(srcSimilarNodes, tempNode, tmp_meta_path, hin_graph_);
	srcSimilarNodes.insert(src);

	hit += 1;

	if(rarity_type_ == 1){
		return getRarity(similarPairsSize, hit);
	}else{
		double normalizedHit = hit*1.0;
		map<int, pair<double, double>> edgeTypeAvgDegree = hin_graph_.edge_type_avg_degree_;
		int meta_path_size = meta_path.size();
		for(int i = 0; i < meta_path_size; i++){
			int curr_edge_type = meta_path[i];
			if(curr_edge_type < 0){
				curr_edge_type = -curr_edge_type;
			}
			if(edgeTypeAvgDegree.find(curr_edge_type) != edgeTypeAvgDegree.end()){
				normalizedHit /= pow(edgeTypeAvgDegree[curr_edge_type].first*edgeTypeAvgDegree[curr_edge_type].second, STRENGTH_ALPHA);
				if(normalizedHit <= 1){
					break;
				}
			}
		}
		return getRarity(similarPairsSize, hit);
	}
}

double TopKCalculator::getRarity(int src, int dst, MetaNode* src_middle_score_p, MetaNode* dst_middle_score_p, set<int> & srcSimilarNodes, set<int> & dstSimilarNodes, vector<int> meta_path_l, vector<int> meta_path_r, HIN_Graph & hin_graph_, bool direction_flag){
	
	if(rarity_type_ != 1){
		return 1.0;
	}

	if(!direction_flag){
		MetaNode* tmp_p = src_middle_score_p;
		src_middle_score_p = dst_middle_score_p;
		dst_middle_score_p = tmp_p;
		vector<int> tmp_meta_path = meta_path_l;
		meta_path_l = meta_path_r;
		meta_path_r = tmp_meta_path;
	}

	set<int> srcMiddleNodesSet;	
	set<int> dstMiddleNodesSet;
	int hit_src;
	int hit_dst; 
	
	vector<int> reverse_meta_path_r;
	vector<int> reverse_meta_path_l;
	map<int, set<int>> srcMiddleNodes;
	map<int, set<int>> dstMiddleNodes;
	if(src_middle_score_p == NULL){
		dstMiddleNodesSet.insert(src);
		srcMiddleNodesSet.insert(src);
		hit_src = dst_middle_score_p->curr_nodes_with_parents_.size();
	}else{
		srcMiddleNodes = src_middle_score_p->curr_nodes_with_parents_;
		for(auto iter = srcMiddleNodes.begin(); iter != srcMiddleNodes.end(); iter++){
			srcMiddleNodesSet.insert(iter->first);
		}

	}
	for(int i = meta_path_l.size() - 1; i >= 0; i--){
		reverse_meta_path_l.push_back(-meta_path_l[i]);	
	}
	
	if(dst_middle_score_p == NULL){
		srcMiddleNodesSet.insert(dst);
		dstMiddleNodesSet.insert(dst);
		hit_dst = src_middle_score_p->curr_nodes_with_parents_.size();
	}else{
		dstMiddleNodes = dst_middle_score_p->curr_nodes_with_parents_;
		for(auto iter = dstMiddleNodes.begin(); iter != dstMiddleNodes.end(); iter++){
			dstMiddleNodesSet.insert(iter->first);
		}
	}
	for(int i = meta_path_r.size() - 1; i >= 0; i--){
		reverse_meta_path_r.push_back(-meta_path_r[i]);	
	}


	hit_dst = getHit(srcMiddleNodesSet, dstSimilarNodes, reverse_meta_path_r, hin_graph_);
	hit_src = getHit(dstMiddleNodesSet, srcSimilarNodes, reverse_meta_path_l, hin_graph_);
	for(auto iter = srcMiddleNodes.begin(); iter != srcMiddleNodes.end(); iter++){
		iter->second.clear(); 
	} 
	srcMiddleNodes.clear();
	for(auto iter = dstMiddleNodes.begin(); iter != dstMiddleNodes.end(); iter++){
		iter->second.clear(); 
	} 
	dstMiddleNodes.clear();


	TopKCalculator::similarPairsSize = srcSimilarNodes.size() + dstSimilarNodes.size() - 1;
	srcMiddleNodesSet.clear();
	dstMiddleNodesSet.clear();
	return getRarity(TopKCalculator::similarPairsSize, hit_src+hit_dst-1);
}


double TopKCalculator::getPCRW(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_){
	Bi_BFS_SimCalculator simCalculator = Bi_BFS_SimCalculator();
	//double result = simCalculator.getMetaPathBasedSim(src, dst, meta_path, hin_graph_, false, BPCRW_GAMMA);
	double result = simCalculator.getMetaPathBasedSim(src, dst, meta_path, hin_graph_, false, 1);
	return result;
}

double TopKCalculator::getBPCRW(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_){
	
	Bi_BFS_SimCalculator simCalculator = Bi_BFS_SimCalculator();
	double result = simCalculator.getMetaPathBasedSim(src, dst, meta_path, hin_graph_, false, 0.5);
	return result;
}



double TopKCalculator::getMaxMeta(map<MetaNode*, map<int, set<int>>> & currMetaNodeListL, map<MetaNode*, map<int, set<int>>> & currMetaNodeListR, double maxRarity){
	double max_support_penalty = 0.0;
	for(map<MetaNode*, map<int, set<int>>>::iterator iter_i = currMetaNodeListL.begin(); iter_i != currMetaNodeListL.end(); iter_i++){
		MetaNode* this_score_node_p = iter_i->first;
		map<int, set<int>> this_typed_nodes = iter_i->second;
		for(map<MetaNode*, map<int, set<int>>>::iterator iter_j = currMetaNodeListR.begin(); iter_j != currMetaNodeListR.end(); iter_j++){
			MetaNode* opp_score_node_p = iter_j->first;
			map<int, set<int>> opp_typed_nodes = iter_j->second;
			double tmp_max_support = min(this_score_node_p->max_mni_, opp_score_node_p->max_mni_)*this_score_node_p->strength_*opp_score_node_p->strength_;
			double tmp_length = this_score_node_p->meta_path_.size() + opp_score_node_p->meta_path_.size();
			for(map<int, set<int>>::iterator iter = this_typed_nodes.begin(); iter != this_typed_nodes.end(); iter++){
				double tmp_max_support_penalty;
				if(opp_typed_nodes.find(iter->first) != opp_typed_nodes.end()){
					tmp_max_support_penalty = tmp_max_support*penalty(tmp_length);
				}else{
					tmp_max_support_penalty = tmp_max_support*penalty(tmp_length + 1);
				}
				if(tmp_max_support_penalty > max_support_penalty){
					max_support_penalty = tmp_max_support_penalty;
				}
			}
		}
	}	
	return max_support_penalty*maxRarity;
}

double TopKCalculator::getMaxMNI(double candidateMNI){
	if(support_type_ == 1 || support_type_ == 3){
		return candidateMNI;
	}else{
		return 1.0;
	}

	return 0;
}
pair<int, set<int>> TopKCalculator::getMNI(set<int> eids, int dst, MetaNode* curr_score_node_p, vector<int> meta_path, HIN_Graph & hin_graph_){ 
	MetaNode* temp_score_node_p = curr_score_node_p;
	set<int> curr_nodes = eids;
	set<int> next_nodes;
	set<int> tmp_next_entities;
	int min_instances_num = curr_nodes.size();

	while(temp_score_node_p->parent_ != NULL){
		next_nodes.clear();
		map<int, set<int>> temp_nodes_with_parents = temp_score_node_p->curr_nodes_with_parents_;
		for(set<int>::iterator iter = curr_nodes.begin(); iter != curr_nodes.end(); iter++){
			int temp_node = *iter;
			tmp_next_entities.clear();	
			if(temp_nodes_with_parents.find(temp_node) != temp_nodes_with_parents.end()){
				set<int> parents = temp_nodes_with_parents[temp_node];
				next_nodes.insert(parents.begin(), parents.end());
			} 
		}		

			
		curr_nodes = next_nodes;
		int curr_nodes_size = curr_nodes.size();
		if(min_instances_num > curr_nodes_size){
			min_instances_num = curr_nodes_size;
		}
		temp_score_node_p = temp_score_node_p->parent_;
			
	}	
	
	return make_pair(min_instances_num, curr_nodes);

}
double TopKCalculator::getEdgeTypeStrength(int edge_type, HIN_Graph & hin_graph_){
	double strength_ratio = 1.0;
	if(support_type_ != 3 && support_type_ != 4 && support_type_ != 5){
		return 1.0;
	}
	if(edge_type < 0){
		edge_type = -edge_type;
	}
	map<int, pair<double, double>> edgeTypeAvgDegree = hin_graph_.edge_type_avg_degree_;
	if(edgeTypeAvgDegree.find(edge_type) != edgeTypeAvgDegree.end()){
		
		if(support_type_ == 3){
			if(edgeTypeAvgDegree[edge_type].first < edgeTypeAvgDegree[edge_type].second){
				return 1.0/edgeTypeAvgDegree[edge_type].first;
			}else{
				return 1.0/edgeTypeAvgDegree[edge_type].second;
			}
		}
		return 1.0/pow(edgeTypeAvgDegree[edge_type].first*edgeTypeAvgDegree[edge_type].second, STRENGTH_ALPHA);
	}
	return 1.0;
}
double TopKCalculator::getMetaPathStrength(vector<int> meta_path,  HIN_Graph & hin_graph_){
	double strength_ratio = 1.0;
	if(support_type_ != 3 && support_type_ != 4 && support_type_ != 5){
		return 1.0;
	}
	map<int, pair<double, double>> edgeTypeAvgDegree = hin_graph_.edge_type_avg_degree_;
	int meta_path_size = meta_path.size();
	for(int i = 0; i < meta_path_size; i++){
		int curr_edge_type = meta_path[i];
		if(curr_edge_type < 0){
			curr_edge_type = -curr_edge_type;
		}
		if(edgeTypeAvgDegree.find(curr_edge_type) != edgeTypeAvgDegree.end()){
			//strength_ratio /= pow(edgeTypeAvgDegree[curr_edge_type].first*edgeTypeAvgDegree[curr_edge_type].second, STRENGTH_ALPHA);
			
			if(support_type_ != 3){
				strength_ratio /= pow(edgeTypeAvgDegree[curr_edge_type].first*edgeTypeAvgDegree[curr_edge_type].second, STRENGTH_ALPHA);
			}else{
				if(edgeTypeAvgDegree[curr_edge_type].first < edgeTypeAvgDegree[curr_edge_type].second){
					strength_ratio /= edgeTypeAvgDegree[curr_edge_type].first;
				}else{
					strength_ratio /= edgeTypeAvgDegree[curr_edge_type].second;
				}
			}
		}
	}
	return strength_ratio;
}

vector<int> TopKCalculator::getMergedMetaPath(vector<int> meta_path_l, vector<int> meta_path_r, bool direction_flag){
	vector<int> merged_meta_path;
	if(!direction_flag){
		vector<int> tmp_meta_path = meta_path_r;
		meta_path_r = meta_path_l;
		meta_path_l = tmp_meta_path;
	}
	for(int i = 0; i < meta_path_l.size(); i++){
		merged_meta_path.push_back(meta_path_l[i]);
	}
	for(int i = (int)meta_path_r.size() - 1; i >= 0; i--){
		merged_meta_path.push_back(-meta_path_r[i]);
	}
	return merged_meta_path;
}
vector<int> TopKCalculator::getReversedMetaPath(vector<int> meta_path){
	vector<int> reversed_meta_path;
	for(int i = (int)meta_path.size() - 1; i >= 0; i--){
		reversed_meta_path.push_back(-meta_path[i]);
	}
	return reversed_meta_path;
}
double TopKCalculator::getSupport(set<int> eids, int src, int dst, bool direction_flag, MetaNode* curr_score_node_pl, MetaNode* curr_score_node_pr, vector<int> meta_path_l, vector<int> meta_path_r, set<int> & srcNextNodes, set<int> & dstNextNodes, HIN_Graph & hin_graph_){
	if(support_type_ == 0){// Binary Support
		return 1.0;
	}else if(support_type_ == 1 || support_type_ == 3 ||  support_type_ == 4 || support_type_ == 5){// 1 -> MNI Support; 3 -> Normalized MNI Support
		double strength_ratio = 1.0;
		if(support_type_ != 1){
			strength_ratio = getMetaPathStrength(meta_path_l, hin_graph_)*getMetaPathStrength(meta_path_r, hin_graph_);
			if(support_type_ == 4){
				return strength_ratio;
			}else if(support_type_ == 5){
				return exp(strength_ratio);
			}

		}

		int min_instances_num = 1;
		//cout << eids.size() << endl;
		if(!TopKCalculator::bi_directional_flag_){
			curr_score_node_pl = curr_score_node_pl->parent_;
			meta_path_l = curr_score_node_pl->meta_path_;
			pair<int, set<int>> result = getMNI(eids, src, curr_score_node_pl, meta_path_l, hin_graph_);
			min_instances_num = result.first;
			srcNextNodes = result.second;
			dstNextNodes = eids;
		} else if(curr_score_node_pl != NULL && curr_score_node_pr != NULL && eids.size() != 0){
			int min_instances_num_l, min_instances_num_r;
			if(direction_flag){
			
				pair<int, set<int>> result_l = getMNI(eids, src, curr_score_node_pl, meta_path_l, hin_graph_);
				min_instances_num_l = result_l.first;
	
				pair<int, set<int>> result_r = getMNI(eids, dst, curr_score_node_pr, meta_path_r, hin_graph_); 

				min_instances_num_r = result_r.first;
				
				srcNextNodes = result_l.second;
				dstNextNodes = result_r.second;
			
			result_r.second.clear();
			result_l.second.clear();
			}else{

				pair<int, set<int>> result_l = getMNI(eids, dst, curr_score_node_pl, meta_path_l, hin_graph_);
				min_instances_num_l = result_l.first;
				pair<int, set<int>> result_r = getMNI(eids, src, curr_score_node_pr, meta_path_r, hin_graph_); 
				min_instances_num_r = result_r.first;
				srcNextNodes = result_r.second;
				dstNextNodes = result_l.second;
			result_r.second.clear();
			result_l.second.clear();
			}
			min_instances_num = min_instances_num_l < min_instances_num_r ? min_instances_num_l : min_instances_num_r;
			srcNextNodes.clear();
			dstNextNodes.clear();
			
			
		}

		if(support_type_ == 1){
			return min_instances_num*1.0;
		}else{
			return min_instances_num*strength_ratio;
		}

	}else if(support_type_ == 2){// PCRW Support
		//Meta_Paths tempmetapath(hin_graph_);
		//map<int, double> id_sim;

		//tempmetapath.weights_.push_back(1.0);
		//tempmetapath.linkTypes_.push_back(meta_path);

		//double pcrw = SimCalculator::calSim_PCRW(src, dst, tempmetapath, id_sim);
		double pcrw = 0.0;
		if(eids.size() != 0){
			vector<int> reversed_meta_path_r = getReversedMetaPath(meta_path_r);
			for(set<int>::iterator iter=eids.begin(); iter != eids.end(); iter++){
				int curr_node = *iter;
				pcrw += getBPCRW(src, curr_node, meta_path_l, hin_graph_)*getBPCRW(curr_node, dst, reversed_meta_path_r, hin_graph_);
			}
		}else{
			pcrw = getBPCRW(src, dst, meta_path_l, hin_graph_);
		}
		return pcrw;
	}
}

void TopKCalculator:: updateTopKMetaPaths(double score, double support, double rarity, vector<int> meta_path, int k,priority_queue<pair<vector<double>, vector<int>>, vector<pair<vector<double>, vector<int>>>, TopKMetaPathsCmp> & topKMetaPathsQ, set<string> & topKMetaPathsSet ){
	string meta_path_str = metapathToString(meta_path);
	if(topKMetaPathsSet.find(meta_path_str) != topKMetaPathsSet.end()){
		return;
	}
	topKMetaPathsSet.insert(meta_path_str);
	//cout << meta_path_str << " score: " << score << endl;

	vector<double> score_info = vector<double>();
	score_info.push_back(score);
	score_info.push_back(support);
	score_info.push_back(rarity);
	topKMetaPathsQ.push(make_pair(score_info, meta_path));
	if(topKMetaPathsQ.size() > k){
		topKMetaPathsQ.pop();
	}
	
}

// update visited nodes
bool updateVisitedNodes(map<int, map<int, vector<MetaNode*>>> & visited_typed_nodes, MetaNode* p, int eid, int curr_node_type){
	bool visited_flag = false;
	if(visited_typed_nodes.find(curr_node_type) == visited_typed_nodes.end()){
		visited_typed_nodes[curr_node_type]=map<int, vector<MetaNode*>>();
		visited_typed_nodes[curr_node_type][eid] = vector<MetaNode*>();
	}else if(visited_typed_nodes[curr_node_type].find(eid) != visited_typed_nodes[curr_node_type].end()){
		visited_flag = true;
	}
	visited_typed_nodes[curr_node_type][eid].push_back(p);
	return visited_flag;
}
bool updateVisitedNodes(map<int, map<int, vector<MetaNode*>>> & visited_typed_nodes, MetaNode* p, int eid, map<int, HIN_Node> & hin_nodes_){
	bool visited_flag = false;
	vector<int> curr_types_id_ = hin_nodes_[eid].types_id_;
	for(int i = 0; i < curr_types_id_.size(); i++){
		bool tmp_flag = updateVisitedNodes(visited_typed_nodes, p, eid, curr_types_id_[i]);
		visited_flag = visited_flag || tmp_flag;	
	}
	return visited_flag;
}
bool updateVisitedNodes(map<int, map<int, vector<MetaNode*>>> & visited_typed_nodes, MetaNode* p, map<int, set<int>> & newNodes){
	bool visited_flag = true;	
	for(auto iter = newNodes.begin(); iter != newNodes.end(); iter++){
		int curr_node_type = iter->first;
		for(auto iter_n = iter->second.begin(); iter_n != iter->second.end(); iter_n++){
			bool tmp_flag = updateVisitedNodes(visited_typed_nodes, p, *iter_n, curr_node_type);
			 visited_flag = visited_flag && tmp_flag;
		}
	}
	return visited_flag;
}

// update next_nodes_id_
void exploreNextNodes(vector<HIN_Edge> & curr_edges_src_, vector<HIN_Edge> & curr_edges_dst_, map<int, map<int, set<int>>> & next_nodes_id_, map<int, set<int>> & edge_max_instances_num, int eid, map<int, HIN_Node> & hin_nodes_){

	vector<int> curr_types_id_ = hin_nodes_[eid].types_id_;
	// out-relation:
	for(vector<HIN_Edge>::iterator iter = curr_edges_src_.begin() ; iter != curr_edges_src_.end(); iter++){
		int temp_edge_type_ = iter->edge_type_;
				
		if(next_nodes_id_.find(temp_edge_type_) == next_nodes_id_.end()){
			next_nodes_id_[temp_edge_type_] = map<int, set<int>>();
		}
		if(next_nodes_id_[temp_edge_type_].find(iter->dst_) == next_nodes_id_[temp_edge_type_].end()){
			next_nodes_id_[temp_edge_type_][iter->dst_] = set<int>();
		}
		next_nodes_id_[temp_edge_type_][iter->dst_].insert(eid);
		if (edge_max_instances_num.find(temp_edge_type_) == edge_max_instances_num.end()) {
			edge_max_instances_num[temp_edge_type_] = set<int>();
		}
		edge_max_instances_num[temp_edge_type_].insert(iter->dst_);
	}
	// in-relation
	for(vector<HIN_Edge>::iterator iter = curr_edges_dst_.begin() ; iter != curr_edges_dst_.end(); iter++){
		int temp_edge_type_ = -(iter->edge_type_);
		

		if(next_nodes_id_.find(temp_edge_type_) == next_nodes_id_.end()){
			next_nodes_id_[temp_edge_type_] = map<int,set<int>>();
		}
		if(next_nodes_id_[temp_edge_type_].find(iter->src_) == next_nodes_id_[temp_edge_type_].end()){
						next_nodes_id_[temp_edge_type_][iter->src_] = set<int>();
				}
		next_nodes_id_[temp_edge_type_][iter->src_].insert(eid);
		if (edge_max_instances_num.find(temp_edge_type_) == edge_max_instances_num.end()) {
			edge_max_instances_num[temp_edge_type_] = set<int>();
		}
		edge_max_instances_num[temp_edge_type_].insert(iter->src_);
	}
}


map<MetaNode*, set<int>> getHitMetaNode(map<int, set<int>> & curr_nodes_with_parents, map<int, map<int, vector<MetaNode*>>> & visitedNodes, map<int, HIN_Node> & hin_nodes_){
	map<MetaNode*, set<int>> hit_score_node_p_list;
	for(auto iter = curr_nodes_with_parents.begin(); iter != curr_nodes_with_parents.end(); iter++){
		int curr_node = iter->first;
		if(hin_nodes_.find(curr_node) != hin_nodes_.end()){
			vector<int> curr_types_id = hin_nodes_[curr_node].types_id_;
			for(int i = 0; i < curr_types_id.size(); i++){
				int curr_type = curr_types_id[i];
				if(visitedNodes.find(curr_type) != visitedNodes.end()){
					if(visitedNodes[curr_type].find(curr_node) != visitedNodes[curr_type].end()){
						vector<MetaNode*> tmp_score_node_p_list = visitedNodes[curr_type][curr_node];
						for(int i = 0; i < tmp_score_node_p_list.size(); i++){
							MetaNode* tmp_score_node_p = tmp_score_node_p_list[i];
							if(hit_score_node_p_list.find(tmp_score_node_p) == hit_score_node_p_list.end()){
								hit_score_node_p_list[tmp_score_node_p] = set<int>();
							}						
							hit_score_node_p_list[tmp_score_node_p].insert(curr_node);
						}						
						tmp_score_node_p_list.clear();
					}
					//visitedNodes[curr_type].erase(curr_node);
				}
				
				if(visitedNodes[curr_type].size() == 0){
					visitedNodes.erase(curr_type);
				}
			}
		}
	}
	return hit_score_node_p_list;
}

map<int, set<int>> getTypedNodes(map<int, set<int>> nodes_with_parents, map<int, HIN_Node> hin_nodes_){
	map<int, set<int>> typed_nodes;
	for(map<int, set<int>>::iterator iter = nodes_with_parents.begin(); iter != nodes_with_parents.end(); iter++){
		int next_node = iter->first;
		vector<int> curr_types_id_ = hin_nodes_[next_node].types_id_;
		for(int i = 0; i < curr_types_id_.size(); i++){
			int curr_node_type = curr_types_id_[i];
			if(typed_nodes.find(curr_node_type) == typed_nodes.end()){
				typed_nodes[curr_node_type] = set<int>();
			}
			typed_nodes[curr_node_type].insert(next_node);
		}		
	}
	return typed_nodes;
}

bool isVisited(map<int, set<int>> visited_typed_nodes, int eid, map<int, HIN_Node> hin_nodes_){
	return false;
	vector<int> curr_types_id_ = hin_nodes_[eid].types_id_;
	for(int i = 0; i < curr_types_id_.size(); i++){
		int curr_node_type = curr_types_id_[i];
		if(visited_typed_nodes.find(curr_node_type) != visited_typed_nodes.end()){
			set<int> curr_type_nodes = visited_typed_nodes[curr_node_type];
			if(curr_type_nodes.find(eid) != curr_type_nodes.end()){
				return true;
			}
		}
	}
	return false;
}

//return minimum (strength_*penalty(length))
void TopKCalculator::expandTfIdFNodes(int src, set<int> srcSimilarNodes, int dst, set<int> dstSimilarNodes, int k, bool direction_flag, MetaNode* last_p, map<int, map<int, set<int>>> & next_nodes_id_, map<int, int> minSimilarNodesNum, double oppo_max_sl, map<int, set<int>> & edge_max_instances_num, map<int, map<int, vector<MetaNode*>>> & visitedNodes, map<int, map<int, vector<MetaNode*>>> & oppo_visitedNodes, priority_queue<MetaNode*, vector<MetaNode*>, MetaNodePointerCmp> & q, priority_queue<pair<vector<double>, vector<int>>, vector<pair<vector<double>, vector<int>>>, TopKMetaPathsCmp> & topKMetaPathsQ, set<string> & topKMetaPathsSet, set<MetaNode*> & sl_set, HIN_Graph & hin_graph_){
	double curr_max_mni;
	vector<int> meta_path;
	map<int, HIN_Node> hin_nodes_ = hin_graph_.nodes_;
	if(last_p != NULL){
		curr_max_mni = last_p->max_mni_;
		meta_path = last_p->meta_path_;
	}else{
		curr_max_mni = hin_nodes_.size();
	}
	TopKCalculator::similarPairsSize = srcSimilarNodes.size() + dstSimilarNodes.size() - 1;
	double maxRarity = getRarity(TopKCalculator::similarPairsSize, 1);
	double least_score = maxRarity*curr_max_mni;
	double topK_full_flag = topKMetaPathsQ.size() == k;
	if(topKMetaPathsQ.size() != 0)
		least_score = topKMetaPathsQ.top().first[0];
	for(auto iter = next_nodes_id_.begin(); iter != next_nodes_id_.end(); iter++){
		int curr_edge_type = iter->first;
		vector<int> temp_meta_path = meta_path;
		temp_meta_path.push_back(curr_edge_type);
		map<int, set<int>> next_nodes_with_parents = iter->second;
		if(direction_flag){
			if(next_nodes_with_parents.find(src) != next_nodes_with_parents.end()){
				next_nodes_with_parents.erase(src);
			}
		}else{
			
			if(next_nodes_with_parents.find(dst) != next_nodes_with_parents.end()){
				next_nodes_with_parents.erase(dst);
			}
		}
		
		double candidate_max_mni = 1.0*edge_max_instances_num[curr_edge_type].size();	
		if (candidate_max_mni > curr_max_mni) { // keep the minimum instance number in the meta path expanding
			candidate_max_mni = curr_max_mni;
		}
		double min_similar_nodes_num;
		double strength = getEdgeTypeStrength(curr_edge_type, hin_graph_);
		if(last_p != NULL){
			if(support_type_ == 5){
				strength = pow(last_p->strength_, strength);
			}else{
				strength *= last_p->strength_;
			}
			min_similar_nodes_num = last_p->min_similar_nodes_num_;
		}else{
			min_similar_nodes_num = minSimilarNodesNum[curr_edge_type];
			if(support_type_ == 5){
				strength = exp(strength);
			}
		}
		
		candidate_max_mni = getMaxMNI(candidate_max_mni);	
		MetaNode* temp_score_node_p = new MetaNode(curr_edge_type, next_nodes_with_parents, candidate_max_mni, strength, temp_meta_path, min_similar_nodes_num, last_p);

		if(topK_full_flag){
			if(temp_score_node_p->strength_times_penalty_*( getRarity(TopKCalculator::similarPairsSize, min_similar_nodes_num))*oppo_max_sl*candidate_max_mni < least_score){
			//if(temp_score_node_p->strength_times_penalty_*( getRarity(TopKCalculator::similarPairsSize, min_similar_nodes_num))*candidate_max_mni < least_score){
			//if(temp_score_node_p->strength_times_penalty_*maxRarity*oppo_max_sl*candidate_max_mni < least_score){
				delete temp_score_node_p;
				continue;
			}
		}


		map<int, set<int>> next_typed_nodes = getTypedNodes(next_nodes_with_parents, hin_nodes_);
		updateVisitedNodes(visitedNodes, temp_score_node_p, next_typed_nodes);

		q.push(temp_score_node_p);
		sl_set.insert(temp_score_node_p);
		map<MetaNode*, set<int>> middle_score_node_p_list;
		if(TopKCalculator::bi_directional_flag_){
			middle_score_node_p_list =  getHitMetaNode(next_nodes_with_parents, oppo_visitedNodes, hin_nodes_);
		}else{
			if(next_nodes_with_parents.find(dst) != next_nodes_with_parents.end()){
				middle_score_node_p_list[NULL]=	next_nodes_with_parents[dst];
			}
		}
		if(middle_score_node_p_list.size() != 0){
			for(map<MetaNode*, set<int>>::iterator iter_n = middle_score_node_p_list.begin(); iter_n != middle_score_node_p_list.end(); iter_n++){
				MetaNode* tmp_middle_score_node_p = iter_n->first;
				set<int> tmp_middle_nodes = iter_n->second;	
				vector<int> meta_path_r = vector<int>();
				double tmp_strength_times_penalty = 1.0;
				int tmp_min_similar_nodes_num = 1;
				if(tmp_middle_score_node_p != NULL){
					meta_path_r = tmp_middle_score_node_p->meta_path_;
					tmp_strength_times_penalty = tmp_middle_score_node_p->strength_times_penalty_;
					tmp_min_similar_nodes_num = tmp_middle_score_node_p->min_similar_nodes_num_;
				}

				vector<int> merged_meta_path = getMergedMetaPath(temp_meta_path, meta_path_r, direction_flag);
				string merged_meta_path_str = metapathToString(merged_meta_path);
				//cout << "connected meta paths: " << merged_meta_path_str << " curr meta paths: " << metapathToString(temp_meta_path) << endl;
				if(topKMetaPathsSet.find(merged_meta_path_str) != topKMetaPathsSet.end()){
					continue;
				}
				maxRarity = getRarity(TopKCalculator::similarPairsSize, (tmp_min_similar_nodes_num + min_similar_nodes_num - 1));
				if(topK_full_flag){
				double tmp_score_ub = maxRarity*candidate_max_mni*tmp_strength_times_penalty*temp_score_node_p->strength_times_penalty_;
					if(tmp_score_ub < least_score)
						continue;
				}
				set<int> srcNextNodes;
				set<int> dstNextNodes;
				double support = getSupport(tmp_middle_nodes, src, dst, direction_flag, temp_score_node_p, tmp_middle_score_node_p, temp_meta_path, meta_path_r, srcNextNodes, dstNextNodes, hin_graph_);
				if(topK_full_flag){
					/*
					set<int> tmpSrcSimilarNodes;
					SimCalculator::getNextEntities(srcNextNodes, -merged_meta_path.front(), tmpSrcSimilarNodes, hin_graph_);
					set<int> tmpDstSimilarNodes;
					SimCalculator::getNextEntities(dstNextNodes, merged_meta_path.back(), tmpDstSimilarNodes, hin_graph_);
					maxRarity = log(1 + similarPairsSize/(tmpSrcSimilarNodes.size() + tmpDstSimilarNodes.size() - 1));	*/
					double tmp_score_ub = support*maxRarity*penalty(merged_meta_path.size());
					if(tmp_score_ub < least_score)
						continue;
				}

				double rarity = getRarity(src, dst, srcSimilarNodes, dstSimilarNodes, merged_meta_path, hin_graph_); // light weight
				//double rarity = getRarity(src, dst, temp_score_node_p, tmp_middle_score_node_p, srcSimilarNodes, dstSimilarNodes, temp_meta_path, meta_path_r, hin_graph_, direction_flag); // light weight
				double score = support * rarity * penalty(merged_meta_path.size());
				updateTopKMetaPaths(score, support, rarity, merged_meta_path, k, topKMetaPathsQ, topKMetaPathsSet);
				tmp_middle_nodes.clear();
				iter_n->second.clear();
				srcNextNodes.clear();
				dstNextNodes.clear();
				iter_n->second.clear();
			} 
					
		}

		//clear memory
		for(auto it = next_typed_nodes.begin(); it != next_typed_nodes.end(); it++){
			it->second.clear();
		}
		next_typed_nodes.clear();
		for(auto it = next_nodes_with_parents.begin(); it != next_nodes_with_parents.end(); it++){
			it->second.clear();
		}
		next_nodes_with_parents.clear();
		for(auto it = iter->second.begin(); it != iter->second.end(); it++){
			it->second.clear();
		}
		iter->second.clear();
	
	}
	hin_nodes_.clear();
			
}

double getMaxSL(set<MetaNode*> sl_set){
	double max_sl = 0.0;
	MetaNode* tmp_score_node_p = NULL;
	for(auto iter = sl_set.begin(); iter != sl_set.end(); iter++){
		if(max_sl < (*iter)->strength_times_penalty_){
			max_sl = (*iter)->strength_times_penalty_;
			tmp_score_node_p = *iter;
		}
	}
	return max_sl;
}

void TopKCalculator::updateMinSimilarNodesNum(map<int, map<int, set<int>>> & next_nodes_id_, map<int, int> & minSimilarNodesNum, HIN_Graph & hin_graph_){
	int max_similar_nodes_num = hin_graph_.nodes_.size();
	for(auto iter = next_nodes_id_.begin(); iter != next_nodes_id_.end(); iter++){
		int curr_edge_type = iter->first;
		set<int> tmp_eids;
		int min_similar_nodes_num = max_similar_nodes_num;
		if(rarity_type_ == 0) continue;
		for(auto iter_n = iter->second.begin(); iter_n != iter->second.end(); iter_n++){
			set<int> tmp_next_nodes;
			SimCalculator::getNextEntities(iter_n->first, -curr_edge_type, tmp_next_nodes, hin_graph_);		
			if(tmp_next_nodes.size() < min_similar_nodes_num){
				min_similar_nodes_num = tmp_next_nodes.size();
			}
		} 
		
		minSimilarNodesNum[curr_edge_type] = min_similar_nodes_num;
		//minSimilarNodesNum[curr_edge_type] = 1;
		
	}	
}

vector<pair<vector<double>, vector<int>>> TopKCalculator::getTopKMetaPath_TFIDF(int src, int dst, int k, HIN_Graph & hin_graph_)
{
	vector<pair<vector<double>, vector<int>>> topKMetaPathsVector;
	set<string> topKMetaPathsSet;
	priority_queue<pair<vector<double>, vector<int>>, vector<pair<vector<double>, vector<int>>>, TopKMetaPathsCmp> topKMetaPathsQ;
	map<int, vector<HIN_Edge> > hin_edges_src_ = hin_graph_.edges_src_;
	map<int, vector<HIN_Edge> > hin_edges_dst_ = hin_graph_.edges_dst_;
	map<int, HIN_Node> hin_nodes_ = hin_graph_.nodes_;
    	map<int, pair<double, double>> edgeTypeAvgDegree = hin_graph_.edge_type_avg_degree_;	

	bool max_candidates_flag = false;
	int max_candidates_num = 0;

	if((hin_edges_src_.find(src) == hin_edges_src_.end() && hin_edges_dst_.find(src) == hin_edges_dst_.end()) || (hin_edges_src_.find(dst) == hin_edges_src_.end() && hin_edges_dst_.find(dst) == hin_edges_dst_.end()))
	{
		return topKMetaPathsVector;
	}
	// /*
	int temp_node;
	int src_edges = hin_edges_src_[src].size() + hin_edges_dst_[src].size();
	//cout << src_edges << endl;
	int dst_edges = hin_edges_src_[dst].size() + hin_edges_dst_[dst].size();
	//cout << dst_edges << endl;
	// similar pairs information
	set<int> srcSimilarNodes = getSimilarNodes(src, hin_graph_.nodes_);
	srcSimilarNodes.insert(src);
	set<int> dstSimilarNodes = getSimilarNodes(dst, hin_graph_.nodes_);
	dstSimilarNodes.insert(dst);
	//int similarPairsSize = srcSimilarNodes.size()*dstSimilarNodes.size(); // orignal 
	TopKCalculator::similarPairsSize = srcSimilarNodes.size() + dstSimilarNodes.size() - 1; // light version
	double maxRarity =  getRarity(similarPairsSize, 1);
	if(rarity_type_ == 0){
		maxRarity = 1.0;
	}	
	//cout << "max rarity: " << maxRarity << endl;
	double maxSupport = 1.0;
	double maxMeta = hin_nodes_.size()*maxRarity;
	/*
	if(support_type_ == 1){
		maxSupport = ???;	
	}
	*/
	
	// cout << maxRarity << endl; 
	// queue initialize
	clock_t t2, t1;
	t1 = clock();
	priority_queue<MetaNode*, vector<MetaNode*>, MetaNodePointerCmp> qL;
	priority_queue<MetaNode*, vector<MetaNode*>, MetaNodePointerCmp> qR;
	set<MetaNode*> sl_set_L;
	set<MetaNode*> sl_set_R;
	
	vector<MetaNode*> tmpMetaNodeList;
	map<int, map<int, vector<MetaNode*>>> visitedNodesL;
	map<int, map<int, vector<MetaNode*>>> visitedNodesR;
	updateVisitedNodes(visitedNodesL, NULL, src, hin_nodes_);
	updateVisitedNodes(visitedNodesR, NULL, dst, hin_nodes_);

	map<int, int> minSimilarNodesNumSrc; // edge_type -> minimum similar nodes size
	map<int, int> minSimilarNodesNumDst;
	map<int, map<int, set<int>>> next_nodes_id_; // edge type -> (destination id -> parent node ids)
	map<int, set<int>> edge_max_instances_num; // edge type -> instances
	
	srand((unsigned)time(NULL));

	vector<HIN_Edge> curr_edges_src_ = hin_edges_src_[src];
	vector<HIN_Edge> curr_edges_dst_ = hin_edges_dst_[src];
	
	vector<int> init_meta_path = vector<int>();	

	exploreNextNodes(curr_edges_src_, curr_edges_dst_, next_nodes_id_, edge_max_instances_num, src, hin_nodes_);
	curr_edges_src_.clear();
	curr_edges_dst_.clear();
	updateMinSimilarNodesNum(next_nodes_id_, minSimilarNodesNumSrc, hin_graph_);
	expandTfIdFNodes(src, srcSimilarNodes, dst, dstSimilarNodes, k, true, NULL, next_nodes_id_, minSimilarNodesNumSrc, 1, edge_max_instances_num, visitedNodesL, visitedNodesR, qL, topKMetaPathsQ, topKMetaPathsSet, sl_set_L, hin_graph_);	
	next_nodes_id_.clear();	
	edge_max_instances_num.clear();
	double max_sl_l = getMaxSL(sl_set_L);
	
	double max_sl_r = 1.0;
	if(TopKCalculator::bi_directional_flag_){
		exploreNextNodes(hin_edges_src_[dst], hin_edges_dst_[dst], next_nodes_id_, edge_max_instances_num, dst, hin_nodes_);	
		updateMinSimilarNodesNum(next_nodes_id_, minSimilarNodesNumDst, hin_graph_);
		expandTfIdFNodes(src, srcSimilarNodes, dst, dstSimilarNodes, k, false, NULL, next_nodes_id_, minSimilarNodesNumDst, max_sl_l, edge_max_instances_num, visitedNodesR, visitedNodesL, qR, topKMetaPathsQ, topKMetaPathsSet, sl_set_R, hin_graph_);	
		next_nodes_id_.clear();	
		edge_max_instances_num.clear();
		max_sl_r = getMaxSL(sl_set_R);

	}
	int iteration = -1;	
	// BFS
	while(!qL.empty() && (!TopKCalculator::bi_directional_flag_ || !qR.empty())){
		bool leftFlag = true;
		iteration += 1;
		t1 = clock();
		MetaNode* curr_score_node_p;
		MetaNode* opp_score_node_p;
		MetaNode* qLTop = qL.top();
		MetaNode* qRTop = NULL;
		if(qR.size() != 0) qRTop = qR.top();
		max_sl_l = getMaxSL(sl_set_L);
		if(TopKCalculator::bi_directional_flag_)	max_sl_r = getMaxSL(sl_set_R);
	//cout << "max_sl_l: " << max_sl_l << "\t";
	//cout << "max_sl_r: " << max_sl_r << "\t" << endl;
		//cout << qL.size() <<" " << qR.size() << endl;
		double max_mni = qLTop->max_mni_;
		if(TopKCalculator::bi_directional_flag_){
			if(max_mni < qRTop->max_mni_){
				max_mni = qRTop->max_mni_;
			}
		}
		double qLTopMaxMeta =  qLTop->partial_ub_score*max_sl_r;
		//double qLTopMaxMeta =  qLTop->partial_ub_score;
		double qRTopMaxMeta = TopKCalculator::bi_directional_flag_ ? qRTop->partial_ub_score*max_sl_l:maxMeta;
		//double qRTopMaxMeta = TopKCalculator::bi_directional_flag_ ? qRTop->partial_ub_score:maxMeta;
		double tmp_max_score;
	//cout << "qL score: " << qLTopMaxMeta << "\t";
	//cout << "qR score: " << qRTopMaxMeta << "\t" << endl;
	//cout << "l max strength length: " << max_sl_l  << "\t";
	//cout << "r max strength length: " << max_sl_r  << "\t" << endl;
	//cout << endl;
		double opp_max_strength_times_penalty;
		double curr_max_strength_times_penalty;
		if(!TopKCalculator::bi_directional_flag_ ||  qLTopMaxMeta < qRTopMaxMeta || (qLTopMaxMeta == qRTopMaxMeta && qLTop->size() < qRTop->size())){
			curr_score_node_p = qLTop;
			qL.pop();
			opp_score_node_p = qRTop;
			sl_set_L.erase(curr_score_node_p);
			opp_max_strength_times_penalty = max_sl_r;
			curr_max_strength_times_penalty = max_sl_l;
			tmp_max_score = qLTopMaxMeta;
		}else{
			leftFlag = false;
			curr_score_node_p = qRTop;
			qR.pop();
			opp_score_node_p = qLTop;
			sl_set_R.erase(curr_score_node_p);
			opp_max_strength_times_penalty = max_sl_l;
			curr_max_strength_times_penalty = max_sl_r;
			tmp_max_score = qRTopMaxMeta;
		}
		tmpMetaNodeList.push_back(curr_score_node_p); 

		vector<int> meta_path = curr_score_node_p->meta_path_;		
		//cout << leftFlag << " " << metapathToString(meta_path) << endl;
		
		double curr_max_mni = curr_score_node_p->max_mni_;


		if(topKMetaPathsQ.size() == k){ // stop if maximum score in the queue is less than the minimum one in found meta paths
			//cout << curr_max_support << " " << meta_path.size() << " " << curr_max_support*penalty(meta_path.size())*maxRarity << " " << topKMetaPath_.back().first[0] << endl;
			//cout << curr_score_node_p->max_mni_ << endl; 
			//double tmp_max_support = getMaxMeta(currMetaNodeListL, currMetaNodeListR, maxRarity);
			//tmp_max_score = qLTopMaxMeta + qRTopMaxMeta - tmp_max_score;
			double stop_upper_bound = topKMetaPathsQ.top().first[0];
			//cout << "Iteration: " << iteration << "\tUB Score: " << tmp_max_score << "\tExplored Minimum Score: "  << stop_upper_bound << endl;
			if(tmp_max_score == stop_upper_bound){
				max_candidates_num += 1;
				if(max_candidates_num == MAX_CANDIDATES_NUM){
					max_candidates_flag == true;
				}
			}


			if(tmp_max_score < stop_upper_bound || max_candidates_flag){
				//cout << tmp_max_score << " " << stop_upper_bound << endl;
				break;
			}	
		}
		
		
		map<int, set<int>> curr_nodes_with_parents = curr_score_node_p->curr_nodes_with_parents_;
		
		next_nodes_id_.clear(); // edge type -> (destination id -> parent node id)
		for(map<int, set<int>>::iterator iter = curr_nodes_with_parents.begin(); iter != curr_nodes_with_parents.end(); iter++){
			int eid = iter->first;
			iter->second.clear();
			if(hin_edges_src_.find(eid) == hin_edges_src_.end() && hin_edges_dst_.find(eid) == hin_edges_dst_.end()){
				continue;
			}
			if(eid == src || eid == dst) continue;
			vector<HIN_Edge> temp_edges_src_ = hin_edges_src_[eid];
			vector<HIN_Edge> temp_edges_dst_ = hin_edges_dst_[eid];
			exploreNextNodes(temp_edges_src_, temp_edges_dst_, next_nodes_id_, edge_max_instances_num, eid, hin_nodes_);
			temp_edges_src_.clear();
			temp_edges_dst_.clear();	
		}

		if(leftFlag){
			expandTfIdFNodes(src, srcSimilarNodes, dst, dstSimilarNodes, k, true, curr_score_node_p, next_nodes_id_, minSimilarNodesNumSrc, max_sl_r, edge_max_instances_num, visitedNodesL, visitedNodesR, qL, topKMetaPathsQ, topKMetaPathsSet, sl_set_L, hin_graph_);
		}else{
			expandTfIdFNodes(src, srcSimilarNodes, dst, dstSimilarNodes, k, false, curr_score_node_p, next_nodes_id_, minSimilarNodesNumDst, max_sl_l, edge_max_instances_num, visitedNodesR, visitedNodesL, qR, topKMetaPathsQ, topKMetaPathsSet, sl_set_R, hin_graph_);
		}

		//clear memory
		for(auto iter = curr_nodes_with_parents.begin(); iter != curr_nodes_with_parents.end(); iter++){
			iter->second.clear();
		}		
		curr_nodes_with_parents.clear();
		for(auto iter = next_nodes_id_.begin(); iter != next_nodes_id_.end(); iter++){
			for(auto it = iter->second.begin(); it != iter->second.end(); it++){
				it->second.clear();
			}
			iter->second.clear();
		}
		next_nodes_id_.clear();
		for(auto iter = edge_max_instances_num.begin(); iter != edge_max_instances_num.end(); iter++){
			iter->second.clear();
		}
		edge_max_instances_num.clear();

	}

	while(!topKMetaPathsQ.empty()){
		topKMetaPathsVector.insert(topKMetaPathsVector.begin(), topKMetaPathsQ.top());
		topKMetaPathsQ.pop();
	}

	int l_left_max = 1;
	for(auto iter = visitedNodesL.begin(); iter != visitedNodesL.end(); iter++){
		for(auto iter_n = iter->second.begin(); iter_n != iter->second.end(); iter_n++){
			for(int i = 0; i < iter_n->second.size(); i++){
				if(iter_n->second[i] != NULL && iter_n->second[i]->meta_path_.size() > l_left_max) l_left_max = iter_n->second[i]->meta_path_.size();
			}
		}
	}
	cout << "Maximum depth in left side: " <<  l_left_max << endl;

	int l_right_max = 1;
	for(auto iter = visitedNodesR.begin(); iter != visitedNodesR.end(); iter++){
		for(auto iter_n = iter->second.begin(); iter_n != iter->second.end(); iter_n++){
			for(int i = 0; i < iter_n->second.size(); i++){
				if( iter_n->second[i] != NULL && iter_n->second[i]->meta_path_.size() > l_right_max) l_right_max = iter_n->second[i]->meta_path_.size();
			}
		}
	}
	cout << "Maximum depth in right side: " << l_right_max << endl;

	for(auto i = 0; i < tmpMetaNodeList.size(); i++){
		delete tmpMetaNodeList[i];
	}

	 while(!qL.empty()){
		MetaNode* tmp_score_node_p = qL.top();
		qL.pop();
		delete tmp_score_node_p;
	}

	for(auto iter = visitedNodesL.begin(); iter != visitedNodesL.end(); iter++){
		for(auto iter_n = iter->second.begin(); iter_n != iter->second.end(); iter_n++){
			iter_n->second.clear();
		}
		iter->second.clear();
	}
	visitedNodesL.clear();

	while(!qR.empty()){
		MetaNode* tmp_score_node_p = qR.top();
		qR.pop();
		delete tmp_score_node_p;
	}
	for(auto iter = visitedNodesR.begin(); iter != visitedNodesR.end(); iter++){
		for(auto iter_n = iter->second.begin(); iter_n != iter->second.end(); iter_n++){
			iter_n->second.clear();
		}
		iter->second.clear();
	}
	visitedNodesR.clear();
	sl_set_L.clear();
	sl_set_R.clear();
	
	for(auto iter = hin_edges_src_.begin(); iter != hin_edges_src_.end(); iter++){
		iter->second.clear();
	}
	hin_edges_src_.clear();
	
	for(auto iter = hin_edges_dst_.begin(); iter != hin_edges_dst_.end(); iter++){
		iter->second.clear();
	}
	hin_edges_dst_.clear();
	srcSimilarNodes.clear();
	dstSimilarNodes.clear();
	hin_nodes_.clear();
	
	return topKMetaPathsVector;
}

vector<pair<vector<double>, vector<int>>> TopKCalculator::getTopKMetaPath_Refiner(int src, int dst, int k, vector<vector<int>> & meta_paths, int score_function, HIN_Graph & hin_graph_){
	vector<pair<vector<double>, vector<int>>> topKMetaPath_;	
	priority_queue<pair<double, int>, vector<pair<double, int>>, ScorePairCmp> q;	

	for(int i = 0; i < meta_paths.size(); i++){
		double score = getMetaPathScore(src, dst, meta_paths[i], score_function, hin_graph_);
		cerr << score << "\t" << metapathToString(meta_paths[i]) << endl;
		q.push(make_pair(score, i));
	}

	for(int j = 0; j < k; j++){
		pair<double, int> currScorePair = q.top();
		topKMetaPath_.push_back(make_pair(vector<double> (1, currScorePair.first), meta_paths[currScorePair.second]));
		q.pop();
	}	
	
	return topKMetaPath_;

}

void TopKCalculator::getDstEntities(int src, vector<int> meta_path, set<int> & dst_entities, HIN_Graph & hin_graph_){
	bool full_flag = false;
	dst_entities.clear();
	set<int> curr_entities;
	set<int> next_entities;
	map<int, int> type2counter;

	curr_entities.insert(src);
	for(int i = 0; i < meta_path.size(); i++){
		for(set<int>::iterator iter = curr_entities.begin(); iter != curr_entities.end(); iter++){
			set<int> tmp_next_entities;
			SimCalculator::getNextEntities(*iter, meta_path[i], tmp_next_entities, hin_graph_);
			for(set<int>::iterator iter_n = tmp_next_entities.begin(); iter_n != tmp_next_entities.end(); iter_n++){
				next_entities.insert(*iter_n);
				if(full_flag){
					break;
				}
			}
			if(full_flag){
				break;
			}
		}

		if(!full_flag){

			if(type2counter.find(meta_path[i]) == type2counter.end()){
				int eid = *(next_entities.begin());	
				set<int> candidates = getSimilarNodes(eid, hin_graph_.nodes_, false, false);
				type2counter[meta_path[i]] = candidates.size();
				
				
			}
			if(next_entities.size() >= type2counter[meta_path[i]]){ 
				full_flag = true;	
			}	
		}
		curr_entities = next_entities;
		next_entities.clear();
	}
	if(!full_flag){
		dst_entities = curr_entities;
	}else{
		dst_entities = getSimilarNodes(*(curr_entities.begin()), hin_graph_.nodes_, false, false);
	}
}

string TopKCalculator::metapathToString(vector<int> meta_path){
	string result="";
	if(meta_path.size() != 0){
		result = result + to_string(meta_path[0]);
		for(auto i = 1; i < meta_path.size(); i++){
			result = result + "\t" + to_string(meta_path[i]);
		}	
	}
	return result;
}

vector<int> TopKCalculator::stringToMetapath(string meta_path_str){
	vector<string> strs = split(meta_path_str, "\t");	
	vector<int> meta_path;
	for(auto i = 0; i < strs.size(); i++){
		meta_path.push_back(atoi(strs[i].c_str()));
	}
	return meta_path;
}

void TopKCalculator::saveToFile(vector<vector<int>> topKMetaPaths, string file_name){
	cerr << "start saving the top k meta-paths to " << file_name << "..." << endl;	
	ofstream topKMetaPathsOut(file_name, ios::out);
	for(int i = 0; i < topKMetaPaths.size(); i++){
		vector<int> currMetaPath = topKMetaPaths[i];
		for(int j = 0; j < currMetaPath.size() - 1; j++){
			topKMetaPathsOut << currMetaPath[j] << "\t";
		}
		topKMetaPathsOut << currMetaPath.back() << endl;

	} 
	topKMetaPathsOut.close();
	cerr << "finished saving meta-paths" << endl;

}

vector<vector<int>> TopKCalculator::loadMetaPaths(string file_name){
	cerr << "start loading the meta-paths..." << endl;
	ifstream topKMetaPathsIn(file_name.c_str(), ios::in);
	string line;
	vector<vector<int>> meta_paths;
	while(getline(topKMetaPathsIn, line)){
		vector<string> strs = split(line, "\t");
		meta_paths.push_back(vector<int>());
		for(int i = 0; i < strs.size(); i++){
			meta_paths.back().push_back(atoi(strs[i].c_str()));
		}

	}
	topKMetaPathsIn.close();
	return meta_paths;
}
