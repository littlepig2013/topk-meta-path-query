
#include <cmath>

#include "SimCalculator.h"

double SimCalculator::getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_){
	if(isConnected(src, dst, meta_path, hin_graph_)){
		return  getMetaPathBasedSim(src, dst, meta_path, hin_graph_, heteSim_flag, gamma);
	}else{
		return 0.0;
	}
}

double DFS_SimCalculator::getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSim_flag, double gamma){
	
	double result = 0.0;
	if(meta_path.size() == 0){
		return result;
	}
	set<int> next_entities;
	getNextEntities(src, meta_path.front(), next_entities, hin_graph_);
	if(next_entities.size() == 0){
		return result;
	}	
	double multiplier = 1.0/pow(next_entities.size(), gamma);
	if(meta_path.size() == 1){
		if(next_entities.find(dst) != next_entities.end()){
			if(heteSim_flag){
				next_entities.clear();
				getNextEntities(dst, -meta_path.front(), next_entities, hin_graph_);
				multiplier *= 1.0/pow(next_entities.size(), gamma);
			}
			return multiplier;
		}else{
			return result;
		}
	}
	vector<int> temp_meta_path (meta_path.begin() + 1, meta_path.end());
	for(set<int>::iterator iter = next_entities.begin(); iter != next_entities.end(); iter++){
		double tmp_multiplier = multiplier;
		if(heteSim_flag){
			set<int> tmp_next_entities;
			getNextEntities(*iter, -meta_path.front(), tmp_next_entities, hin_graph_);
			tmp_multiplier *= 1.0/pow(tmp_next_entities.size(), gamma);
		}
		result += tmp_multiplier*getMetaPathBasedSim(*iter, dst, temp_meta_path, hin_graph_, heteSim_flag, gamma); 
	}

	return result;
}


double BFS_SimCalculator::getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSim_flag, double gamma){
	
	map<int, double> curr_weight_entities;
	map<int, double> next_weight_entities;
	curr_weight_entities[src] = 1.0;
	
	int i = 0;
	int meta_path_size = meta_path.size();
	while(i < meta_path_size){
		next_weight_entities.clear();
		for(map<int, double>::iterator iter=curr_weight_entities.begin(); iter != curr_weight_entities.end(); iter++){
			set<int> next_entities;
			int curr_entity = iter->first;
			double curr_weight = iter->second;
			getNextEntities(curr_entity, meta_path[i], next_entities, hin_graph_);
			if(next_entities.size() != 0){
				
				double weight = curr_weight/(pow(next_entities.size(), gamma));
				for(set<int>::iterator iter_n = next_entities.begin(); iter_n != next_entities.end();iter_n++){
					int tmp_entity = *iter_n;
					double tmp_weight = weight;

					if(heteSim_flag){
						set<int> pre_entities;
						getNextEntities(tmp_entity, -meta_path[i], pre_entities, hin_graph_);
						int pre_entities_size = pre_entities.size();
						if(pre_entities_size != 0){
							tmp_weight *= 1.0/pre_entities_size;
						}else{
							tmp_weight = 0.0;	
						}
					}
					
					if(next_weight_entities.find(tmp_entity) != next_weight_entities.end()){
						next_weight_entities[tmp_entity] += tmp_weight;
					}else{
						next_weight_entities[tmp_entity] = tmp_weight;
					}
				}
			}

		}
		curr_weight_entities.clear();
		curr_weight_entities = next_weight_entities;
		i++;
	}

	if(curr_weight_entities.find(dst) != curr_weight_entities.end()){
		return curr_weight_entities[dst];
	}else{
		return 0.0;
	}
}

double getMergedPCRWMap(map<int, double> entities1, map<int, double> entities2){
	map<int, double> tmp_entities1;
	map<int, double> tmp_entities2;
	if(entities1.size() > entities2.size()){
		tmp_entities1 = entities1;
		tmp_entities2 = entities2;
	}else{
		tmp_entities1 = entities2;
		tmp_entities2 = entities1;
	}
	double result = 0.0;
	for(map<int, double>::iterator iter= tmp_entities2.begin(); iter != tmp_entities2.end(); iter++){
		if(tmp_entities1.find(iter->first) != tmp_entities1.end()){
			result += iter->second*tmp_entities1[iter->first];
		}	
	}
	return result;
}

bool isOverlapped(set<int> entities1, set<int> entities2){
	set<int> tmp_entities1;
	set<int> tmp_entities2;
	if(entities1.size() > entities2.size()){
		tmp_entities1 = entities1;
		tmp_entities2 = entities2;
	}else{
		tmp_entities1 = entities2;
		tmp_entities2 = entities1;
	}

	for(auto iter = entities2.begin(); iter != entities2.end(); iter++){
		if(entities1.find(*iter) != entities1.end()){
			return true;
		}
	}
	return false;
}

