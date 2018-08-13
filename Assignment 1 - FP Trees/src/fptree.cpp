#include "fpTree.h"

#include <algorithm>
#include <cmath>

using namespace std;

/*
    FP Node
*/
fpNode::fpNode() : parent(NULL), next(NULL) {};
fpNode::fpNode(int i, int c) : item(i), count(c), parent(NULL), next(NULL) {};
fpNode::fpNode(int i, fpNode* p) : item(i), parent(p), next(NULL) {};
fpNode::fpNode(int i, int c, fpNode* p) : item(i), count(c), parent(p), next(NULL) {};

vector<int> fpNode::getTransaction() {
    
    vector<int> transaction;
    fpNode* curr = this;   
    while (curr->parent->parent != NULL) {
        curr = curr->parent;
        transaction.push_back(curr->item);
        // cout << curr->item << endl;
    }
    reverse(transaction.begin(), transaction.end());
    // cout << "5" << endl;
    return transaction;
}

/*
    FP Tree
*/
fpTree::fpTree(string &inFileName) {
    inFile = inFileName;
}

fpTree::fpTree(int rawSupportThreshold) {
    rawSuppThold = rawSupportThreshold;
}

void fpTree::firstPass(double suppThold) {
    numTransactions = 0;

    // first pass over db - get frequencies of items
    ifstream inputStream(inFile);
    if (inputStream.is_open()) {
        string line;
        while (getline(inputStream, line)) {
            vector<int> transaction = parseLineVec(line);
            for (auto it = transaction.begin(); it != transaction.end(); it++) {
                priorityMap[*it] += 1;
            }
            numTransactions += 1;
        }
        inputStream.close();
    }

    // calculate frequent items and keep in priority map
    for (auto it = priorityMap.begin(); it != priorityMap.end();) {
        if ((it->second * 100.0) / numTransactions < suppThold) {
            it = priorityMap.erase(it);
        }
        else {
            it++;
        }
    }

    // keep the raw support threshold - used in later passes
    rawSuppThold = ceil((suppThold / 100.0) * numTransactions);
}

struct fpTree::sortByFrequency {
    fpTree* parent;    // TODO - look into this
    sortByFrequency(fpTree* p) : parent(p) {};
    bool operator()(int const &a, int const &b) const {
        int p_a = -1, p_b = -1;
        // On checking priorityMap[a] vs [b], if either does not exist in it,
        // new key added and value set to zero - thus this comparision
        if (parent->priorityMap.find(a) != parent->priorityMap.end()) {
            p_a = parent->priorityMap[a];
        }
        if (parent->priorityMap.find(b) != parent->priorityMap.end()) {
            p_b = parent->priorityMap[b];
        }
        return p_a > p_b; 
    }
};

void fpTree::initialiseTree() {
    root = new fpNode;
    // for (auto it = priorityMap.begin(); it != priorityMap.end(); it++) {
    //     headPointers[it->first] = new fpNode();
    //     currPointers[it->first] = headPointers[it->first];
    // }
}

void fpTree::addTransaction(vector<int> transaction, int count, bool priorityCheck) {

    fpNode* par = root;
    // build tree per transaction
    for (int item : transaction) {
        // if priorityCheck is false, add all items
        // else check if item is frequent
        if (!priorityCheck || (priorityMap.find(item) != priorityMap.end())) {
            fpNode* curr;
            auto it = par->children.find(item);
            // cout << "Here" << endl;
            // for (auto it1 = par->children.begin(); it1 != par->children.end(); it1++) {
            //     cout << it1->first << " ";
            // }
            // cout << endl;
            if (it == par->children.end()) {
                // cout << "Hello" << endl;
                // new prefix - new node
                curr = new fpNode(item, count, par);
                par->children[item] = curr;
                auto it1 = headPointers.find(item);
                if (it1 == headPointers.end()) {
                    headPointers[item] = new fpNode(item,count);
                    currPointers[item] = headPointers[item];
                }
                else {
                    headPointers[item]->count += count;
                }
                // update current pointers
                currPointers[item]->next = curr;
                currPointers[item] = curr;
            } 
            else {
                // cout << "Hi" << endl;
                // prefix already in the tree
                curr = it->second;
                curr->count += count;
                headPointers[item]->count += count;
            }
            par = curr;
        }
        else {
            // cout << "There" << endl;
            // all infrequent items - ignore afterwards
            break;
        }
    }
}

