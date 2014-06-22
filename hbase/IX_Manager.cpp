#include "stdafx.h"
#include "IX_Manager.h"

//创建索引文件：1.创建文件2.相关头信息初始化
RC IX_CreateFile(char *fileName, AttrType attrType, int attrLength){
	RC rc;
	PF_FileHandle pfFileHandle;
	PF_PageHandle pfPageHandle;
	char *pData;
	IX_FileSubHeader *ixFileSubHeader;
	IX_Node *ixnode;
	rc = CreateFile(fileName);
	if (rc != SUCCESS){
		return rc;
	}
	//打开系统文件
	rc = openFile(fileName, &pfFileHandle);
	if (rc != SUCCESS){
		return rc;
	}
	//重新分配一页
	rc = AllocatePage(&pfFileHandle, &pfPageHandle);
	if (rc != SUCCESS){
		return rc;
	}
	//获取这页的数据区首址
	rc = GetData(&pfPageHandle, &pData);
	if (rc != SUCCESS){
		return rc;
	}
	//索引文件头的相关信息初始化
	ixFileSubHeader = (IX_FileSubHeader *)pData;
	ixFileSubHeader->attrLength = attrLength;			
	ixFileSubHeader->keyLength = attrLength + sizeof(RID);			 //关键字长，每个关键字长除了他的值本身，还要加上其rid来使他唯一
	ixFileSubHeader->attrType = attrType;					
	ixFileSubHeader->rootPage = 1;									//根页
	ixFileSubHeader->order = (PF_PAGE_SIZE - sizeof(IX_FileSubHeader)-sizeof(IX_Node)) / (2 * sizeof(RID)+attrLength);
	//根节点相关信息初始化   好像有错啊！！！！！！！！！！！！！！！！！！！！！！！！
	ixnode = (IX_Node *)(pData + sizeof(IX_FileSubHeader));
	ixnode->is_leaf = 1;			//初始化为叶节点
	ixnode->keynum = 0;				//该节点含有的关键字的个数
	ixnode->keys = (char *)(pData + sizeof(int) + sizeof(IX_FileSubHeader) + sizeof(IX_Node));
	ixnode->rids = (RID *)(pData + sizeof(int) + sizeof(IX_FileSubHeader) + sizeof(IX_Node)
								+ ixFileSubHeader->keyLength * (ixFileSubHeader->order - 1));
	ixnode->parent = 0;
	memset(pData + sizeof(IX_FileSubHeader) + sizeof(IX_Node), 0, PF_PAGE_SIZE - sizeof(IX_FileSubHeader) - sizeof(IX_Node));
	rc = MarkDirty(&pfPageHandle);
	if (rc != SUCCESS){
		return rc;
	}
	rc = UnpinPage(&pfPageHandle);
	if (rc != SUCCESS){
		return rc;
	}
	rc = CloseFile(&pfFileHandle);
	if (rc != SUCCESS){
		return rc;
	}
	return SUCCESS;
}

RC IX_OpenFile(char *fileName, IX_FileHandle *indexHandle){
	char *pData;
	if (indexHandle->bOpen){
		return RM_FHOPENNED;
	}
	RC rc;
	rc = openFile(fileName, &(indexHandle->pfFileHandle));
	if (rc != SUCCESS){
		return rc;
	}
	rc = GetThisPage(&(indexHandle->pfFileHandle), 1, &(indexHandle->pfPageHandle_FileHdr));
	if (rc != SUCCESS){
		return rc;
	}
	rc = GetData(&(indexHandle->pfPageHandle_FileHdr), &pData);
	if (rc != SUCCESS){
		return rc;
	}
	indexHandle->fileSubHeader = (IX_FileSubHeader *)pData;
	indexHandle->pageNum_FileHdr = 1;
	indexHandle->bOpen = true;
	return SUCCESS;
}

RC IX_CloseFile(IX_FileHandle *indexHandle){
	RC rc;
	if (indexHandle->bOpen == false){
		return RM_FHCLOSED;
	}
	rc = UnpinPage(&(indexHandle->pfPageHandle_FileHdr));
	if (rc != SUCCESS){
		return rc;
	}
	rc = CloseFile(&(indexHandle->pfFileHandle));
	if (rc != SUCCESS){
		return rc;
	}
	indexHandle->bOpen = false;
	return SUCCESS;
}

//读
RC OpenIndexScan(IX_IndexScan *indexScan, IX_FileHandle *indexhandle, CompOp compOp, char *value, ClientHint pinHint){
	//if(indexScan->bOpen){
	//	return RM_FSOPEN;
	//}
	//indexScan->pRMFileHandle=fileHandle;
	//indexScan->conNum=conNum;
	//indexScan->pinHint=pinHint;
	//if(conNum==0)
	//	indexScan->conditions=NULL;
	//else
	//{
	//	indexScan->conditions=(Con *)malloc(conNum*sizeof(Con));
	//	memcpy(indexScan->conditions,conditions,conNum*sizeof(Con));
	//}
	//indexScan->pinnedPageCount=0;
	//indexScan->phIx=0;
	//indexScan->snIx=0;
	//indexScan->pnLast=1;
	//indexScan->bOpen=true;
	return SUCCESS;
}