bool Bi_BFS_SimCalculator::isConnected(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_){
	set<int> src_entities;
	set<int> dst_entities;
	src_entities.insert(src);
	dst_entities.insert(dst);
	return isConnected(src_entities, dst_entities, meta_path, hin_graph_);
}

bool Bi_BFS_SimCalculator::isConnected(set<int> src_entities, set<int> dst_entities, vector<int> meta_path, HIN_Graph & hin_graph_){
	int meta_path_size = meta_path.size();
	int src_entities_size = src_entities.size();
	int dst_entities_size = dst_entities.size();

	if(meta_path_size == 0 || src_entities_size == 0 || dst_entities_size == 0){
		return 0.0;
	}else{
		vector<int> tmp_meta_path = meta_path;

		set<int> tmp_src_entities = src_entities;
		set<int> tmp_dst_entities = dst_entities;

		while(true){
			int tmp_meta_path_size = tmp_meta_path.size();
			set<int> next_entities;	
			int edge_type;

			if(tmp_src_entities.size() > tmp_dst_entities.size()){
				set<int> tmp_next_entities;
				edge_type = tmp_meta_path.back();	
				getNextEntities(tmp_dst_entities, -edge_type, next_entities, hin_graph_);
				if(tmp_meta_path_size == 1){
					return isOverlapped(tmp_src_entities, next_entities); 
				}else{
					tmp_dst_entities = next_entities;
					vector<int> next_meta_path (tmp_meta_path.begin(), tmp_meta_path.end() - 1); 
					tmp_meta_path = next_meta_path;
				}
			}else{
				edge_type = tmp_meta_path.front();
				getNextEntities(tmp_src_entities, edge_type, next_entities, hin_graph_);
				if(tmp_meta_path_size == 1){
					return isOverlapped(next_entities, tmp_dst_entities);
				}else{
					tmp_src_entities = next_entities;
					vector<int> next_meta_path (tmp_meta_path.begin() + 1, tmp_meta_path.end());
					tmp_meta_path = next_meta_path;
				}
			}

		}

	}

}

double Bi_BFS_SimCalculator::getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSim_flag, double gamma){

	map<int, double> src_entities;
	map<int, double> dst_entities;
	src_entities[src] = 1.0;
	dst_entities[dst] = 1.0;
	return getMetaPathBasedSim(src_entities, dst_entities, meta_path, hin_graph_, heteSim_flag, gamma);
}

double Bi_BFS_SimCalculator::getMetaPathBasedSim(map<int, double> src_entities, map<int, double> dst_entities, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSim_flag, double gamma){
	int meta_path_size = meta_path.size();
	int src_entities_size = src_entities.size();
	int dst_entities_size = dst_entities.size();

	if(meta_path_size == 0 || src_entities_size == 0 || dst_entities_size == 0){
		return 0.0;
	}else{
		vector<int> tmp_meta_path = meta_path;

		map<int, double> tmp_src_entities = src_entities;
		map<int, double> tmp_dst_entities = dst_entities;

		while(true){
			int tmp_meta_path_size = tmp_meta_path.size();
			map<int, double> next_entities;	
			int edge_type;

			if(tmp_src_entities.size() > tmp_dst_entities.size()){
				set<int> tmp_next_entities;
				edge_type = tmp_meta_path.back();	
				getNextWeightedEntities(tmp_dst_entities, -edge_type, next_entities, hin_graph_, heteSim_flag, true, gamma);
				if(tmp_meta_path_size == 1){
					return getMergedPCRWMap(tmp_src_entities, next_entities); 
				}else{
					tmp_dst_entities = next_entities;
					vector<int> next_meta_path (tmp_meta_path.begin(), tmp_meta_path.end() - 1); 
					tmp_meta_path = next_meta_path;
				}
			}else{
				edge_type = tmp_meta_path.front();
				getNextWeightedEntities(tmp_src_entities, edge_type, next_entities, hin_graph_, heteSim_flag, false, gamma);
				if(tmp_meta_path_size == 1){
					return getMergedPCRWMap(next_entities, tmp_dst_entities);
				}else{
					tmp_src_entities = next_entities;
					vector<int> next_meta_path (tmp_meta_path.begin() + 1, tmp_meta_path.end());
					tmp_meta_path = next_meta_path;
				}
			}

		}

	}
}

