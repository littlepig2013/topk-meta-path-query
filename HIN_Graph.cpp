//
//  HIN_Graph.cpp
//
//  Created by 黄智鹏 on 15/10/7.
//  Copyright (c) 2015年 黄智鹏. All rights reserved.
//

#include "HIN_Graph.h"
#include <algorithm>
#include <fstream>

using namespace std;

HIN_Edge::HIN_Edge(int s, int d, int e):src_(s), dst_(d), edge_type_(e)
{
    
}
HIN_Edge::HIN_Edge(){}

bool HIN_Edge::operator==(const HIN_Edge &e)
{
    if(this->dst_ != e.dst_)
        return false;
    if(this->src_ != e.src_)
        return false;
    if(this->edge_type_ != e.edge_type_)
        return false;
    return true;
}

void HIN_Graph::initialDBLPGraph()
{
    /*
    node_types_.push_back("author");
    node_types_.push_back("article");
    node_types_.push_back("venue");
    */

    node_types_[0] = "author";
    node_types_[1] = "article";
    node_types_[2] = "venue";

    for(int i = 0; i < node_types_.size(); ++i){
        node_types_id_[node_types_[i]] = i;
    }
    
    edge_types_.push_back("write");
    edge_types_.push_back("cite");
    edge_types_.push_back("publish");
    
    for(int i = 0; i < edge_types_.size(); ++i){
        edge_types_id_[edge_types_[i]] = i;
    }
}



// -----------------------------------------------------
//
//             YAGO PART
//
// ------------------------------------------------------

void HIN_Graph::buildYAGOGraph(map<int, string> node_name, map<int, vector<Edge> > adj, map<int, string> node_type_name, map<int, int> node_type_num, map<int, vector<int> > node_id_to_type, map<int, string> edge_name)
{
/*
 
    
 */   map<int, string>::iterator iter;
    node_types_ = node_type_name;
    for(iter = node_type_name.begin(); iter != node_type_name.end(); ++iter){
        node_types_id_[iter->second] = iter->first;
        //node_types_.push_back(iter->second);
    }
    edge_types_.push_back("NONERELATIONFORYAGO");
    for(iter = edge_name.begin(); iter != edge_name.end(); ++iter){
        edge_types_id_[iter->second] = iter->first;
        edge_types_.push_back(iter->second);
    }
    
    for(map<int, string>::iterator i = node_name.begin(); i != node_name.end(); ++i){
        int TheOldID = i->first;
        string key = i->second;
        HIN_Node tempNode;
        tempNode.id_ = TheOldID;
        tempNode.title_ = TheOldID;
        tempNode.key_ = key;
        
        nodes_[TheOldID] = tempNode;
        key2id_[key] = TheOldID;
    }
    
    for(map<int, vector<int> >::iterator iter2 = node_id_to_type.begin(); iter2 != node_id_to_type.end(); ++iter2){
        
        int TheOldID = iter2->first;
	vector<int> curr_types_id_ = iter2->second;

        for(int i = 0; i < curr_types_id_.size(); ++i){
            nodes_[TheOldID].types_id_.push_back(curr_types_id_[i]);
            //cout << node_id_to_type[TheOldID][i] << endl;
        }
    }
    
    map<int, vector< Edge> >::iterator iterator;
    for(iterator = adj.begin(); iterator != adj.end(); ++iterator){
        int src = iterator->first;
        for(int j = 0; j < iterator->second.size(); ++j){
            int dst = iterator->second[j].dst_;
            int oldType = iterator->second[j].type_;
            if(oldType < 0){
                /*
                int type = -oldType;
                HIN_Edge tempEdge(dst, src, type);
                tempEdge.pro_ = iterator->second[j].rpro_;
                tempEdge.rpro_ = iterator->second[j].pro_;
                if(find(edges_src_[tempEdge.src_].begin(), edges_src_[tempEdge.src_].end(), tempEdge) == edges_src_[tempEdge.src_].end()){
                    edges_src_[tempEdge.src_].push_back(tempEdge);
                }
                if(find(edges_dst_[tempEdge.dst_].begin(), edges_dst_[tempEdge.dst_].end(), tempEdge) == edges_dst_[tempEdge.dst_].end()){
                    edges_dst_[tempEdge.dst_].push_back(tempEdge);
                }
                 */
                
            }else{
                int type = oldType;
                HIN_Edge tempEdge(src, dst, type);
                tempEdge.pro_ = iterator->second[j].pro_;
                tempEdge.rpro_ = iterator->second[j].rpro_;
               
                edges_src_[tempEdge.src_].push_back(tempEdge);
                edges_dst_[tempEdge.dst_].push_back(tempEdge);
            }
            
        }
    }
    for(int i = 0; i < node_types_.size(); ++i){
        idOftheSameType_[i] = (vector<int>());
    }
    for(int i = 0; i < nodes_.size(); ++i){
        for(vector<int>::iterator j = nodes_[i].types_id_.begin(); j != nodes_[i].types_id_.end(); ++j){
            int type = *j;
            idOftheSameType_[type].push_back(i);
        }
    }
}

