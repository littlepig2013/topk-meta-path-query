#!/bin/bash
dataset="ACM"
label_file=$dataset"_labels.txt"

safe_rm(){
        if [ -f "$1" ]; then
                rm "$1"
        fi
}

sample_pairs_add(){
        for i in `seq 1 $2`
        do
                awk -F'\t' '{if($2 == '$1') print $1}' "$label_file" | sort -R | head -2 > pos_pair
                paste -s -d'\t' pos_pair >> "$pos_file"
                head -1 pos_pair > curr_src
                awk -F'\t' '{if($2 != '$1') print $1}' "$label_file" | sort -R | head -$m > neg_pair
                paste -s -d',' neg_pair > tmp_neg_pair
                paste -d'\t' curr_src tmp_neg_pair >> "$neg_file"

                safe_rm pos_pair
                safe_rm neg_pair
		safe_rm tmp_neg_pair
                safe_rm curr_src
        done
}


pos_file=$dataset"_pos_pairs.txt"
neg_file=$dataset"_neg_pairs.txt"
m=1
M=100

#area_list=`awk -F'\t' '{print $2}' $label_file | sort | uniq -c | awk '{if($1 > 1) print $2}' `


awk -F'\t' '{print $2}' "$label_file" > tmp_labels.txt


LEN=`awk -F'\t' '{print $2}' "$label_file" | awk 'END{print NR}' `
echo $LEN
#for area in $area_list
#do
        #awk -F'\t' '{if($2 == '$area') print $1}' "$label_file" > "tmp_area_$area.txt"
#done

# generate positive and negative pairs
safe_rm $pos_file
safe_rm $neg_file

#for area in $area_list
#do
#	sample_pairs_add $area $M
#done

for i in `seq 1 $M`
do
	R=$(($(($RANDOM%$LEN))+1))
	echo $R
	area=`head -$R tmp_labels.txt | tail -1`
	sample_pairs_add $area 1
done

for area in $area_list
do
	safe_rm "tmp_area_$area.txt"
done

safe_rm tmp_labels.txt
