#include "stdafx.h"
#include "SYS_Manager.h"

	//select���ܵ�ȫ�־�̬����
static int *_recordcount;//�������ݱ�����ļ�¼����
static int tablecount; //����
static int counterIndex;
static int *counter;
typedef struct printrel{
	int i;	//�������Զ�Ӧ������Ӧ���±�
	SysColumn col;//����������Ϣ
}printrel;

//done
RC CreateDb(char *dbPath){
	//���õ�ǰĿ¼Ϊdbpath������dbPath������dbName��
	SetCurrentDirectory(dbPath);
	RC rc;
	//����ϵͳ���ļ���ϵͳ���ļ�
	rc = RM_CreateFile("SYSTABLES", sizeof(SysTable));
	if (rc != SUCCESS)
		return rc;
	rc = RM_CreateFile("SYSCOLUMNS", sizeof(SysColumn));
	if (rc != SUCCESS)
		return rc;
	return SUCCESS;
}
//done

RC DropDb(char *dbName){
	CFileFind tempFind;
	char sTempFileFind[200];
	sprintf_s(sTempFileFind, "%s\\*.*", dbName);
	BOOL IsFinded = tempFind.FindFile(sTempFileFind);
	while (IsFinded)
	{
		IsFinded = tempFind.FindNextFile();
		if (!tempFind.IsDots())
		{
			char sFoundFileName[200];
			strcpy_s(sFoundFileName, tempFind.GetFileName().GetBuffer(200));
			if (tempFind.IsDirectory())
			{
				char sTempDir[200];
				sprintf_s(sTempDir, "%s\\%s", dbName, sFoundFileName);
				DropTable(sTempDir);
			}
			else
			{
				char sTempFileName[200];
				sprintf_s(sTempFileName, "%s\\%s", dbName, sFoundFileName);
				DeleteFile(sTempFileName);
			}
		}
	}
	tempFind.Close();
	if (!RemoveDirectory(dbName))
	{
		return FAIL;
	}
	return SUCCESS;
}
//done
RC OpenDb(char *dbName){
	return SUCCESS;
}

RC CloseDb(){
	return SUCCESS;
}
/*
���������
1.ϵͳ���ļ���ϵͳ���ļ��ĳ�ʼ��
2.�������ݱ��ļ�
*/
//done
RC CreateTable(char *relName, int attrCount, AttrInfo *attributes){
	RC rc;
	char  *pData;
	RM_FileHandle *rm_table, *rm_column;
	RID *rid;
	int recordsize;//���ݱ��ÿ����¼�Ĵ�С
	AttrInfo * attrtmp = attributes;

	//��ϵͳ���ļ���ϵͳ���ļ�
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	rc = RM_OpenFile("SYSTABLES", rm_table);
	if (rc != SUCCESS)
		return rc;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS)
		return rc;
	//��ϵͳ���������Ϣ
	pData = (char *)malloc(sizeof(SysTable));
	memcpy(pData, relName,21);//������
	memcpy(pData + 21, &attrCount, sizeof(int));//�����������
	rid = (RID *)malloc(sizeof(RID));
	rid->bValid = false;
	rc = InsertRec(rm_table, pData, rid);
	if (rc != SUCCESS){
		return rc;
	}
	rc = RM_CloseFile(rm_table);
	if (rc != SUCCESS){
		return rc;
	}
	free(rm_table);
	free(pData);
	free(rid);//�ͷ�������ڴ�ռ�

	//��ϵͳ����ѭ�������Ϣ һ�����а�����������У�����Ҫѭ��
	for (int i = 0,offset = 0; i < attrCount; i++, attrtmp++){
		pData = (char *)malloc(sizeof(SysColumn));
		memcpy(pData, relName,21);
		memcpy(pData + 21, attrtmp->attrName,21);
		memcpy(pData + 42, &(attrtmp->attrType), sizeof(AttrType));
		memcpy(pData + 42 + sizeof(AttrType), &(attrtmp->attrLength), sizeof(int));
		memcpy(pData + 42 + sizeof(int) + sizeof(AttrType), &offset, sizeof(int));
		memcpy(pData + 42 + 2 * sizeof(int)+sizeof(AttrType), "0", sizeof(char));
		rid = (RID *)malloc(sizeof(RID));
		rid->bValid = false;
		rc = InsertRec(rm_column, pData, rid);
		if (rc != SUCCESS){
			return rc;
		}
		free(pData);
		free(rid);
		offset += attrtmp->attrLength;
	}
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS){
		return rc;
	}
	free(rm_column);
	//�������ݱ�
	//����recordsize�Ĵ�С
	recordsize = 0;
	attrtmp = attributes;
	for (int i = 0; i < attrCount; i++, attrtmp++){
		recordsize += attrtmp->attrLength;
	}
	rc = RM_CreateFile(relName, recordsize);
	if (rc != SUCCESS)
		return rc;
	return SUCCESS;
}


//ɾ�����
//1.�ѱ���ΪrelName�ı�ɾ�� 
//2.�����ݿ��иñ��Ӧ����Ϣ�ÿ�
RC DropTable(char *relName){
	CFile tmp;
	RM_FileHandle *rm_table, *rm_column;
	RC rc;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	int attrcount;//��ʱ �������������Գ��ȣ�����ƫ��

	//ɾ�����ݱ��ļ�
	tmp.Remove((LPCTSTR)relName);
	
	//��ϵͳ���ϵͳ���ж�Ӧ�����ؼ�¼ɾ��
	//��ϵͳ��ϵͳ���ļ�
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	rc = RM_OpenFile("SYSTABLES", rm_table);
	if (rc != SUCCESS)
		return rc;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS)
		return rc;

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������rectab��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			DeleteRec(rm_table, &(rectab.rid));
			break;
		}
	}
	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������reccol��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	//����֮ǰ��ȡ��ϵͳ������Ϣ����ȡ������Ϣ�����������ctmp��
	for (int i = 0; i < attrcount && GetNextRec(&FileScan, &reccol) == SUCCESS;){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			DeleteRec(rm_column, &(reccol.rid));
			i++;
		}
	}

	//�ر��ļ����
	rc = RM_CloseFile(rm_table);
	if (rc != SUCCESS)
		return rc;
	free(rm_table);
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS)
		return rc;
	free(rm_column);
	return SUCCESS;
}

