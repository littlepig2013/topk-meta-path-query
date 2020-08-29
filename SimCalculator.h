#include "HIN_Graph.h"

#define BPCRW_GAMMA 0.5

#ifndef _SIM_CALCULATOR_
#define _SIM_CALCULATOR_

using namespace std;

class SimCalculator
{
private:
	virtual double getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSim_flag, double gamma){};
public:
	bool heteSim_flag;
	double gamma;
	double getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_);

	static void getNextEntities(int eid, int edge_type, set<int> & next_entities, HIN_Graph & hin_graph_);
	static void getNextEntities(set<int> eids, int edge_type, set<int> & next_entities, HIN_Graph & hin_graph_);
	static void getNextWeightedEntities(map<int, double> eids, int edge_type, map<int, double> & next_entities, HIN_Graph & hin_graph_, bool heteSimFlag, bool inheritFlag, double gamma=BPCRW_GAMMA);
	static bool isConnected(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_);
	static bool isConnectedMain(int src, int dst, set<int> src_next_entities, set<int> dst_next_entities, vector<int> meta_path, HIN_Graph & hin_graph_);

};

class DFS_SimCalculator: public SimCalculator
{
	
public:
	double getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSim_flag, double gamma);
};

class BFS_SimCalculator: public SimCalculator
{
	
public:
	double getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSim_flag, double gamma);
};

class Bi_BFS_SimCalculator: public SimCalculator
{
	
public:
	double getMetaPathBasedSim(map<int, double> src_entities, map<int, double> dst_entities, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSimFlag, double gamma);
	double getMetaPathBasedSim(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_, bool heteSim_flag, double gamma);
	static bool isConnected(int src, int dst, vector<int> meta_path, HIN_Graph & hin_graph_);
	static bool isConnected(set<int> src_entities, set<int> dst_entities, vector<int> meta_path, HIN_Graph & hin_graph_);
	
};

#endif