void fpTree::buildFPTree() {

    // initialise root, head and current pointers for each frequent item
    initialiseTree();

    ifstream inputStream(inFile);
    if (inputStream.is_open()) {

        string line;
        while (getline(inputStream, line)) {
            vector<int> transaction = parseLineVec(line);
            // sort according to frequency
            sort(transaction.begin(), transaction.end(), sortByFrequency(this));

            addTransaction(transaction, 1, true);
            
        }
        inputStream.close();
    }
}

bool fpTree::singlePrefixPath() {
    
    fpNode* curr = root;
    while (!curr->children.empty()) {
        if (curr->children.size() > 1)
            return false;
        curr = curr->children.begin()->second;
    }
    return true;
}

vector<item_set> getPowerSet(item_set transaction) {
    
    vector<item_set> powerset;    
    if (transaction.size() == 1) {
        powerset.push_back(transaction);
    }
    if (transaction.size() > 1) {
        int item  = transaction[0];
        item_set item1;
        item1.push_back(item);
        powerset.push_back(item1);
        item_set sub(transaction.begin()+1,transaction.end());
        vector<item_set> powerset1 = getPowerSet(sub);
        for (auto itemset : powerset1) {
            powerset.push_back(itemset);
            itemset.push_back(item);
            sort(itemset.begin(),itemset.end());
            powerset.push_back(itemset);
        }        
    }    
    return powerset; 
}

void printTree (fpNode* node) {
    cout << node->item << " -> " << node->count << endl;
    for (auto it = node->children.begin(); it != node->children.end(); it++){
        cout << node->item << " : ";
        printTree(it->second);
    }
}

void fpTree::printHeadPointers() {
    cout << "headPointers : " << endl;
    for (auto it = headPointers.begin(); it != headPointers.end(); it++){
        cout << it->first << " : " << it->second->count << endl;
    }
    cout << "Done" << endl;
}

void fpTree::fpGrowth() {

    if (root->children.empty()) {
        return;
    }
    if (singlePrefixPath()) {
        // Make all possible subsets and return them, since the infrequent items were pruned while construction of fpTree
        item_set transaction;
        // Root has a child, children not empty
        fpNode* curr = root;
        while (!curr->children.empty()) {
            curr = curr->children.begin()->second;
            if (curr->count < rawSuppThold)
                break;
            transaction.push_back(curr->item);
        }
        freqItemsets = getPowerSet(transaction);
    }
    else {
        // Multi Prefix Path, traverse over the pointer table and go on one by one
        for (auto it = headPointers.begin(); it != headPointers.end(); it++) {
            int item = it->first;
            fpNode* node = it->second;
            if (node->count < rawSuppThold) {
                continue;
            }
            freqItemsets.push_back(item_set{item});
            fpTree* subTree = new fpTree(rawSuppThold);
            subTree->root = new fpNode;
            while (node->next != NULL) {
                node = node->next;
                int count = node->count;
                // Get all the items from root to this item (excluding this item)
                vector<int> transaction = node->getTransaction();
                subTree->addTransaction(transaction, node->count, false);
            }
            // printTree(subTree->root);
            // subTree->printHeadPointers();
            subTree->fpGrowth();
            vector<item_set> freqItemsetsSubtree = subTree->getFrequentItemsets();
            for (auto& itemset : freqItemsetsSubtree) {
                itemset.push_back(item);
                sort(itemset.begin(), itemset.end());
            }
            freqItemsets.insert(freqItemsets.end(), freqItemsetsSubtree.begin(), freqItemsetsSubtree.end());
        }
    }
} 

vector<item_set> fpTree::getFrequentItemsets() {

    return freqItemsets;
}


vector<item_set> fpTree::getFrequentItemsets(double suppThold) {

    // run the first pass - initialise priority map
    firstPass(suppThold);

    // build the FP Tree
    buildFPTree();

    // for (auto it = headPointers.begin(); it != headPointers.end(); it++) {
    //     cout << it->first << " : " << it->second->count << endl;
    // }

    // cout << rawSuppThold << endl;

    // printTree(root);

    // run the fp tree growth
    fpGrowth();

    sort(freqItemsets.begin(), freqItemsets.end());    
    return freqItemsets;
}