bool SimCalculator::isConnected(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_){
	return isConnectedMain(src, dst, set<int>(), set<int>(), meta_path, hin_graph_);
}
bool SimCalculator::isConnectedMain(int src, int dst, set<int> src_next_entities, set<int> dst_next_entities, vector<int> meta_path, HIN_Graph & hin_graph_){
	int meta_path_size = meta_path.size();
		if(meta_path_size == 0){
				return false;
		}else if(meta_path_size == 1){
		if(src_next_entities.size() == 0){
						getNextEntities(src, meta_path.front(), src_next_entities, hin_graph_);
				}

		if(src_next_entities.size() != 0 && src_next_entities.find(dst) != src_next_entities.end()){
			return true;
		}

		return false;
	}else{
		if(src_next_entities.size() == 0){
						getNextEntities(src, meta_path.front(), src_next_entities, hin_graph_);
				}
		if(src_next_entities.size() == 0){
			return false;
		}
		
		if(dst_next_entities.size() == 0){
						getNextEntities(dst, -meta_path.back(), dst_next_entities, hin_graph_);
				}
				if(dst_next_entities.size() == 0){
						return false;
				}

		if(src_next_entities.size() > dst_next_entities.size()){
			vector<int> left_meta_path (meta_path.begin(), meta_path.end() - 1);
						for(set<int>:: iterator iter = dst_next_entities.begin(); iter != dst_next_entities.end(); iter++){	
				if(isConnectedMain(src, *iter, src_next_entities, set<int> (), left_meta_path, hin_graph_)){
					return true;
				}
						}
		}else{
						for(set<int>::iterator iter = src_next_entities.begin(); iter != src_next_entities.end(); iter++){
								if(isConnectedMain(*iter, dst, set<int> (), dst_next_entities, vector<int> (meta_path.begin() + 1, meta_path.end()), hin_graph_)){
					return true;
				}
						}

		}

		return false;	
					
	}

}

void SimCalculator::getNextEntities(set<int> eids, int edge_type, set<int> & next_entities, HIN_Graph & hin_graph_){
	
	next_entities.clear();
	if(edge_type > 0){
		if(hin_graph_.edges_src_.size() != 0){
			for(set<int>::iterator iter = eids.begin(); iter != eids.end(); iter++){
				int eid = *iter;
				if(hin_graph_.edges_src_.find(eid) != hin_graph_.edges_src_.end()){
				vector<HIN_Edge> candidate_edges =hin_graph_.edges_src_[eid];
					for(int i = 0; i < candidate_edges.size(); i++){
						if(candidate_edges[i].edge_type_ == edge_type){
							next_entities.insert(candidate_edges[i].dst_);
						}
					}
				} 
			}
		}
	}else if(edge_type < 0){
		edge_type = -edge_type;

		if(hin_graph_.edges_dst_.size() != 0){
			for(set<int>::iterator iter = eids.begin(); iter != eids.end(); iter++){
				int eid = *iter;
				if(hin_graph_.edges_dst_.find(eid) != hin_graph_.edges_dst_.end()){
					vector<HIN_Edge> candidate_edges = hin_graph_.edges_dst_[eid];
					for(int i = 0; i < candidate_edges.size(); i++){
						if(candidate_edges[i].edge_type_ == edge_type){
							next_entities.insert(candidate_edges[i].src_);
						}
					}
				} 
			}
		}
	}

	

}
void SimCalculator::getNextEntities(int eid, int edge_type, set<int> & next_entities, HIN_Graph & hin_graph_){
	set<int> eids;
	eids.insert(eid);
	getNextEntities(eids, edge_type, next_entities, hin_graph_);
}
void SimCalculator::getNextWeightedEntities(map<int, double> eids, int edge_type, map<int, double> & next_entities, HIN_Graph & hin_graph_, bool heteSimFlag, bool inheritFlag, double gamma){
	next_entities.clear();
	for(map<int, double>::iterator iter = eids.begin(); iter != eids.end(); iter++){
		set<int> tmp_entities;
		int tmpSrc = iter->first;
		getNextEntities(tmpSrc, edge_type, tmp_entities, hin_graph_);
		if(tmp_entities.size() != 0){
			double tmpWeight = iter->second;
			if(heteSimFlag || !inheritFlag){
				tmpWeight /= pow(tmp_entities.size(), gamma);
			}
			for(set<int>::iterator iter_n = tmp_entities.begin(); iter_n != tmp_entities.end(); iter_n++){
				int tmp_entity = *iter_n;
				if(next_entities.find(tmp_entity) != next_entities.end()){
					next_entities[tmp_entity] += tmpWeight;
				}else{
					next_entities[tmp_entity] = tmpWeight;
				}
			}	
		}
	}

	if(heteSimFlag || inheritFlag){
		vector<int> null_entities;
		for(map<int, double>::iterator iter = next_entities.begin(); iter != next_entities.end(); iter++){
			int tmp_entity = iter->first;
			set<int> tmp_entities;
			getNextEntities(tmp_entity, -edge_type, tmp_entities, hin_graph_);
			if(tmp_entities.size() == 0){
				iter->second = 0.0;
				null_entities.push_back(tmp_entity);
			}else{
				iter->second /= pow(tmp_entities.size(), gamma);
			}
		}
		for(int i = 0; i < null_entities.size(); i++){
			int null_entity = null_entities[i];
			if(next_entities.find(null_entity) != next_entities.end()){
				next_entities.erase(null_entity);
			}
		}
	}

}
