#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<string>
#include<map>
#include<list>
#include<algorithm>
using namespace std;
/************************************************************************/
/* predefined value                                                                                   */
/************************************************************************/
#define TRANSACTION_NUMBER 9
#define MINMUN_SUPPORT_RATE 0.2
#define NON_EXIST -100
/************************************************************************/
/* inline function                                                                                        */
/************************************************************************/
inline int getThreshold()
{
	return ceil(TRANSACTION_NUMBER*MINMUN_SUPPORT_RATE);
}

/************************************************************************/
/* type definition                                                                                      */
/************************************************************************/
/*FP-Tree节点的数据结构*/
typedef struct FpNode
{
	unsigned int frequence;
	string itemName;
	FpNode * parent;
	vector<FpNode *>children;
	FpNode * nextfriend;
	bool operator < (FpNode const & Node) const
	{
		return frequence < Node.frequence;
	}
}node;
/*Header table中的表项*/
struct headerItem
{
	string item;
	unsigned int frequence;
	list<node *> phead;
	bool operator <(headerItem const & other) const
	{
		return  frequence < other.frequence;
	}
};
/*读入交易记录的数据保存格式*/
typedef struct transaction
{
	string tranId;
	unsigned int ItemNum;
	list<node> Items;
}transaction;
/*最终存储的频繁模式集格式*/
struct frequentMode
{
	string mode;
	int count;
};
/************************************************************************/
/* 读取文件中所有交易记录，存储在vector<transaction>                            */
/************************************************************************/
vector<transaction> readFile(string file)
{
	vector<transaction> transactions;
	ifstream fin(file);
	if (fin.fail())
	{
		cout << "ERROR: CAN NOT OPEN FILE!" << endl;
		exit(-1);
	}
	string rowContent;
	while (getline(fin, rowContent))
	{
		istringstream row(rowContent);
		transaction trans_temp;
		row >> trans_temp.tranId;
		row >> trans_temp.ItemNum;
		for (auto i = 0; i < trans_temp.ItemNum; i++)
		{
			node tempNode;
			tempNode.children.clear();
			tempNode.parent = NULL;
			tempNode.nextfriend = NULL;
			row >> tempNode.itemName;
			tempNode.frequence = -1;
			trans_temp.Items.push_back(tempNode);
		}
		transactions.push_back(trans_temp);
	}
	return transactions;
}
/************************************************************************/
/* 对节点排序的比较函数                                                                     */
/************************************************************************/
bool greaterThan(const node &a, const node &b)
{
	return (a.frequence > b.frequence);
}
/************************************************************************/
/* 遍历vector<transaction>,统计每个item出现次数                                    */
/************************************************************************/
map<string, int> getFrequentItem(vector<transaction>const & transactions)
{
	map<string, int > result;
	for (auto p = transactions.cbegin(); p != transactions.cend(); p++)
	{
		for (auto q = p->Items.cbegin(); q != p->Items.cend(); q++)
		{
			result[q->itemName]++;
		}
	}
	return result;
}
/************************************************************************/
/* 去除map<string,int> 1-频繁项中不频繁项                                                 */
/************************************************************************/
void deleteUnfrequentItem(map<string, int>& result)
{
	vector<string> deleteKeys;
	for (auto p = result.begin(); p != result.end(); p++)
	{
		if (p->second < getThreshold())
		{
			deleteKeys.push_back(p->first);
		}
	}
	for (auto p = deleteKeys.begin(); p != deleteKeys.end(); p++)
	{
		result.erase(*p);
	}
}
/************************************************************************/
/* 根据map结果，对transactions中的Item重排，如果map里面没有，则直接delete  */
/************************************************************************/
vector<vector<node>> reRankFrequentItem(map<string, int> const & result, vector<transaction>& transactions)
{
	vector<vector<node>> reRankResult;
	for (auto p = transactions.begin(); p != transactions.end(); p++)
	{
		auto q = p->Items.begin();
		while (q != p->Items.end())
		{
			if (result.find(q->itemName) == result.end())
			{
				//map里面没有q,直接delete
				q = p->Items.erase(q);
			}
			else
			{//map里面有，修改其频率为map后的频率
				q->frequence = result.at(q->itemName);
				q++;
			}
		}
		//删除非频繁元素，修改完频率后，对其依据频率排序
		p->Items.sort(greaterThan);
		//排序完后，此时vector中所有项即为准备生成Fp-tree的所有项，将其所有频率清为1
		for (auto ptr = p->Items.begin(); ptr != p->Items.end(); ptr++)
		{
			ptr->frequence = 1;
		}
	}
	//提取每个transaction的重排后的frequent item
	for (auto p = transactions.begin(); p != transactions.end(); p++)
	{
		vector<node>temp(p->Items.begin(), p->Items.end());
		reRankResult.push_back(temp);
	}
	return reRankResult;
}
/************************************************************************/
/* 根据vector<vector<node>>的列表，生成由大到小排列的HeaderTable  */
/************************************************************************/
vector<headerItem> makeheaderTable(vector<vector<node>>const &Fitems)
{
	//遍历Fitems,用map对其进行计数
	map<string, int> mapItem;
	vector<headerItem> headerTable;
	for (auto p = Fitems.cbegin(); p != Fitems.cend(); p++)
	{
		for (auto q = p->cbegin(); q != p->cend(); q++)
		{
			if (mapItem.find(q->itemName) == mapItem.end())
			{
				//该频繁项没有在mapItem中,则加进去，计数为其初始频率
				mapItem[q->itemName] = q->frequence;
			}
			else
			{
				//在mapItem中，直接加node的频率
				mapItem[q->itemName] += q->frequence;
			}
		}
	}
	//遍历完后，去除map中不频繁的项目
	deleteUnfrequentItem(mapItem);
	//将map后的值复制进headerItem中
	for (auto p = mapItem.begin(); p != mapItem.end(); p++)
	{
		headerItem tempItem;
		tempItem.item = p->first;
		tempItem.frequence = p->second;
		tempItem.phead.clear();
		headerTable.push_back(tempItem);
	}
	//对headerTable中的元素由大到小排列
	sort(headerTable.rbegin(), headerTable.rend());
	return headerTable;
}
/************************************************************************/
/* 给定Item,查找该节点是否在headerTable中，并返回其位置                    */
/************************************************************************/
int findPosInheaderTable(string Item, vector<headerItem>const& headerTable)
{
	int pos = NON_EXIST;
	for (int p = 0; p != headerTable.size(); p++)
	{
		if (headerTable[p].item == Item)
		{
			pos = p;
		}
	}
	return pos;
}
/************************************************************************/
/* 给定一个node,判断此node是否在vector<node*>中                                                                     */
/************************************************************************/
int findPosInChildren(node s, vector<node*>const & children)
{
	int pos = NON_EXIST;
	for (int i = 0; i < children.size(); i++)
	{
		if (children[i]->itemName == s.itemName)
		{
			pos = i;
		}
	}
	return pos;
}
/************************************************************************/
/* 根据vector<vector<node>>的列表，生成FP-tree                                   */
/************************************************************************/
node *  CreateFpTree(vector<vector<node>>const &Fitems, vector<headerItem>& headerTable)
{
	//创建FP-tree的头节点并初始化
	node* HeaderNode = new node;
	HeaderNode->itemName = "";
	HeaderNode->nextfriend = NULL;
	HeaderNode->parent = NULL;
	HeaderNode->frequence = 0;
	HeaderNode->children.clear();
	//注意：建FP-tree时，需要保证Fitems中，必须是按照出现次数递减排列的，否则无法建立FP-tree
	for (int i = 0; i < Fitems.size(); i++)
	{
		//每一系列的交易记录当做一个处理单位
		//首先定位此交易记录的首元素在FP-tree中的位置
		node* NodeInFp = HeaderNode;
		int j = 0;//记录当前待比对的Fitems[i](vector)中的node的序号
		while (j != Fitems[i].size())
		{
			//在当前的FPNode中查找待加入的节点
			int pos = findPosInChildren(Fitems[i][j], NodeInFp->children);
			if (pos != NON_EXIST)
			{
				//如果找到，将当前孩子的频率增加，FPNode指向该孩子，在其孩子中查找下一个节点
				NodeInFp->children[pos]->frequence += Fitems[i][j].frequence;
				NodeInFp = NodeInFp->children[pos];
				j++;
			}
			else
			{
				//没找到，将Fitems[i][j]以后的元素(包括j)全部new 节点，并连接到headerTable中
				while (j != Fitems[i].size())
				{
					node * new_node = new node;
					*new_node = Fitems[i][j];
					//每分配一个节点，检查其是否在HeaderTable中，如果在，则建立指向它的指针
					int pos = findPosInheaderTable(new_node->itemName, headerTable);
					if (pos != NON_EXIST)
					{
						headerTable[pos].phead.push_back(new_node);
					}
					//当前节点new_node的parent是NodeInFp,把当前节点加入FP-tree,并使它成为新的NodeInFp
					NodeInFp->children.push_back(new_node);
					new_node->parent = NodeInFp;
					NodeInFp = new_node;
					j++;
				}
				break;
			}
		}
	}
	return HeaderNode;
}
/************************************************************************/
/* 根据HeaderTable中的某一项和相应的FP-tree,获取相应的频繁模式基vector<vector<node>> */
/************************************************************************/
vector<vector<node>> getFrequentModeBase(headerItem const &hitem, node * const &fpTreeRoot)
{
	vector<vector<node>> frequentModes;
	for (auto p = hitem.phead.begin(); p != hitem.phead.end(); p++)
	{
		vector<node> frequentMode;
		node * ptr = *p;
		int currentFrequency = ptr->frequence;
		//由ptr指针逆行寻找父亲节点
		while (ptr->parent != NULL)
		{
			if (ptr->parent->itemName != "")
			{
				node temp = *(ptr->parent);
				temp.frequence = currentFrequency;
				temp.children.clear();
				temp.parent = NULL;
				frequentMode.push_back(temp);
			}
			ptr = ptr->parent;
		}
		reverse(frequentMode.begin(), frequentMode.end());
		frequentModes.push_back(frequentMode);
	}
	return frequentModes;
}
/************************************************************************/
/*对于列表中的任意一项，递归产生所有的频繁项集                                             */
/************************************************************************/
void  generateFrequentItems(vector<headerItem>& headerTable, node * &fpTreeRoot, string &base,
	list<frequentMode>& modeLists)
{
	if (headerTable.empty())
	{
		return;
	}
	for (auto p = headerTable.rbegin(); p != headerTable.rend(); p++)
	{
		/*对于headerTable中的每一项*/
		frequentMode model;
		model.mode = base + " " + p->item;
		model.count = p->frequence;
		modeLists.push_back(model);
		//产生currentMode的条件模式基
		vector<vector<node>> ConditionalmodeBase = getFrequentModeBase(*p, fpTreeRoot);
		//根据条件模式基，生成新的HeaderTable和Fp-tree
		vector<headerItem> headerTable2 = makeheaderTable(ConditionalmodeBase);
		node * fptree2 = CreateFpTree(ConditionalmodeBase, headerTable2);
		generateFrequentItems(headerTable2, fptree2, base + " " + p->item, modeLists);
	}
}
/************************************************************************/
/* output all frequent Items                                                                     */
/************************************************************************/
void outputItems(list<frequentMode> const &modeLists)
{
	cout << "Frequent Items:" << endl;
	for (auto p = modeLists.cbegin(); p != modeLists.cend(); p++)
	{
		cout << p->mode << "\t" << p->count << endl;
	}
}

int main(int argc, char * argv[])
{
	vector<transaction> transactions = readFile(string(argv[1]));
	map<string, int> result = getFrequentItem(transactions);
	deleteUnfrequentItem(result);//获取一阶频繁项
	vector<vector<node>>rerankResult = reRankFrequentItem(result, transactions);//将所有结果按一阶频繁项顺序重排
	vector<headerItem> headerTable = makeheaderTable(rerankResult);//初始化HeaderTable
	node * fpTreeRoot = CreateFpTree(rerankResult, headerTable);//初始化Fp-tree
	list<frequentMode> modeLists;//保存频繁项
	generateFrequentItems(headerTable, fpTreeRoot, string(""), modeLists);//递归挖掘Fp-tree
	outputItems(modeLists);//输出所有频繁项集
	system("pause");
	//vector<vector<node>> ConditionalmodeBase = getFrequentModeBase(headerTable[2],fpTreeRoot);
	//vector<headerItem> headerTable2 = makeheaderTable(ConditionalmodeBase);
	//node * fptree2 = CreateFpTree(ConditionalmodeBase,headerTable2);
}
