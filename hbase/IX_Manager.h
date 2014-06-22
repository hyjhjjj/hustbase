#ifndef IX_MANAGER_H_H
#define IX_MANAGER_H_H
#include "RM_Manager.h"
#include "PF_Manager.h"
typedef struct{
	int attrLength;    //建立索引的属性值的长度
	int keyLength;    //B+树中关键字的长度
	AttrType attrType;   //建立索引的属性值的类型
	PageNum rootPage;   //B+树根节点的页面号
	int order;     //B+树的阶数
}IX_FileSubHeader;

typedef struct{
	int is_leaf;          //该节点是否为叶子节点
	int keynum;           //该节点实际包含的关键字个数
	PageNum parent;       //指向父节点所在的页面号
	char *keys;			  //指向关键字的指针
	RID *rids;            //指向记录标识符的指针
}IX_Node;

typedef struct{
	bool bOpen;                         //该文件句柄是否已经与一个文件关联
	PF_FileHandle pfFileHandle;         //页面文件操作
	PF_PageHandle pfPageHandle_FileHdr; //头页面的操作
	PageNum pageNum_FileHdr;            //头页面的页面号
	IX_FileSubHeader *fileSubHeader;    //存储文件子头文件数据的副本
}IX_FileHandle;

typedef struct{
	bool bOpen;                                     //扫描是否打开
	IX_FileHandle *pIXIndexHandle;                  //指向索引操作的指针
	CompOp compOp;                                  //用于比较的操作符
	char *value;                                    //与属性值进行比较的值
	ClientHint pinHint;                             //用于记录管理组件指定页面的固定策略
	int N;                                          // 固定在缓冲区中的页，与指定的页面固定策略有关
	int pinnedPageCount;                            // 实际固定在缓冲区的页面数
	PF_PageHandle pfPageHandles[PF_BUFFER_SIZE];    // 固定在缓冲区页面所对应的页面句柄数组
	int phIx;                                       //当前被扫描页面的页面句柄索引
	int ridIx;
	PageNum pnNext;      //下一个将被写入缓冲的页面号
}IX_IndexScan;

RC IX_CreateFile(char *fileName, AttrType attrType, int attrLength);
RC IX_OpenFile(char *fileName, IX_FileHandle *indexHandle);
RC IX_CloseFile(IX_FileHandle *indexHandle);
//上述已完成

//扫描
RC OpenIndexScan(IX_IndexScan *indexScan, IX_FileHandle *indexhandle, CompOp compOp, char *value, ClientHint pinHint);
RC IX_GetNextEntry(IX_IndexScan *indexScan, RID *rid);

//插入与删除
RC InsertEntry(IX_FileHandle *indexHandle, void *pData, RID *rid);
RC DeleteEntry(IX_FileHandle *indexHandle, void *pData, RID *rid);

//分裂函数
IX_FileHandle * splitPage(IX_FileHandle *indexHandle, void *pData, RID *rid);//分页,只有这个函数跟root有直接关系？

#endif