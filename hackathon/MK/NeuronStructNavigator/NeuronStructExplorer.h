#ifndef NEURONSTRUCTEXPLORER_H
#define NEURONSTRUCTEXPLORER_H

#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <string>

#include <boost/config.hpp>
#include <boost/graph/prim_minimum_spanning_tree.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/algorithm/string.hpp>

#include <qlist.h>
#include <qstring.h>
#include <qstringlist.h>

#include "basic_surf_objs.h"
#include "v_neuronswc.h"
#include "NeuronStructUtilities.h"

using namespace std;

enum edge_lastvoted_t { edge_lastvoted };
namespace boost 
{
	BOOST_INSTALL_PROPERTY(edge, lastvoted);
}

typedef boost::property<boost::edge_weight_t, double> weights;
typedef boost::property<edge_lastvoted_t, int, weights> lastVoted;
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, lastVoted> undirectedGraph;

#ifndef PI
#define PI 3.1415926
#endif

static enum connectOrientation { head_head, head_tail, tail_head, tail_tail, head, tail };

struct topoCharacter
{
	topoCharacter(NeuronSWC centerNode) : topoCenter(centerNode) {};
	NeuronSWC topoCenter;
	NeuronSWC topoCenterPa;
	vector<NeuronSWC> topoCenterImmedChildren;
	double childAngle;
	map<int, double> immedChildrenLengths;
	map<int, double> subsequentChildrenAngles;
};

struct segUnit
{
	segUnit() : to_be_deted(false) {};

	int segID;
	int head;
	vector<int> tails;
	QList<NeuronSWC> nodes;
	map<int, size_t> seg_nodeLocMap;
	map<int, vector<size_t>> seg_childLocMap;
	vector<topoCharacter> topoCenters;

	bool to_be_deted;
};

struct profiledTree
{
	profiledTree() {};
	profiledTree(const NeuronTree& inputTree);

	NeuronTree tree;
	map<string, vector<int>> nodeTileMap; // tile label -> node ID
	map<string, vector<int>> segHeadMap;  // tile label -> seg ID
	map<string, vector<int>> segTailMap;  // tile label -> seg ID

	map<int, size_t> node2LocMap;
	map<int, vector<size_t>> node2childLocMap;

	map<int, segUnit> segs; // key = seg ID
};

class NeuronStructExplorer
{
public:
	/********* Constructors and basic data members *********/
	NeuronStructExplorer() {};
	NeuronStructExplorer(QString neuronFileName);
	NeuronStructExplorer(const NeuronTree inputTree) { this->treeEntry(inputTree, "originalTree"); }

	NeuronTree* singleTreePtr;
	NeuronTree singleTree;
	NeuronTree processedTree;

	map<string, profiledTree> treeDataBase;
	void treeEntry(const NeuronTree& inputTree, string treeName);
	/*******************************************************/

	/***************** Neuron Struct Connecting Functions *****************/
	static map<int, segUnit> findSegs(const QList<NeuronSWC>& inputNodeList, map<int, vector<size_t>>& node2childLocMap);
	static map<string, vector<int>> segTileMap(const vector<segUnit>& inputSegs, bool head = true, float xyLength = 30);
	
	NeuronTree SWC2MSTtree(NeuronTree const& inputTreePtr);
	static inline NeuronTree MSTtreeCut(NeuronTree& inputTree, double distThre = 10);
	static NeuronTree MSTbranchBreak(const profiledTree& inputProfiledTree, bool spikeRemove = true);
	vector<segUnit> MSTtreeTrim(vector<segUnit>& inputSegUnits); 
	
	NeuronTree segElongate(const profiledTree& inputProfiledTree);
	segUnit segUnitConnect(const pair<int, int>& elongConnPair, const profiledTree& currProfiledTree, const segUnit& segUnit1, const segUnit& segUnit2, connectOrientation connOrt);
	/**********************************************************************/

	/***************** Geometry *****************/
	inline static vector<float> getDispUnitVector(const vector<float>& headVector, const vector<float>& tailVector);
	inline static double getRadAngle(const vector<float>& vector1, const vector<float>& vector2);
private:
	double segPointingCompare(const segUnit& elongSeg, const segUnit& connSeg, connectOrientation connOrt);
	double segTurningAngle(const segUnit& elongSeg, const segUnit& connSeg, connectOrientation connOrt);
	map<int, segUnit> segRegionConn_angle(const vector<int>& currTileHeadSegIDs, const vector<int>& currTileTailSegIDs, profiledTree& currProfiledTree, bool length = false);
	inline void tileSegConnOrganizer_angle(const map<string, double>& segAngleMap, set<int>& connectedSegs, map<int, int>& elongConnMap);
	/********************************************/

public:
	/********* Pixel-based deep neural network result refining/cleaning *********/
	unordered_map<string, unordered_map<int, float>> zProfileMap;
	void detectedPixelStackZProfile(NeuronTree* inputTreePtr, NeuronTree* outputTreePtr);
	/****************************************************************************/

