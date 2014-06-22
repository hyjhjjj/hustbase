#ifndef RM_MANAGER_H_H
#define RM_MANAGER_H_H

#include "PF_Manager.h"
#include "str.h"
//record manager 
typedef int SlotNum;

typedef enum{
	NO_HINT
}ClientHint;


typedef struct {				//����洢���ļ�ͷ������
	int nRecords;			//�洢��ǰ�ļ��а����ļ�¼��
	int recordSize;			//�洢ÿ����¼�Ĵ�С
	int recordsPerPage;		//�洢һ����ҳ����װ�صļ�¼��
	int firstRecordOffset;	//�洢��һ����¼��ƫ��ֵ���ֽڣ�
} RM_FileSubHeader;

typedef struct {	
	PageNum pageNum;	//��¼����ҳ��ҳ��
	SlotNum slotNum;		//��¼�Ĳ�ۺ�
	bool bValid; 			//true��ʾΪһ����Ч��¼�ı�ʶ��
}RID;

typedef struct{
	bool bValid;		 // False��ʾ��δ�������¼
	RID  rid; 		 // ��¼�ı�ʶ�� 
	char *pData; 		 //��¼���洢������ 
}RM_Record;


typedef struct
{
	int bLhsIsAttr,bRhsIsAttr;//���������ԣ�1������ֵ��0��
	AttrType attrType;
	int LattrLength,RattrLength;
	int LattrOffset,RattrOffset;
	CompOp compOp;
	void *Lvalue,*Rvalue;
}Con;


typedef struct {
	int nRecords;		//ҳ�浱ǰ�ļ�¼��
} RM_PageSubHeader;

typedef struct{
	bool bOpen;		//���ļ�����Ƿ��Ѿ���һ���ļ�����
	PF_FileHandle pfFileHandle;	//ҳ���ļ�����
	PF_PageHandle pfPageHandle_FileHdr;	//ͷҳ��Ĳ���
	PageNum pageNum_FileHdr;	//ͷҳ���ҳ���
	RM_FileSubHeader *fileSubHeader;	//�洢�ļ���ͷ�ļ����ݵĸ���
	char *pBitmap;
}RM_FileHandle;

typedef struct{
	bool bOpen;		//ɨ���Ƿ�� 
	RM_FileHandle *pRMFileHandle;	//ָ���¼�ļ�������ָ��
	int conNum;		//�漰���������� 
	Con *conditions;	//�漰������
	ClientHint pinHint;  //���ڼ�¼�������ָ��ҳ��Ĺ̶�����
    int N;		     // �̶��ڻ������е�ҳ����ָ����ҳ��̶������й�
    int pinnedPageCount; // ʵ�ʹ̶��ڻ�������ҳ����
    PF_PageHandle pfPageHandles[PF_BUFFER_SIZE]; // �̶��ڻ�����ҳ������Ӧ��ҳ������б�
	int phIx;		//��ǰ��ɨ��ҳ��Ĳ�������
	SlotNum snIx;		//��һ����ɨ��Ĳ�۵Ĳ�ۺ�
	PageNum pnLast; 	//��һ���̶�ҳ���ҳ���
	//RM_FileSubHeader rmFileSubHeader; //�洢��¼�ļ����ļ�ͷ�ĸ���
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