//����������
//1.���������ļ�
//2.��ϵͳ���й���������������Ϣ�ĸ��� 
//3.��ȡ������Ӧ�ı������ݣ�������Щ���ݰ���b+����ʽ����д�������ļ��� 
//4.�ر�����ļ�
RC CreateIndex(char *indexName, char *relName, char *attrName){
	RM_FileHandle  *rm_table, *rm_column,*rm_data;
	RC rc;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	SysColumn *column, *cindex;
	IX_FileHandle *indexhandle;
	char *data;
	int attrcount;//��ʱ ��������
	
	//��ϵͳ��ϵͳ���ļ�
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	rc = RM_OpenFile("SYSTABLES", rm_table);
	if (rc != SUCCESS)
		return rc;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS)
		return rc;
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	rc = RM_OpenFile(relName, rm_data);
	if (rc != SUCCESS)
		return rc;

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������rectab��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������reccol��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	column = (SysColumn *)malloc(attrcount*sizeof(SysColumn));
	cindex = (SysColumn *)malloc(sizeof(SysColumn));
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++, column++){
				memcpy(column->tabName, reccol.pData, 21);
				memcpy(column->attrName, reccol.pData + 21, 21);
				if ((strcmp(relName, column->tabName) == 0) && (strcmp(attrName, column->attrName) == 0)){
					cindex = column;
					//�ҵ�Ҫ����������Ӧ������,�������Ϣ������cindex��
					break;
				}
				rc = GetNextRec(&FileScan, &reccol);
				if (rc != SUCCESS)
					break;
			}
			break;
		}
	}

	//��һ����ɴ��������ļ�
	IX_CreateFile(indexName, cindex->attrType, cindex->attrLength);
	
	//�ڶ�����ɶ�ϵͳ���й���������������Ϣ�ĸ���
	memcpy(&(cindex->ix_flag), "1", sizeof(char));
	memcpy(cindex->ixName, indexName, 21);
	memcpy(&reccol, cindex, sizeof(SysColumn));
	UpdateRec(rm_column, &reccol);
	
	//��������ȡ������Ӧ�ı������ݣ�������Щ���ݰ���b+����ʽ����д�������ļ���
	//��һ����һ������һ����ֱ��ȡ����������ɲ���
	indexhandle = (IX_FileHandle *)malloc(sizeof(IX_FileHandle));
	rc = IX_OpenFile(indexName, indexhandle);
	if(rc != SUCCESS){
		return rc;
	}//�������ļ�

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������reccol��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_data, 0, NULL, NO_HINT);
	if (rc != SUCCESS){
		AfxMessageBox("��ʼ�����ݱ��ļ�ɨ��ʧ��");
		return rc;
	}
	column = (SysColumn *)malloc(sizeof(SysColumn));
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		data = (char *)malloc(cindex->attrLength);
		memcpy(data, reccol.pData + cindex->attrOffset, cindex->attrLength);
		InsertEntry(indexhandle, data, &(reccol.rid));
	}
	
	//���Ĳ����ر��ļ�����ɴ���
	rc = IX_CloseFile(indexhandle);
	if (rc != SUCCESS)
		return rc;
	rc = RM_CloseFile(rm_table);
	if (rc != SUCCESS)
		return rc;
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS)
		return rc;
	rc = RM_CloseFile(rm_data);
	if (rc != SUCCESS)
		return rc;
	return SUCCESS;
}

RC DropIndex(char *indexName){
	CFile tmp;
	RM_FileHandle *rm_table, *rm_column;
	SysColumn *column;
	RC rc;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	char index[21];
	
	//��һ��ɾ�������ļ�
	tmp.Remove((LPCTSTR)indexName);

	//�ڶ����޸�ϵͳ���й�����������Ϣ
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS){
		AfxMessageBox("��ϵͳ���ļ�ʧ��");
		return rc;
	}
	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������reccol��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS){
		AfxMessageBox("��ʼ���������ļ�ɨ��ʧ��");
		return rc;
	}
	column = (SysColumn *)malloc(sizeof(SysColumn));
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		memcpy(index, reccol.pData + 43 + 2 * sizeof(int) + sizeof(AttrType), 21);//�ҵ�����ϵͳ���й���������������¼
		if (strcmp(index, indexName) == 0){
			memcpy(reccol.pData + 42 + 2 * sizeof(int)+sizeof(AttrType), "0", 1);//��ix_flag��Ϊ0��
			rc = UpdateRec(rm_column, &reccol);
			if ( rc != SUCCESS){
				return rc;
			}
		}
	}

	// ������ �ر��ļ������ɾ��
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS)
		return rc;
	return SUCCESS;
}

