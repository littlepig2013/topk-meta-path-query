from itertools import combinations
import random

SAMPLE_SIZE = 50
def find_lcsubstr(s1, s2):
	s1 = s1.upper()
	s2 = s2.upper()
	m=[[0 for i in range(len(s2)+1)]  for j in range(len(s1)+1)]
	max_matched_length = 0
	p = 0
	for i in range(len(s1)):
		for j in range(len(s2)):
			if s1[i] == s2[j]:
				m[i+1][j+1]=m[i][j]+1
				if m[i+1][j+1] > max_matched_length:
					max_matched_length = m[i+1][j+1]
					p=i+1
	return s1[p-max_matched_length:p], max_matched_length
	

def isIdentical(lcsubstr):
	if len(lcsubstr) <= 4:
		return False
	if '_OF_' in lcsubstr and len(lcsubstr) <= 8:
		return False
	if '<WIKICATEGORY' in lcsubstr and len(lcsubstr) <= 14:
		return False
	if 'CONSORT_' in lcsubstr and len(lcsubstr) <= 9:
		return False
	if 'EMPRESS_' in lcsubstr and len(lcsubstr) <= 9:
		return False
	if 'QUEEN_' in lcsubstr and len(lcsubstr) <= 7:
		return False
	if 'PRINCESS_' in lcsubstr and len(lcsubstr) <= 10:
		return False
	return True


'''
obj2Types=dict()
obj2Types_fin = open('totalType.txt','r')
for line in obj2Types_fin:
	tmp = line.strip().split(' ')
	obj=tmp[0]
	obj2Types[obj] = set()
	for type_str in tmp[1:]:
		obj2Types[obj].add(type_str)	
obj2Types_fin.close()
'''
married_couples=dict()
adj_fin = open('oldyagoadj.txt','r')
all_persons=set()
adj_data = adj_fin.readlines()
for line in adj_data:
	tmp = line.strip().split('\t')
	if tmp[2] == '8':
		all_persons.add(tmp[0])
for line in adj_data:
	tmp = line.strip().split('\t')
	if tmp[2] == '26' or tmp[2] == '-26':
		person1 = tmp[0]
		person2 = tmp[1]
		
		if person1 not in all_persons:
			continue
		if person2 not in all_persons:
			continue
		
		if tmp[0] not in married_couples:
			married_couples[tmp[0]] = set()
		married_couples[tmp[0]].add(tmp[1])
		
		if tmp[1] not in married_couples:
			married_couples[tmp[1]] = set()
		married_couples[tmp[1]].add(tmp[0])
		
adj_fin.close()

id2Name_fin = open('yagoTaxID.txt','r')
id2Name=dict()
for line in id2Name_fin:
	tmp = line.strip().split(' ')
	if '<wordnet_' in tmp[0] or '<wikicategory_' in tmp[0]:
		continue
	id2Name[tmp[1]]=tmp[0]
id2Name_fin.close()

test_fout = open('married_persons.txt','w')
pos_pairs = []
other_pairs = []
neg_pairs = []
all_candidate_persons=set()
for person1 in married_couples:
	'''
	if person1 not in all_persons:
		continue
	tmp_persons = married_couples[person1].intersection(all_persons)
	all_persons = all_persons.union(tmp_persons)
	'''
	tmp_persons = married_couples[person1]
	persons = list(tmp_persons)
	if len(persons) == 1:
		continue
	test_fout.write(person1+"\t"+",".join(persons) +"\n")
	person_with_shortest_name = ""
	min_name_length = float('inf')
	for person in persons:
		name = id2Name[person]
		if len(name) < min_name_length:
			min_name_length = len(name)
			person_with_shortest_name = name
	comb = combinations(persons, 2)
	person_with_shortest_name = person_with_shortest_name.upper()[1:-1]
	for i in comb:
		name1 = id2Name[i[0]]
		name2 = id2Name[i[1]]
		lcsubstr, length = find_lcsubstr(name1, name2)
		if person_with_shortest_name in lcsubstr:
			pos_pairs.append((i[0], i[1], person1))
		elif not isIdentical(lcsubstr):
			neg_pairs.append((i[0], i[1], person1))
		else:
			other_pairs.append((i[0], i[1], person1))
test_fout.close()



neg_pairs_fout = open('neg_pairs.txt','w')
neg_samples = random.sample(neg_pairs, SAMPLE_SIZE)
for pair in neg_samples:
	neg_pairs_fout.write(pair[0]+'\t'+pair[1]+'\n')
neg_pairs_fout.close()

pos_pairs_fout = open('pos_pairs.txt','w')
pos_samples = pos_pairs
if len(pos_pairs) > SAMPLE_SIZE:
	pos_samples = random.sample(pos_pairs, SAMPLE_SIZE) 
for pair in pos_samples:
	#pos_pairs_fout.write(pair[0]+'\t'+pair[1]+"\t"+id2Name[pair[0]]+"\t"+id2Name[pair[1]] +'\n')
	pos_pairs_fout.write(pair[0]+'\t'+pair[1]+'\n')
pos_pairs_fout.close()


other_pairs_fout = open('candidate_pos_pairs.txt','w')
for pair in other_pairs:
	lcsubstr, length = find_lcsubstr(id2Name[pair[0]], id2Name[pair[1]])
	other_pairs_fout.write(pair[0]+'\t'+id2Name[pair[0]]+"\t"+pair[1]+"\t"+id2Name[pair[1]]+"\t" + str(length) + "\t" + lcsubstr + "\n")
other_pairs_fout.close()

def checkExists(id1, id2, pairs):
	for pair in pairs:
		if id1 == pair[0] and id2 == pair[2]:
			return True
		'''
		if id1 == pair[2] and id2 == pair[0]:
			return True
		'''
		if id1 == pair[1] and id2 == pair[2]:
			return True
		'''
		if id1 == pair[2] and id2 == pair[1]:
			return True
		'''
	return False

adj_fout = open('yagoadj.txt','w')
for line in adj_data:
	tmp = line.strip().split('\t')
	if tmp[2] == '26':	
		if checkExists(tmp[0], tmp[1], pos_samples):
			continue
		if checkExists(tmp[0], tmp[1], neg_samples):
			continue
	adj_fout.write(tmp[0]+"\t"+tmp[1]+"\t"+tmp[2]+"\n")
adj_fout.close()	
