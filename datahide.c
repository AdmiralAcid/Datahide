#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 2)
typedef struct 
{
	unsigned char  bfType[2];
	unsigned int   bfSize;
	unsigned short bfReserve1;
	unsigned short bfReserve2;
	unsigned int   bfByteShift;

} BITMAPFILEHEADER;
#pragma pack(pop)

#pragma pack(push, 2)
typedef struct
{
	unsigned int   biSize;
	int            biWidth;
	int            biHeight;
	unsigned short biPlanes;
	unsigned short biColorDepth;
	unsigned int   biCompression;
	unsigned int   biImageSize;
	int            biPixPerMeterHor;
	int            biPixPerMeterVer;
	unsigned int   biUsedColors;
	unsigned int   biColorsNeeded;

} BITMAPINFOHEADER;
#pragma pack(pop)

typedef struct
{
	unsigned char average;
	short         diff;
	char          group;
	char          finalGroup;

} haarWavelet;

typedef struct
{
	unsigned char average;
	short         diff;
	unsigned char status;

} markingStruct;

struct freqOrder
{
	unsigned char symbol;
	unsigned int weight;
	struct freqOrder* left;
	struct freqOrder* right;

	struct freqOrder* lowerLeft;
	struct freqOrder* lowerRight;
	unsigned char leaf;
};

typedef struct freqOrder FREQORDER;
typedef FREQORDER* FREQORDERPTR;

//filetype check and meta output
int bmpInfoCheck(FILE *ptr, int showStats, BITMAPFILEHEADER *bfh, BITMAPINFOHEADER *bih)
{
	fread(bfh, sizeof(BITMAPFILEHEADER), 1, ptr);

	if (bfh->bfType[0]!='B' || bfh->bfType[1]!='M')
	{
		printf("Error: Wrong type of file.\n");
		return 0;
	}

	if (bfh->bfByteShift!=1078)
	{
		printf("Error: Wrong type of BMP file.\n");
		return 0;
	}

	fread(bih, sizeof(BITMAPINFOHEADER), 1, ptr);

	if (bih->biColorDepth!=8)
	{
		printf("Error: Wrong type of BMP file.\n");
		return 0;
	}

	if (showStats == 1)
	{
		printf("Type of the file: %c%c\n", bfh->bfType[0], bfh->bfType[1]);
		printf("Size of file: %d bytes\n", bfh->bfSize);
		printf("Content of reserve 1: %hd\n", bfh->bfReserve1);
		printf("Content of reserve 2: %hd\n", bfh->bfReserve2);
		printf("Byte shift to the image bytes: %d\n", bfh->bfByteShift);
		printf("Size of BITMAPINFOHEADER structure: %d bytes\n", bih->biSize);
		printf("Resolution: %dx%d pixels\n", bih->biWidth, bih->biHeight);
		printf("Color planes number: %hd\n", bih->biPlanes);
		printf("Color depth: %hd\n", bih->biColorDepth);
		printf("Compression: %d\n", bih->biCompression);
		printf("Image size: %d\n", bih->biImageSize);
		printf("Horizontal pixel-per-meter resolution: %d\n", bih->biPixPerMeterHor);
		printf("Vertical pixel-per-meter resolution: %d\n", bih->biPixPerMeterVer);
		printf("Color indexes used: %d\n", bih->biUsedColors);
		printf("Important color indexes: %d\n", bih->biColorsNeeded);
	}

	return 1;
}

int minimal(int a, int b)
{
	int c = (a<b) ? a : b;
	return c;
}

int absval (int a)
{
	int c = (a<0) ? -1*a : a;
	return c;
}

int divideByTwo(int a)
{
	if (a>=0)
	{
		a = a/2;
	}
	else
	{
		if (absval(a%2)==1)
		{
			a = a/2-1;
		}
		else
		{
			a = a/2;
		}
	}
	return a;
}

int locmapPainter(short limit, unsigned char locationMap[], haarWavelet hw[], int locMapSize)
{
	int a, b;
	int lsbAmount =0;
	for (a=0; a<locMapSize; a++)
	{
		if (hw[a].group == 'Z' || (hw[a].group == 'E' && absval(hw[a].diff) <= limit))
		{
			hw[a].finalGroup = 'X';
			
			int digit = 7-a%8;
			unsigned char power = 1;
			for (b=1; b<=digit; b++)
			{
				power = power*2;
			}
			locationMap[a/8] += power;
		}
		else
		{
			hw[a].finalGroup = 'C';
			if (hw[a].diff!=1 && hw[a].diff!=-2)
			{
				lsbAmount++;
			}
		}
	}

	return lsbAmount;
}