RC IX_GetNextEntry(IX_IndexScan *indexScan, RID *rid){
	//if(indexScan->bOpen==false){
	//	return RM_FSCLOSED;
	//}
	//RC rc;
	//while(GetNextRecInMem(indexScan,rec)==RM_NOMORERECINMEM)
	//{
	//	rc=FetchPages(indexScan);
	//	if(rc!=SUCCESS){
	//		return rc;
	//}
 //}
 return SUCCESS;
}

//写    一页一节点

//不同通过分页控制页的位图来表示满页，而是通过比较关键字数是否等于order - 1来判断是否满，是否需要分！！
RC InsertEntry(IX_FileHandle *indexHandle, void *pData, RID *rid){
	int byte,bit,offset;
	RC rc;
	char *pdata;
	PF_PageHandle pfPageHandle;
	IX_FileSubHeader *ixFileSubHeader;
	IX_Node *ixnode;
	char *keytmp;
	RID *ridtmp;
	int index;
	int intvalue, inttmp;
	char *charvalue, *chartmp;
	float floatvalue, floattmp;
	int isfind;//是否找到插入的位置
	RID *ixrid;

	if (indexHandle->bOpen == false){
		return RM_FHCLOSED;
	}//是否已经打开

	byte = indexHandle->fileSubHeader->rootPage / 8;
	bit = indexHandle->fileSubHeader->rootPage % 8;
	indexHandle->pfFileHandle.pBitmap[byte] |= (1 << bit);//标志该页为满

	//读取第一页信息，提取整个索引头信息
	rc = GetThisPage(&(indexHandle->pfFileHandle), 1, &pfPageHandle);
	if (rc != SUCCESS){
		return rc;
	}
	rc = GetData(&pfPageHandle, &pdata);
	if (rc != SUCCESS){
		return rc;
	}
	ixFileSubHeader = (IX_FileSubHeader *)(pdata + sizeof(int));
	
	
	//从索引头信息中得到根页页号，从根节点开始插入
	rc = GetThisPage(&(indexHandle->pfFileHandle), ixFileSubHeader->rootPage, &pfPageHandle);
	if (rc != SUCCESS){
		return rc;
	}
	rc = GetData(&pfPageHandle, &pdata);
	if (rc != SUCCESS){
		return rc;
	}
	ixnode = (IX_Node *)(pdata + sizeof(int)+sizeof(IX_FileSubHeader));
	keytmp = ixnode->keys;
	ridtmp = ixnode->rids;
	//如果节点满了，分节点为两个
	if (ixnode->keynum >= ixFileSubHeader->order - 1){
		splitPage(indexHandle,pData,rid);//需要实现。。。。。。。。。。。。。。。。。。。。
	}
	for (index = 0; index < ixnode->keynum; index++){
		if (ixFileSubHeader->attrType == ints){
			memcpy(&intvalue, keytmp + index * ixFileSubHeader->keyLength + sizeof(RID), ixFileSubHeader->attrLength);
			memcpy(&inttmp, pData, ixFileSubHeader->attrLength);
			if (intvalue > inttmp){
				isfind = 1;
			}
			else if (intvalue == inttmp){
				ixrid = (RID *)malloc(sizeof(RID));
				memcpy(ixrid, keytmp + index * ixFileSubHeader->keyLength, sizeof(RID));
				if (ixrid->pageNum > rid->pageNum){
					isfind = 1;
				}
				else if (rid->pageNum == ixrid->pageNum){
					if (ixrid->slotNum > rid->slotNum){
						isfind = 1;
					}
					else{
						index += 1;
						isfind = 1;
					}
				}
				else {
					index += 1;
					isfind = 1;
				}
			}
			else
				isfind = 0;
		}
		else if (ixFileSubHeader->attrType == chars){
			charvalue = (char *)malloc(ixFileSubHeader->attrLength);
			chartmp = (char *)malloc(ixFileSubHeader->attrLength);
			memcpy(charvalue, keytmp + index*ixFileSubHeader->keyLength + sizeof(RID), ixFileSubHeader->attrLength);
			memcpy(chartmp, pData, ixFileSubHeader->attrLength);
			if (charvalue > chartmp){

			}//chars型与float型与int类似，待完成
		}
		if (isfind == 1){
			ixnode->keynum++;  //？？？？？？？是在这加加吗？？？
			break;
		}
	}
	if (ixnode->is_leaf == 0){   //这有问题！！！！
		memcpy(ixrid, keytmp + index * ixFileSubHeader->keyLength, sizeof(RID));
		indexHandle->fileSubHeader->rootPage = ixrid->pageNum;
		InsertEntry(indexHandle, pData, rid);
	}
	if (ixnode->keynum > 0){
		for (int i = ixnode->keynum - 1; i > index; i--){
			memcpy(keytmp + i * ixFileSubHeader->keyLength, keytmp + (i - 1) * ixFileSubHeader->keyLength, ixFileSubHeader->keyLength);
		}
	}
	memcpy(keytmp + index * ixFileSubHeader->keyLength, rid, sizeof(RID));
	memcpy(keytmp + index * ixFileSubHeader->keyLength + sizeof(RID),pData,ixFileSubHeader->attrLength);

	//第三步 修改相关的头信息  索引的话无非就是分页控制页，索引控制页
	rc=GetData(&pfPageHandle,&pdata);
	if(rc!=SUCCESS){
		return rc;
	}
	
	rc=MarkDirty(&pfPageHandle);
	if(rc!=SUCCESS){
		return rc;
	}
	rc=UnpinPage(&pfPageHandle);
	if(rc!=SUCCESS){
		return rc;
	}
	rc = MarkDirty(&(indexHandle->pfPageHandle_FileHdr));
	if(rc!=SUCCESS){
		return rc;	
	}
	return SUCCESS;
}

