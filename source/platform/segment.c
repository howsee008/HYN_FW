//*****************************************************************************
// File Name:	Segment.c
//
// Description:	The source file of segment module, which segment the signal map
//              into object.
//
// Created by:	Sea Hou, Feb. 2020
// Revised by:	Rev.1, Sea Hou, Feb. 2020
//
// Copyright 2020 Hynitron, All Rights Reserved.
//*****************************************************************************
#include "segment.h"


//=============================================================================
// Macro Definition
//=============================================================================

#define SEGCONTOURMAX (100)

//=============================================================================
// Local Data Type Definition
//=============================================================================
 typedef struct
 {
	segmentLabel_t * headP;
	segmentLabel_t * tailP;
	uint16 hookcount;
 }hook_t;

typedef struct
{
  uint16 row: 8;
  uint16 col: 8;
}segnode_t;

//=============================================================================
// Local (Static) Variables Definition
//=============================================================================
static segPublic_t segPublicinfo;
static segmentLabelImage_t segLabelImage;
static segmentLabel_t seghooks[SEGHOOKSIZE];
static segmentLabel_t * segObjects;
static segmentLabel_t * segBackground;
static segmentLabel_t * segUnSegmented;
static uint16 segObjBitmask;
//static segmentLabel_t * segDam;
//static segmentLabel_t * segMinb;

static uint8p8 segCentroidRow[CFG_MAX_OBJECTS];
static uint8p8 segCentroidCol[CFG_MAX_OBJECTS];
static int32 segSigSum[CFG_MAX_OBJECTS];


static int16 seg_cfg_maxSig;
static int16 seg_cfg_minSig;
static int16 seg_cfg_minPeak;
static int16 seg_cfg_mergeCoef;
static int16 seg_cfg_contourRes;

static uint16 seg_contourBin;
static uint16 seg_minpeakContourLevel;
static uint16 segLable;
static uint16 segBigSegCount;

//=============================================================================
// Local (Static) Functions Declaration
//=============================================================================
//static segmentLabel_ptr segmentation_searchRoi(touchImage_t* imgarray, hook_t* seghook);
static void segmentation_resetReport(void);
static uint16 segmentation_sortbyContour(touchImage_t* imgarray, hook_t* seghook, hook_t* contour);
static void segmentation_fillFlood(segnode_t * seedNode, uint16 lableId,segmentLabel_t * src, hook_t * segments, uint16 option );
static void segmentation_resetHooks(hook_t* hook);
static int16 segmentation_pushQueue(hook_t* hooks,  uint16 hookid, segnode_t* node);
static int16 segmentation_removeQueue(hook_t* hooks,uint16 hookid,segnode_t* node );
static int16 segmentation_popQueue(hook_t* hooks,uint16 hookid,segnode_t* node );
static int16 segmentation_getNeighbourPosition(uint16 neighbour, segnode_t* curNode, segnode_t* nbNode);
static int16 segmentation_searchBlobByWatershed(hook_t* contour,hook_t* segments);
static int16 segmentation_mergeblobs(touchImage_t* imgarray,int16* conTable,hook_t* seghook);
static uint16 segmentation_calcMergeCoef(uint16 i, uint16 j,uint16 damsum);
static void segmentation_doMergeblobs(uint16 merged, uint16 killed,hook_t* seghook);
static void segmentation_calcSigsumAndCentroid(touchImage_t* imgarray,uint16 i);
static void segmentation_sortDilatedRoiToSegments(touchImage_t* img, hook_t* seghook);

//static void segmentation_dilateRoi(hook_t* roi);

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
//=============================================================================
// Local (Static) Functions Definition
//=============================================================================
//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static void segmentation_resetHooks(hook_t* hook)
{
  uint16 len;
  len=sizeof(segmentLabel_t)*hook->hookcount;
  memset(hook->headP,0xff,len);
  memset(hook->tailP,0xff,len);
}
//-----------------------------------------------------------------------------
// Function Name:	segmenting_resetPubinfo
// Description:		reset segment result buffer
//-----------------------------------------------------------------------------
static void segmentation_resetReport(void)
{

	memset((void*)&segLabelImage.buffer[0],0xff,sizeof(segLabelImage.buffer));
    memset((void*)&seghooks[0],0xff,sizeof(seghooks));
	segLable=0;
	segBigSegCount =0;
	segObjBitmask =0;
}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static int16 segmentation_pushQueue(hook_t* hooks,  uint16 hookid, segnode_t* node)
{
  segmentLabel_ptr head, tail;

  if (hookid>=hooks->hookcount)
  	return RES_ERROR0;
 
  head=&(hooks->headP[hookid]);
  tail=&(hooks->tailP[hookid]);
  segLabelImage.matrix[node->row][node->col].feilds.nextRow=END_IDX;
  segLabelImage.matrix[node->row][node->col].feilds.nextCol=END_IDX;
  
  if(RESET_VAL!=head->value)
  {
   uint8 hookrow, hookcol;

   hookrow=tail->feilds.nextRow;   
   hookcol=tail->feilds.nextCol;
    
   segLabelImage.matrix[hookrow][hookcol].feilds.nextRow=node->row;
   segLabelImage.matrix[hookrow][hookcol].feilds.nextCol=node->col;

   tail->feilds.nextRow = node->row;
   tail->feilds.nextCol = node->col;
	
  }
  else
  {
    head->feilds.nextRow=node->row;
	head->feilds.nextCol=node->col;
	
	tail->value = head->value;
	
  }
  

  return RES_OK;
}


