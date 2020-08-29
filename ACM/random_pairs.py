
import random
ratio = 0.3
fin = open("ACM_labels.txt","r")
data = fin.readlines()

labeled_authors = dict()
for line in data:
	tmp = line.strip().split("\t")
	authorID = int(tmp[0])
	label = int(tmp[1])
	if label not in labeled_authors:
		labeled_authors[label] = set()
	labeled_authors[label].add(authorID)
fin.close()

fout_test = open("test_entities.txt","w")
fout_train_x = open("train_entities.txt","w")
author2label = dict()
for label, authors in labeled_authors.items():
	authors = list(authors)
	num = len(authors)
	random.shuffle(authors)
	num_test_authors = int(round(num*ratio))
	for author in authors[:num_test_authors]:
		fout_test.write(str(author) + "\t" + str(label) + "\n")
	train_authors = authors[num_test_authors:]
	for author in train_authors:
		fout_train_x.write(str(author) + "\t" + str(label) +"\n")
	for author in train_authors:
		author2label[author] = label
	labeled_authors[label] = set(train_authors)
fout_test.close()
fout_train_x.close()
print(len(author2label))
num_samples = 10
fout_train = open("train_pairs.txt","w")
for author, label in author2label.items():
	authors_with_same_label = labeled_authors[label]
	if author in authors_with_same_label:
		authors_with_same_label.remove(author)
	if num_samples > len(authors_with_same_label):
		samples = list(authors_with_same_label)
	else:
		samples = random.sample(list(authors_with_same_label), num_samples)
	if len(samples) == 0:
		continue
	fout_train.write(str(author) + "\t" + str(samples[0]))
	for i in range(len(samples)-1):
		fout_train.write(","+str(samples[i+1]))
	fout_train.write("\n")
	authors_with_same_label.add(author)
		
fout_train.close()

