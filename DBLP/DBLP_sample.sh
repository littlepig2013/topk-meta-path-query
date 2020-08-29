#!/bin/bash

safe_rm(){
        if [ -f "$1" ]; then
                rm "$1"
        fi
}

sample_pairs_add(){
        for i in `seq 1 $2`
        do
                awk -F'\t' '{if($2 == '$1') print $1}' "DBLP_labels.txt" | sort -R | head -2 > pos_pair
		awk -F'\t' '{if($2 == '$1') print $1}' "DBLP_labels.txt" | sort -R | head -1 > neg_pair
		awk -F'\t' '{if($2 != '$1') print $1}' "DBLP_labels.txt" | sort -R | head -1 >> neg_pair
                paste -s -d'\t' pos_pair >> "$pos_file"
                paste -s -d'\t' neg_pair >> "$neg_file"

                safe_rm pos_pair
                safe_rm neg_pair
        done
}


pos_file="DBLP_pos_pairs.txt"
neg_file="DBLP_neg_pairs.txt"
m=1
M=25

area_list=`awk -F'\t' '{print $2}' "DBLP_labels.txt" | sort | uniq`


# generate positive and negative pairs
safe_rm $pos_file
safe_rm $neg_file

for area in $area_list
do
	sample_pairs_add $area $M
done


