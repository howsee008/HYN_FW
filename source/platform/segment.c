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

#define SEGHOOKSIZE  (CFG_MAX_OBJECTS+3)
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
static segmentLabel_t * segDam;



static int16 seg_cfg_maxSig;
static int16 seg_cfg_minSig;
static int16 seg_cfg_minPeak;
static int16 seg_cfg_mergeCoef;
static int16 seg_cfg_contourRes;

static int16 seg_contourBin;
static uint16 segLable;

//static segmentLabel_t damLinks;


#if 0
static int16 N9[8]={-(CFG_COL-1),-CFG_COL,-(CFG_COL+1),-1,1,(CFG_COL-1),CFG_COL,(CFG_COL+1)};
static int16 N4[4]={-CFG_COL,-1,1,CFG_COL};
#endif

//=============================================================================
// Local (Static) Functions Declaration
//=============================================================================
static void segmentation_resetReport(void);
static uint16 segmentation_sortbyContour(touchImage_t* imgarray, segmentLabel_ptr unsegment, hook_t* contour);
static void segmentation_fillFlood(segnode_t * seedNode, uint16 lableId,segmentLabel_t * src, hook_t * segments, uint16 option );
static void segmentation_resetHooks(hook_t* hook);
static int16 segmentation_pushQueue(hook_t* hooks,  uint16 hookid, segnode_t* node);
static int16 segmentation_removeQueue(hook_t* hooks,uint16 hookid,segnode_t* node );
static int16 segmentation_popQueue(hook_t* hooks,uint16 hookid,segnode_t* node );
static int16 segmentation_getNeighbourPosition(uint16 neighbour, segnode_t* curNode, segnode_t* nbNode);
static int16 segmentation_searchBlobByWatershed(hook_t* contour,hook_t* segments,uint16 minpeaklevel);


#if 0
static void segmenting_searchROI(int16* imgarray, uint16 imgsize);
static int16 segmenting_pushObjLink(uint16 objid, uint16 pos, int16* imgarray);
static int16 segmenting_pushDamLink(uint16 damflag, uint16 pos, int16* imgarray);
static void segmenting_fifoInit(segQue_t* fifo);
static int16 segmenting_fifoPop(segQue_t * fifo);
static int16 segmenting_fifoPush(segQue_t *fifo, uint16 p);
static uint16 segmenting_contourbinCal(int16 minsig, uint16 contourlevel);
static void segmenting_contourSorting(int16 * imgarray, uint16 imgsize,
                                                segQue_t* contourarray,uint16 arraysize, 
                                                uint16 contourbin, 
                                                int16 minsig);
static void segmenting_watershedCal(int16 * imgarray, uint16 imgsize,
                                             segQue_t * contourarray, uint16 arraysize,
                                             int16 minpeak);

static void segmenting_mergeObj(uint16 liveObj, uint16 deadObj, uint16 deaddam);

static uint16 segmenting_ShouldbeMerge(int16* imgarray, uint16 imgsize, 
	                                            uint16 damid,uint16*bloblist);
