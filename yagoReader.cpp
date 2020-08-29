//
//  yagoReader.cpp
//  GetPathInstance
//
//  Created by 黄智鹏 on 15/9/29.
//  Copyright (c) 2015年 黄智鹏. All rights reserved.
//

#include "yagoReader.h"
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

#include <sstream>

vector<string> YagoReader::split(string s, char c)
{
    vector<string> rtn;
    int start = 0;
    int now = 0;
    while(true){
        now = s.find(c, start);
        if(now == -1){
            rtn.push_back(s.substr(start, s.size()));
            break;
        }else{
            rtn.push_back(s.substr(start, now));
            start = now + 1;
        }
    }
    
    return rtn;
}


bool YagoReader::readADJ(string path, map<int, vector<Edge> > & adja)
{
    ifstream adj_in(path.c_str(), ios::in);
    string sline;
    int src = 0, dst, type;
    
    while(adj_in >> src >> dst >> type)
    {
        Edge tempedge(dst, type, 0.0, 0.0);
        if(adja.find(src) == adja.end()){
            vector<Edge> t;
            //t.clear();
            //adj[src] = t;
            adja.insert(make_pair(src, t));
        }
        
        adja[src].push_back(tempedge);
    }
    adj_in.close();
    return true;
}

bool YagoReader::readMetaPath(string path, vector<vector<int> > &linkType, vector< vector<int> > & nodeType, vector<double>& weight)
{

    ifstream meta_in(path.c_str(), ios::in);
    string line;
    while(getline(meta_in, line)){
        stringstream in(line);
        vector<int> tempN, tempL;
        int n_id, t_id;
        double w;
        in >> w >> n_id;
        weight.push_back(w);
        tempN.push_back(n_id);
        while(in >> t_id >> n_id){
            tempN.push_back(n_id);
            tempL.push_back(t_id);
        }
        linkType.push_back(tempL);
        nodeType.push_back(tempN);
    }
    meta_in.close();
    return true;
}

bool YagoReader::readTuple(string path, vector<pair<int, int> > &input_pairs)
{
    ifstream input_in(path.c_str(), ios::in);
    char line[MAX];
    while(true){
        input_in.getline(line, MAX);
        if(strlen(line) == 0){
            break;
        }
        vector<string> tempStrings = YagoReader::split(line, ' ');
        input_pairs.push_back(make_pair(atoi(tempStrings[0].c_str()), atoi(tempStrings[1].c_str())));
        //cout << atoi(tempStrings[0].c_str()) << '\t' <<  atoi(tempStrings[1].c_str()) << endl;
        line[0] = '\0';
    }
    input_in.close();

    return true;
}

bool YagoReader::readNodeName(string path, map<int, string> &node_name,map<int,string> &node_type_name){
    ifstream input_in(path.c_str(), ios::in);
    string name;
    int id;
    while (input_in >> name >> id)
    {
        if (name.find("<wordnet_")==string::npos && name.find("<wikicategory_")==string::npos){
            map<int,string>::iterator iter;
            iter=node_name.find(id);
            if (iter==node_name.end()){
                node_name[id]=name;
            }
        }
        else {           
            map<int,string>::iterator iter;
            iter=node_type_name.find(id);
            if (iter==node_type_name.end()){
                node_type_name[id]=name;
            }
        }
    }
    input_in.close();
    return true;
    
}

bool YagoReader::readNodeTypeNum(string path, map<int,int> &node_type_num){
    ifstream input_in(path.c_str(), ios::in);
    int id,num;
    while (input_in >> id >> num){
        node_type_num[id]=num;
    }
    input_in.close();
    return true;
}

bool YagoReader::readNodeIdToType(string path, map<int,vector<int> > &node_id_to_type){
    ifstream input_in(path.c_str(), ios::in);
    
    string myline;
    int id, baseid;
    while (getline(input_in, myline))
    {
        istringstream iss(myline);
        iss >> baseid;
        while (iss >> id)
        {
            node_id_to_type[baseid].push_back(id);
        }
    }
    input_in.close();
    return true;
}

bool YagoReader::readEdgeName(string path,map<int,string> &edge_name){
    ifstream input_in(path.c_str(), ios::in);
    string name;
    int id;
    while (input_in >> name >> id){
        edge_name[id]=name;
    }
    input_in.close();
    return true;
}