//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static int16 segmentation_removeQueue(hook_t* hooks,uint16 hookid,segnode_t* node )
{
  int16 res;
  uint8 nextrow, nextcol;
  uint8 nowrow, nowcol;
  segmentLabel_ptr head;
  
  if (hookid>=hooks->hookcount) return RES_ERROR0;

  head = &(hooks->headP[hookid]);
  
  nextrow = head->feilds.nextRow;
  nextcol = head->feilds.nextCol;
  nowrow=nextrow;
  nowcol=nextcol;

  res =RES_ERROR0;

  if(head->feilds.nextRow==node->row && head->feilds.nextCol==node->col )
  {
	  head->feilds.nextRow = segLabelImage.matrix[node->row][node->col].feilds.nextRow;
	  head->feilds.nextCol = segLabelImage.matrix[node->row][node->col].feilds.nextCol;
	  segLabelImage.matrix[node->row][node->col].feilds.nextRow=END_IDX;
	  segLabelImage.matrix[node->row][node->col].feilds.nextCol=END_IDX;
	  res=RES_OK;
  }
  else
  {
	  while(!(nextrow==0x3F && nextcol==0x3F))
	  {
		 if(nextrow==node->row && nextcol==node->col)
		 {
			
			segLabelImage.matrix[nowrow][nowcol].feilds.nextRow=segLabelImage.matrix[node->row][node->col].feilds.nextRow;
			segLabelImage.matrix[nowrow][nowcol].feilds.nextCol=segLabelImage.matrix[node->row][node->col].feilds.nextCol;
			segLabelImage.matrix[node->row][node->col].feilds.nextRow=END_IDX;
			segLabelImage.matrix[node->row][node->col].feilds.nextCol=END_IDX;
	  
			res=RES_OK;
			break;
			
		 }
		 else
		 {
			nowrow=nextrow;
			nowcol=nextcol;
			nextrow=segLabelImage.matrix[nowrow][nowcol].feilds.nextRow;
			nextcol=segLabelImage.matrix[nowrow][nowcol].feilds.nextCol;
		 }
	  
	  }

  }
  return res;

}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static int16 segmentation_popQueue(hook_t* hooks,uint16 hookid,segnode_t* node )
{
	segmentLabel_ptr head;
	int16 res=RES_OK;
	
	 if (hookid>=hooks->hookcount)
	 {
	 	node->row=END_IDX;
		node->col=END_IDX;
	 	return RES_ERROR0;
	 }
	
	 head=&(hooks->headP[hookid]);

     
	 if(!(head->feilds.nextRow==END_IDX&&head->feilds.nextCol==END_IDX))
	 {
	 	uint8 row, col;
		row = head->feilds.nextRow;
		col = head->feilds.nextCol;
        node->row = row;
		node->col = col;
		head->feilds.nextRow=segLabelImage.matrix[row][col].feilds.nextRow;
		head->feilds.nextCol=segLabelImage.matrix[row][col].feilds.nextCol;

		segLabelImage.matrix[row][col].feilds.nextRow=END_IDX;
		segLabelImage.matrix[row][col].feilds.nextCol=END_IDX;

		
	 }
	 else
	 {
	 	res=RES_ERROR0;
	 	node->row=END_IDX;
		node->col=END_IDX;
		
	 }

	 return res;
}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static int16 segmentation_getNeighbourPosition(uint16 neighbour, segnode_t* curNode, segnode_t* nbNode)
{
  int16 res;
   res=RES_ERROR0;
   switch(neighbour)
   {
	 case 0: // it is left nb
	 {
	 	if(curNode->col>0)
	 	{
	   		nbNode->row=curNode->row;
	   		nbNode->col=curNode->col-1;
	   		res=RES_OK;
	 	}
	 }
	 break;
	 case 1:// it is right
	 {
		 if(curNode->col<CFG_NUM_2D_COLS-1)
		 {
		   nbNode->row=curNode->row;
		   nbNode->col=curNode->col+1;
		   res=RES_OK;
		 }
	 }

	 break;
	 case 2:// it is top	 
	 {
		 if(curNode->row>0)
		 {
		   nbNode->row=curNode->row-1;
		   nbNode->col=curNode->col;
		   res=RES_OK;
		 }
	 }

	 break;
	 case 3: // it is bottom	 
	 {
		 if(curNode->row<CFG_NUM_2D_ROWS-1)
		 {
		   nbNode->row=curNode->row+1;
		   nbNode->col=curNode->col;
		   res=RES_OK;
		 }
	 }

	 break;
	 default:
	 {
		res=RES_ERROR0;
	 }
	 	

   }
 return res;
}


