# Top-k Meta Path Query

## Brief Introduction

This is the project for top k meta path query. There are 4 executable files in the Makefile:

* topKQuery Usage: (fast means bi-directional searching algorithm) In this repository, ACM and DBLP dataset are already there. To run top-k meta path query on Yago dataset, you have to go to YAGO directory and download Yago.zip (sh download_Yago.sh) and extract these files into YAGO directory. 
    1. ./topKQuery --default dataset entityId1 entityId2 k
    2. ./topKQuery --advance dataset entityId1 entityId2 k output-type Importance-Type length-penalty (beta)
    3. ./topKQuery --fast_advance dataset entityId1 entityId2 k output-type Importance-Type length-penalty (beta)
    4. ./topKQuery --refine dataset entityId1 entityId2 k score-function
    5. ./topKQuery --train dataset


    --advance/fast_advance mode:
         output-type:
                 1 -> typing ranking details in std::cout
                 2 -> saving to a file
                 3 -> both 1 and 2
         TF-IDF-type:
                 MNIS -> ours
                 SLV1 -> Strength & Length based Version 1
                 SLV2 -> Strength & Length based Version 2
                 SMP -> Shortest Meta Path
         length-penalty(l is the meta-path's length):
                 1 -> beta^l (beta < 1)
                 2 -> 1/factorial(l)
                 3 -> 1/l
                 4 -> 1/e^l

    --default mode:
	 fast mode
         output-type -> 1
         TF-IDF-type -> MNIS
         length-penalty -> 1
         beta -> 0.2

    --refine mode:
         refine k meta-paths from previous generated meta-paths (you can specify the output type as 2 when you first-time generate meta paths and they will be stored under topKResult/ directory. Refine mode load meta paths from that directory)
         default meta-paths file name: dataset_entityId1_entityId2.txt
         score-function: 1 -> BPCRW
         output-type -> 1

    --train mode:
         get statistics of different node types and edge types(including the number of each node type, each edge type and the weight of each edge type. Normally you don't need to run this because all meta info have been pre-populated under cache/ folder.


* ./topKQueryTest Usage: This program check the accuracy according to the given positive pairs and negative pairs. They are pre-generated in the corresponding folder. If you want to re-generate the positive pairs and negative pairs using the scripts in the dataset folder. (DBLP_sample.sh for DBLP and pos_neg_pairs_sample.sh for ACM)
    1. ./topKQueryTest --classifier dataset (positive pairs file name) (negative pairs file name) k TF-IDF-type length-penalty (beta)
    2. ./topKQueryTest --refine_classifier dataset (positive pairs file name) (negative pairs file name) k score-function
    3. ./topKQueryTest --metapath_classifier dataset (positive pairs file name) (negative pairs file name) (meta paths file name)

    --refine_classifier mode:
        refine k meta-paths from shortest k' meta-paths (default k' is 15)
        score-function: 1 -> PCRW, 2 -> BPCRW ( this mode could take a very long time, as pointed out in our top-k paper )

* ./genTopKMPs Usage: run this to generate top-k meta paths and store them in "Classifier/topKMPs/" for training pairs if you generate a new training set. These top-k results will be used later in MNIS-based-metapath2vec

    ./genTopKMPs [dataset] [num_threads]
    
* ./genRandomWalks Usage: run this to generate random walks based the specified #walks and walk length. If fix_mp_switcher is set to 1, it will load [dataset]/fix_mps.txt and use these fixed meta paths to generate random walks. (The name format of the generated file is "FIX_MP_[dataset]-(edge_type)-..-(edge_type).txt".) Otherwise, it perform MNIS-based random walks and will use the top-k results from "genTopKMPs". Results are stored in "Classifier/RandomWalks/". The pre-generated random walks are compressed in RandomWalks.zip and you can uncompress it can directly use it in the metapath2vec embedding.

    ./genRandomWalk [dataset] [num_walks] [walk_length] [fix_mp_switcher]

The data structure ( yagoReader.cpp, yagoReader.h, HIN_Graph.h and HIN_Graph.cpp ) to store HIN in this project follows [Meta Structure][1]. 

## Experiments

* To run the first label-basd connectivity experiment in our paper, refer to topKQueryTest program. The pre-generated results used in our paper are stored in exp/ folder and the case studey is put in Bound-Exp/ folder. 

* To run the embedding-based experiment based pre-generated embedding results, go to "Classifier" folder and run 
    <pre><code>
    python clf-evaluation.py [dataset] [embedding-method] [clf-method] 
    </code></pre>

    Be sure that [dataset]-[embedding-method].txt exists in Embedding folder when you run this. Examples:
    <pre><code>
    python clf-evaluation.py DBLP MNIS KNN
    python clf-evaluation.py ACM FIX-PVP GaussianNB
    </code></pre>
   
    Here we employ [scikit-learn][3] to implement these classification methods and thus the corresponding package should be installed (python >=3.6). Embedding results are pre-generated using [metapath2vec][4] where the codes come from [here][5]. We use parameters as follows whenever we run the embedding program <code>-pp 1 -size 128 -window 7 -negative 5</code>.

    Since embedding results are already generated, you can directly go into Classifier directory and run clf-evaluation.py. If you want to start the metapath2vec-based classification experiment from scratch (which may require much longer time and work), you can do as follows:

    1. Go to the corresponding data folder and run "python random_pairs.py". You can also set the split ratio in random_pairs.py
    2. Go back to the previous directoray and run "make && genTopKMPs [dataset] [#threads]". This phase would cost much time and you can reduce it by employing more threads. To skip this step, you can download topKMPs.zip (sh download_topKMPs.sh) and uncompress it into topKMPs/ foloder.
    3. Run "./genTopRandomWalk [dataset] [num_walks] [walk_length] [fix_mp_switcher]". If you want to skip this step, you can download the RandomWalks.zip (sh download_RWs.sh) and uncompress it into "Classifier/RandomWalks/". If you want to generate random walks by yourself, please note that currently there are memory leakage bugs in our program. The whole generation process possibly takes up 128GB or even more for ACM dataset during the random walk generation. If you want to avoid it, you can download the zip files into "Classifier/one_hop_neighbors/" and unzip them as "[dataset].txt". Download scipts are put under "Classifier/one_hop_neighbors/". 
    4. Download the [metapath2vec code][5] and run it with parameters you want. Make sure you specify the output path is under "Classifier/Embedding/" folder and the input path is under "Classifier/RandomWalks" folder.
    5. Finally you can run clf-evaluation.py to examine the classification performance.

Besides, the DBLP and the YAGO dataset are from [Meta Structure][1] and the ACM dataset is from [HeteSim][2].

## References

* Huang, Z., Zheng, Y., Cheng, R., Sun, Y., Mamoulis, N. and Li, X., 2016, August. Meta structure: Computing relevance in large heterogeneous information networks. In Proceedings of the 22nd ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (pp. 1595-1604).
* Shi, C., Kong, X., Huang, Y., Philip, S.Y. and Wu, B., 2014. Hetesim: A general framework for relevance measure in heterogeneous networks. IEEE Transactions on Knowledge and Data Engineering, 26(10), pp.2479-2492.
* Pedregosa, F., Varoquaux, G., Gramfort, A., Michel, V., Thirion, B., Grisel, O., Blondel, M., Prettenhofer, P., Weiss, R., Dubourg, V. and Vanderplas, J., 2011. Scikit-learn: Machine learning in Python. the Journal of machine Learning research, 12, pp.2825-2830.
* Dong, Y., Chawla, N.V. and Swami, A., 2017, August. metapath2vec: Scalable representation learning for heterogeneous networks. In Proceedings of the 23rd ACM SIGKDD international conference on knowledge discovery and data mining (pp. 135-144).

[1]: https://dl.acm.org/doi/10.1145/2939672.2939815

[2]: https://ieeexplore.ieee.org/document/6702458

[3]: https://scikit-learn.org/

[4]: https://dl.acm.org/doi/10.1145/3097983.3098036

[5]: https://ericdongyx.github.io/metapath2vec/m2v.html
