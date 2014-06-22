#ifndef RM_MANAGER_H_H
#define RM_MANAGER_H_H

#include "PF_Manager.h"
#include "str.h"
//record manager 
typedef int SlotNum;

typedef enum{
	NO_HINT
}ClientHint;


typedef struct {				//定义存储在文件头的数据
	int nRecords;			//存储当前文件中包含的记录数
	int recordSize;			//存储每个记录的大小
	int recordsPerPage;		//存储一个分页可以装载的记录数
	int firstRecordOffset;	//存储第一个记录的偏移值（字节）
} RM_FileSubHeader;

typedef struct {	
	PageNum pageNum;	//记录所在页的页号
	SlotNum slotNum;		//记录的插槽号
	bool bValid; 			//true表示为一个有效记录的标识符
}RID;

typedef struct{
	bool bValid;		 // False表示还未被读入记录
	RID  rid; 		 // 记录的标识符 
	char *pData; 		 //记录所存储的数据 
}RM_Record;


typedef struct
{
	int bLhsIsAttr,bRhsIsAttr;//左、右是属性（1）还是值（0）
	AttrType attrType;
	int LattrLength,RattrLength;
	int LattrOffset,RattrOffset;
	CompOp compOp;
	void *Lvalue,*Rvalue;
}Con;


typedef struct {
	int nRecords;		//页面当前的记录数
} RM_PageSubHeader;

typedef struct{
	bool bOpen;		//该文件句柄是否已经与一个文件关联
	PF_FileHandle pfFileHandle;	//页面文件操作
	PF_PageHandle pfPageHandle_FileHdr;	//头页面的操作
	PageNum pageNum_FileHdr;	//头页面的页面号
	RM_FileSubHeader *fileSubHeader;	//存储文件子头文件数据的副本
	char *pBitmap;
}RM_FileHandle;

typedef struct{
	bool bOpen;		//扫描是否打开 
	RM_FileHandle *pRMFileHandle;	//指向记录文件操作的指针
	int conNum;		//涉及的条件数量 
	Con *conditions;	//涉及的条件
	ClientHint pinHint;  //用于记录管理组件指定页面的固定策略
    int N;		     // 固定在缓冲区中的页，与指定的页面固定策略有关
    int pinnedPageCount; // 实际固定在缓冲区的页面数
    PF_PageHandle pfPageHandles[PF_BUFFER_SIZE]; // 固定在缓冲区页面所对应的页面操作列表
	int phIx;		//当前被扫描页面的操作索引
	SlotNum snIx;		//下一个被扫描的插槽的插槽号
	PageNum pnLast; 	//上一个固定页面的页面号
	//RM_FileSubHeader rmFileSubHeader; //存储记录文件的文件头的副本
}RM_FileScan;


RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec);

RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions,ClientHint pinHint);

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec);

RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid);

RC InsertRec (RM_FileHandle *fileHandle, char *pData, RID *rid); 

RC GetRec (RM_FileHandle *fileHandle, RID *rid, RM_Record *rec); 

RC RM_CloseFile (RM_FileHandle *fileHandle);

RC RM_OpenFile (char *fileName, RM_FileHandle *fileHandle);

RC RM_CreateFile (char *fileName, int recordSize);

#endif