//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static void segmentation_fillFlood(segnode_t * seedNode, uint16 lableId,segmentLabel_t * src, hook_t * segments, uint16 option )
{

	segnode_t cur,nb;
	segmentLabel_t fifoHead, fifoTail, srcTail;

	hook_t srcHook, fifo;

	srcHook.headP=src;
	srcHook.tailP=&srcTail;
	srcHook.hookcount=1;

	fifo.headP=&fifoHead;
	fifo.tailP=&fifoTail;
	fifo.hookcount=1;

	segmentation_resetHooks(&fifo);

	cur.row = seedNode->row;
	cur.col = seedNode->col;

	while(!(END_IDX==cur.row && END_IDX==cur.col))
	{
		uint16 nbloop;
		// check the poped data
		segLabelImage.matrix[cur.row][cur.col].feilds.lable=lableId;
		segmentation_pushQueue(segments,lableId,&cur);

		// search 4 neighbours of the data
		for(nbloop=0;nbloop<4;nbloop++)
		{

		  if(RES_OK==segmentation_getNeighbourPosition(nbloop,&cur,&nb))
		  {

			  
			  switch(option)
			  {
				case 0:
					break;
				case 1:
				{
					if(segLabelImage.matrix[nb.row][nb.col].feilds.lable!=OBJ_MINB)
					{
						continue;
					}
				}break;
				default:
					continue;

			  }
			  if(RES_OK==segmentation_removeQueue(&srcHook,0,&nb))
			  {
				  segmentation_pushQueue(&fifo,0,&nb);
			  }
			  


		  }

		}
				
		
		segmentation_popQueue(&fifo,0,&cur);

	}
}
//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
static void segmentation_sortDilatedRoiToSegments(touchImage_t* img, hook_t* seghook)
{
	segnode_t cur,nb;
	uint16 lab,nbId;

	segmentation_popQueue(seghook, SEGHOOKID_DIA, &cur);

	while(!(cur.row==END_IDX&&cur.col==END_IDX))
	{
		for(nbId=0;nbId<4;nbId++)
		{
			if(RES_OK==segmentation_getNeighbourPosition(nbId,&cur,&nb))
			{
				lab=segLabelImage.matrix[nb.row][nb.col].feilds.lable;
				if(lab<CFG_MAX_OBJECTS)
				{
					break;
				}
					
			}
		}
		
		if(lab<CFG_MAX_OBJECTS)
		{
			segLabelImage.matrix[cur.row][cur.col].feilds.lable=lab;
			segmentation_pushQueue(seghook, lab, &cur);
		}
		else
		{
			segmentation_pushQueue(seghook, SEGHOOKID_BCKGND, &cur);
		}
		segmentation_popQueue(seghook, SEGHOOKID_DIA, &cur);
	}

}

//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
static uint16 segmentation_getFreeLableID(void)
{
	uint16 res;
	if(segLable<CFG_MAX_OBJECTS)
	{
		res=segLable;
		
	}
	else
	{
		res=SEGHOOKID_UNSEG;
	}
	segLable ++;
	return res;
}

