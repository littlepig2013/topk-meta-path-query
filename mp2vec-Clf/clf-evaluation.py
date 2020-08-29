import os, sys
from sklearn.metrics import accuracy_score, balanced_accuracy_score,f1_score, roc_auc_score

import pickle
from sklearn import svm
from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler
from sklearn.naive_bayes import GaussianNB,BernoulliNB
from sklearn.neural_network import MLPClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.neighbors import KNeighborsClassifier
from sklearn.model_selection import train_test_split, cross_val_score

import numpy as np

dataset = sys.argv[1]
method = sys.argv[2]
clf_method = sys.argv[3]

embedding_filename = "Embedding/" + dataset + "-" + method + ".txt"
fin = open(embedding_filename, "r")
meta_line = fin.readline()
tmp = meta_line.strip().split(' ')
N = int(tmp[0])
dim = int(tmp[1])
embeddings = dict()
data_embeddings = fin.readlines()
for line in data_embeddings:
	tmp = line.strip().split(' ')
	if tmp[0].startswith("a"):
		embeddings[int(tmp[0][1:])] = [float(x) for x in tmp[1:]]
fin.close()

X_train = []
X_test = []
Y_train = []
Y_test = []
training_filename = "../" + dataset + "/train_entities.txt"
testing_filename = "../" + dataset + "/test_entities.txt"
train_fin = open(training_filename, "r")
test_fin = open(testing_filename, "r")

train_labels = set()
for line in train_fin:
	tmp = line.strip().split("\t")
	train_labels.add(int(tmp[1]))
train_fin.close()

labels = set()
for line in test_fin:
	tmp = line.strip().split("\t")
	if int(tmp[1]) not in train_labels:
		continue
	labels.add(int(tmp[1]))
test_fin.close()

train_fin = open(training_filename, "r")
test_fin = open(testing_filename, "r")
for line in train_fin:
	tmp = line.strip().split("\t")
	eid = int(tmp[0])
	if int(tmp[1]) not in labels:
		continue
	if eid in embeddings:
		X_train.append(np.array(embeddings[eid]))
	else:
		X_train.append(np.random.random_sample(dim))
	Y_train.append(int(tmp[1]))
train_fin.close()

for line in test_fin:
	tmp = line.strip().split("\t")
	eid = int(tmp[0])
	if int(tmp[1]) not in labels:
		continue
	if eid in embeddings:
		X_test.append(np.array(embeddings[eid]))
	else:
		X_test.append(np.random.random_sample(dim))
	Y_test.append(int(tmp[1]))
test_fin.close()


if clf_method == "SVM":
    clf = svm.SVC(kernel='linear',decision_function_shape='ovo',gamma='auto')
# Evaluation
#scores = cross_val_score(clf, X, Y, cv=5)
#print(scores)
elif clf_method == "LR":
    clf = LogisticRegression(random_state=0).fit(X_train, Y_train)

elif clf_method == "GaussianNB":
    clf = GaussianNB()

elif clf_method == "BernoulliNB":
    clf = BernoulliNB()

elif clf_method == "KNN":
    clf = KNeighborsClassifier(10)

elif clf_method == "Decision-Tree":
    clf = DecisionTreeClassifier(max_depth=5)
elif clf_method == "MLP":
    clf = MLPClassifier(alpha=1, max_iter=1000)

elif clf_method == "Random-Forest":
    clf = RandomForestClassifier(max_depth=5, n_estimators=10)

clf.fit(X_train, Y_train)
Y_predict = clf.predict(X_test)

print("CLF Mean Accuracy:")
print(clf.score(X_test, Y_test))
print("Metric Accuracy: \n" + str(accuracy_score(Y_test, Y_predict)))
print("Metric Balanced Accuracy: \n" + str(balanced_accuracy_score(Y_test, Y_predict)))
print("Metric F1: ")
print("Macro\tMicro\tWeighted")
print(str(round(f1_score(Y_test, Y_predict, average="macro"),4)) + "\t" + str(round(f1_score(Y_test, Y_predict, average="micro"),4)) + "\t" + str(round(f1_score(Y_test,Y_predict, average="weighted"),4)))

mappings = dict()
index = 0
for label in labels:
	if label not in mappings:
		mappings[label] = index
		index += 1
num_labels = len(labels)
Y_predict_tmp = np.zeros((len(Y_predict), num_labels))
Y_test_tmp = np.zeros((len(Y_predict), num_labels))
for i in range(len(Y_predict)):
	Y_predict_tmp[i][mappings[Y_predict[i]]] = 1.0
	Y_test_tmp[i][mappings[Y_test[i]]] = 1.0
	
print("Metric AUC: ")
#print(round(roc_auc_score(Y_test, Y_predict_tmp, multi_class="ovr"),4))
print("ovr-Macro\tovr-Weighted\tovo-Macro\tovo-Weighted\traise-Micro\traise-Macro")
print(str(round(roc_auc_score(Y_test_tmp, Y_predict_tmp, average="macro", multi_class="ovr"), 4)), end="\t\t")
print(str(round(roc_auc_score(Y_test_tmp, Y_predict_tmp, average="weighted", multi_class="ovr"), 4)), end="\t\t")
print(str(round(roc_auc_score(Y_test_tmp, Y_predict_tmp, average="macro", multi_class="ovo"), 4)), end="\t\t")
print(str(round(roc_auc_score(Y_test_tmp, Y_predict_tmp, average="weighted", multi_class="ovo"), 4)), end="\t\t")
print(str(round(roc_auc_score(Y_test_tmp, Y_predict_tmp, average="micro"), 4)), end="\t\t")
print(str(round(roc_auc_score(Y_test_tmp, Y_predict_tmp, average="macro"), 4)), end="\t")
print()
