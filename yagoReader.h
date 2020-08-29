//
//  yagoReader.h
//  GetPathInstance
//
//  Created by 黄智鹏 on 15/9/29.
//  Copyright (c) 2015年 黄智鹏. All rights reserved.
//

#ifndef __GetPathInstance__yagoReader__
#define __GetPathInstance__yagoReader__

#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>



#define MAX 100000

using namespace std;

class Edge
{
public:
    int dst_;
    int type_;
    double pro_;
    double rpro_;
    Edge(int x, int y, double d1, double d2): dst_(x), type_(y), pro_(d1), rpro_(d2){}
};

class YagoReader
{
public:
    static vector<string> split(string s, char c = ' ');
    
    static bool readADJ(string path, map<int, vector<Edge> > & adj);
    static bool readMetaPath(string path, vector< vector<int> > & linktype, vector< vector<int> > & nodetype, vector<double> & weight_);
    static bool readTuple(string path, vector< pair<int, int> > & input_pairs);
    
    ////////////////////////////////////////////
    static bool readNodeName(string path,map<int,string> &node_name, map<int,string> &node_type_name);
    
    static bool readNodeTypeNum(string path, map<int, int> &node_type_num);
    static bool readNodeIdToType(string path, map<int, vector<int> > &node_id_to_type);
    
    static bool readEdgeName(string path,map<int,string> &edge_name);
    
};

#endif /* defined(__GetPathInstance__yagoReader__) */