RC DeleteEntry(IX_FileHandle *indexHandle, void *pData, RID *rid){
	//int byte,bit;
	//char *pBitmap;
	//RC rc;
	//if (indexHandle->bOpen == false){
	//	return RM_FHCLOSED;
	//}

	//PageNum pageNum=rid->pageNum;
	//SlotNum slotNum=rid->slotNum;
	//char *pData;
	//RM_PageSubHeader *rmPageSubHeader;
	//PF_PageHandle pfPageHandle;

	//rc = GetThisPage(&(indexHandle->pfFileHandle), pageNum, &pfPageHandle);
	//if(rc!=SUCCESS){
	//	return rc;
	//}
	//rc=GetData(&pfPageHandle,&pBitmap);
	//if(rc!=SUCCESS){
	//	return rc;
	//}
	//pBitmap+=sizeof(RM_PageSubHeader);
	//byte=slotNum/8;
	//bit=slotNum%8;
	//if((pBitmap[byte]&(1<<bit))==0){
	//	return RM_INVALIDRID;
	//}
	//pBitmap[byte]&=~(1<<bit);
	//rc=GetData(&pfPageHandle,&pData);
	//if(rc!=SUCCESS){
	//	return rc;
	//}
	//rmPageSubHeader=(RM_PageSubHeader *)pData;
	//rmPageSubHeader->nRecords--;
	//indexHandle->fileSubHeader->nRecords--;
	//byte=pageNum/8;
	//bit=pageNum%8;
	//indexHandle->pBitmap[byte] &= ~(1 << bit);
	//rc=MarkDirty(&pfPageHandle);
	//if(rc!=SUCCESS){
	//	return rc;
	//}
	//rc=UnpinPage(&pfPageHandle);
	//if(rc!=SUCCESS){
	//	return rc;
	//}
	//rc = MarkDirty(&(indexHandle->pfPageHandle_FileHdr));
	//if(rc!=SUCCESS){
	//	return rc;
	//}
	return SUCCESS;
}