void HIN_Graph::buildType2ID(string path)
{
    vector< set<int> > idx;
    map<int, HIN_Node>::iterator iter;
    for(iter = nodes_.begin(); iter != nodes_.end(); ++iter){
        int id = iter->first;
        for(int i = 0; i < iter->second.types_id_.size(); ++i){
            int type = iter->second.types_id_[i];
            while(idx.size() < type + 1)
                idx.push_back(set<int>());
            idx[type].insert(id);
        }
    }
    for(int i = 0; i < idx.size(); ++i){
        if(idx[i].size() == 0)
            continue;
        char outpath[200];
        sprintf(outpath, "%s%d.txt", path.c_str(), i);
        ofstream output(outpath, ios::out);
        for(set<int>::iterator iter = idx[i].begin(); iter != idx[i].end(); ++iter){
            output << *iter << endl;
        }
        output.close();
    }
}

void HIN_Graph::removeLink(string path)
{
    ifstream in(path, ios::in);
    int src, dst, type;
    
    while(in >> src >> dst >> type){
        for(int i = 0; i < edges_src_[src].size(); ++i){
            if(edges_src_[src][i].dst_ == dst){
                edges_src_[src].erase(edges_src_[src].begin() + i);
                break;
            }
        }
        for(int i = 0; i < edges_dst_[dst].size(); ++i){
            if(edges_dst_[dst][i].src_ == src){
                edges_dst_[dst].erase(edges_dst_[dst].begin() + i);
                break;
            }
        }
    }
    
    in.close();
}

void HIN_Graph::buildYAGOGraphbyDefault()
{
    map<int, vector<Edge> > adj;
    map<int,string> node_name;
    map<int,string> node_type_name;
    map<int,int> node_type_num;
    map<int,vector<int> > node_id_to_type;
    map<int,string> edge_name;
    
    YagoReader::readADJ("/Users/huangzhipeng/research/Yago/yagoadj.txt", adj);
    YagoReader::readNodeName("/Users/huangzhipeng/XcodeWorkPlace/Join/Data/yagoTaxID.txt",node_name,node_type_name);
    YagoReader::readNodeTypeNum("/Users/huangzhipeng/XcodeWorkPlace/Join/Data/yagoTypeNum.txt", node_type_num);
    YagoReader::readNodeIdToType("/Users/huangzhipeng/XcodeWorkPlace/Join/Data/totalType.txt", node_id_to_type);
    YagoReader::readEdgeName("/Users/huangzhipeng/XcodeWorkPlace/Join/Data/yagoType.txt",edge_name);
    
    buildYAGOGraph(node_name, adj, node_type_name, node_type_num, node_id_to_type, edge_name);
}