//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
static int16 segmentation_searchBlobByWatershed(hook_t* contour,hook_t* segments)
{
  int16 labeid =0;
  uint16 nbId;
  segnode_t cur, next,nb; 
  int16 loop;
  segmentLabel_t fifo_head, fifo_tail;
  hook_t fifo;

  // create a fifo
  fifo.headP=&fifo_head;
  fifo.tailP=&fifo_tail;
  fifo.hookcount=1;
  segmentation_resetHooks(&fifo);

  // watershed
  for(loop=seg_cfg_contourRes;loop>=0;loop--)
  {

    if(contour->headP[loop].value==RESET_VAL) 
		continue;
	
	// search each level of the contour and flood fill the same label nodes.
	// step1, give a target level a specail label "OBJ_MASK".
	// here "pop" can't be used because the node needn't to be removed from the list.just label it.
	cur.row=contour->headP[loop].feilds.nextRow;
	cur.col=contour->headP[loop].feilds.nextCol;
	while(!(cur.row==END_IDX&&cur.col==END_IDX))
	{
        next.row=segLabelImage.matrix[cur.row][cur.col].feilds.nextRow;
		next.col=segLabelImage.matrix[cur.row][cur.col].feilds.nextCol;
		
		segLabelImage.matrix[cur.row][cur.col].feilds.lable=OBJ_MASK;

		for(nbId=0;nbId<4;nbId++)
		{
			if(RES_OK==segmentation_getNeighbourPosition(nbId,&cur,&nb))
			{
				// if there is a labled node in its neighbour, fifo it as seed of flood-fill
				if(segLabelImage.matrix[nb.row][nb.col].feilds.lable<OBJ_DAM)
				{
					segLabelImage.matrix[cur.row][cur.col].feilds.lable=OBJ_INQE;
					segmentation_removeQueue(contour,loop,&cur );
					segmentation_pushQueue(&fifo,  0, &cur);
					break;
					
				}
			}
		}
		cur.row=next.row;
		cur.col=next.col;
	}

	// step2, flood-fill the nodes with labled neighbour.
	// the flood-fill expands nodes circle by circle from the seeds to its neighbour.
	segmentation_popQueue(&fifo,0,&cur);
	while(!(cur.row==END_IDX&&cur.col==END_IDX))
	{
		uint16 curLab, nbLab, maskedCount;
		segnode_t maskednb[4];

		maskedCount =0;
		for(nbId=0;nbId<4;nbId++)
		{
			if(RES_OK==segmentation_getNeighbourPosition(nbId,&cur,&nb))
			{
				// the two variables should be updated each looping
				// because they maybe change
				curLab=segLabelImage.matrix[cur.row][cur.col].feilds.lable;
				nbLab = segLabelImage.matrix[nb.row][nb.col].feilds.lable;

				// the neighbour is object lable.
				if(nbLab<OBJ_DAM)
				{
					if (curLab==OBJ_INQE)
					{
						// cur is not labled. so give the same lable as the nb.
						segLabelImage.matrix[cur.row][cur.col].feilds.lable=nbLab;
					}
					else if ((curLab<OBJ_DAM)&&(curLab!=nbLab))// it is labed as DAM for it was labed early by another neighbour.
					{
					  // cur is labled at previous loop, and it != nb.
					  // so cur must be a dam
					   segLabelImage.matrix[cur.row][cur.col].feilds.lable=OBJ_DAM;
					  // Moreover...
					  // if nb is MINB, there could be other more MINB nodes connected to the nb,but locate at the higher level.
					  // therefore, give the nb a new OBJ lable and the flood-fill all connected MINB node.
					   if(nbLab==OBJ_MINB)
					   {
							labeid=segmentation_getFreeLableID();
							segmentation_removeQueue(contour,loop,&nb);
							segmentation_fillFlood(&nb,labeid,&seghooks[SEGHOOKID_MINB],segments,1);
					   }
					   
					}
					
				}
				else if(nbLab==OBJ_MASK)
				{
				  // the nb is blank node but in the same contour level. save it and check if it could change to a seed later.
				   maskednb[maskedCount].row=nb.row;
				   maskednb[maskedCount].col=nb.col;
				   maskedCount++;
				}
				
			}

		}
		// so far, the cur node has a decided lable.
		// push current node to the respective hook according to its lable.
		curLab=segLabelImage.matrix[cur.row][cur.col].feilds.lable;
		if((curLab<OBJ_MINB)||(curLab==OBJ_DAM))
		{
			segmentation_pushQueue(segments,  curLab, &cur);
		}
		
		// current node is a object label. the saved neighbour must be a new seed.  fifo it.
		if(curLab<OBJ_MINB)
		{
		  for(nbId=0;nbId<maskedCount;nbId++)
		  {
			  segLabelImage.matrix[maskednb[nbId].row][maskednb[nbId].col].feilds.lable=OBJ_INQE;
			  if(RES_OK==segmentation_removeQueue(contour,loop,&maskednb[nbId]))
			  {
				segmentation_pushQueue(&fifo,  0, &maskednb[nbId]);
			  }
			  
		  }
		}
	
		segmentation_popQueue(&fifo,0,&cur );
	}

	// step3, flood-fill the isolated nodes
	cur.row=contour->headP[loop].feilds.nextRow;
	cur.col=contour->headP[loop].feilds.nextCol;
	segmentation_resetHooks(&fifo);
	if(cur.row!=END_IDX || cur.col!=END_IDX)
	{
		// if the node < minpeak, just label it as MINB and flood fill it.
		if(loop<=seg_minpeakContourLevel)
		{
			segmentation_popQueue(contour,loop,&cur);
			while(!(cur.row==END_IDX&&cur.col==END_IDX))
			{
				segmentation_fillFlood(&cur,OBJ_MINB,&contour->headP[loop],segments,0);
				segmentation_popQueue(contour,loop,&cur);
			}
			
		}
		// if the node > minpeak, give it a new lable and flood fill it
		else
		{
			
			segmentation_popQueue(contour,loop,&cur);
			while(!(cur.row==END_IDX&&cur.col==END_IDX))
			{
				labeid=segmentation_getFreeLableID();
				segBigSegCount=labeid;
				segmentation_fillFlood(&cur,labeid,&contour->headP[loop],segments,0);
				segmentation_popQueue(contour,loop,&cur);
			}
		}

	}

	

  }

  return ((labeid<CFG_MAX_OBJECTS)?labeid+1:labeid);
  
}
//-----------------------------------------------------------------------------
// Function Name:	segmentation_sortbyContour
// Description:		contour sorting unsegment node. if the node>seg_cfg_minSig
//  rasie its contour level by 1
//-----------------------------------------------------------------------------
static uint16 segmentation_sortbyContour(touchImage_t* imgarray, hook_t* seghook, hook_t* contour)
{
	uint16 n;
	segnode_t cur,nb,nxt;
	uint8 row, col;


 // step1 : contour ROI 
	for(row=0;row<CFG_NUM_2D_ROWS;row++)
	{
		
		for(col=0;col<CFG_NUM_2D_COLS;col++)
		{
			int16 sig;
						
			sig=imgarray->matrix[row][col];

			if(sig>seg_cfg_minSig)
			{
				segLabelImage.matrix[row][col].feilds.lable=OBJ_INIT;
				
				n=((sig-seg_cfg_minSig)+(seg_contourBin>>1))/seg_contourBin;
				
				if(n>seg_cfg_contourRes) n=seg_cfg_contourRes;

				cur.row=row;
				cur.col=col;
				segmentation_pushQueue(contour, n, &cur);
			}
			
			
		}

	}
	// at minpeak level, there are some sig bigger than minpeak, push it to the above level.
	if(seg_minpeakContourLevel<seg_cfg_contourRes-1)
	{
		hook_t unsegmentHook;
		segmentLabel_t unseghead, unsegtail;
		// put minpeak contour to a new hook/ fifo
		unseghead = contour->headP[seg_minpeakContourLevel];
		
		unsegmentHook.headP=&unseghead;
		unsegmentHook.tailP=&unsegtail;
		unsegmentHook.hookcount=1;
		
		// reset minpeak level 
		contour->headP[seg_minpeakContourLevel].value=RESET_VAL;
		contour->tailP[seg_minpeakContourLevel].value=RESET_VAL;
		// pop each of the fifo and reassign it to the contour
		segmentation_popQueue(&unsegmentHook, 0, &cur);
		while(!(cur.row==END_IDX && cur.col==END_IDX))
		{
			int16 sig;
			sig=imgarray->matrix[cur.row][cur.col];
		
			if(sig>seg_cfg_minPeak)
			{
				segmentation_pushQueue(contour, seg_minpeakContourLevel+1, &cur);
			}
			else
			{
				segmentation_pushQueue(contour, seg_minpeakContourLevel, &cur);
			}
		
			segmentation_popQueue(&unsegmentHook, 0, &cur);
		}
	}

	// step2: dial contour data
	for(n=0;n<contour->hookcount;n++)
	{
		cur.row=contour->headP[n].feilds.nextRow;
		cur.col=contour->headP[n].feilds.nextCol;

		while(!(cur.row==END_IDX&&cur.col==END_IDX))
		{
	  		uint16 nbId;
	  		for(nbId=0;nbId<4;nbId++)
	  		{
				if(RES_OK==segmentation_getNeighbourPosition(nbId,&cur,&nb))
				{
					if(segLabelImage.matrix[nb.row][nb.col].value==RESET_VAL)
					{
						segLabelImage.matrix[nb.row][nb.col].feilds.lable=OBJ_INIT;
						segmentation_pushQueue(seghook,SEGHOOKID_DIA,&nb);
					}
				}
	  		}

	  
	  		nxt.row=segLabelImage.matrix[cur.row][cur.col].feilds.nextRow;
	  		nxt.col=segLabelImage.matrix[cur.row][cur.col].feilds.nextCol;

	  		cur=nxt;
	
	
  		}
	}

	//step3: collect the background nodes
	for(row=0;row<CFG_NUM_2D_ROWS;row++)
	{
		for(col=0;col<CFG_NUM_2D_COLS;col++)
		{
			if(segLabelImage.matrix[row][col].value==RESET_VAL)
			{
				cur.col=col;
				cur.row=row;
				segmentation_pushQueue(seghook,SEGHOOKID_BCKGND, &cur);
			}
		}
	}

	return RES_OK;
}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static void segmentation_calcSigsumAndCentroid(touchImage_t* imgarray,uint16 i)
{
	int32 sigSum, centroidRow, centroidCol;
	segnode_t curnode,nextnode;

	curnode.row=segObjects[i].feilds.nextRow;
	curnode.col=segObjects[i].feilds.nextCol;
	sigSum=0;
	centroidCol=0;
	centroidRow=0;
	while(!(curnode.row==END_IDX&&curnode.col==END_IDX))
	{
		sigSum+=imgarray->matrix[curnode.row][curnode.col];
		centroidRow+=curnode.row*imgarray->matrix[curnode.row][curnode.col];
		centroidCol+=curnode.col*imgarray->matrix[curnode.row][curnode.col];

		nextnode.row=segLabelImage.matrix[curnode.row][curnode.col].feilds.nextRow;
		nextnode.col=segLabelImage.matrix[curnode.row][curnode.col].feilds.nextCol;

		curnode=nextnode;
	}

	segSigSum[i]=sigSum;
	segCentroidCol[i]=(uint8p8)((centroidCol<<8)/sigSum);
	segCentroidRow[i]=(uint8p8)((centroidRow<<8)/sigSum);

}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static uint16 segmentation_calcMergeCoef(uint16 i, uint16 j,uint16 damsum)
{
	uint8p8 deltaRow, deltaCol;
	uint32 coef;

	if(0==damsum)
		return 0xffff;
	deltaRow=(segCentroidRow[i]>segCentroidRow[j])?(segCentroidRow[i]-segCentroidRow[j]):(segCentroidRow[j]-segCentroidRow[i]);
	deltaCol=(segCentroidCol[i]>segCentroidCol[j])?(segCentroidCol[i]-segCentroidCol[j]):(segCentroidCol[j]-segCentroidCol[i]);

	coef=(deltaRow*deltaRow+deltaCol*deltaCol)>>16;

	coef*=((segSigSum[i]/damsum)+(segSigSum[j]/damsum));

	coef =(coef>0xffff)?0xfffe:coef; // 0xffff is error flag. it can't be set
	
	return coef;
}
//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static void segmentation_doMergeblobs(uint16 merged, uint16 killed,hook_t* seghook)
{
	segnode_t curnode,nextnode,tailnode;
	
	curnode.row=seghook->headP[killed].feilds.nextRow;
	curnode.col=seghook->headP[killed].feilds.nextCol;
	
	while(!(curnode.row==END_IDX&&curnode.col==END_IDX))
	{

		segLabelImage.matrix[curnode.row][curnode.col].feilds.lable=merged;
		nextnode.row=segLabelImage.matrix[curnode.row][curnode.col].feilds.nextRow;
		nextnode.col=segLabelImage.matrix[curnode.row][curnode.col].feilds.nextCol;
		curnode=nextnode;
	}

	//segmentation_pushQueue(seghook, merged, &curnode);
	tailnode.row=seghook->tailP[merged].feilds.nextRow;
	tailnode.col=seghook->tailP[merged].feilds.nextCol;
	segLabelImage.matrix[tailnode.row][tailnode.col].feilds.nextRow=seghook->headP[killed].feilds.nextRow;
	segLabelImage.matrix[tailnode.row][tailnode.col].feilds.nextCol=seghook->headP[killed].feilds.nextCol;
	
	seghook->tailP[merged].value=seghook->tailP[killed].value;
	tailnode.row=seghook->tailP[merged].feilds.nextRow;
	tailnode.col=seghook->tailP[merged].feilds.nextCol;
	segLabelImage.matrix[tailnode.row][tailnode.col].feilds.nextRow=END_IDX;
	segLabelImage.matrix[tailnode.row][tailnode.col].feilds.nextCol=END_IDX;
	
	
	segObjects[killed].value=RESET_VAL;

}