/*
����ʵ��˼�룺
ͨ����ȡϵͳ���еı���Ϣ�����������Ϣ��
Ȼ�������Щ��Ϣ�����ݱ��в������ݣ�
�����������createtable����ϵͳ���ϵͳ�е�����������Ϣ�Ĳ���
*/
//done
RC Insert(char *relName, int nValues, Value *values){
	RM_FileHandle *rm_data, *rm_table, *rm_column;
	char *value;//��ȡ���ݱ���Ϣ
	RID *rid;
	RC rc;
	SysColumn *column, *ctmp;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	int attrcount;//��ʱ �������������Գ��ȣ�����ƫ��

	//�����ݱ�,ϵͳ��ϵͳ���ļ�
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	rc = RM_OpenFile(relName, rm_data);
	if (rc != SUCCESS){
		AfxMessageBox("�����ݱ��ļ�ʧ��");
		return rc;
	}
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	rc = RM_OpenFile("SYSTABLES", rm_table);
	if (rc != SUCCESS){
		AfxMessageBox("��ϵͳ���ļ�ʧ��");
		return rc;
	}
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS){
		AfxMessageBox("��ϵͳ���ļ�ʧ��");
		return rc;
	}
		//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������rectab��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS){
		AfxMessageBox("��ʼ���ļ�ɨ��ʧ��");
		return rc;
	}
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}	
	}

	//�ж��Ƿ�Ϊ��ȫ���룬������ǣ�����fail
	if (attrcount != nValues){
		AfxMessageBox("����ȫ��¼���룡");
		return FAIL;
	}
	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������reccol��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS){
		AfxMessageBox("��ʼ���������ļ�ɨ��ʧ��");
		return rc;
	}
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	//����֮ǰ��ȡ��ϵͳ������Ϣ����ȡ������Ϣ�����������ctmp��
	column = (SysColumn *)malloc(attrcount*sizeof(SysColumn));
	ctmp = column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++, ctmp++){
				memcpy(ctmp->tabName, reccol.pData, 21);
				memcpy(ctmp->attrName, reccol.pData + 21, 21);
				memcpy(&(ctmp->attrType), reccol.pData + 42, sizeof(AttrType));
				memcpy(&(ctmp->attrLength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
				memcpy(&(ctmp->attrOffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));
				rc = GetNextRec(&FileScan, &reccol);
				if (rc != SUCCESS)
					break;
			}
			break;
		}
	}
	ctmp = column;

	//�����ݱ���ѭ�������¼
	value = (char *)malloc(rm_data->fileSubHeader->recordSize);
	values = values + nValues -1;
	for (int i = 0; i < nValues; i++, values--,ctmp++){
		memcpy(value + ctmp->attrOffset, values->data, ctmp->attrLength);
	}
	rid = (RID*)malloc(sizeof(RID));
	rid->bValid = false;
	InsertRec(rm_data, value, rid);
	free(value);
	free(rid);
	free(column);
	
	//�ر��ļ����
	rc = RM_CloseFile(rm_data);
	if (rc != SUCCESS){
		AfxMessageBox("�ر����ݱ�ʧ��");
		return rc;
	}
	free(rm_data);
	rc = RM_CloseFile(rm_table);
	if (rc != SUCCESS){
		AfxMessageBox("�ر�ϵͳ��ʧ��");
		return rc;
	}
	free(rm_table);
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS){
		AfxMessageBox("�ر�ϵͳ��ʧ��");
		return rc;
	}
	free(rm_column);
	return SUCCESS;
}