void lsbCollector(haarWavelet hw[], unsigned char lsbStream[], int size)
{
	int a, b;
	int cBit = 0;
	
	for (a=0; a<size; a++)
	{
		if (hw[a].finalGroup=='C' && hw[a].group!='N' && hw[a].diff!=1 && hw[a].diff!=-2)
		{
			short mask = 1;
			unsigned char lsb = hw[a].diff & mask ? 1 : 0;
			if (lsb==1)
			{
				int digit = 7 - cBit%8;
				unsigned char power = 1;
				for (b=0; b<digit; b++)
				{
					power = power*2;
				}

				lsbStream[cBit/8] += power;
			}

			cBit++;
		}
	}
}

FREQORDERPTR frequencyList(unsigned char array[], int freqSize)
{
	int a, b, size = 1;
	FREQORDERPTR startPtr = malloc(sizeof(FREQORDER));
	FREQORDERPTR currPtr;
	startPtr->symbol = array[0];
	startPtr->weight = 1;
	startPtr->left = NULL;
	startPtr->lowerLeft = NULL;
	startPtr->lowerRight = NULL;
	startPtr->leaf = 1;
	startPtr->right = NULL;

	for (a=1; a<freqSize; a++)
	{
		for (b=0; b<size; b++)
		{
			if (b==0)
			{
				currPtr = startPtr;
			}
			else
			{
				currPtr = currPtr->right;
			}

			if (currPtr->symbol==array[a])
			{
				currPtr->weight += 1;

				FREQORDERPTR leftPtr = currPtr->left;
				FREQORDERPTR rightPtr = currPtr->right;
				
				while (leftPtr!=NULL && currPtr->weight > leftPtr->weight)
				{
					FREQORDERPTR leftSidePtr = leftPtr->left;
					currPtr->left = leftSidePtr;
					leftPtr->left = currPtr;
					currPtr->right = leftPtr;
					leftPtr->right = rightPtr;
					if (leftSidePtr!=NULL)
					{
						leftSidePtr->right = currPtr;
					}
					if (rightPtr!=NULL)
					{
						rightPtr->left = leftPtr;
					}

					leftPtr = currPtr->left;
					rightPtr = currPtr->right;

					if (currPtr->left==NULL)
					{
						startPtr = currPtr;
					}
				}
				break;
			}
			else
			{
				if (b==size-1)
				{
					FREQORDERPTR newPtr = malloc(sizeof(FREQORDER));
					newPtr->symbol = array[a];
					newPtr->weight = 1;
					newPtr->left = currPtr;
					newPtr->lowerLeft = NULL;
					newPtr->lowerRight = NULL;
					newPtr->leaf = 1;
					currPtr->right = newPtr;
					newPtr->right = NULL;
					size++;

					break;
				}
			}
		}
	}

	return startPtr;
}

FREQORDERPTR treeConstruction(FREQORDERPTR endPtr)
{
	FREQORDERPTR currPtr = endPtr;

	while (currPtr->left!=NULL || currPtr->right!=NULL)
	{
		FREQORDERPTR leftPtr = currPtr->left;

		FREQORDERPTR newPtr = malloc(sizeof(FREQORDER));
		
		newPtr->lowerLeft = currPtr;
		newPtr->lowerRight = leftPtr;
		newPtr->weight = leftPtr->weight + currPtr->weight;
		newPtr->symbol = 0;
		newPtr->leaf = 0;
		newPtr->left = leftPtr->left;
		newPtr->right = NULL;
		
		if (leftPtr->left!=NULL)
		{
			FREQORDERPTR leftSidePtr = leftPtr->left;
			leftSidePtr->right = newPtr;
		}
		
		currPtr->left = NULL;
		currPtr->right = NULL;
		leftPtr->left = NULL;
		leftPtr->right = NULL;

		currPtr = newPtr;

		if (newPtr->left!=NULL)
		{
			leftPtr = newPtr->left;
			FREQORDERPTR rightPtr = newPtr->right;
			while (newPtr->weight > leftPtr->weight)
			{
				if (newPtr->right==NULL)
				{
					currPtr=newPtr->left;
				}

				FREQORDERPTR leftSidePtr = NULL;
				if (leftPtr->left!=NULL)
				{
					leftSidePtr = leftPtr->left;
				}				
				newPtr->left = leftSidePtr;
				leftPtr->left = newPtr;
				newPtr->right = leftPtr;
				leftPtr->right = rightPtr;
				if (leftSidePtr!=NULL)
				{
					leftSidePtr->right = newPtr;
				}
				if (rightPtr!=NULL)
				{
					rightPtr->left = leftPtr;
				}

				leftPtr = newPtr->left;
				rightPtr = newPtr->right;

				if (leftPtr==NULL)
				{
					break;
				}
			}
		}
	}
	return currPtr;
}