static void segmenting_moresegment(int16* imgarray, uint16 imgsize);
#endif


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
    memset((void*)&seghooks[0],0xff,sizeof(segObjects));

}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static int16 segmentation_pushQueue(hook_t* hooks,  uint16 hookid, segnode_t* node)
{
  segmentLabel_ptr head, tail;

  if (hookid>=hooks->hookcount || hooks->tailP==(segmentLabel_ptr)0) 
  	return RES_ERROR0;
 
  head=&(hooks->headP[hookid]);
  tail=&(hooks->tailP[hookid]);
  
  if(0xffff!=head->value)
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
	  segLabelImage.matrix[node->row][node->col].feilds.nextRow=0x3f;
	  segLabelImage.matrix[node->row][node->col].feilds.nextCol=0x3f;
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
			segLabelImage.matrix[node->row][node->col].feilds.nextRow=0x3f;
			segLabelImage.matrix[node->row][node->col].feilds.nextCol=0x3f;
	  
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
	uint16 res=RES_OK;
	
	 if (hookid>=hooks->hookcount)
	 {
	 	node->row=0x3f;
		node->col=0x3f;
	 	return RES_ERROR0;
	 }
	
	 head=&(hooks->headP[hookid]);

     
	 if(!(head->feilds.nextRow==0x3f&&head->feilds.nextCol==0x3f))
	 {
	 	uint8 row, col;
		row = head->feilds.nextRow;
		col = head->feilds.nextCol;
        node->row = row;
		node->col = col;
		head->feilds.nextRow=segLabelImage.matrix[row][col].feilds.nextRow;
		head->feilds.nextCol=segLabelImage.matrix[row][col].feilds.nextCol;

		segLabelImage.matrix[row][col].feilds.nextRow=0x3f;
		segLabelImage.matrix[row][col].feilds.nextCol=0x3f;

		
	 }
	 else
	 {
	 	res=RES_ERROR0;
	 	node->row=0x3f;
		node->col=0x3f;
		
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
	segmentLabel_t fifoHead, fifoTail;

	hook_t srcHook, fifo;

	srcHook.headP=src;
	srcHook.hookcount=1;

	fifo.headP=&fifoHead;
	fifo.tailP=&fifoTail;
	fifo.hookcount=0;

	segmentation_resetHooks(&fifo);

	cur.row = seedNode->row;
	cur.col = seedNode->col;

	while(!(0x3f==cur.row && 0x3f==cur.col))
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
static void segmentation_dilateSegments(hook_t* segments)
{
  segmentLabel_t dilationHead, dilationTail;
  hook_t dilation;
  uint16 i;
  dilation.headP=&dilationHead;
  dilation.tailP=&dilationTail;
  dilation.hookcount=1;
  
  for(i=0;i<segments->hookcount;i++)
  {
  	
	uint16 dilationCount;
	segnode_t cur,nb;
	
	if(segments->headP[i].value==0xffff) break;

	cur.row=segments->headP[i].feilds.nextRow;
	cur.col=segments->headP[i].feilds.nextCol;
	dilationCount=0;
	segmentation_resetHooks(&dilation);
	while(!(cur.row==0x3f&&cur.col==0x3f))
	{
	  uint16 nbId;
	  for(nbId=0;nbId<4;nbId++)
	  {
		if(RES_OK==segmentation_getNeighbourPosition(nbId,&cur,&nb))
		{
			if(segLabelImage.matrix[nb.row][nb.col].feilds.lable==OBJ_INIT)
			{
				segmentation_pushQueue(&dilation,0,&nb);
				dilationCount++;
			}
		}
	  }


	  cur.row=segLabelImage.matrix[cur.row][cur.col].feilds.nextRow;
	  cur.col=segLabelImage.matrix[cur.row][cur.col].feilds.nextCol;
		
	}

	
	if(dilationCount>0)
	{
	  cur.row=dilationHead.feilds.nextRow;
	  cur.col=dilationHead.feilds.nextCol;
	  segmentation_pushQueue(segments,i,&cur);
	}
	
	
  }
}


#if 0
//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
static void segmentation_searchSegments(touchImage_t* imgarray, hook_t * segments)
{
	int16 i; 
	uint16 minlevel,level;
	uint16 row,col;
	segnode_t cur;
	segmentLabel_t roihead[SEGHOOKSIZE], fifohead, fifotail;
	hook_t fifo;
	hook_t contour;

	// create ROI contour link
	contour.headP=&roihead[SEGHOOKSIZE];
	contour.hookcount=SEGHOOKSIZE;
	memcopy(roihead,seghooks,sizeof(roihead));

	// create a FIFO link
	fifo.headP=&fifohead;
	fifo.tailP=&fifotail;
	fifo.hookcount=1;
	segmentation_resetHooks(&fifo);

	minlevel = 1+(seg_cfg_minPeak-seg_cfg_minSig)>>4;


	for(i=SEGHOOKSIZE-1;i>minlevel;i--)
	{
		if(roihead[i].value==0xffff) continue;
		
		segmentation_popQueue(&contour,i,&cur);

		while(!(cur.row==0x3f&&cur.col==0x3f))
		{

			segmentation_fillFlood(&cur,segLable++,imgarray,contour,1,segments,fifo);		
			segmentation_popQueue(&contour,i,&cur);

		}
	}

	segmentation_popQueue(&contour,minlevel,&cur);

	while(!(cur.row==0x3f&&cur.col==0x3f))
	{
		segmentation_fillFlood(&cur,segLable++,imgarray,contour,1,segments,fifo);
		segmentation_popQueue(&contour,minlevel,&cur);
	}


  
}
#endif
//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
static int16 segmentation_searchBlobByWatershed(hook_t* contour,hook_t* segments,uint16 minpeaklevel)
{
  int16 labeid =0;
  uint16 nbId;
  segnode_t cur, next,nb; 
  int16 loop;
  segmentLabel_t fifo_head, fifo_tail;
  hook_t fifo;


  fifo.headP=&fifo_head;
  fifo.tailP=&fifo_tail;
  fifo.hookcount=1;

  segmentation_resetHooks(&fifo);

  // watershed
  for(loop=seg_cfg_contourRes;loop>=0;loop--)
  {

    if(contour->headP[loop].value==0xffff) continue;
    // here pop can't be used because the node needn't to be removed from the list.just label it.
	cur.row=contour->headP[loop].feilds.nextRow;
	cur.col=contour->headP[loop].feilds.nextCol;
	while(!(cur.row==0x3f&&cur.col==0x3f))
	{
        next.row=segLabelImage.matrix[cur.row][cur.col].feilds.nextRow;
		next.col=segLabelImage.matrix[cur.row][cur.col].feilds.nextCol;
		
		segLabelImage.matrix[cur.row][cur.col].feilds.lable=OBJ_MASK;

		for(nbId=0;nbId<4;nbId++)
		{
			if(RES_OK==segmentation_getNeighbourPosition(nbId,&cur,&nb))
			{
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
	
	segmentation_popQueue(&fifo,0,&cur);
	
	while(!(cur.row==0x3f&&cur.col==0x3f))
	{
		uint16 curLab, nbLab, maskedCount;
		segnode_t maskednb[4];

		maskedCount =0;
		for(nbId=0;nbId<4;nbId++)
		{
			if(RES_OK==segmentation_getNeighbourPosition(nbId,&cur,&nb))
			{
				curLab=segLabelImage.matrix[cur.row][cur.col].feilds.lable;
				nbLab = segLabelImage.matrix[nb.row][nb.col].feilds.lable;
				
				if(nbLab<OBJ_DAM)// the neighbour is a obj
				{
					if (curLab==OBJ_INQE)//it is labled same.
					   segLabelImage.matrix[cur.row][cur.col].feilds.lable=nbLab;
					else if ((curLab<OBJ_DAM)&&(curLab!=nbLab))// it is labed as DAM for it was labed early by another neighbour.
					{
					   segLabelImage.matrix[cur.row][cur.col].feilds.lable=OBJ_DAM;

					   if(nbLab==OBJ_MINB)
					   {
							labeid=segLable++;
							segmentation_removeQueue(contour,loop,&nb);
							segmentation_fillFlood(&nb,labeid,segUnSegmented,segments,1);
					   }
					   else if(curLab==OBJ_MINB)
					   {
							labeid=segLable++;
							segmentation_removeQueue(contour,loop,&cur);
							segmentation_fillFlood(&cur,labeid,segUnSegmented,segments,1);
					   }
					   
					}
					
				}
				else if(nbLab==OBJ_MASK)// it is at the same level but be not labled, save it.
				{					
				   maskednb[maskedCount].row=nb.row;
				   maskednb[maskedCount].col=nb.col;
				   maskedCount++;
				}
				
			}

		}
		curLab=segLabelImage.matrix[cur.row][cur.col].feilds.lable;
		if((curLab<OBJ_MINB)||(curLab==OBJ_DAM))
		{
			segmentation_removeQueue(contour,loop,&cur );
			segmentation_pushQueue(segments,  curLab, &cur);
		}
		if(curLab<OBJ_MINB)// push the saved neighbour in case of cur lab isn't DAM
		{
		  for(nbId=0;nbId<maskedCount;nbId++)
		  {
			  segLabelImage.matrix[maskednb[nbId].row][maskednb[nbId].col].feilds.lable=OBJ_INQE;
			  segmentation_removeQueue(contour,loop,&maskednb[nbId]);
			  segmentation_pushQueue(&fifo,  0, &maskednb[nbId]);
		  }
		}
	
		segmentation_popQueue(&fifo,0,&cur );
	}

	
	cur.row=contour->headP[loop].feilds.nextRow;
	cur.col=contour->headP[loop].feilds.nextCol;

	if(cur.row!=0x3f || cur.col!=0x3f)
	{
		if(loop<=minpeaklevel)
		{
			segmentation_pushQueue(contour, OBJ_MINB, &cur);
			
			while(!(cur.row==0x3f&&cur.col==0x3f))
			{
				segmentLabel_t * node;
				node = & segLabelImage.matrix[cur.row][cur.col];
				node->feilds.lable=OBJ_MINB;
				cur.row = node->feilds.nextRow;
				cur.col = node->feilds.nextCol;				
			}
			
			contour->headP[loop].value=0xffff;
		}
		else
		{
			segmentation_resetHooks(&fifo);
			segmentation_popQueue(contour,loop,&cur);
			while(!(cur.row==0x3f&&cur.col==0x3f))
			{
				labeid=segLable++;
				segmentation_fillFlood(&cur,labeid,&contour->headP[loop],segments,0);
				segmentation_popQueue(contour,loop,&cur);
			}
		}

	}

	

  }

  return segLable;
  
}


//////////////////////////////////////////////////////////////////////////////////////////////



//-----------------------------------------------------------------------------
// Function Name:	segmentation_sortbyContour
// Description:		contour sorting unsegment node. if the node>seg_cfg_minSig
//  rasie its contour level by 1
//-----------------------------------------------------------------------------
static uint16 segmentation_sortbyContour(touchImage_t* imgarray, segmentLabel_ptr unsegment, hook_t* contour)
{
	uint16 bin,level,n;
	segnode_t curnode;
	hook_t unsegmentHook;

	unsegmentHook.headP=unsegment;
	unsegmentHook.hookcount=1;

	bin=(uint16)((seg_cfg_maxSig-seg_cfg_minSig)%seg_cfg_contourRes);

	if(bin)
	{
		bin=(uint16)(1+((seg_cfg_maxSig-seg_cfg_minSig)/seg_cfg_contourRes));
	}
	else
	{
		bin=(uint16)((seg_cfg_maxSig-seg_cfg_minSig)/seg_cfg_contourRes);
	}

	level=((seg_cfg_minPeak-seg_cfg_minSig)+bin>>1)/bin;

	if(level>seg_cfg_contourRes) level=seg_cfg_contourRes;

	segmentation_popQueue(&unsegmentHook, 0, &curnode);

	while(!(curnode.row==0x3f && curnode.col==0x3f))
	{
		int16 sig;

		sig=imgarray->matrix[curnode.row][curnode.col];

		n=((sig-seg_cfg_minSig)+(bin>>1))/bin;

		if(n>seg_cfg_contourRes) n=seg_cfg_contourRes;

		segmentation_pushQueue(contour, n, &curnode);

		segmentation_popQueue(&unsegmentHook, 0, &curnode);
		
	}

	// at minpeak level, there are some sig bigger than minpeak, push it to the up level.
	if(level<seg_cfg_contourRes-1)
	{
		unsegmentHook.headP=&contour->headP[level];
		segmentation_popQueue(&unsegmentHook, 0, &curnode);
		while(!(curnode.row==0x3f && curnode.col==0x3f))
		{
			int16 sig;
			sig=imgarray->matrix[curnode.row][curnode.col];
		
			if(sig>seg_cfg_minPeak)
			{
				segmentation_pushQueue(contour, level+1, &curnode);
			}
			else
			{
				segmentation_pushQueue(contour, level, &curnode);
			}
		
			segmentation_popQueue(&unsegmentHook, 0, &curnode);
		}
	}

	return level;
}


//-----------------------------------------------------------------------------
// Function Name:	segmentation_sortbyContour
// Description:		contour sorting unsegment node. if the node>seg_cfg_minSig
//  rasie its contour level by 1
//-----------------------------------------------------------------------------




#if 0

//-----------------------------------------------------------------------------
// Function Name:	segmenting_pushObjLink
// Description:	 push a piexl to a specific object	
//-----------------------------------------------------------------------------
static int16 segmenting_pushObjLink(uint16 objid, uint16 pos, int16* imgarray)
{
  objLink_t* obj_ptr ;
  labMap_t*  lab_ptr ;

  if(objid>=MAX_OBJECTS)
  {
     return -1 ;
  }
  obj_ptr = &segPublicinfo.objLink[objid];
  lab_ptr = &segPublicinfo.lableMap[0];
  if (0==obj_ptr->linksize)
  {
    obj_ptr->headP = pos;
	obj_ptr->tailP = pos;
	obj_ptr->peakP = pos;
	segPublicinfo.objCount+=1;
  }
  else
  {
    uint16 offset = obj_ptr->tailP;
	lab_ptr[offset].nextP=pos;
	obj_ptr->tailP = pos;
	if(imgarray[obj_ptr->peakP]<imgarray[pos])
	{
       obj_ptr->peakP = pos;
	}
  }
  lab_ptr[pos].lab = objid;
  obj_ptr->linksize+=1;

  return (int16)obj_ptr->linksize;
  
}

//-----------------------------------------------------------------------------
// Function Name:	segmenting_pushDamLink
// Description:	put a pixel to a specific dam 
//-----------------------------------------------------------------------------
static int16 segmenting_pushDamLink(uint16 damflag, uint16 pos, int16* imgarray)
{
  uint16 isfound=0;
  uint16 i;
  damLink_t* damptr ;
  labMap_t*  lab_ptr ;
  if(0==damflag)
  {
    return -1;
  }
  damptr=&segPublicinfo.damLink[0];
  lab_ptr = &segPublicinfo.lableMap[0];
  for(i=0; i<MAX_OBJECTS; i++)
  {
     if(damflag==damptr[i].flagbits)
     {
		isfound=1;
		break;
	 }
  }
 // if no existing dam, assign a new hook.
  if(!isfound)
  {
    for(i=0;i<MAX_OBJECTS;i++)
    {
       if(0==damptr[i].flagbits)
       {
         isfound=1;
		 break;
	   }
	}
  }

  if(!isfound)
  {
    return -1;
  }

  if(!damptr[i].linksize)
  {
    damptr[i].headP=pos;
	damptr[i].tailP=pos;
	damptr[i].peakP=pos;
	segPublicinfo.damCount+=1;
  }
  else
  {
    uint16 offset = damptr[i].tailP;
	lab_ptr[offset].nextP=pos;
	damptr[i].tailP=pos;
	if(imgarray[damptr[i].peakP]<imgarray[pos])
    {
       damptr[i].peakP = offset;
	}
  }
  damptr[i].flagbits = damflag;
  damptr[i].linksize +=1;

  return (int16)damptr[i].linksize;

  
}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:	initial a FIFO	
//-----------------------------------------------------------------------------
static void segmenting_fifoInit(segQue_t* fifo)
{
  fifo->linksize=0;
}


//-----------------------------------------------------------------------------
// Function Name:	
// Description:	pop the fifo and get a element	
//-----------------------------------------------------------------------------
static int16 segmenting_fifoPop(segQue_t * fifo)
{
  uint16 pop;
  labMap_t*  lab_ptr;

  if(!fifo->linksize)
  	return -1;
  
  pop = fifo->headP;
  lab_ptr = &segPublicinfo.lableMap[0];

  fifo->linksize-=1;

  if(fifo->headP == fifo->tailP)
  	fifo->linksize=0;
  else
  	fifo->headP= lab_ptr[pop].nextP;

  return pop;
  
}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		push a element into a fifo
//-----------------------------------------------------------------------------
static int16 segmenting_fifoPush(segQue_t *fifo, uint16 p)
{
  labMap_t*  lab_ptr = &segPublicinfo.lableMap[0];

  if(!fifo->linksize)
  {
    fifo->headP=p;
	fifo->tailP=p;
  }
  else
  {
    lab_ptr[fifo->tailP].nextP=p;
	fifo->tailP = p;
  }

  fifo->linksize+=1;

  return (int16)fifo->linksize;
}
//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
static uint16 segmenting_contourbinCal(int16 minsig, uint16 contourlevel)
{
   uint16 bin;

   bin = (SATURATESIG-minsig);

   bin +=(contourlevel>>1);

   bin /=contourlevel;

   //bin=((SATURATESIG-minsig)+(contourlevel>>1))/contourlevel;

   bin=(bin<1)?1:bin;

   return bin;
}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:		sort a img by contour with a specific bin
//-----------------------------------------------------------------------------
static void segmenting_contourSorting(int16 * imgarray, uint16 imgsize,
                                                segQue_t* contourarray,uint16 arraysize, 
                                                uint16 contourbin, 
                                                int16 minsig)
{
  uint16 i;

  for(i=0;i<imgsize;i++)
  {
    int16 sig;
	uint16 n;
	sig = imgarray[i];
	if(sig<minsig) continue;
	n=((sig-minsig)+(contourbin>>1))/contourbin;
	n=(n+1>arraysize)?n=(n+1>arraysize)-1:n;

	segmenting_fifoPush(&contourarray[n],i);
	
  }

}


//-----------------------------------------------------------------------------
// Function Name:	
// Description:		watershed algorithm
//-----------------------------------------------------------------------------
static void segmenting_watershedCal(int16 * imgarray, uint16 imgsize,
                                             segQue_t * contourarray, uint16 arraysize,
                                             int16 minpeak)
{
  
  segQue_t fifo;
  segQue_t* contourptr;
  uint16 lab_cur=0;
//  uint16 damflag=0;
  int16 loop;
  labMap_t*  lab_ptr;

  segmenting_fifoInit(&fifo);
  
  lab_ptr = &segPublicinfo.lableMap[0];
  
  for(loop=(int16)(arraysize-1);loop>0;loop--)
  {
    uint16 linksize;
	uint16 i,nextP;
	
   
      contourptr=&contourarray[loop];
	  linksize = contourptr->linksize;
      nextP = contourptr->headP;
	  for(i=0;i<linksize;i++)
	  {
        uint16 curID,n;
		
		curID = nextP;
		nextP=lab_ptr[curID].nextP;
		lab_ptr[curID].lab = OBJ_MASK;

		for(n=0;n<4;n++)// firstly, find the pixel which has object label in its neighbour.
	    {
          int16 nidx;

		  if(lab_ptr[curID].edge==SEG_EDGE_L && 1==n) continue;
		  if(lab_ptr[curID].edge==SEG_EDGE_R && 2==n) continue;
		  nidx=(int16)curID+N4[n];
		  if(nidx<0 || nidx>=(int16)(imgsize-1)) continue;

		  if(lab_ptr[nidx].lab<OBJ_DAM)
		  {
             segmenting_fifoPush(&fifo, curID);
			 lab_ptr[curID].lab=OBJ_INQE;
			 break;
		  }  
		} // checking the neighbourhood to find if there is a object. if yes, 
	  }
	  // label the one which has object in its neighbour. if some new created, label it after the ones found early have been labled.
	  while(fifo.linksize)
	  {
          int16 curID,n;
		  uint16 damflag;
		  curID = segmenting_fifoPop(&fifo);
		  if(curID<0) break;
		  damflag=0;
		  for(n=0;n<4;n++)// searching the neighbour of the current sensor
		  {
            int16 nidx;
			uint16 cur_lab,nei_lab;
			if(lab_ptr[curID].edge==SEG_EDGE_L && 1==n) continue;
			if(lab_ptr[curID].edge==SEG_EDGE_R && 2==n) continue;
			nidx=(int16)curID+N4[n];
			if(nidx<0 || nidx>=(int16)(imgsize-1)) continue;

			cur_lab=lab_ptr[curID].lab;// cur lab must be updated every time when looping, because it could be changed at previous looping.
			nei_lab=lab_ptr[nidx].lab;

			if(nei_lab<OBJ_DAM)// the neighbour is a obj
			{
				if ((cur_lab==OBJ_INQE)||(cur_lab==OBJ_DAM2))//it is labled same.
				   lab_ptr[curID].lab=nei_lab;
				else if ((cur_lab<OBJ_DAM)&&(cur_lab!=nei_lab))// it is labed as DAM for it was labed early by another neighbour.
				{
				   lab_ptr[curID].lab=OBJ_DAM;
				   damflag=(1<<curID)|(1<<nidx);
				}
				
			}
			else if(nei_lab==OBJ_DAM) // the neighbour is watershed/dam
			{
               if(cur_lab==OBJ_INQE) // it is labled as DAM2. it isn't a dam, but it is near to a dam. it could be changed by other obj label.
               {
                  lab_ptr[curID].lab=OBJ_DAM2;
			   }
			}
			else if(nei_lab==OBJ_MASK)// the neighbour has no special label but be at same contour level.
			{                   
               lab_ptr[nidx].lab=OBJ_INQE;
			   segmenting_fifoPush(&fifo, nidx);
			}
			
		  }

		  if(lab_ptr[curID].lab<OBJ_DAM)
		  	segmenting_pushObjLink(lab_ptr[curID].lab, curID, imgarray);
		  else if(lab_ptr[curID].lab==OBJ_DAM)
		  	segmenting_pushDamLink(damflag, curID, imgarray);
		  
	 }// widefirst searching


	  
   //here, no adjancent label is found, so assign a new one
      nextP = contourptr->headP;
      for(i=0;i<linksize;i++)
      {
        uint16 curID;
		curID=nextP;
		nextP=lab_ptr[curID].nextP;

		if(imgarray[curID]<minpeak) continue;

		if((lab_ptr[curID].lab==OBJ_MASK)||(lab_ptr[curID].lab==OBJ_DAM2))
	    {
           lab_ptr[curID].lab=lab_cur;
		   segmenting_pushObjLink(lab_ptr[curID].lab, curID, imgarray);

		   segmenting_fifoPush(&fifo, curID);

		   while(fifo.linksize)
		   {
		      uint16 curID, n;
              curID=segmenting_fifoPop(&fifo);
             // Wide First Search. checking the neighbour if it can be labled
			  for(n=0;n<4;n++)
			  {
                int16 nidx;
				if(lab_ptr[curID].edge==SEG_EDGE_L && 1==n) continue;
				if(lab_ptr[curID].edge==SEG_EDGE_R && 2==n) continue;
				nidx=(int16)curID+N4[n];
				if(nidx<0 || nidx>=(int16)(imgsize-1)) continue;
				if(lab_ptr[nidx].lab==OBJ_MASK || lab_ptr[nidx].lab==OBJ_DAM2)
				{
                   lab_ptr[nidx].lab=lab_cur;
				   segmenting_pushObjLink(lab_ptr[nidx].lab, nidx, imgarray);
				   segmenting_fifoPush(&fifo, nidx);
				}
				
			  }
			  			  			  
		   }

		   if(lab_cur<MAX_OBJECTS) lab_cur+=1;
		   
		}

	  }
		
			  
	  
  }
  
  
}


//-----------------------------------------------------------------------------
// Function Name:	
// Description:	merge two objects 
//-----------------------------------------------------------------------------
static void segmenting_mergeObj(uint16 liveObj, uint16 deadObj, uint16 deaddam)
{
 objLink_t* liveptr =&segPublicinfo.objLink[liveObj] ;
 objLink_t* deadptr =&segPublicinfo.objLink[deadObj];
 labMap_t * labptr  =&segPublicinfo.lableMap[0];
 damLink_t* damptr  =&segPublicinfo.damLink[deaddam];
 uint16 i,len,idx,tailid;

 len = deadptr->linksize;
 idx=deadptr->headP;
 for(i=0;i<len;i++)
 {
   labptr[idx].lab=liveObj;
   idx=labptr[idx].nextP;
 }
 tailid=liveptr->tailP;
 labptr[tailid].nextP=deadptr->headP;
 liveptr->linksize+=deadptr->linksize;
 deadptr->linksize=0;
 segPublicinfo.objCount-=1;

 len= damptr->linksize;
 idx= damptr->headP;
 for(i=0;i<len;i++)
 {
   labptr[idx].lab=liveObj;
   idx=labptr[idx].nextP;
 }
 tailid = liveptr->tailP;
 labptr[tailid].nextP=damptr->headP;
 liveptr->linksize+=damptr->linksize;
 damptr->linksize=0;
 damptr->flagbits=0;
 segPublicinfo.damCount-=1;
 
}

//-----------------------------------------------------------------------------
// Function Name:	
// Description:	check if two objects should be merged or not	
//-----------------------------------------------------------------------------
static uint16 segmenting_ShouldbeMerge(int16* imgarray, uint16 imgsize, uint16 damid,uint16*bloblist)
{
  
  uint16 damflag = segPublicinfo.damLink[damid].flagbits;
  objLink_t* objptr =&segPublicinfo.objLink[0];
  labMap_t * labptr  =&segPublicinfo.lableMap[0];
  uint16 offset;
  uint16 shift;
  uint16 n, idx;
  int16 blobpeak,dampeak ;
  
  if (!damflag)
  	return 0;
  
  offset=0;
  shift=0;
  while(damflag)
  {
     if(damflag&0x0001)
     {
        bloblist[offset]=shift;
		offset++;
	 }
	 if(offset>1) break;
	 damflag>>=1;
	 shift++;
  }
  // id 0 containing small object, id 1 containing big one.
  if((offset==2)&&(imgarray[objptr[bloblist[0]].peakP]>imgarray[objptr[bloblist[1]].peakP]))
  {
    uint16 tmp;

	tmp=bloblist[1];
	bloblist[1]=bloblist[0];
	bloblist[0]=tmp;		
  }
 // cal the average of peak and the max neighour to stand for blob sigal
  idx = objptr[bloblist[0]].peakP;
  blobpeak=0;
  for(n=0;n<8;n++)
  {
   int16 nidx;
    if((labptr[idx].edge==SEG_EDGE_L)&&(n==0||n==3||n==5)) continue;
	if((labptr[idx].edge==SEG_EDGE_R)&&(n==2||n==4||n==7))continue;
	nidx=(int16)idx+N9[n];
	if(nidx<0 || nidx>=(int16)(imgsize-1)) continue;

	if(labptr[nidx].lab==labptr[idx].lab)
	{
       if(imgarray[nidx]>blobpeak) blobpeak=imgarray[nidx];
	}
  }
  blobpeak=(imgarray[idx]+blobpeak)/2;
// cal the average of peak and subpeak to stand for the dam signal
  idx=segPublicinfo.damLink[damid].peakP;
  dampeak=0;
  for(n=0;n<8;n++)
  {
    uint16 nidx;
    if((labptr[idx].edge==SEG_EDGE_L)&&(n==0||n==3||n==5)) continue;
	if((labptr[idx].edge==SEG_EDGE_R)&&(n==2||n==4||n==7))continue;
	nidx=idx+N9[n];
	if(nidx<0 || nidx>=(int16)(imgsize-1)) continue;
	if(labptr[nidx].lab==labptr[idx].lab)
	{
       if(imgarray[nidx]>dampeak) dampeak=imgarray[nidx];
	}		
  }
  dampeak=(imgarray[idx]+dampeak)/2;
 // if dam signal > 70% of blob's, merge the two blob
  if(dampeak*10>blobpeak*7)
  {
    return 1;
  }
  else
  {
    return 0;
  }
  
}
//-----------------------------------------------------------------------------
// Function Name:	
// Description:	more segment by merging.	
//-----------------------------------------------------------------------------
static void segmenting_moresegment(int16* imgarray, uint16 imgsize)
{
  uint16 damcount, count, damidx;

  count=1;
  damcount =segPublicinfo.damCount;

  for(damidx=0;damidx<MAX_OBJECTS;damidx++)
  {
    uint16 damflag;
	
    if(count>damcount) break;

	damflag = segPublicinfo.damLink[damidx].flagbits;

	if(damflag>0)
	{
	    uint16 ismerge=0;
		uint16 bloblist[2];
		
        count+=1;

		ismerge=segmenting_ShouldbeMerge(imgarray, imgsize,damidx,bloblist);

		if(ismerge)
		{
           segmenting_mergeObj(bloblist[1], bloblist[0], damidx);
		}

	}

	 

  }
   
 
}


//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
#endif
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

   seg_contourBin= (seg_cfg_maxSig+seg_cfg_contourRes>>1)/seg_cfg_contourRes;
   seg_contourBin=(seg_contourBin<1)?1:seg_contourBin;

 }



//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
void segmentation_init(void)
{  
  segObjects 	 = &seghooks[0];            // 0~9th for object link  
  segUnSegmented = &seghooks[SEGHOOKSIZE-3];// 10th for unsegment
  segDam		 = &seghooks[SEGHOOKSIZE-2];// 11th for dam
  segBackground  = &seghooks[SEGHOOKSIZE-1];// 12th for background
  
  segPublicinfo.lableMap=&segLabelImage;
  segPublicinfo.segments=segObjects; 
  segPublicinfo.unsegmented=segUnSegmented;
  segPublicinfo.background=segBackground;
  
  segmentation_resetReport();

  segLable=0;

}


//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
segmentLabel_ptr segmentation_searchRoi(touchImage_t* imgarray)
{
	uint8 row,col;
	segnode_t node;
	segmentLabel_t backgroundTail,unsegmentTail;
	hook_t	backgroundFifo,unsegmentFifo;

	// clear seg report
	segmentation_resetReport();


	backgroundFifo.headP=segBackground;
	backgroundFifo.tailP=&backgroundTail;
	backgroundFifo.hookcount=1;
	segmentation_resetHooks(&backgroundFifo);


	unsegmentFifo.headP=segUnSegmented;
	unsegmentFifo.tailP=&unsegmentTail;
	unsegmentFifo.hookcount=1;
	segmentation_resetHooks(&unsegmentFifo);

	for(row=0;row<CFG_NUM_2D_ROWS;row++)
	{
		for(col=0;col<CFG_NUM_2D_COLS;col++)
		{
			int16 sig;

			sig = imgarray->matrix[row][col];
			node.col=col;
			node.row=row;
			if(sig<seg_cfg_minSig)
			{
				segmentation_pushQueue(&backgroundFifo, 0, &node);
			}
			else
			{
				segmentation_pushQueue(&unsegmentFifo, 0, &node);
			}
		}

	}
	return segBackground;
}


//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
uint16 segmentation_segments(touchImage_t* imgarray)
{
	uint16 minpeakLevel,segmentCount;
	segmentLabel_t headContour[SEGCONTOURMAX],tailContour[SEGCONTOURMAX];
	segmentLabel_t tailSegment[SEGHOOKSIZE];
	segmentLabel_t unsegData;
	hook_t contour,segments;

	unsegData = *segUnSegmented;

 // create contour hook
	contour.headP=headContour;
 	contour.tailP=tailContour;
	contour.hookcount=SEGCONTOURMAX;
 // create segments hook
 	segments.headP=seghooks;
 	segments.tailP=tailSegment;
	segments.hookcount=SEGHOOKSIZE;

	segmentation_resetHooks(&contour);
	segmentation_resetHooks(&segments);
 // sort by contour

 	minpeakLevel=segmentation_sortbyContour(imgarray, &unsegData, &contour);
 // watershed contour
	segmentCount=segmentation_searchBlobByWatershed(&contour, &segments, minpeakLevel);
 // merge

	return segmentCount;
}



#if 0

//-----------------------------------------------------------------------------
// Function Name:	segmenting_segment
// Description:	 main process of segment	
//-----------------------------------------------------------------------------
void segmenting_segment(int16* imgarray, int16 imgsize)
{
  uint16 contourbin, contourlen;
  segQue_t contourlist[SEGCONLEVEL];
  // calclulate contour distance
  contourbin = segmenting_contourbinCal(seg_cfg_minsig, SEGCONLEVEL);

  contourlen = SEGCONLEVEL;
  // fist of all, sort img by contour
  segmenting_contourSorting(imgarray, imgsize,contourlist,contourlen, contourbin, seg_cfg_minsig);
  // segment by watershed algorithm
  segmenting_watershedCal(imgarray, imgsize,contourlist, contourlen, seg_cfg_minpeak);
  // more segment by merging 
  segmenting_moresegment(imgarray, imgsize);
}
#endif




