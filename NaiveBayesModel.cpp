#include "NaiveBayesModel.h"
#include "ICTCLAS50.h"
#include <Windows.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <math.h>
using namespace std;

void split(const string& src, const string& separator, vector<string>& dest)
{
	string str = src;
	string substring;
	string::size_type start = 0, index;

	do
	{
		index = str.find_first_of(separator, start);
		if (index != string::npos)
		{
			substring = str.substr(start, index - start);
			dest.push_back(substring);
			start = str.find_first_not_of(separator, index);
			if (start == string::npos) return;
		}
	} while (index != string::npos);

	//the last token
	substring = str.substr(start);
	dest.push_back(substring);
}
void NaiveBayesModel::buildDictionary(string docPath)
{
	if (!ICTCLAS_Init()) //初始化分词组件。
	{
		printf("Init fails\n");
		return;
	}
	else
	{
		printf("Init ok\n");
	}

	//设置词性标注集(0 计算所二级标注集，1 计算所一级标注集，2 北大二级标注集，3 北大一级标注集)
	ICTCLAS_SetPOSmap(2);
	char szFind[MAX_PATH];
	char tmp[MAX_PATH];
	WIN32_FIND_DATAA FindFileData;
	strcpy(szFind, docPath.c_str());
	strcat(szFind, "*.*");
	HANDLE hFind = ::FindFirstFileA(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return;
	int id = 0;
	while (TRUE)
	{
		if (FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			;
		else
		{
			strcpy(tmp, docPath.c_str());
			strcat(tmp, FindFileData.cFileName);
			char line[1024];
			ifstream in(tmp);
			while (in.getline(line, 1024))
			{
				char aLine[1024 * 4];
				int nRstLen = 0; //分词结果的长度

				nRstLen = ICTCLAS_ParagraphProcess(line, strlen(line) , aLine, CODE_TYPE_UNKNOWN, 0);  //字符串处理
				string s(aLine);
				vector<string> tokens;
				split(line, " ", tokens);
				for (int i = 0; i < tokens.size(); i++)
				{
					if (stopWords.find(tokens[i])==stopWords.end()&&//不是停用词
						vDict.find(tokens[i]) == vDict.end()) //没有在词典中发现
					{
						vDict[tokens[i]] = id++;
					}
				}
			}
		}
		if (!FindNextFileA(hFind, &FindFileData))
			break;
	}
	ICTCLAS_Exit();	//释放资源退出
	this->init(classNum, vDict.size());
}
/*
method: train
parameter:classPathRoot 代表需要训练的数据的根文件夹
注：根文件夹下面会有几个小的文件夹，每一个文件夹代表的是一个类别的文章，文件夹下面会有很多的文档
*/
void NaiveBayesModel::train(string classPathRoot)
{
	char szFind[MAX_PATH];
	char tmp[MAX_PATH];
	vector<string> paths;
	WIN32_FIND_DATAA FindFileData;
	strcpy(szFind, classPathRoot.c_str());
	strcat(szFind, "*.*");
	HANDLE hFind = ::FindFirstFileA(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return;
	int c_num = 0;
	while (TRUE)
	{
		if (FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			strcpy(tmp, classPathRoot.c_str());
			strcat(tmp, FindFileData.cFileName);
			strcat(tmp, "\\");
			paths.push_back(string(tmp));
		}
		if (!FindNextFileA(hFind, &FindFileData))
			break;
	}
	train(paths);
}
void NaiveBayesModel::trainClass(string classRoot, int c)
{
	if (!ICTCLAS_Init()) //初始化分词组件。
	{
		printf("Init fails\n");
		return;
	}
	else
	{
		printf("Init ok\n");
	}

	//设置词性标注集(0 计算所二级标注集，1 计算所一级标注集，2 北大二级标注集，3 北大一级标注集)
	ICTCLAS_SetPOSmap(2);
	char szFind[MAX_PATH];
	char tmp[MAX_PATH];
	WIN32_FIND_DATAA FindFileData;
	strcpy(szFind, classRoot.c_str());
	strcat(szFind, "*.*"); 
	HANDLE hFind = ::FindFirstFileA(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return;
	int c_num = 0;
	while (TRUE)
	{
		if (FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}
		else
		{
			c_num++;
			strcpy(tmp, classRoot.c_str());
			strcat(tmp, FindFileData.cFileName);
			ifstream in(tmp);
			char line[1024];
			while (in.getline(line, 1024))
			{
				char aLine[1024 * 4];
				int nRstLen = 0; //分词结果的长度
				nRstLen = ICTCLAS_ParagraphProcess(line, strlen(line), aLine, CODE_TYPE_UNKNOWN, 0);  //字符串处理
				string s(aLine);
				vector<string> tokens;
				split(line, " ", tokens);
				for (int i = 0; i < tokens.size(); i++)
				{
					if (vDict.find(tokens[i]) != vDict.end())
					{
						this->cv[c][vDict[tokens[i]]]++;
					}
				}
			}
			in.close();
		}
		if (!FindNextFileA(hFind, &FindFileData))
			break;
	}
	ICTCLAS_Exit();	//释放资源退出
	this->prior[c] = c_num;
	
}
/*
 method:train
 parameter:classPaths
*/
void NaiveBayesModel::train(vector<string>& classPaths)
{
	for (int i = 0; i < classPaths.size(); i++)
	{
		this->trainClass(classPaths[i], i);//for every class , train it
	}
	double sum = 0.0;
	for (int i = 0; i < classPaths.size(); i++)
		sum += prior[i];
	//归一化操作
	for (int i = 0; i < classPaths.size(); i++)
		prior[i] /= sum;
	//归一化操作
	for (int i = 0; i < this->classNum; i++)
	{
		double sum = 0.0;
		for (int j = 0; j < this->V; j++)
		{
			sum += cv[i][j];
		}
		for (int j = 0; j < this->V; j++)
			cv[i][j] =(1+cv[i][j]) /(sum+V);//此处使用了加1平滑
	}
}

int NaiveBayesModel::predict(string predictPath)
{
	if (!ICTCLAS_Init()) //初始化分词组件。
	{
		printf("Init fails\n");
		return -1;
	}
	else
	{
		printf("Init ok\n");
	}
	//设置词性标注集(0 计算所二级标注集，1 计算所一级标注集，2 北大二级标注集，3 北大一级标注集)
	ICTCLAS_SetPOSmap(2);
	ifstream in(predictPath); 
	char line[1024];
	double *predictScores = new double[classNum];
	for (int i = 0; i < classNum; i++)
		predictScores[i] = log(prior[i]);
	while ((in.getline(line, 1024)))
	{
		char aLine[1024 * 4];
		int nRstLen = 0; //分词结果的长度
		nRstLen = ICTCLAS_ParagraphProcess(line, strlen(line), aLine, CODE_TYPE_UNKNOWN, 0);  //字符串处理
		string s(aLine);
		vector<string> tokens;
		split(aLine, " ", tokens);
		for (int i = 0; i < tokens.size(); i++)
		{
			if (vDict.find(tokens[i]) == vDict.end())
				continue;
			for (int j = 0; j < classNum; j++)
			{
				predictScores[j] += log(cv[j][vDict[tokens[i]]]);
			}
		}
	}
	
	int max = predictScores[0];
	int maxIndex = 0;
	for (int i = 1; i < classNum; i++)
	{
		if (predictScores[i]>max)
		{
			max = predictScores[i];
			maxIndex = i;
		}
	}
	return maxIndex;
}