//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static int16 segmentation_mergeblobs(touchImage_t* imgarray,int16* conTable,hook_t* seghook)
{
	int16 (*damTab)[10]; // when col>row, contain sum of dam, when col<row, contain merge coef
	uint16 newCount,totalCount, minCoef;
	segnode_t curDam,nextDam, nb;
	uint16 i,j,nbId,nbLable,maxnbLable;
	uint16 mergedId;
	uint16 goMerge,goKill;
	uint16 damloop;

	damTab=(int16(*)[10])conTable;
	totalCount=(segLable<CFG_MAX_OBJECTS)?segLable:CFG_MAX_OBJECTS;
	newCount = totalCount;
	mergedId = 0xffff;
	damloop=0;
	while(damloop<CFG_NUM_2D_ROWS * CFG_NUM_2D_COLS)
	{
		damloop++;
		// calc Damsum and remove invalid dam
		curDam.col=seghook->headP[SEGHOOKID_DAM].feilds.nextCol;
		curDam.row=seghook->headP[SEGHOOKID_DAM].feilds.nextRow;
		if(curDam.col==END_IDX && curDam.row==END_IDX)
			break;
		memset(conTable,0,SEGCONTOURMAX*sizeof(int16));
		while(!(curDam.col==END_IDX && curDam.row==END_IDX))
		{
			i=j=0xffff; // i,j contain objects label in the neighbour. lable of i <=lable of j
			for(nbId=0;nbId<4;nbId++)
			{
				if(RES_OK==segmentation_getNeighbourPosition(nbId,&curDam,&nb))
				{
					nbLable = segLabelImage.matrix[nb.row][nb.col].feilds.lable;
					if(nbLable<CFG_MAX_OBJECTS)
					{
						if(i==0xffff)
						{
							i=nbLable;
							j=i;
						}
						else if(j!=nbLable)
						{
							if(nbLable>i)
							{
								j=nbLable;
							}
							else
							{
								i=nbLable;
							}

							break;							
						}
						
					}
				}
			}
			// get next node
			nextDam.row = segLabelImage.matrix[curDam.row][curDam.col].feilds.nextRow;
			nextDam.col = segLabelImage.matrix[curDam.row][curDam.col].feilds.nextCol;
			
			if(i!=j)
			{
				// the dam still has two object neighbours.
				damTab[i][j]+=imgarray->matrix[curDam.row][curDam.col];

			}
			else if((i<CFG_MAX_OBJECTS)||(i==0xffff))
			{
			 	// no objects or only one in the neighbour, the dam isn't vaild anymore, 
			 	//so remove it from Dam and push to a obj list according to lable i
				segmentation_removeQueue(seghook, SEGHOOKID_DAM, &curDam);
				segLabelImage.matrix[curDam.row][curDam.col].feilds.lable=i;
				segmentation_pushQueue(seghook, i, &curDam);
				
			}
			// update cur mode
			curDam.row=nextDam.row;
			curDam.col=nextDam.col;
			
		}

		// if there is no dam left, break out;
		if(RESET_VAL==seghook->headP[SEGHOOKID_DAM].value) 
			break;

		// calc segments' sum and position for the object merge calcuation below
		for(i=0;i<totalCount;i++)
		{
		 	if((mergedId==0xffff)||(mergedId==i))
		 	{
				// 0xffff means update all of segments. or only update the merged segment[i]
				// calc sigsum
				segmentation_calcSigsumAndCentroid(imgarray,i);
			}
		}

		// calc coef of all dams and find the minCoef
		minCoef=0xffff;
		for(i=0;i<totalCount;i++)
		{
			for(j=i+1;j<totalCount;j++)
			{
				if(damTab[i][j]!=0)
				{
					uint16 coef;

					coef = segmentation_calcMergeCoef(i, j, damTab[i][j]);
					if(minCoef>coef)
					{
						minCoef=coef;
						goMerge=i;
						goKill=j;
					}
				}
			}
		}


		if((newCount>0)&&(minCoef<seg_cfg_mergeCoef))
		{
			// merge gokill to gomerge
			segmentation_doMergeblobs(goMerge,goKill,seghook);
			mergedId=goMerge;
			newCount-=1;
		}
		else
		{
			// here, there is no Coef < mergeCoef. the while should be breakout.
			if(RESET_VAL!=seghook->headP[SEGHOOKID_DAM].value)
			{
				// assign the left dam to the biggest signal neighbour
				segmentation_popQueue(seghook, SEGHOOKID_DAM, &curDam);
				while(!(curDam.row==END_IDX && curDam.col==END_IDX))
				{
					int16 maxsig=-3000;
					maxnbLable=CFG_MAX_OBJECTS;
					for(nbId=0;nbId<4;nbId++)
					{						
						if(RES_OK==segmentation_getNeighbourPosition(nbId,&curDam,&nb))
						{
							nbLable = segLabelImage.matrix[nb.row][nb.col].feilds.lable;

							if(nbLable<CFG_MAX_OBJECTS)
							{
								if(nbLable>segBigSegCount)
								{
									// oops!, the lable>segBigSegCount, the neighbour must be below minpeak, put it to "background"
									segmentation_doMergeblobs(SEGHOOKID_BCKGND,nbLable,seghook);
									newCount-=1;
								}
								else
								{
									if(maxsig<imgarray->matrix[nb.row][nb.col])
									{
										maxsig = imgarray->matrix[nb.row][nb.col];
										maxnbLable = segLabelImage.matrix[nb.row][nb.col].feilds.lable;
									}
								}
							}
						}
					}
					if(maxnbLable<CFG_MAX_OBJECTS)
					{
						segLabelImage.matrix[curDam.row][curDam.col].feilds.lable=maxnbLable;
						segmentation_pushQueue(seghook, maxnbLable, &curDam);
					}

					segmentation_popQueue(seghook, SEGHOOKID_DAM, &curDam);
					
				}
			}
			break;
		}

	}

	// merge MINB to backgournd
	if(!(seghook->headP[SEGHOOKID_MINB].feilds.nextRow==END_IDX &&seghook->headP[SEGHOOKID_MINB].feilds.nextCol==END_IDX ))
	{
		segmentation_doMergeblobs(SEGHOOKID_BCKGND,SEGHOOKID_MINB,seghook);
	}
	
	return newCount;
}