RC Delete(char *relName, int nConditions, Condition *conditions){	
	RM_FileHandle *rm_data,*rm_table,*rm_column;
	RC rc;
	RM_FileScan FileScan;
	RM_Record recdata,rectab,reccol;
	SysColumn *column, *ctmp,*ctmpleft,*ctmpright;
	Condition *contmp;
	int i,torf;//�Ƿ����ɾ������
	int attrcount;//��ʱ ��������
	int intleft,intright; 
	char *charleft,*charright;
	float floatleft,floatright;//��ʱ ���Ե�ֵ
	AttrType attrtype;

	//�����ݱ�,ϵͳ��ϵͳ���ļ�
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	rc = RM_OpenFile(relName, rm_data);
	if (rc != SUCCESS)
		return rc;
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	rc = RM_OpenFile("SYSTABLES", rm_table);
	if (rc != SUCCESS)
		return rc;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS)
		return rc;

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������rectab��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������reccol��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	//����֮ǰ��ȡ��ϵͳ������Ϣ����ȡ������Ϣ�����������ctmp��
	column = (SysColumn *)malloc(attrcount*sizeof(SysColumn));
	ctmp = column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++){
				memcpy(ctmp->tabName, reccol.pData, 21);
				memcpy(ctmp->attrName, reccol.pData + 21, 21);
				memcpy(&(ctmp->attrType), reccol.pData + 42, sizeof(AttrType));
				memcpy(&(ctmp->attrLength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
				memcpy(&(ctmp->attrOffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));
				ctmp++;
				rc = GetNextRec(&FileScan, &reccol);
				if (rc != SUCCESS)
					break;
			}
			break;
		}
	}

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������recdata��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_data, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ�����ݱ��еļ�¼,������¼��Ϣ������recdata��
	while (GetNextRec(&FileScan, &recdata) == SUCCESS){	//ȡ��¼���ж�
		for (i = 0, torf = 1,contmp = conditions;i < nConditions; i++, contmp++){//conditions������һ�ж�
			ctmpleft = ctmpright = column;//ÿ��ѭ����Ҫ����������ϵͳ���ļ����ҵ�����������Ӧ������
			//��������ֵ
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->lhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpleft++;
				}
				//��conditions��ĳһ�����������ж�
				switch (ctmpleft->attrType){//�ж����Ե�����
					case ints:
						attrtype = ints;
						memcpy(&intleft, recdata.pData + ctmpleft->attrOffset, sizeof(int));
						memcpy(&intright, contmp->rhsValue.data, sizeof(int));
						break;
					case chars:
						attrtype = chars;
						charleft = (char *)malloc(ctmpleft->attrLength);
						memcpy(charleft, recdata.pData + ctmpleft->attrOffset, ctmpleft->attrLength);
						charright = (char *)malloc(ctmpleft->attrLength);
						memcpy(charright, contmp->rhsValue.data, ctmpleft->attrLength);
						break;
					case floats:
						attrtype = floats;
						memcpy(&floatleft, recdata.pData + ctmpleft->attrOffset, sizeof(float));
						memcpy(&floatright, contmp->rhsValue.data, sizeof(float));
						break;
				}
			}
			//��������ֵ
			if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->rhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpright++;
				}
				//��conditions��ĳһ�����������ж�
				switch (ctmpright->attrType){
				case ints:
					attrtype = ints;
					memcpy(&intleft, contmp->lhsValue.data, sizeof(int));
					memcpy(&intright, recdata.pData + ctmpright->attrOffset, sizeof(int));
					break;
				case chars:
					attrtype = chars;
					charleft = (char *)malloc(ctmpright->attrLength);
					memcpy(charleft, contmp->lhsValue.data, ctmpright->attrLength);
					charright = (char *)malloc(ctmpright->attrLength);
					memcpy(charright, recdata.pData + ctmpright->attrOffset, ctmpright->attrLength);
					break;
				case floats:
					attrtype = floats;
					memcpy(&floatleft, contmp->lhsValue.data, sizeof(float));
					memcpy(&floatright, recdata.pData + ctmpright->attrOffset, sizeof(float));
					break;
				}
			}
			//���Ҿ�����
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->lhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpleft++;
				}
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->rhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpright++;
				}
				//��conditions��ĳһ�����������ж�
				switch (ctmpright->attrType){
					case ints:
						attrtype = ints;
						memcpy(&intleft, recdata.pData + ctmpleft->attrOffset, sizeof(int));
						memcpy(&intright, recdata.pData + ctmpright->attrOffset, sizeof(int));
						break;
					case chars:
						attrtype = chars;
						charleft = (char *)malloc(ctmpright->attrLength);
						memcpy(charleft, recdata.pData + ctmpleft->attrOffset, ctmpright->attrLength);
						charright = (char *)malloc(ctmpright->attrLength);
						memcpy(charright, recdata.pData + ctmpright->attrOffset, ctmpright->attrLength);
						break;
					case floats:
						attrtype = floats;
						memcpy(&floatleft, recdata.pData + ctmpleft->attrOffset, sizeof(float));
						memcpy(&floatright, recdata.pData + ctmpright->attrOffset, sizeof(float));
						break;
				}
			}
			if (attrtype == ints){
				if ((intleft == intright && contmp->op == EQual) ||
					(intleft > intright && contmp->op == GreatT) ||
					(intleft >= intright && contmp->op == GEqual) ||
					(intleft < intright && contmp->op == LessT) ||
					(intleft <= intright && contmp->op == LEqual) ||
					(intleft != intright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else if (attrtype == chars){
				if ((strcmp(charleft, charright) == 0 && contmp->op == EQual) ||
					(strcmp(charleft, charright) > 0 && contmp->op == GreatT) ||
					((strcmp(charleft, charright) > 0 || strcmp(charleft, charright) == 0) && contmp->op == GEqual) ||
					(strcmp(charleft, charright) < 0 && contmp->op == LessT) ||
					((strcmp(charleft, charright) < 0 || strcmp(charleft, charright) == 0) && contmp->op == LEqual) ||
					(strcmp(charleft, charright) != 0 && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
				free(charleft);
				free(charright);
			}
			else if (attrtype == floats){
				if ((floatleft == floatright && contmp->op == EQual) ||
					(floatleft > floatright && contmp->op == GreatT) ||
					(floatleft >= floatright && contmp->op == GEqual) ||
					(floatleft < floatright && contmp->op == LessT) ||
					(floatleft <= floatright && contmp->op == LEqual) ||
					(floatleft != floatright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else
				torf &= 0;
		}

		if (torf == 1){
			DeleteRec(rm_data, &(recdata.rid));
		}	
	}
	free(column);

	//�ر��ļ����
	rc = RM_CloseFile(rm_table);
	if (rc != SUCCESS)
		return rc;
	free(rm_table);
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS)
		return rc;
	free(rm_column);
	rc = RM_CloseFile(rm_data);
	if (rc != SUCCESS)
		return rc;
	free(rm_data);
	return SUCCESS;
}

RC Update(char *relName, char *attrName, Value *Value, int nConditions, Condition *conditions){
	//ֻ�ܽ��е�ֵ����
	RM_FileHandle *rm_data, *rm_table, *rm_column;
	RC rc;
	RM_FileScan FileScan;
	RM_Record recdata, rectab, reccol;
	SysColumn *column, *ctmp,*cupdate,*ctmpleft,*ctmpright;
	Condition *contmp;
	int i, torf;//�Ƿ����ɾ������
	int attrcount;//��ʱ ��������
	int intleft,intright;
	char *charleft,*charright;
	float floatleft,floatright;//��ʱ ���Ե�ֵ
	AttrType attrtype;

	//�����ݱ�,ϵͳ��ϵͳ���ļ�
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	rc = RM_OpenFile(relName, rm_data);
	if (rc != SUCCESS)
		return rc;
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	rc = RM_OpenFile("SYSTABLES", rm_table);
	if (rc != SUCCESS)
		return rc;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS)
		return rc;

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������rectab��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������reccol��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ��ϵͳ���еļ�¼,������¼��Ϣ������rectab��
	//����֮ǰ��ȡ��ϵͳ������Ϣ����ȡ������Ϣ�����������ctmp��
	column = (SysColumn *)malloc(attrcount*sizeof(SysColumn));
	cupdate = (SysColumn *)malloc(sizeof(SysColumn));
	ctmp = column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//�ҵ�����ΪrelName�ĵ�һ����¼�����ζ�ȡattrcount����¼
			for (int i = 0; i < attrcount; i++){
				memcpy(ctmp->tabName, reccol.pData, 21);
				memcpy(ctmp->attrName, reccol.pData + 21, 21);
				memcpy(&(ctmp->attrType), reccol.pData + 42, sizeof(AttrType));
				memcpy(&(ctmp->attrLength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
				memcpy(&(ctmp->attrOffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));
				if ((strcmp(relName,ctmp->tabName) == 0) && (strcmp(attrName,ctmp->attrName) == 0)){
					cupdate = ctmp;//�ҵ�Ҫ�������� ��Ӧ������
				}
				rc = GetNextRec(&FileScan, &reccol);
				if (rc != SUCCESS)
					break;
				ctmp++;
			}
			break;
		}
	}

	//ͨ��getdata������ȡϵͳ����Ϣ,�õ�����Ϣ������recdata��
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_data, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//ѭ�����ұ���ΪrelName��Ӧ�����ݱ��еļ�¼,������¼��Ϣ������recdata��
	while (GetNextRec(&FileScan, &recdata) == SUCCESS){
		for (i = 0, torf = 1, contmp = conditions; i < nConditions; i++, contmp++){//conditions������һ�ж�
			ctmpleft = ctmpright = column;//ÿ��ѭ����Ҫ����������ϵͳ���ļ����ҵ�����������Ӧ������
			//��������ֵ
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->lhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpleft++;
				}
				//��conditions��ĳһ�����������ж�
				if (ctmpleft->attrType == ints){//�ж����Ե�����
					attrtype = ints;
					memcpy(&intleft, recdata.pData + ctmpleft->attrOffset, sizeof(int));
					memcpy(&intright, contmp->rhsValue.data, sizeof(int));
				}
				else if (ctmpleft->attrType == chars){
					attrtype = chars;
					charleft = (char *)malloc(ctmpleft->attrLength);
					memcpy(charleft, recdata.pData + ctmpleft->attrOffset, ctmpleft->attrLength);
					charright = (char *)malloc(ctmpleft->attrLength);
					memcpy(charright, contmp->rhsValue.data, ctmpleft->attrLength);
				}
				else if (ctmpleft->attrType == floats){
					attrtype = floats;
					memcpy(&floatleft, recdata.pData + ctmpleft->attrOffset, sizeof(float));
					memcpy(&floatright, contmp->rhsValue.data, sizeof(float));
				}
				else
					torf &= 0;
			}
			//��������ֵ
			else  if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->rhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpright++;
				}
				//��conditions��ĳһ�����������ж�
				if (ctmpright->attrType == ints){//�ж����Ե�����
					attrtype = ints;
					memcpy(&intleft, contmp->lhsValue.data, sizeof(int));
					memcpy(&intright, recdata.pData + ctmpright->attrOffset, sizeof(int));
				}
				else if (ctmpright->attrType == chars){
					attrtype = chars;
					charleft = (char *)malloc(ctmpright->attrLength);
					memcpy(charleft, contmp->lhsValue.data, ctmpright->attrLength);
					charright = (char *)malloc(ctmpright->attrLength);
					memcpy(charright, recdata.pData + ctmpright->attrOffset, ctmpright->attrLength);
				}
				else if (ctmpright->attrType == floats){
					attrtype = floats;
					memcpy(&floatleft, contmp->lhsValue.data, sizeof(float));
					memcpy(&floatright, recdata.pData + ctmpright->attrOffset, sizeof(float));
				}
				else
					torf &= 0;
			}
			//���Ҿ�����
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->lhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpleft++;
				}
				for (int j = 0; j < attrcount; j++){//attrcount��������һ�ж�
					if (contmp->rhsAttr.relName == NULL){//��������δָ������ʱ��Ĭ��ΪrelName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					ctmpright++;
				}
				//��conditions��ĳһ�����������ж�
				if (ctmpright->attrType == ints && ctmpleft->attrType == ints){//�ж����Ե�����
					attrtype = ints;
					memcpy(&intleft, recdata.pData + ctmpleft->attrOffset, sizeof(int));
					memcpy(&intright, recdata.pData + ctmpright->attrOffset, sizeof(int));
				}
				else if (ctmpright->attrType == chars &&ctmpleft->attrType == chars){
					attrtype = chars;
					charleft = (char *)malloc(ctmpright->attrLength);
					memcpy(charleft, recdata.pData + ctmpleft->attrOffset, ctmpright->attrLength);
					charright = (char *)malloc(ctmpright->attrLength);
					memcpy(charright, recdata.pData + ctmpright->attrOffset, ctmpright->attrLength);
				}
				else if (ctmpright->attrType == floats &&ctmpleft->attrType == floats){
					attrtype = floats;
					memcpy(&floatleft, recdata.pData + ctmpleft->attrOffset, sizeof(float));
					memcpy(&floatright, recdata.pData + ctmpright->attrOffset, sizeof(float));
				}
				else
					torf &= 0;
			}
			if (attrtype == ints){
				if ((intleft == intright && contmp->op == EQual) ||
					(intleft > intright && contmp->op == GreatT) ||
					(intleft >= intright && contmp->op == GEqual) ||
					(intleft < intright && contmp->op == LessT) ||
					(intleft <= intright && contmp->op == LEqual) ||
					(intleft != intright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else if (attrtype == chars){
				if ((strcmp(charleft, charright) == 0 && contmp->op == EQual) ||
					(strcmp(charleft, charright) > 0 && contmp->op == GreatT) ||
					((strcmp(charleft, charright) > 0 || strcmp(charleft, charright) == 0) && contmp->op == GEqual) ||
					(strcmp(charleft, charright) < 0 && contmp->op == LessT) ||
					((strcmp(charleft, charright) < 0 || strcmp(charleft, charright) == 0) && contmp->op == LEqual) ||
					(strcmp(charleft, charright) != 0 && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
				free(charleft);
				free(charright);
			}
			else if (attrtype == floats){
				if ((floatleft == floatright && contmp->op == EQual) ||
					(floatleft > floatright && contmp->op == GreatT) ||
					(floatleft >= floatright && contmp->op == GEqual) ||
					(floatleft < floatright && contmp->op == LessT) ||
					(floatleft <= floatright && contmp->op == LEqual) ||
					(floatleft != floatright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else
				torf &= 0;
		}
		if (torf == 1){
			memcpy(recdata.pData + cupdate->attrOffset,Value->data,cupdate->attrLength);
			UpdateRec(rm_data, &recdata);
		}
	}

	free(column);
	//�ر��ļ����
	rc = RM_CloseFile(rm_table);
	if (rc != SUCCESS)
		return rc;
	free(rm_table);
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS)
		return rc;
	free(rm_column);
	rc = RM_CloseFile(rm_data);
	if (rc != SUCCESS)
		return rc;
	free(rm_data);
	return SUCCESS;	
}

// �ϱߵ�����û����ɣ�ɾ�������²����е�����ֻ����������ұ�ֵ��������������жϣ���ֻ������Ϊints�͵������ж�
//ע��˳�򣡣�������������������

RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char ** relations,
int nConditions, Condition *conditions, char **RES){
	RM_FileHandle *rm_data, *rm_table, *rm_column;
	RM_FileScan FileScan;
	RM_FileScan *relscan;
	RM_Record recdata, rectab, reccol;
	RM_Record *recdkr;//����һ���ѿ�����¼
	SysColumn *column, *ctmp,*ctmpleft,*ctmpright;
	Condition *contmp;
	int allattrcount = 0;//�漰��nRelations�����ݱ����������֮��
	int *attrcount;//�������ݱ��������������
	int allcount = 1;//�ѿ�����������
	int torf;//�Ƿ����ɾ������
	int resultcount = 0;//�����
	int intvalue,intleft,intright;
	char *charvalue,*charleft,*charright;
	float floatvalue,floatleft,floatright;
	RC rc;
	printrel *pr;
	char *res;
	AttrType attrtype;
	char *attention;

	pr = (printrel*)malloc(nSelAttrs * sizeof(printrel));
	//�����ݱ�,ϵͳ��ϵͳ���ļ�
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	rc = RM_OpenFile("SYSTABLES", rm_table);
	if (rc != SUCCESS)
		return rc;
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS)
		return rc;
	//��nRelations�����ݱ��������ļ������rm_data + i��
	rm_data = (RM_FileHandle *)malloc(nRelations*sizeof(RM_FileHandle));
	for (int i = 0; i <nRelations; i++){
		(rm_data + i)->bOpen = false;
		rc = RM_OpenFile(relations[i], rm_data + i);
		if (rc != SUCCESS)
			return rc;
	}//ע��˳�� rm_data -> relations   : 0-(nRelations-1)  -> 0-(nRelations-1)

	//��ȡϵͳ����ȡ����������Ϣ��
	//������������_allattrcount,������ϵ����������������*��_attrcount + i ����
	attrcount = (int *)malloc(nRelations*sizeof(int));
	//�ҵ�����������Ӧ��������������������_attrcount������
	for (int i = 0; i < nRelations; i++){
		FileScan.bOpen = false;
		rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);//ÿ�β��ҳ�ʼ����
		if (rc != SUCCESS)
			return rc;
		while (GetNextRec(&FileScan, &rectab) == SUCCESS){
			if (strcmp(relations[i], rectab.pData) == 0){
				memcpy(attrcount + i, rectab.pData + 21, sizeof(int));
				allattrcount += *(attrcount + i);//�������漰��ϵ�����Ե��ܺͣ�������_allattrcount��
				break;
			}
		}
	}//ע��˳��_attrcount -> relations : 0-(nRelations-1) -> 0-(nRelations-1)
	//��ȡϵͳ�У���ȡ�漰���ݱ��ȫ��������Ϣ
	//��������columnΪ��ʼ��ַ�Ŀռ��У�˳���ǰ���relations������˳��
	column = (SysColumn *)malloc(allattrcount*sizeof(SysColumn));//����_allattrcount��Syscolumn�ռ䣬���ڱ���nRealtions����ϵ������
	ctmp = column;
	for (int i = 0; i < nRelations; i++){
		FileScan.bOpen = false;
		rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
		if (rc != SUCCESS)
			return rc;//����ÿ�����������Ϣ��Ҫ��ʼ��ɨ��
		while (GetNextRec(&FileScan, &reccol) == SUCCESS){
			if (strcmp(relations[i], reccol.pData) == 0){//�ҵ�����Ϊ*��relations + i���ĵ�һ����¼
				for (int j = 0; j < *(attrcount + i); j++){  //���ζ�ȡ*��_attrcount + i��������
					memcpy(ctmp->tabName, reccol.pData, 21);
					memcpy(ctmp->attrName, reccol.pData + 21, 21);
					memcpy(&(ctmp->attrType), reccol.pData + 42, sizeof(AttrType));
					memcpy(&(ctmp->attrLength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
					memcpy(&(ctmp->attrOffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));
					ctmp++;
					rc = GetNextRec(&FileScan, &reccol);
					if (rc != SUCCESS)
						break;
				}
				break;
			}
		}
	}//column -> relations : 0-(nRelations-1) -> 0-(nRelations-1)
	//��ȡselAttrs��Ӧ�ĵѿ�������λ��
	
	for (int i = 0; i < nSelAttrs; i++){
		for (int j = 0; j < nRelations; j++){
			ctmp = column;
			if (strcmp(selAttrs[i]->relName, relations[j]) == 0){
				pr[i].i = j;
				for (int k = 0; k < allattrcount; k++, ctmp++){
					if (strcmp(selAttrs[i]->relName, ctmp->tabName) == 0 && strcmp(selAttrs[i]->attrName, ctmp->attrName) == 0){
						pr[i].col = *ctmp;
						break;
					}
				}
				break;
			}
		}
	}
	//��ȡ���ݱ���ȡ��¼��
	//������recordcount�У�˳���ǰ��ձ���relations��˳��
	_recordcount = (int *)malloc(nRelations*sizeof(int));
	for (int i = 0; i < nRelations; i++){
		FileScan.bOpen = false;
		rc = OpenScan(&FileScan, rm_data + i, 0, NULL, NO_HINT);
		if (rc != SUCCESS)
			return rc;//����ÿ�����������Ϣ��Ҫ��ʼ��ɨ��
		*(_recordcount + i) = 0;
		while (GetNextRec(&FileScan, &recdata) == SUCCESS){
			*(_recordcount + i) += 1;
		}
	}
	//��ȡ����Ϣ���������ݱ����������������ݱ�������Ϣ�������ݱ�ļ�¼��
	//��һ���ַ������������������ڶ����ַ��������¼������������nSelAttrs���ֽڱ���������
	res = (char *)malloc((20 * 20 + 2 + nSelAttrs) * 20);
	*RES = res;
	charvalue = (char *)malloc(20);
	itoa(nSelAttrs, charvalue, 10);
	memcpy(res, charvalue, 20);
	free(charvalue);
	for (int i = 0; i < nSelAttrs; i++){
		memcpy(res + i * 20 + 40, selAttrs[i]->attrName, 20);
	}
	char *data = res + (2 + nSelAttrs) * 20;
	//ͨ���ѿ����㷨��ʵ�ָ�����ϵ������
	counter = (int *)malloc(nRelations * sizeof(int));
	for (int i = 0; i < nRelations; i++){
		counter[i] = 0;
	}
	counterIndex = nRelations - 1;
	tablecount = nRelations;
	relscan = (RM_FileScan *)malloc(nRelations*sizeof(RM_FileScan));
	recdkr = (RM_Record *)malloc(nRelations*sizeof(RM_Record));
	//��nRelations�����ݱ�ֱ��ʼ��ɨ�裬������ѿ���������Ŀ��
	for (int i = 0; i < nRelations; i++){
		allcount *= _recordcount[i];
		(relscan + i)->bOpen = false;   
		rc = OpenScan(relscan + i, rm_data + i, 0, NULL, NO_HINT);
		if (rc != SUCCESS)
			return rc;
	}
	
	for (int i = 0; i < allcount; i++){
		//ȡ��һ���ѿ�����¼��һ���ѿ�����¼��ӦnRelations�����е�һ����¼
		//ȡ��relations +j�����ݱ�ĵĵ�counter[j]����¼������recdkr + j��
		//����һ����¼֮��Ҫ��relscan ���³�ʼ��ɨ��
		for (int j = 0; j < nRelations; j++){
			for (int k = 0; k != counter[j] + 1; k++){
				GetNextRec(relscan + j, recdkr + j);
			}
			(relscan + j)->bOpen = false;
			rc = OpenScan(relscan + j, rm_data + j, 0, NULL, NO_HINT);
			if (rc != SUCCESS)
				return rc;
		}
		//�����ж�
		torf = 1;
		contmp = conditions;
		for (int x = 0; x < nConditions; x++, contmp++){//��nSelAttrs��������һ�ж�
			ctmpleft = ctmpright = column;//��ctmp���õ�column��
			//��������ֵ
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int y = 0; y < allattrcount; y++, ctmpleft++){//��attrcount�������ҵ���Ӧ��conditions��������������Ӧ�ļ�¼
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)//�ҵ������е����Զ�Ӧϵͳ���е����������Ϣ
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){
						break;
					}
					if (y == allattrcount - 1){
						attention = strcat(contmp->lhsAttr.relName, ".");
						attention = strcat(attention, contmp->lhsAttr.attrName);
						attention = strcat(attention, "\r\n�����ڣ�����sql���");
						AfxMessageBox(attention);
						return FAIL;
					}
				}
				
				for (int z = 0; z < nRelations; z++){//�ҵ�������������Զ�Ӧ�ı���relations�����е��±�λ��    
					if (strcmp(relations[z], ctmpleft->tabName) == 0){
						//��conditions��ĳһ�����������ж�
						if (ctmpleft->attrType == ints){//�ж����Ե�����
							attrtype = ints;
							memcpy(&intleft, (recdkr + z)->pData + ctmpleft->attrOffset, sizeof(int));
							memcpy(&intright, contmp->rhsValue.data, sizeof(int));
						}
						else if (ctmpleft->attrType == chars){
							attrtype = chars;
							charleft = (char *)malloc(ctmpleft->attrLength);
							memcpy(charleft, (recdkr + z)->pData + ctmpleft->attrOffset, ctmpleft->attrLength);
							charright = (char *)malloc(ctmpleft->attrLength);
							memcpy(charright, contmp->rhsValue.data, ctmpleft->attrLength);
						}
						else{
							attrtype = floats;
							memcpy(&floatleft, (recdkr + z)->pData + ctmpleft->attrOffset, sizeof(float));
							memcpy(&floatright, contmp->rhsValue.data, sizeof(float));
						}
						break;
					}
				}
			}

			//��������ֵ
			else  if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int y = 0; y < allattrcount; y++, ctmpright++){//attrcount��������һ�ж�
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//���ݱ����������ҵ���Ӧ����
						break;
					}
					if (y == allattrcount - 1){
						attention = strcat(contmp->rhsAttr.relName, ".");
						attention = strcat(attention, contmp->rhsAttr.attrName);
						attention = strcat(attention, "\r\n�����ڣ�����sql���");
						AfxMessageBox(attention);
						return FAIL;
					}
				}
				for (int z = 0; z < nRelations; z++){//�ҵ�������������Զ�Ӧ�ı���relations�����е��±�λ��    
					if (strcmp(relations[z], ctmpleft->tabName) == 0){
						//��conditions��ĳһ�����������ж�
						if (ctmpright->attrType == ints){//�ж����Ե�����
							attrtype = ints;
							memcpy(&intleft, contmp->lhsValue.data, sizeof(int));
							memcpy(&intright, (recdkr + z)->pData + ctmpright->attrOffset, sizeof(int));
						}
						else if (ctmpleft->attrType == chars){
							attrtype = chars;
							charleft = (char *)malloc(ctmpleft->attrLength);
							memcpy(charleft, contmp->lhsValue.data, ctmpleft->attrLength);
							charright = (char *)malloc(ctmpleft->attrLength);
							memcpy(charright, (recdkr + z)->pData + ctmpright->attrOffset, ctmpright->attrLength);
						}
						else{
							attrtype = floats;
							memcpy(&floatleft, contmp->lhsValue.data, sizeof(float));
							memcpy(&floatright, (recdkr + z)->pData + ctmpright->attrOffset, sizeof(float));
						}
						break;
					}
				}
			}
			//���Ҿ�����
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int y = 0; y < allattrcount; y++, ctmpleft++){
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){
						break;
					}
					if (y == allattrcount - 1){
						attention = strcat(contmp->lhsAttr.relName, ".");
						attention = strcat(attention, contmp->lhsAttr.attrName);
						attention = strcat(attention, "\r\n�����ڣ�����sql���");
						AfxMessageBox(attention);
						return FAIL;
					}
				}
				for (int y = 0; y < allattrcount; y++, ctmpright++){
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){
						break;
					}
					if (y == allattrcount - 1){
						attention = strcat(contmp->rhsAttr.relName, ".");
						attention = strcat(attention, contmp->rhsAttr.attrName);
						attention = strcat(attention, "\r\n�����ڣ�����sql���");
						AfxMessageBox(attention);
						return FAIL;
					}
				}
				//��conditions��ĳһ�����������ж�
				for (int z = 0; z < nRelations; z++){//�ҵ�������������Զ�Ӧ�ı���relations�����е��±�λ��    
					if (strcmp(relations[z], ctmpleft->tabName) == 0){
						//��conditions��ĳһ�����������ж�
						if (ctmpleft->attrType == ints){//�ж����Ե�����
							attrtype = ints;
							memcpy(&intleft, (recdkr + z)->pData + ctmpleft->attrOffset, sizeof(int));
						}
						else if (ctmpleft->attrType == chars){
							attrtype = chars;
							charleft = (char *)malloc(ctmpleft->attrLength);
							memcpy(charleft, (recdkr + z)->pData + ctmpleft->attrOffset, ctmpleft->attrLength);
						}
						else{
							attrtype = floats;
							memcpy(&floatleft, (recdkr + z)->pData + ctmpleft->attrOffset, sizeof(float));
						}
					}
					if (strcmp(relations[z], ctmpright->tabName) == 0){
						//��conditions��ĳһ�����������ж�
						if (ctmpright->attrType == ints){//�ж����Ե�����
							memcpy(&intright, (recdkr + z)->pData + ctmpright->attrOffset, sizeof(int));
						}
						else if (ctmpright->attrType == chars){
							charright = (char *)malloc(ctmpright->attrLength);
							memcpy(charright, (recdkr + z)->pData + ctmpright->attrOffset, ctmpright->attrLength);
						}
						else{
							memcpy(&floatright, (recdkr + z)->pData + ctmpright->attrOffset, sizeof(float));
						}
					}
				}
			}
			if (attrtype == ints){
				if ((intleft == intright && contmp->op == EQual) ||
					(intleft > intright && contmp->op == GreatT) ||
					(intleft >= intright && contmp->op == GEqual) ||
					(intleft < intright && contmp->op == LessT) ||
					(intleft <= intright && contmp->op == LEqual) ||
					(intleft != intright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else if (attrtype == chars){
				if ((strcmp(charleft, charright) == 0 && contmp->op == EQual) ||
					(strcmp(charleft, charright) > 0 && contmp->op == GreatT) ||
					((strcmp(charleft, charright) > 0 || strcmp(charleft, charright) == 0) && contmp->op == GEqual) ||
					(strcmp(charleft, charright) < 0 && contmp->op == LessT) ||
					((strcmp(charleft, charright) < 0 || strcmp(charleft, charright) == 0) && contmp->op == LEqual) ||
					(strcmp(charleft, charright) != 0 && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
				free(charleft);
				free(charright);
			}
			else if (attrtype == floats){
				if ((floatleft == floatright && contmp->op == EQual) ||
					(floatleft > floatright && contmp->op == GreatT) ||
					(floatleft >= floatright && contmp->op == GEqual) ||
					(floatleft < floatright && contmp->op == LessT) ||
					(floatleft <= floatright && contmp->op == LEqual) ||
					(floatleft != floatright && contmp->op == NEqual))
					torf &= 1;
				else
					torf &= 0;
			}
			else
				torf &= 0;
		}
		if (torf == 1){
			resultcount++;
			for (int z = 0; z < nSelAttrs; z++){
				if (pr[z].col.attrType == ints){
					memcpy(&intvalue, (recdkr + pr[z].i)->pData + pr[z].col.attrOffset, sizeof(int));
					charvalue = (char *)malloc(20);
					itoa(intvalue, charvalue, 10);
					memcpy(data + z * 20, charvalue, 20);
					free(charvalue);
				}
				else if (pr[z].col.attrType == chars){
					memcpy(data + z * 20, (recdkr + pr[z].i)->pData + pr[z].col.attrOffset, 20);
				}
				else{
					memcpy(&floatvalue, (recdkr + pr[z].i)->pData + pr[z].col.attrOffset, sizeof(int));
					sprintf(data + z * 20, "%f", floatvalue);
				}
			}
			data += 20 * nSelAttrs;
		}
		handle();
	}
	resultcount++;//��Ϊ��Ҫ�洢һ�б�ͷ�����Դ˴�Ҫ++
	charvalue = (char *)malloc(20);
	itoa(resultcount, charvalue, 10);
	memcpy(res + 20, charvalue, 20);
	free(charvalue);

	// �ͷ��ڴ棬�ر��ļ����
	free(column);
	free(pr);
	free(attrcount);
	free(counter);
	free(recdkr);
	free(relscan);
	rc = RM_CloseFile(rm_table);
	if (rc != SUCCESS)
		return rc;
	free(rm_table);
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS)
		return rc;
	free(rm_column);
	for (int i = 0; i < nRelations; i++){
		rc = RM_CloseFile(rm_data + i);
		if (rc != SUCCESS)
			return rc;
	}
	free(rm_data);
	return SUCCESS;
}

void handle(){
	counter[counterIndex]++;
	if (counter[counterIndex] >= _recordcount[counterIndex]) {
		counter[counterIndex] = 0;
		counterIndex--;
		if (counterIndex >= 0) {
			handle();
		}
		counterIndex = tablecount - 1;
	}
}
