
#ifndef __NAIVE_BAYES_MODEL__
#define __NAIVE_BAYES_MODEL__
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <hash_set>
using namespace std;
void split(const string& src, const string& separator, vector<string>& dest);
class NaiveBayesModel;

class NaiveBayesModel
{
private:
	int classNum;
	int V;//词的总个数
    double **cv;//类别——词的概率表
	double *prior;
	map<string, int> vDict;
	map<string, int> classDict;
	hash_set<string> stopWords;
	void trainClass(string classRoot, int c);
public:
	NaiveBayesModel() {
		classNum = 8;
	}
	void buildDictionary(string docPath);
	/**
	**方法名称：train
	**参数：类别目录列表
	**说明：每个类别目录列表中都有一个配置文件，说明该目录下的文档时什么类别的文档
	*/
	void train(string classPathRoot);
	void train(vector<string>& classPaths);
	int predict(string predictPath);
	void init(int c, int v)
	{
		this->classNum = c;
		this->V = v;
		prior = new double[c];
		for (int i = 0; i < c; i++)
			this->prior[i] = 0.0;
		cv = new double*[c];
		for (int i = 0; i < c; i++)
		{
			cv[i] = new double[v];
			for (int j = 0; j < V; j++)
				cv[i][j] = 0.0;
		}
	}
	NaiveBayesModel(int c, int v) :classNum(c), V(v)
	{
		prior = new double[c];
		cv = new double*[c];
		for (int i = 0; i < c; i++)
		{
			cv[i] = new double[v];
		}
	}
};

#endif