//=============================================================================
// Global Functions Definition (Declared in segment.h)
//=============================================================================

//-----------------------------------------------------------------------------
// Function Name: segmenting_getpubinfo
// Description:	it is a API to get segment result.	
//-----------------------------------------------------------------------------
 segPublic_t * segmentation_getSegmentInfo(void)
 {
   return &segPublicinfo;
 }


 //-----------------------------------------------------------------------------
 // Function Name:	 segmenting_init
 // Description:	 
 //-----------------------------------------------------------------------------
 void segmentation_getConfig(segConfig_t const * segConfigPtr)
 {
   segConfig_t const * cfgPtr= segConfigPtr;
   seg_cfg_maxSig =cfgPtr->segSigMax;
   seg_cfg_minSig =cfgPtr->segSigROI;
   seg_cfg_minPeak = cfgPtr->segMinPeak;
   seg_cfg_mergeCoef = cfgPtr->segMergeCoef;
   seg_cfg_contourRes= (cfgPtr->segResolution>SEGCONTOURMAX)?SEGCONTOURMAX:cfgPtr->segResolution;

   seg_contourBin=(uint16)((seg_cfg_maxSig-seg_cfg_minSig)%seg_cfg_contourRes);
   
   if(seg_contourBin)
   {
	   seg_contourBin=(uint16)(1+((seg_cfg_maxSig-seg_cfg_minSig)/seg_cfg_contourRes));
   }
   else
   {
	   seg_contourBin=(uint16)((seg_cfg_maxSig-seg_cfg_minSig)/seg_cfg_contourRes);
   }
   
   seg_minpeakContourLevel=((seg_cfg_minPeak-seg_cfg_minSig)+(seg_contourBin>>1))/seg_contourBin;
   
   if(seg_minpeakContourLevel>seg_cfg_contourRes) seg_minpeakContourLevel=seg_cfg_contourRes;

 }



