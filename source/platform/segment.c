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

 #define SEGCONLEVEL  20

//=============================================================================
// Local Data Type Definition
//=============================================================================
 typedef struct
 {
	uint16 headP;
	uint16 tailP;
	int16 linksize;
 }segQue_t;



//=============================================================================
// Local (Static) Variables Definition
//=============================================================================
static segPublic_t segPublicinfo;
static int16 N9[8]={-(CFG_COL-1),-CFG_COL,-(CFG_COL+1),-1,1,(CFG_COL-1),CFG_COL,(CFG_COL+1)};
static int16 N4[4]={-CFG_COL,-1,1,CFG_COL};

static sigData_t seg_cfg_minsig;
static sigData_t seg_cfg_minpeak;



//=============================================================================
// Local (Static) Functions Declaration
//=============================================================================
static void segmenting_resetPubinfo(void);
static int16 segmenting_pushObjLink(uint16 objid, uint16 pos, sigData_t* imgarray);
static int16 segmenting_pushDamLink(uint16 damflag, uint16 pos, sigData_t* imgarray);
static void segmenting_fifoInit(segQue_t* fifo);
static int16 segmenting_fifoPop(segQue_t * fifo);
static int16 segmenting_fifoPush(segQue_t *fifo, uint16 p);
static uint16 segmenting_contourbinCal(sigData_t minsig, uint16 contourlevel);
static void segmenting_contourSorting(sigData_t * imgarray, uint16 imgsize,
                                                segQue_t* contourarray,uint16 arraysize, 
                                                uint16 contourbin, 
                                                sigData_t minsig);
static void segmenting_watershedCal(sigData_t * imgarray, uint16 imgsize,
                                             segQue_t * contourarray, uint16 arraysize,
                                             sigData_t minpeak);

static void segmenting_mergeObj(uint16 liveObj, uint16 deadObj, uint16 deaddam);

static uint16 segmenting_ShouldbeMerge(sigData_t* imgarray, uint16 imgsize, 
	                                            uint16 damid,uint16*bloblist);
static void segmenting_moresegment(sigData_t* imgarray, uint16 imgsize);

//=============================================================================
// Global Functions Definition (Declared in segment.h)
//=============================================================================

//-----------------------------------------------------------------------------
// Function Name: segmenting_getpubinfo
// Description:	it is a API to get segment result.	
//-----------------------------------------------------------------------------
 segPublic_t * segmenting_getpubinfo(void)
 {
   return &segPublicinfo;
 }


//-----------------------------------------------------------------------------
// Function Name:	segmenting_init
// Description:		
//-----------------------------------------------------------------------------
void segmenting_init(void)
{
  segmenting_resetPubinfo();

  seg_cfg_minsig = 5;

  seg_cfg_minpeak = 40;

}

//-----------------------------------------------------------------------------
// Function Name:	segmenting_segment
// Description:	 main process of segment	
//-----------------------------------------------------------------------------
void segmenting_segment(sigData_t* imgarray, uint16 imgsize)
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


//-----------------------------------------------------------------------------
// Function Name:	
// Description:		
//-----------------------------------------------------------------------------
//=============================================================================
// Local (Static) Functions Definition
//=============================================================================

//-----------------------------------------------------------------------------
// Function Name:	segmenting_resetPubinfo
// Description:		reset segment result buffer
//-----------------------------------------------------------------------------
static void segmenting_resetPubinfo(void)
{
	uint16 i,j;
	labMap_t *labptr;
	
	segPublicinfo.objCount=0;
	segPublicinfo.damCount=0;

	labptr=&segPublicinfo.lableMap[0];
	
	for(i=0;i<CFG_ROW;i++)
	{
	   labptr[0].lab=OBJ_INIT;
	   labptr[0].edge=SEG_EDGE_L;
	   labptr[CFG_COL-1].lab=OBJ_INIT;
	   labptr[CFG_COL-1].edge=SEG_EDGE_R;
       for(j=1;j<CFG_COL-1;j++)
       {
          labptr[j].lab=OBJ_INIT;
		  labptr[j].edge=SEG_EDGE_N;	  
	   }

	   labptr+=CFG_COL;
	}

	for(i=0; i<MAX_OBJECTS; i++)
	{
	  segPublicinfo.objLink[i].linksize=0;

	  segPublicinfo.damLink[i].flagbits=0;
	  segPublicinfo.damLink[i].linksize=0;
	}

}

//-----------------------------------------------------------------------------
// Function Name:	segmenting_pushObjLink
// Description:	 push a piexl to a specific object	
//-----------------------------------------------------------------------------
static int16 segmenting_pushObjLink(uint16 objid, uint16 pos, sigData_t* imgarray)
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
static int16 segmenting_pushDamLink(uint16 damflag, uint16 pos, sigData_t* imgarray)
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
static uint16 segmenting_contourbinCal(sigData_t minsig, uint16 contourlevel)
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
static void segmenting_contourSorting(sigData_t * imgarray, uint16 imgsize,
                                                segQue_t* contourarray,uint16 arraysize, 
                                                uint16 contourbin, 
                                                sigData_t minsig)
{
  uint16 i;

  for(i=0;i<imgsize;i++)
  {
    sigData_t sig;
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
static void segmenting_watershedCal(sigData_t * imgarray, uint16 imgsize,
                                             segQue_t * contourarray, uint16 arraysize,
                                             sigData_t minpeak)
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
static uint16 segmenting_ShouldbeMerge(sigData_t* imgarray, uint16 imgsize, uint16 damid,uint16*bloblist)
{
  
  uint16 damflag = segPublicinfo.damLink[damid].flagbits;
  objLink_t* objptr =&segPublicinfo.objLink[0];
  labMap_t * labptr  =&segPublicinfo.lableMap[0];
  uint16 offset;
  uint16 shift;
  uint16 n, idx;
  sigData_t blobpeak,dampeak ;
  
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
static void segmenting_moresegment(sigData_t* imgarray, uint16 imgsize)
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




