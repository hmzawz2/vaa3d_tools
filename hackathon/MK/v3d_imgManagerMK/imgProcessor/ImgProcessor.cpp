#include <iostream>

#include <boost\container\flat_set.hpp>

#include "ImgProcessor.h"

using namespace std;

void ImgProcessor::skeleton2D(unsigned char inputImgPtr[], unsigned char outputImgPtr[], int imgDims[])
{
	unsigned char** inputImg2Dptr = new unsigned char*[imgDims[1] + 2]; // imputImg2DPtr -> enlarged 2D binaray image. (1 v 0)
	for (int j = 1; j <= imgDims[1]; ++j)
	{
		inputImg2Dptr[j] = new unsigned char[imgDims[0] + 2];
		for (int i = 1; i <= imgDims[0]; ++i)
		{
			if (int(inputImgPtr[imgDims[0] * (j - 1) + (i - 1)]) > 0) inputImg2Dptr[j][i] = 1;
			else inputImg2Dptr[j][i] = 0;
		}
		inputImg2Dptr[j][0] = 0;
		inputImg2Dptr[j][imgDims[0] + 1] = 0;
	}
	inputImg2Dptr[0] = new unsigned char[imgDims[0] + 2];
	inputImg2Dptr[imgDims[1] + 1] = new unsigned char[imgDims[0] + 2];
	for (int i = 0; i < imgDims[0] + 2; ++i)
	{
		inputImg2Dptr[0][0] = 0;
		inputImg2Dptr[imgDims[1] + 1][i] = 0;
	}

	boost::container::flat_set<vector<int>> whitePixAdds; // whitePixAdds stores the coordinate of each white pixel.
	for (int j = 1; j <= imgDims[1]; ++j)
	{
		for (int i = 1; i <= imgDims[0]; ++i)
		{
			if (int(inputImg2Dptr[j][i]) > 0)
			{
				vector<int> whitePixAdd(2);
				whitePixAdd.at(0) = i;
				whitePixAdd.at(1) = j;
				whitePixAdds.insert(whitePixAdd);
			}
		}
	}
	
	int kernel[8];
	boost::container::flat_set<vector<int>> pixToBeDeleted;
	vector<ptrdiff_t> delPositions;
	while (true)
	{
		for (boost::container::flat_set<vector<int>>::iterator it = whitePixAdds.begin(); it != whitePixAdds.end(); ++it)
		{
			kernel[0] = int(inputImg2Dptr[it->at(1) - 1][it->at(0) - 1]);
			kernel[1] = int(inputImg2Dptr[it->at(1) - 1][it->at(0)]);
			kernel[2] = int(inputImg2Dptr[it->at(1) - 1][it->at(0) + 1]);
			kernel[3] = int(inputImg2Dptr[it->at(1)][it->at(0) + 1]);
			kernel[4] = int(inputImg2Dptr[it->at(1) + 1][it->at(0) + 1]);
			kernel[5] = int(inputImg2Dptr[it->at(1) + 1][it->at(0)]);
			kernel[6] = int(inputImg2Dptr[it->at(1) + 1][it->at(0) - 1]);
			kernel[7] = int(inputImg2Dptr[it->at(1)][it->at(0) - 1]);
			
			int kernelSum = 0;
			for (int keri = 0; keri < 8; ++keri) kernelSum = kernelSum + kernel[keri];
			
			if (kernelSum < 2) continue;
			if (kernelSum > 6) continue;
			
			switch (kernelSum)
			{
				case 2:
				{
					if ((kernel[1] == 1 && kernel[3] == 1) || (kernel[3] == 1 && kernel[5] == 1) || (kernel[5] == 1 && kernel[7] == 1) || (kernel[7] == 1 && kernel[1] == 1))
					{
						vector<int> delPix = *it;
						pixToBeDeleted.insert(delPix);
						delPositions.push_back(it - whitePixAdds.begin());
						goto WHITEPIX_ADDR_TO_BE_DELETED;
					}
					break;
				}

				case 3:
				{
					if ((kernel[0] + kernel[6] + kernel[7]) == 3 || (kernel[0] + kernel[1] + kernel[7] == 3))
					{
						vector<int> delPix = *it;
						pixToBeDeleted.insert(delPix);
						delPositions.push_back(it - whitePixAdds.begin());
						goto WHITEPIX_ADDR_TO_BE_DELETED;
					}
					if (kernel[7] == 1 && kernel[5] == 1)
					{
						if (kernel[4] == 1 || kernel[0] == 1)
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
						
						if (kernel[1] == 1)
						{
							if (kernel[2] == 1 || kernel[6] == 1)
							{
								vector<int> delPix = *it;
								pixToBeDeleted.insert(delPix);
								delPositions.push_back(it - whitePixAdds.begin());
								goto WHITEPIX_ADDR_TO_BE_DELETED;
							}
						}
					}
					for (int k = 0; k < 6; ++k)
					{
						if (kernel[k] + kernel[k + 1] + kernel[k + 2] == 3)
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					}
					break;
				}

				case 4:
				{
					if (kernel[6] == 0 || kernel[0] == 0)
					{
						if ((kernel[4] + kernel[5] + kernel[7] + kernel[0]) == 4 || (kernel[1] + kernel[2] + kernel[6] + kernel[7]) == 4)
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					}
					for (int k = 0; k < 4; ++k)
					{
						if ((kernel[k] + kernel[k + 1] + kernel[k + 2] + kernel[k + 3]) == 0 || (kernel[k] + kernel[k + 1] + kernel[k + 2] + kernel[k + 3]) == 4)
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					}
					break;
				}

				case 5:
				{
					if ((kernel[0] + kernel[1] + kernel[7]) == 0)
					{
						vector<int> delPix = *it;
						pixToBeDeleted.insert(delPix);
						delPositions.push_back(it - whitePixAdds.begin());
						goto WHITEPIX_ADDR_TO_BE_DELETED;
					}
					if ((kernel[0] + kernel[6] + kernel[7] == 0) && (inputImg2Dptr[it->at(1)][it->at(0) + 2] == 1))
					{
						vector<int> delPix = *it;
						pixToBeDeleted.insert(delPix);
						delPositions.push_back(it - whitePixAdds.begin());
						goto WHITEPIX_ADDR_TO_BE_DELETED;
					}
					if ((kernel[0] + kernel[1] + kernel[2] == 0) && (inputImg2Dptr[it->at(1) + 2][it->at(0)] == 1))
					{
						vector<int> delPix = *it;
						pixToBeDeleted.insert(delPix);
						delPositions.push_back(it - whitePixAdds.begin());
						goto WHITEPIX_ADDR_TO_BE_DELETED;
					}
					for (int k = 1; k < 6; ++k)
					{
						if ((kernel[k] + kernel[k + 1] + kernel[k + 2] == 0))
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					}
					break;
				}

				case 6:
				{
					if (kernel[3] == 0)
					{
						if (kernel[2] == 0 || kernel[4] == 0)
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					}
					else if (kernel[5] == 0)
					{
						if (kernel[4] == 0 || kernel[6] == 0)
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					}
					if ((kernel[0] + kernel[7] == 0) || (kernel[6] + kernel[7] == 0))
					{
						if (inputImg2Dptr[it->at(1)][it->at(0) + 2] == 1)
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					}
					if ((kernel[0] + kernel[1] == 0) || (kernel[1] + kernel[2] == 0))
					{
						if (inputImg2Dptr[it->at(1) + 2][it->at(0)] == 1)
						{
							vector<int> delPix = *it;
							pixToBeDeleted.insert(delPix);
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					}
					break;
				}
			}

			switch (kernelSum)
			{
				case 3:
				{
					if (kernel[4] + kernel[6] == 2)
						if (kernel[0] == 1 || kernel[2] == 1 || kernel[1] == 1)
						{
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					
					if (kernel[2] + kernel[4] == 2)
						if (kernel[0] == 1 || kernel[6] == 1 || kernel[7] == 1)
						{
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					
					if (kernel[0] + kernel[2] == 2)
						if (kernel[6] == 1 || kernel[4] == 1 || kernel[5] == 1)
						{
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					
					if (kernel[0] + kernel[6] == 2)
						if (kernel[3] == 1 || kernel[2] == 1 || kernel[4] == 1)
						{
							delPositions.push_back(it - whitePixAdds.begin());
							goto WHITEPIX_ADDR_TO_BE_DELETED;
						}
					break;
				}

				case 4:
				{
					if ((kernel[1] + kernel[5] == 0) || (kernel[3] + kernel[7] == 0))
					{
						delPositions.push_back(it - whitePixAdds.begin());
						goto WHITEPIX_ADDR_TO_BE_DELETED;
					}
					break;
				}

				case 5:
				{
					if ((kernel[1] + kernel[5] == 0) || (kernel[3] + kernel[7] == 0))
					{
						delPositions.push_back(it - whitePixAdds.begin());
						goto WHITEPIX_ADDR_TO_BE_DELETED;
					}
					break;
				}
			}

		WHITEPIX_ADDR_TO_BE_DELETED:
				continue;
		}

		if (pixToBeDeleted.empty()) break;

		sort(delPositions.rbegin(), delPositions.rend());
		for (vector<ptrdiff_t>::iterator delWhiteIt = delPositions.begin(); delWhiteIt != delPositions.end(); ++delWhiteIt)
			whitePixAdds.erase(whitePixAdds.begin() + *delWhiteIt);

		for (boost::container::flat_set<vector<int>>::iterator dePixIt = pixToBeDeleted.begin(); dePixIt != pixToBeDeleted.end(); ++dePixIt)
			inputImg2Dptr[dePixIt->at(1)][dePixIt->at(0)] = 0;

		delPositions.clear();
		pixToBeDeleted.clear();
	}

	bool blockErase = true;
	while (blockErase)
	{
		for (boost::container::flat_set<vector<int>>::iterator blockCheckIt = whitePixAdds.begin(); blockCheckIt != whitePixAdds.end(); ++blockCheckIt)
		{
			kernel[0] = int(inputImg2Dptr[blockCheckIt->at(1) - 1][blockCheckIt->at(0) - 1]);
			kernel[1] = int(inputImg2Dptr[blockCheckIt->at(1) - 1][blockCheckIt->at(0)]);
			kernel[2] = int(inputImg2Dptr[blockCheckIt->at(1) - 1][blockCheckIt->at(0) + 1]);
			kernel[3] = int(inputImg2Dptr[blockCheckIt->at(1)][blockCheckIt->at(0) + 1]);
			kernel[4] = int(inputImg2Dptr[blockCheckIt->at(1) + 1][blockCheckIt->at(0) + 1]);
			kernel[5] = int(inputImg2Dptr[blockCheckIt->at(1) + 1][blockCheckIt->at(0)]);
			kernel[6] = int(inputImg2Dptr[blockCheckIt->at(1) + 1][blockCheckIt->at(0) - 1]);
			kernel[7] = int(inputImg2Dptr[blockCheckIt->at(1)][blockCheckIt->at(0) - 1]);

			int kernelSum = 0;
			int conn4count = 0;
			for (int keri = 0; keri < 8; ++keri) kernelSum = kernelSum + kernel[keri];
			conn4count = conn4count + kernel[1] + kernel[3] + kernel[5] + kernel[7];
			if (kernelSum >= 4 && conn4count == 3)
			{
				inputImg2Dptr[blockCheckIt->at(1)][blockCheckIt->at(0)] = 0;
				whitePixAdds.erase(blockCheckIt);
				goto FOUND_2X2_BLOCK;
			}
		}
		blockErase = false;

	FOUND_2X2_BLOCK:
		continue;
	}

	size_t outputCountI = 0;
	for (int j = 1; j <= imgDims[1]; ++j)
	{
		for (int i = 1; i <= imgDims[0]; ++i)
		{
			if (inputImg2Dptr[j][i] == 1) outputImgPtr[outputCountI] = 255;
			else outputImgPtr[outputCountI] = 0;
			++outputCountI;
		}
	}
}