//filling the table
void preOrder(FREQORDERPTR currElem, unsigned char firstGen[][100], unsigned int* NOE,
 unsigned int depth, unsigned int* maximum, unsigned char binCode[])
{
	int a;
	if (currElem!=NULL)
	{
		if (currElem->leaf==1)
		{
			(*NOE)++;
			firstGen[*NOE][0] = currElem->symbol;
			firstGen[*NOE][1] = depth;

			for (a=0; a<depth; a++)
			{
				firstGen[*NOE][a+2] = binCode[a];
			}

			if (depth > *maximum)
			{
				*maximum = depth;
			}
		}
		else
		{
			binCode[depth] = 0;
			depth++;
			preOrder(currElem->lowerLeft, firstGen, NOE, depth, maximum, binCode);
			depth--;
			binCode[depth] = 1;
			depth++;
			preOrder(currElem->lowerRight, firstGen, NOE, depth, maximum, binCode);
			depth --;				
		}
	}
}

void delOrder(FREQORDERPTR currElem)
{
	if (currElem!=NULL)
	{
		if (currElem->leaf==1)
		{
			free(currElem);
		}
		else
		{
			delOrder(currElem->lowerLeft);
			delOrder(currElem->lowerRight);
			free(currElem);		
		}
	}
}

int recode(unsigned char fGen[][100], unsigned char locationMap[], unsigned char recoded[],
 unsigned int fGenSize, unsigned int locMapSize)
{
	int a, b, c, d;
	int currBit = 0;

	for (a=0; a<locMapSize; a++)
	{
		for (b=0; b<fGenSize; b++)
		{
			if (locationMap[a]==fGen[b][0])
			{
				for (c=2; c<fGen[b][1]+2; c++)
				{
					if (fGen[b][c]==1)
					{
						int digit = 7 - currBit%8;
						unsigned char power = 1;
						for (d=0; d<digit; d++)
						{
							power = power*2;
						}

						recoded[currBit/8] += power;
					}
					
					currBit++;
					if ((currBit/8)>=locMapSize)
					{
						return -1;
					}
				}

				break;
			}
		}
	}

	return currBit;
}

void intToChar (unsigned char charLength[], int length)
{
	int mask = 1<<31;
	int a, b;
	for (a=0; a<32; a++)
	{
		unsigned char numb = length & mask ? 1 : 0;
		length = length<<1;
		if (numb==1)
		{
			int digit = 7 - a%8;
			unsigned char power = 1;
			for (b=0; b<digit; b++)
			{
				power = power*2;
			}

			charLength[a/8] += power;
		}
	}	
}

void bitstreamFiller(unsigned char mainstream[], unsigned char absorbentStream[], int absorbentSize)
{
	static int counter = 0;
	int a; 
	for (a=0; a<absorbentSize; a++)
	{
		mainstream[counter] = absorbentStream[a];
		counter++;
	}
}

unsigned int sizeReader(unsigned char bitstream[], unsigned int* globalCounter)
{
	unsigned int a, b, digit, size=0;
	for (a=*globalCounter; a<*globalCounter+32; a++)
	{
		digit = *globalCounter+31 - a;
		if (bitstream[a]==1)
		{
			int power = 1;
			for(b=0; b<digit; b++)
			{
				power = power*2;
			}

			size += power;
		}
	}
	*globalCounter += 32;

	return size;
}

void streamReader(unsigned char bitstream[], unsigned char stream[], unsigned int* globalCounter, int length)
{
	int a, b, digit, byte;
	for (a=*globalCounter; a<*globalCounter+length; a++)
	{
		digit = 7-((a - *globalCounter)%8);
		byte = (a - *globalCounter)/8;
		if (bitstream[a]==1)
		{
			int power = 1;
			for(b=0; b<digit; b++)
			{
				power = power*2;
			}

			stream[byte] += power;
		}
	}
	if (length%8==0)
	{
		*globalCounter += length;
	}
	else
	{
		*globalCounter += (length/8+1)*8;
	}
}