IX_FileHandle * splitPage(IX_FileHandle *indexHandle, void *pData, RID *rid){
	PageNum pageNum = indexHandle->pageNum_FileHdr;
	PageNum pageCount = indexHandle->pfFileHandle.pFileSubHeader->pageCount;
	int byte, bit, offset;
	RC rc;
	char *pdata, *pnode;
	PF_PageHandle pfPageHandle;
	IX_FileSubHeader *ixFileSubHeader;
	IX_Node *ixnode;
	char *keytmp, *ridtmp;
	char *keymid;
	RID *ridmid;
	RID *ixrid;

	//每次都要读取第一页，提取整个索引的信息
	rc = GetThisPage(&(indexHandle->pfFileHandle), 1, &pfPageHandle);
	if (rc != SUCCESS){
		return NULL;
	}
	rc = GetData(&pfPageHandle, &pdata);
	if (rc != SUCCESS){
		return NULL;
	}
	ixFileSubHeader = (IX_FileSubHeader *)(pdata + sizeof(int));

	//第一步，找到一空闲页，找到的话获取页号，未找到的话，重新分配一页后获取页号
	for (; pageNum <= pageCount; pageNum++){
		byte = pageNum / 8;
		bit = pageNum % 8;
		if ((indexHandle->pfFileHandle.pBitmap[byte] & (1 << bit)) != 0){
			break;
		}//找到一空闲页
	}

	if (pageNum>pageCount){//未找到的话重新分配一页
		rc = AllocatePage(&(indexHandle->pfFileHandle), &pfPageHandle);
		if (rc != SUCCESS){
			return NULL;
		}
		rc = GetPageNum(&pfPageHandle, &pageNum);//获取页号
		if (rc != SUCCESS){
			return NULL;
		}
	}
	else{		//找到的话，获取页句柄
		rc = GetThisPage(&(indexHandle->pfFileHandle), pageNum, &pfPageHandle);
		if (rc != SUCCESS){
			return NULL;
		}
	}
	//获取到一空闲页后，标志该页为非空闲
	byte = pageNum / 8;
	bit = pageNum % 8;
	indexHandle->pfFileHandle.pBitmap[byte] |= (1 << bit);

	rc = GetData(&pfPageHandle, &pnode);
	if (rc != SUCCESS){
		return NULL;
	}
	ixnode = (IX_Node *)(pnode + sizeof(int)+sizeof(IX_FileSubHeader));
	keytmp = (char *)(pnode + sizeof(int)+sizeof(IX_FileSubHeader)+sizeof(IX_Node));
	ridtmp = (char *)(keytmp + ixnode->keynum * ixFileSubHeader->keyLength);
	
	memcpy(ridmid, keytmp + (ixFileSubHeader->order - 1) / 2 * ixFileSubHeader->keyLength, sizeof(RID));
	memcpy(keymid, keytmp + (ixFileSubHeader->order - 1) / 2 * ixFileSubHeader->keyLength + sizeof(RID), ixFileSubHeader->attrLength);
	


	//int mid = p->data[DU - 1];            //mid中间节点
	//申请一页，并进行初始化

	return NULL;
	//page* right = pm();                 //新建right节点
	//int index = DU;
	//int i = 0;
	//for (; index<DU * 2 - 1; index++){       //把p节点的关键字和子节点指针移动到right节点上
	//	right->data[i] = p->data[index];
	//	p->data[index] = 0;               //把当前位置置为无效
	//	right->child[i] = p->child[index];
	//	p->child[index] = NULL;           //把当前位置置为无效
	//	if (right->child[i] != NULL)
	//		right->child[i]->parent = right;//重新设置p节点子节点的父节点
	//	i++;
	//}
	//right->child[DU - 1] = p->child[2 * DU - 1];
	//if (right->child[DU - 1] != NULL)
	//	right->child[DU - 1]->parent = right;
	//p->child[2 * DU - 1] = NULL;
	//if (p->isLeaf)                    //如果是叶子节点设置p节点的兄弟节点就是right节点
	//	p->sibling = right;
	//page* result = p;
	//right->isLeaf = p->isLeaf;
	//if (val>mid)
	//	result = right;
	//int isNew = 0;
	//if (p->parent == NULL){             //如果p节点就是根节点，新建root节点
	//	root = pm();
	//	p->parent = root;
	//	isNew = 1;
	//	root->isLeaf = 0;
	//}
	//page* parent = p->parent;
	//int first = 0;
	//for (; first<2 * DU - 1; first++){     //寻找p节点父节点插入mid的合适位置
	//	if (parent->data[first]>mid || parent->data[first] == 0)
	//		break;
	//}
	//index = 2 * DU - 3;
	//for (; index >= first; index--){     //p节点右移操作
	//	parent->data[index + 1] = parent->data[index];
	//	parent->child[index + 2] = parent->child[index + 1];
	//}
	//parent->data[first] = mid;
	//parent->child[first + 1] = right;
	//if (isNew){
	//	parent->child[0] = p;
	//}
	//else if (first + 2 <= 2 * DU - 1 && parent->child[first + 2] != NULL){
	//	right->sibling = parent->child[first + 2];  //把right节点的兄弟节点设置为原节点的兄弟节点
	//}
	//right->parent = parent;
	//return result;

}
/*
for(;pageNum<=pageCount;pageNum++){
byte=pageNum/8;
bit=pageNum%8;
if ((indexHandle->pfFileHandle.pBitmap[byte] & (1 << bit)) != 0 ){
break;
}//找到一空闲页
}

if(pageNum>pageCount){//未找到的话重新分配一页
rc=AllocatePage(&(indexHandle->pfFileHandle),&pfPageHandle);
if(rc!=SUCCESS){
return rc;
}
rc=GetPageNum(&pfPageHandle,&pageNum);//获取页号
if(rc!=SUCCESS){
return rc;
}
}
else{		//找到的话，获取页句柄
rc = GetThisPage(&(indexHandle->pfFileHandle), pageNum, &pfPageHandle);
if(rc!=SUCCESS){
return rc;
}
}
rc = GetData(&pfPageHandle, &pnode);
if (rc != SUCCESS){
return rc;
}
*/