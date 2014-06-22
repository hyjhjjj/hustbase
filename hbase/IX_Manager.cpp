#include "stdafx.h"
#include "IX_Manager.h"

//���������ļ���1.�����ļ�2.���ͷ��Ϣ��ʼ��
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
	//��ϵͳ�ļ�
	rc = openFile(fileName, &pfFileHandle);
	if (rc != SUCCESS){
		return rc;
	}
	//���·���һҳ
	rc = AllocatePage(&pfFileHandle, &pfPageHandle);
	if (rc != SUCCESS){
		return rc;
	}
	//��ȡ��ҳ����������ַ
	rc = GetData(&pfPageHandle, &pData);
	if (rc != SUCCESS){
		return rc;
	}
	//�����ļ�ͷ�������Ϣ��ʼ��
	ixFileSubHeader = (IX_FileSubHeader *)pData;
	ixFileSubHeader->attrLength = attrLength;			
	ixFileSubHeader->keyLength = attrLength + sizeof(RID);			 //�ؼ��ֳ���ÿ���ؼ��ֳ���������ֵ������Ҫ������rid��ʹ��Ψһ
	ixFileSubHeader->attrType = attrType;					
	ixFileSubHeader->rootPage = 1;									//��ҳ
	ixFileSubHeader->order = (PF_PAGE_SIZE - sizeof(IX_FileSubHeader)-sizeof(IX_Node)) / (2 * sizeof(RID)+attrLength);
	//���ڵ������Ϣ��ʼ��   �����д�������������������������������������������������
	ixnode = (IX_Node *)(pData + sizeof(IX_FileSubHeader));
	ixnode->is_leaf = 1;			//��ʼ��ΪҶ�ڵ�
	ixnode->keynum = 0;				//�ýڵ㺬�еĹؼ��ֵĸ���
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

//��
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

//д    һҳһ�ڵ�

//��ͬͨ����ҳ����ҳ��λͼ����ʾ��ҳ������ͨ���ȽϹؼ������Ƿ����order - 1���ж��Ƿ������Ƿ���Ҫ�֣���
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
	int isfind;//�Ƿ��ҵ������λ��
	RID *ixrid;

	if (indexHandle->bOpen == false){
		return RM_FHCLOSED;
	}//�Ƿ��Ѿ���

	byte = indexHandle->fileSubHeader->rootPage / 8;
	bit = indexHandle->fileSubHeader->rootPage % 8;
	indexHandle->pfFileHandle.pBitmap[byte] |= (1 << bit);//��־��ҳΪ��

	//��ȡ��һҳ��Ϣ����ȡ��������ͷ��Ϣ
	rc = GetThisPage(&(indexHandle->pfFileHandle), 1, &pfPageHandle);
	if (rc != SUCCESS){
		return rc;
	}
	rc = GetData(&pfPageHandle, &pdata);
	if (rc != SUCCESS){
		return rc;
	}
	ixFileSubHeader = (IX_FileSubHeader *)(pdata + sizeof(int));
	
	
	//������ͷ��Ϣ�еõ���ҳҳ�ţ��Ӹ��ڵ㿪ʼ����
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
	//����ڵ����ˣ��ֽڵ�Ϊ����
	if (ixnode->keynum >= ixFileSubHeader->order - 1){
		splitPage(indexHandle,pData,rid);//��Ҫʵ�֡���������������������������������������
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

			}//chars����float����int���ƣ������
		}
		if (isfind == 1){
			ixnode->keynum++;  //��������������������Ӽ��𣿣���
			break;
		}
	}
	if (ixnode->is_leaf == 0){   //�������⣡������
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

	//������ �޸���ص�ͷ��Ϣ  �����Ļ��޷Ǿ��Ƿ�ҳ����ҳ����������ҳ
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

	//ÿ�ζ�Ҫ��ȡ��һҳ����ȡ������������Ϣ
	rc = GetThisPage(&(indexHandle->pfFileHandle), 1, &pfPageHandle);
	if (rc != SUCCESS){
		return NULL;
	}
	rc = GetData(&pfPageHandle, &pdata);
	if (rc != SUCCESS){
		return NULL;
	}
	ixFileSubHeader = (IX_FileSubHeader *)(pdata + sizeof(int));

	//��һ�����ҵ�һ����ҳ���ҵ��Ļ���ȡҳ�ţ�δ�ҵ��Ļ������·���һҳ���ȡҳ��
	for (; pageNum <= pageCount; pageNum++){
		byte = pageNum / 8;
		bit = pageNum % 8;
		if ((indexHandle->pfFileHandle.pBitmap[byte] & (1 << bit)) != 0){
			break;
		}//�ҵ�һ����ҳ
	}

	if (pageNum>pageCount){//δ�ҵ��Ļ����·���һҳ
		rc = AllocatePage(&(indexHandle->pfFileHandle), &pfPageHandle);
		if (rc != SUCCESS){
			return NULL;
		}
		rc = GetPageNum(&pfPageHandle, &pageNum);//��ȡҳ��
		if (rc != SUCCESS){
			return NULL;
		}
	}
	else{		//�ҵ��Ļ�����ȡҳ���
		rc = GetThisPage(&(indexHandle->pfFileHandle), pageNum, &pfPageHandle);
		if (rc != SUCCESS){
			return NULL;
		}
	}
	//��ȡ��һ����ҳ�󣬱�־��ҳΪ�ǿ���
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
	


	//int mid = p->data[DU - 1];            //mid�м�ڵ�
	//����һҳ�������г�ʼ��

	return NULL;
	//page* right = pm();                 //�½�right�ڵ�
	//int index = DU;
	//int i = 0;
	//for (; index<DU * 2 - 1; index++){       //��p�ڵ�Ĺؼ��ֺ��ӽڵ�ָ���ƶ���right�ڵ���
	//	right->data[i] = p->data[index];
	//	p->data[index] = 0;               //�ѵ�ǰλ����Ϊ��Ч
	//	right->child[i] = p->child[index];
	//	p->child[index] = NULL;           //�ѵ�ǰλ����Ϊ��Ч
	//	if (right->child[i] != NULL)
	//		right->child[i]->parent = right;//��������p�ڵ��ӽڵ�ĸ��ڵ�
	//	i++;
	//}
	//right->child[DU - 1] = p->child[2 * DU - 1];
	//if (right->child[DU - 1] != NULL)
	//	right->child[DU - 1]->parent = right;
	//p->child[2 * DU - 1] = NULL;
	//if (p->isLeaf)                    //�����Ҷ�ӽڵ�����p�ڵ���ֵܽڵ����right�ڵ�
	//	p->sibling = right;
	//page* result = p;
	//right->isLeaf = p->isLeaf;
	//if (val>mid)
	//	result = right;
	//int isNew = 0;
	//if (p->parent == NULL){             //���p�ڵ���Ǹ��ڵ㣬�½�root�ڵ�
	//	root = pm();
	//	p->parent = root;
	//	isNew = 1;
	//	root->isLeaf = 0;
	//}
	//page* parent = p->parent;
	//int first = 0;
	//for (; first<2 * DU - 1; first++){     //Ѱ��p�ڵ㸸�ڵ����mid�ĺ���λ��
	//	if (parent->data[first]>mid || parent->data[first] == 0)
	//		break;
	//}
	//index = 2 * DU - 3;
	//for (; index >= first; index--){     //p�ڵ����Ʋ���
	//	parent->data[index + 1] = parent->data[index];
	//	parent->child[index + 2] = parent->child[index + 1];
	//}
	//parent->data[first] = mid;
	//parent->child[first + 1] = right;
	//if (isNew){
	//	parent->child[0] = p;
	//}
	//else if (first + 2 <= 2 * DU - 1 && parent->child[first + 2] != NULL){
	//	right->sibling = parent->child[first + 2];  //��right�ڵ���ֵܽڵ�����Ϊԭ�ڵ���ֵܽڵ�
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
}//�ҵ�һ����ҳ
}

if(pageNum>pageCount){//δ�ҵ��Ļ����·���һҳ
rc=AllocatePage(&(indexHandle->pfFileHandle),&pfPageHandle);
if(rc!=SUCCESS){
return rc;
}
rc=GetPageNum(&pfPageHandle,&pageNum);//��ȡҳ��
if(rc!=SUCCESS){
return rc;
}
}
else{		//�ҵ��Ļ�����ȡҳ���
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