//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
void segmentation_init(void)
{  
  segObjects 	 = &seghooks[SEGHOOKID_OBJ];  // 0~9th for object link  
  //segMinb        = &seghooks[SEGHOOKID_MINB];
  segUnSegmented = &seghooks[SEGHOOKID_UNSEG];// 10th for unsegment
  //segDam		 = &seghooks[SEGHOOKID_DAM];// 11th for dam
  segBackground  = &seghooks[SEGHOOKID_BCKGND];// 12th for background
  
  segPublicinfo.lableMap=&segLabelImage;
  segPublicinfo.objects=segObjects; 
  segPublicinfo.unsegmented=segUnSegmented;
  segPublicinfo.background=segBackground;
  segPublicinfo.objbitmask = &segObjBitmask;
  segmentation_resetReport();

  segLable=0;

}


//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
uint16 segmentation_segments(touchImage_t* imgarray)
{
	uint16 segmentCount, loop;
	segmentLabel_t headContour[SEGCONTOURMAX]; //head hook of contour
	segmentLabel_t localBuffer[SEGCONTOURMAX]; // it is used to be contour tail as well as other function's buffer.
	segmentLabel_t tailSegment[SEGHOOKSIZE];// segment hook's tail.
	hook_t contour,segments;


 // create contour hook group
	contour.headP=headContour;
 	contour.tailP=localBuffer;
	contour.hookcount=SEGCONTOURMAX;
 // create segments hook group
 	segments.headP=seghooks;
 	segments.tailP=tailSegment;
	segments.hookcount=SEGHOOKSIZE;
// reset buffer
	segmentation_resetReport();
	segmentation_resetHooks(&contour);
	segmentation_resetHooks(&segments);
 // sort by contour
 	segmentation_sortbyContour(imgarray, &segments, &contour);
 
 // watershed contour
	segmentCount=segmentation_searchBlobByWatershed(&contour, &segments);
 
 // merge over-segments
 	memset(localBuffer,0,sizeof(localBuffer));
 	segmentCount=segmentation_mergeblobs(imgarray,(int16*)localBuffer, &segments);
	
 // dilation
 	segmentation_sortDilatedRoiToSegments(imgarray,&segments);

 // update bitmask
	for(loop=0;loop<CFG_MAX_OBJECTS;loop++)
	{
		if(segObjects[loop].value!=RESET_VAL)
		{
			segObjBitmask|=1<<loop;
		}
	}
	return segmentCount;
}

//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
void segmentation_calcSegmentPosition(touchImage_t * imgarray,uint16 segCount)
{
 	uint16 loop, count;

 	count=0;

	for(loop=0;loop<CFG_MAX_OBJECTS && count<segCount;loop++)
	{
		if(segObjects[loop].value!=RESET_VAL)
		{
			segmentation_calcSigsumAndCentroid(imgarray, loop);
			count++;
		}
	}

}

//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------

int16 segmentation_getSegmentPosition(uint8p8 * posRow, uint8p8* posCol, uint16 offset)
{
	int16 res;

	res=RES_ERROR0;
	if((offset<CFG_MAX_OBJECTS)&&(segObjBitmask&(1<<offset)))
	{
		*posRow=segCentroidRow[offset];
		*posCol=segCentroidCol[offset];
		res=RES_OK;
	}

	return res;

}

//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------

int16 segmentation_getSegmentSignal(int32* sig, uint16 offset)
{
  int16 res;

  res= RES_ERROR0;
  if((offset<CFG_MAX_OBJECTS)&&(segObjBitmask&(1<<offset)))
  {
  	  *sig = segSigSum[offset];
	  res=RES_OK;
  }
  
  return res;
  
}