	/********* Distance-based SWC analysis *********/
	vector<vector<float>> FPsList;
	vector<vector<float>> FNsList;
	void falsePositiveList(NeuronTree* detectedTreePtr, NeuronTree* manualTreePtr, float distThreshold = 20);
	void falseNegativeList(NeuronTree* detectedTreePtr, NeuronTree* manualTreePtr, float distThreshold = 20);
	void detectedDist(NeuronTree* inputTreePtr1, NeuronTree* inputTreePtr2);

	map<int, long int> nodeDistCDF;
	map<int, long int> nodeDistPDF;
	void shortestDistCDF(NeuronTree* inputTreePtr1, NeuronTree* inputTreePtr2, int upperBound, int binNum = 500);
	/*********************************/

	/********* Segment <-> Image assessment functionalities *********/
	V_NeuronSWC_list segmentList;
	void segmentDecompose(NeuronTree* inputTreePtr);
	/**************************************************************/
};

inline NeuronTree NeuronStructExplorer::MSTtreeCut(NeuronTree& inputTree, double distThre)
{
	NeuronTree outputTree;
	for (QList<NeuronSWC>::iterator it = inputTree.listNeuron.begin(); it != inputTree.listNeuron.end(); ++it)
	{
		if (it->parent != -1)
		{
			double x1 = it->x;
			double y1 = it->y;
			double z1 = it->z;
			size_t paID = it->parent;
			double x2 = inputTree.listNeuron.at(paID - 1).x;
			double y2 = inputTree.listNeuron.at(paID - 1).y;
			double z2 = inputTree.listNeuron.at(paID - 1).z;
			double dist = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + zRATIO * zRATIO * (z1 - z2) * (z1 - z2));
			if (dist > distThre)
			{
				outputTree.listNeuron.push_back(*it);
				(outputTree.listNeuron.end() - 1)->parent = -1;
			}
			else outputTree.listNeuron.push_back(*it);
		}
	}

	return outputTree;
}

inline vector<float> NeuronStructExplorer::getDispUnitVector(const vector<float>& headVector, const vector<float>& tailVector)
{
	float disp = sqrt((headVector.at(0) - tailVector.at(0)) * (headVector.at(0) - tailVector.at(0)) +
					  (headVector.at(1) - tailVector.at(1)) * (headVector.at(1) - tailVector.at(1)) +
					  (headVector.at(2) - tailVector.at(2)) * (headVector.at(2) - tailVector.at(2)));
	vector<float> dispUnitVector;
	dispUnitVector.push_back((headVector.at(0) - tailVector.at(0)) / disp);
	dispUnitVector.push_back((headVector.at(1) - tailVector.at(1)) / disp);
	dispUnitVector.push_back((headVector.at(2) - tailVector.at(2)) / disp);

	return dispUnitVector;
}

inline double NeuronStructExplorer::getRadAngle(const vector<float>& vector1, const vector<float>& vector2)
{
	double dot = (vector1.at(0) * vector2.at(0) + vector1.at(1) * vector2.at(1) + vector1.at(2) * vector2.at(2));
	double sq1 = (vector1.at(0) * vector1.at(0) + vector1.at(1) * vector1.at(1) + vector1.at(2) * vector1.at(2));
	double sq2 = (vector2.at(0) * vector2.at(0) + vector2.at(1) * vector2.at(1) + vector2.at(2) * vector2.at(2));

	double angle = acos(dot / sqrt(sq1 * sq2));
	if (isnan(acos(dot / sqrt(sq1 * sq2)))) return -1;
	else return angle / PI;
}

inline void NeuronStructExplorer::tileSegConnOrganizer_angle(const map<string, double>& segAngleMap, set<int>& connectedSegs, map<int, int>& elongConnMap)
{
	map<double, string> r_segAngleMap;
	for (map<string, double>::const_iterator reIt = segAngleMap.begin(); reIt != segAngleMap.end(); ++reIt)
		r_segAngleMap.insert(pair<double, string>(reIt->second, reIt->first));

	for (map<double, string>::iterator angleIt = r_segAngleMap.begin(); angleIt != r_segAngleMap.end(); ++angleIt)
	{
		vector<string> key1key2;
		boost::split(key1key2, angleIt->second, boost::is_any_of("_"));

		int seg1 = stoi(key1key2.at(0));
		int seg2 = stoi(key1key2.at(1));
		if (connectedSegs.find(seg1) == connectedSegs.end() && connectedSegs.find(seg2) == connectedSegs.end())
		{
			connectedSegs.insert(seg1);
			connectedSegs.insert(seg2);
			elongConnMap.insert(pair<int, int>(seg1, seg2));
		}
	}
}

#endif