int main(int argc, char **argv)
{
	FILE *bmpImagePtr;
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	
	//Checking the number of arguments
	if (argc<3 || argc>4)
	{
		printf("Error: Number of arguments is invalid.\n");
		return 1;
	}
	
	//Bitmap Information
	if (strcmp(argv[1], "-i")==0)
	{
		if ((bmpImagePtr = fopen(argv[2], "r")) == NULL)
		{
			printf("File couldn't be opened! Something must be wrong\n");
		}
		else
		{
			if (bmpInfoCheck(bmpImagePtr, 1, &bfh, &bih)==0)
			{
				fclose(bmpImagePtr);
				return 1;
			}
		}
	}
	//-----------------------------------------------------

	//Encoding
	if (strcmp(argv[1], "-e")==0 || strcmp(argv[1], "-ei")==0 || strcmp(argv[1], "-ie")==0)
	{
		//some data should be extracted anyway even until call of the function 
		int showStats = 0;
		if (strcmp(argv[1], "-ei")==0 || strcmp(argv[1], "-ie")==0)
		{
			//+bitmap info
			showStats = 1;
		}

		if ((bmpImagePtr = fopen(argv[2], "r")) == NULL)
		{
			printf("File couldn't be opened! Something must be wrong\n");
			return 1;
		}
		else
		{
			//checking and printing some data
			if (bmpInfoCheck(bmpImagePtr, showStats, &bfh, &bih)==0)
			{
				fclose(bmpImagePtr);
				return 1;
			}
		}
		
		//initialising main variables
		unsigned int a, b, c;
		unsigned int imageSize = bih.biImageSize;
		unsigned int msgSize;
		unsigned int treshold;

		//initialising main arrays
		unsigned char imageBytes[imageSize];
		for (a=0; a<imageSize; a++)
		{
			imageBytes[a] = 0;
		}

		haarWavelet hw[imageSize/2];

		//reading image bytes
		fseek(bmpImagePtr, 1024, SEEK_CUR);
		fread(&imageBytes, imageSize, 1, bmpImagePtr);
		fclose(bmpImagePtr);
		
		//getting our message
		FILE *messagePtr;
		if ((messagePtr = fopen(argv[3], "r")) == NULL)
		{
			printf("Message file couldn't be opened! Something must be wrong\n");
			return 1;
		}
		else
		{
			fseek(messagePtr, 0, SEEK_END);
			msgSize = ftell(messagePtr);		
		}

		unsigned char message[msgSize];
		for (a=0; a<msgSize; a++)
		{
			message[a] = 0;
		}

		fseek(messagePtr, 0, SEEK_SET);
		fread(&message, msgSize, 1, messagePtr);
		fclose(messagePtr);
		
		
		//grouping pixels into pairs and selecting a four sets of them
		for (a=0; a<imageSize/2; a++)
		{
			hw[a].average = (imageBytes[2*a]+imageBytes[2*a+1])/2;
			hw[a].diff = imageBytes[2*a]-imageBytes[2*a+1];

			if (absval(2*hw[a].diff+1) <= minimal(2*(255-hw[a].average), 2*hw[a].average+1) &&
				absval(2*hw[a].diff) <= minimal(2*(255-hw[a].average), 2*hw[a].average+1))
			{
				if (hw[a].diff==0 || hw[a].diff==-1)
				{
					hw[a].group = 'Z';
				}
				else
				{
					hw[a].group = 'E';
				}
			}
			else
			{
				if (absval(2*(divideByTwo(hw[a].diff))+1) <= minimal(2*(255-hw[a].average), 
					 2*hw[a].average+1) &&
					absval(2*(divideByTwo(hw[a].diff))) <= minimal(2*(255-hw[a].average), 
					 2*hw[a].average+1))
				{
					hw[a].group = 'C';
				}
				else
				{
					hw[a].group = 'N';
				}
			}
		}

		//the KEY CYCLE will count the treshold parameter
		treshold = 5;
		unsigned int absLength;
		unsigned int lsbAmount;
		unsigned int numberOfElements;
		unsigned char locationMap[imageSize/16+1];
		unsigned int byteSize;

		do
		{
			//initialising variables for key cycle with null
			lsbAmount = 0;
			absLength = 0;
			numberOfElements = 0;

			//deleting the content from previous iterations
			for (a=0; a<imageSize/16+1; a++)
			{
				locationMap[a] = 0;
			}

			//painting the location map and counting LSBs to save
			lsbAmount = locmapPainter(treshold, locationMap, hw, imageSize/2);

			//creating a linked list of bytes and their frequency in the map
			FREQORDERPTR fqPtr = frequencyList(locationMap, imageSize/16+1);
			FREQORDERPTR endPtr = NULL;
			while (fqPtr!=NULL)
			{
				if (fqPtr->right==NULL)
				{
					endPtr = fqPtr;
				}			
				fqPtr = fqPtr->right;
				numberOfElements++;
			}

			//creating a tree
			FREQORDERPTR binTreeRoot = treeConstruction(endPtr);
			FREQORDERPTR rootCopy = binTreeRoot;

			//creating a table
			unsigned char firstGen[numberOfElements][100];
			for (a=0; a<numberOfElements; a++)
			{
				for (b=0; b<100; b++)
				{
					firstGen[a][b] = 0;
				}
			}

			unsigned int NOE = -1;
			unsigned int depth=0;
			unsigned int maximum = 0;
			unsigned char binCode[98];
			FREQORDERPTR currElem = binTreeRoot;
			preOrder(currElem, firstGen, &NOE, depth, &maximum, binCode);

			//destroying the tree
			delOrder(rootCopy);

			//recoding process
			unsigned char recodedArray[imageSize/16+1];
			for (a=0; a<imageSize/16+1; a++)
			{
				recodedArray[a] = 0;
			}

			unsigned int recSize = recode(firstGen, locationMap, recodedArray,
			 numberOfElements, imageSize/16+1);
			if (recSize<0)
			{
				treshold++;
				continue;
			}

			//table stream size
			byteSize = 0;
			if (maximum%8==0)
			{
				byteSize = maximum/8+2;
			}
			else
			{
				byteSize = maximum/8+1+2;
			}

			//counting absLength
			absLength = 4 + 4 + 4 + recSize/8+1 +byteSize*numberOfElements + 4 + 4 + 
				+ lsbAmount/8+1 + msgSize;
			treshold++;

		}
		while (absLength>imageSize/16);

		//creating the STREAMS and all that stuff
		unsigned char lsbStream[lsbAmount/8+1];
		for (a=0; a<lsbAmount/8+1; a++)
		{
			lsbStream[a] = 0;
		}
		lsbCollector(hw, lsbStream, imageSize/2);

			//creating a linked list of bytes and their frequency in the final version of loc map
			FREQORDERPTR fqPtr = frequencyList(locationMap, imageSize/16+1);
			FREQORDERPTR endPtr = NULL;
			while (fqPtr!=NULL)
			{
				if (fqPtr->right==NULL)
				{
					endPtr = fqPtr;
				}			
				fqPtr = fqPtr->right;
			}

			//creating a final tree
			FREQORDERPTR binTreeRoot = treeConstruction(endPtr);
			FREQORDERPTR rootCopy = binTreeRoot;

			//creating a final table
			unsigned char firstGen[numberOfElements][100];
			for (a=0; a<numberOfElements; a++)
			{
				for (b=0; b<100; b++)
				{
					firstGen[a][b] = 0;
				}
			}

			unsigned int NOE = -1;
			unsigned int depth=0;
			unsigned int maximum = 0;
			unsigned char binCode[98];
			FREQORDERPTR currElem = binTreeRoot;
			preOrder(currElem, firstGen, &NOE, depth, &maximum, binCode);

			//destroying the final tree
			delOrder(rootCopy);

			//recoding process
			unsigned char recodedArray[imageSize/16+1];
			for (a=0; a<imageSize/16+1; a++)
			{
				recodedArray[a] = 0;
			}

			unsigned int recSize = recode(firstGen, locationMap, recodedArray,
			 numberOfElements, imageSize/16+1);

		//creating final mapStream
		unsigned char YCompressedMap[recSize/8+1];
		for (a=0; a<recSize/8+1; a++)
		{
			YCompressedMap[a] = 0;
			YCompressedMap[a] = recodedArray[a];
		}

		//creating final code table stream
		unsigned char YCodeTable[numberOfElements*byteSize];
		for (a=0; a<numberOfElements*byteSize; a++)  
		{
			YCodeTable[a] = 0;
		}

		int currByte = 0;
		for (a=0; a<numberOfElements; a++)
		{
			YCodeTable[currByte] = firstGen[a][0];
			currByte++;
			YCodeTable[currByte] = firstGen[a][1];
			currByte++;
			int currBit = 0;

			int byteBefore = currByte;
			for (b=2; b<firstGen[a][1]+2; b++)
			{
				if (firstGen[a][b]==1)
				{
					int digit = 7 - currBit;
					unsigned char power = 1;
					for (c=0; c<digit; c++)
					{
						power = power*2;
					}

					YCodeTable[currByte] += power;
				}

				if (currBit!=7)
				{
					currBit++;
				}
				else
				{
					currBit = 0;
					currByte++;
				}
			}

			currByte++;
			if (currByte-byteBefore==1)
			{
				currByte += 1;
			}
		}

		int lengthY = 4 + recSize/8+1 + numberOfElements*byteSize;
		unsigned char lengthYChar[4];
		for (a=0; a<4; a++)
		{
			lengthYChar[a] = 0;
		}
		intToChar(lengthYChar, lengthY);

		int lengthC = 4 + lsbAmount/8+1;
		unsigned char lengthCChar[4];
		for (a=0; a<4; a++)
		{
			lengthCChar[a] = 0;
		}
		intToChar(lengthCChar, lengthC);

		unsigned char mainLength[4];
		for (a=0; a<4; a++)
		{
			mainLength[a] = 0;
		}
		intToChar(mainLength, absLength);

		unsigned char recSizeY[4];
		for (a=0; a<4; a++)
		{
			recSizeY[a] = 0;
		}
		intToChar(recSizeY, recSize);

		unsigned char lsbSizeC[4];
		for (a=0; a<4; a++)
		{
			lsbSizeC[a] = 0;
		}
		intToChar(lsbSizeC, lsbAmount);


		//writing all in the FINAL STREAM
		unsigned char MainBitstream[absLength];
		for (a=0; a<absLength; a++)
		{
			MainBitstream[a] = 0;
		}

		bitstreamFiller(MainBitstream, mainLength, 4);
		bitstreamFiller(MainBitstream, lengthYChar, 4);
		bitstreamFiller(MainBitstream, recSizeY, 4);
		bitstreamFiller(MainBitstream, YCompressedMap, recSize/8+1);
		bitstreamFiller(MainBitstream, YCodeTable, numberOfElements*byteSize);
		bitstreamFiller(MainBitstream, lengthCChar, 4);
		bitstreamFiller(MainBitstream, lsbSizeC, 4);
		bitstreamFiller(MainBitstream, lsbStream, lsbAmount/8+1);
		bitstreamFiller(MainBitstream, message, msgSize);
		
		//embedding the data
		int num, digit;
		unsigned char mask = 1<<7, value, bit;

		for (a=0; a<absLength*8; a++)
		{
			num = a/8;
			
			digit = a - num*8;

			value = MainBitstream[num];
			for (b=0; b<digit; b++)
			{
				value = value<<1;
			}
			bit = value & mask ? 1 : 0;
			
			if (hw[a].finalGroup == 'X')
			{
				hw[a].diff = hw[a].diff*2 + bit;
			}
			else
			{
				if(hw[a].finalGroup == 'C' && hw[a].group != 'N')
				{
					hw[a].diff = 2*(divideByTwo(hw[a].diff)) + bit;
				}
			}
		}

		//transforming into pixels
		for (a=0; a<imageSize/2; a++)
		{
			imageBytes[a*2] = hw[a].average + divideByTwo(hw[a].diff+1);
			imageBytes[a*2+1] = hw[a].average - divideByTwo(hw[a].diff);
		}

		//creating palette
		unsigned char palette [1024];
		for (a=0; a<256; a++)
		{
			palette[a*4] = a;
			palette[a*4+1] = a;
			palette[a*4+2] = a;
			palette[a*4+3] = 0;
		}

		//recording
		FILE *newBmpPtr;

		if ((newBmpPtr = fopen("new.bmp", "w")) == NULL)
		{
			printf("File couldn't be opened.\n");
		}
		else
		{
			fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, newBmpPtr);
			fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, newBmpPtr);
			
			for (a=0; a<1024; a++)
			{
				fwrite(&palette[a], sizeof(char), 1, newBmpPtr);
			}

			for (a=0; a<imageSize; a++)
			{
				fwrite(&imageBytes[a], sizeof(char), 1, newBmpPtr);
			}
		}

		fclose(newBmpPtr);

		return 1;
	}
	//-----------------------------------------------------

	
	//Decoding
	if (strcmp(argv[1], "-d")==0 || strcmp(argv[1], "-di")==0 || strcmp(argv[1], "-id")==0)
	{
		int showStats = 0;
		if (strcmp(argv[1], "-di")==0 || strcmp(argv[1], "-id")==0)
		{
			//+bitmap info
			showStats = 1;
		}

		if ((bmpImagePtr = fopen(argv[2], "r")) == NULL)
		{
			printf("File couldn't be opened! Something must be wrong\n");
			return 1;
		}
		else
		{
			//checking and printing some data
			if (bmpInfoCheck(bmpImagePtr, showStats, &bfh, &bih)==0)
			{
				fclose(bmpImagePtr);
				return 1;
			}
		}

		//initialising main variables
		unsigned int a, b, c;
		unsigned int imageSize = bih.biImageSize;

		unsigned char imageBytes[imageSize];
		for (a=0; a<imageSize; a++)
		{
			imageBytes[a] = 0;
		}

		markingStruct ms[imageSize/2];

		fseek(bmpImagePtr, 1024, SEEK_CUR);
		fread(&imageBytes, imageSize, 1, bmpImagePtr);
		fclose(bmpImagePtr);

		unsigned char bitstream[imageSize/2];
		for (a=0; a<imageSize/2; a++)
		{
			bitstream[a] = 0;
		}

		//filling the bitstream
		for (a=0; a<imageSize/2; a++)
		{
			short mask = 1;
			ms[a].average = 0;
			ms[a].diff = 0;
			ms[a].average = (imageBytes[2*a]+imageBytes[2*a+1])/2;
			ms[a].diff = imageBytes[2*a]-imageBytes[2*a+1];

			if (absval(2*(divideByTwo(ms[a].diff))+1) <= minimal(2*(255-ms[a].average), 2*ms[a].average+1) &&
					absval(2*(divideByTwo(ms[a].diff))) <= minimal(2*(255-ms[a].average), 2*ms[a].average+1))
			{
				ms[a].status = 'C';
				bitstream[a] = ms[a].diff & mask ? 1 : 0;
			}
			else
			{
				ms[a].status = 'N';
			}
		}

		//extracting all the components
		unsigned int gc = 0;
		unsigned int streamSize = sizeReader(bitstream, &gc);
		unsigned int YSize = sizeReader(bitstream, &gc);
		unsigned int compressedMapSize = sizeReader(bitstream, &gc);

		unsigned char compressedMap[compressedMapSize/8+1];
		for (a=0; a<compressedMapSize/8+1; a++)
		{
			compressedMap[a] = 0;
		}
		streamReader(bitstream, compressedMap, &gc, compressedMapSize);

		unsigned int tableSize = YSize - (compressedMapSize/8+1) - 4;
		unsigned char codeTable[tableSize];
		for (a=0; a<tableSize; a++)
		{
			codeTable[a] = 0;
		}
		streamReader(bitstream, codeTable, &gc, tableSize*8);

		unsigned int CSize = sizeReader(bitstream, &gc);
		unsigned int lsbSize = sizeReader(bitstream, &gc);
		
		unsigned char lsbStream[lsbSize/8+1];
		for (a=0; a<lsbSize/8+1; a++)
		{
			lsbStream[a] = 0;
		}
		streamReader(bitstream, lsbStream, &gc, lsbSize);

		unsigned int msgSize = streamSize - 4 - 4 - YSize - 4 - CSize;
		unsigned char message[msgSize];
		for (a=0; a<msgSize; a++)
		{
			message[a] = 0;
		}
		streamReader(bitstream, message, &gc, msgSize*8);

		//counting the byteSize
		unsigned int byteSize;
		if (codeTable[1]%8==0)
		{
			byteSize = codeTable[1]/8 + 2;
		}
		else
		{
			byteSize = codeTable[1]/8+1 + 2;
		}

		while (codeTable[byteSize]==0)
		{
			byteSize++;
		}

		//making codeTable easier to use
		unsigned int numberOfElements = tableSize/byteSize;
		unsigned char codes[numberOfElements][byteSize];
		for (a=0; a<numberOfElements; a++)
		{
			for (b=0; b<byteSize; b++)
			{
				codes[a][b] = 0;
				codes[a][b] = codeTable[a*byteSize+b];
			}
		}

		//DECODING the map
		unsigned char locMap[imageSize/16+1];
		for (a=0; a<imageSize/16+1; a++)
		{
			locMap[a] = 0;
		}

		unsigned int locMapByte = 0;
		
		a = 0;
		unsigned char ucmask = 1<<7;

		while (a<compressedMapSize)
		{
			int comprMapByteNum = a/8;
			int comprMapBitNum = a%8;
			int value = compressedMap[comprMapByteNum];
			for (b=0; b<comprMapBitNum; b++)
			{
				value = value<<1;
			}
			int comprBit = ucmask & value ? 1 : 0;
			
			for (b=0; b<numberOfElements; b++)
			{
				int weGotIt = 0;

				int tableLength = codes[b][1];
				int colon = 2;
				int tableValue = codes[b][colon];
				int tableBit = tableValue & ucmask ? 1 : 0;

				if (comprBit == tableBit)
				{
					if (tableLength==1)
					{
						//WRITING
						locMap[locMapByte] = codes[b][0];
						locMapByte++;

						a+=tableLength;
						weGotIt = 1;
					}
					else
					{
						int cmbyteNum = comprMapByteNum;
						int cmbitNum = comprMapBitNum;
						int cmValue = value;
						int cmBit;

						int tBitNum = 0;

						for (c=1; c<tableLength; c++)
						{
							cmbitNum++;
							tBitNum++;

							if (cmbitNum==8)
							{
								cmbitNum = 0;
								cmbyteNum++;
								cmValue = compressedMap[cmbyteNum];
							}
							else
							{
								cmValue = cmValue<<1;
							}

							if (tBitNum==8)
							{
								tBitNum = 0;
								colon++;
								tableValue = codes[b][colon];
							}
							else
							{
								tableValue = tableValue<<1;
							}

							cmBit = cmValue & ucmask ? 1 : 0;
							tableBit = tableValue & ucmask ? 1 : 0;
							
							if (cmBit == tableBit)
							{
								if (c==tableLength-1)
								{
									//WRITING
									locMap[locMapByte] = codes[b][0];
									locMapByte++;

									a+=tableLength;
									weGotIt = 1;
								}
							}
							else
							{
								break;
							}
						}
					}
				}

				if (weGotIt==1)
				{
					break;
				}
			}
		}

		//restoring the difference expansions
		int lsbit = 0;
		unsigned char lsbyteValue;
		int lsb;

		for (a=0; a<streamSize*8; a++)
		{
			unsigned char mask = 1<<7;
			unsigned char curByteValue;
			if (a%8==0)
			{
				curByteValue = locMap[a/8];
			}
			else
			{
				curByteValue = curByteValue<<1;
			}
			int value = mask & curByteValue ? 1 : 0;
			
			if (value==1)
			{
				ms[a].diff = divideByTwo(ms[a].diff);
			}
			else
			{
				if (ms[a].diff==0 || ms[a].diff==1)
				{
					ms[a].diff = 1;
					continue;
				}

				if (ms[a].diff==-2 || ms[a].diff==-1)
				{
					ms[a].diff = -2;
					continue;
				}

				if (lsbit%8==0)
				{
					lsbyteValue = lsbStream[lsbit/8];
				}
				else
				{
					lsbyteValue = lsbyteValue<<1;
				}
				lsb = mask & lsbyteValue ? 1 : 0;

				ms[a].diff = 2*divideByTwo(ms[a].diff) + lsb;
				lsbit++;
			}
		}

		//turning DE into pixel values
		for (a=0; a<imageSize/2; a++)
		{
			imageBytes[a*2] = ms[a].average + divideByTwo(ms[a].diff+1);
			imageBytes[a*2+1] = ms[a].average - divideByTwo(ms[a].diff);
		}

		//creating palette
		unsigned char palette [1024];
		for (a=0; a<256; a++)
		{
			palette[a*4] = a;
			palette[a*4+1] = a;
			palette[a*4+2] = a;
			palette[a*4+3] = 0;
		}

		//recording image
		FILE *newBmpPtr;

		if ((newBmpPtr = fopen("Restored.bmp", "w")) == NULL)
		{
			printf("File couldn't be opened.\n");
		}
		else
		{
			fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, newBmpPtr);
			fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, newBmpPtr);
			
			for (a=0; a<1024; a++)
			{
				fwrite(&palette[a], sizeof(char), 1, newBmpPtr);
			}

			for (a=0; a<imageSize; a++)
			{
				fwrite(&imageBytes[a], sizeof(char), 1, newBmpPtr);
			}
		}

		fclose(newBmpPtr);

		//recording message
		FILE *neuTxtPtr;

		if ((neuTxtPtr = fopen("Message.txt", "w")) == NULL)
		{
			printf("File couldn't be opened.\n");
		}
		else
		{
			for (a=1; a<msgSize; a++)
			{
				fwrite(&message[a], sizeof(char), 1, neuTxtPtr);
			}
		}

		fclose(neuTxtPtr);

		//return 1;
	}
	//-----------------------------------------------------
	return 1;
}
