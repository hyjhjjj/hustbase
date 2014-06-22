#include "stdafx.h"
#include "SYS_Manager.h"

	//select功能的全局静态变量
static int *_recordcount;//各个数据表包含的记录数量
static int tablecount; //表数
static int counterIndex;
static int *counter;
typedef struct printrel{
	int i;	//保存属性对应表名对应的下标
	SysColumn col;//保存属性信息
}printrel;

//done
RC CreateDb(char *dbPath){
	//设置当前目录为dbpath，其中dbPath包含有dbName；
	SetCurrentDirectory(dbPath);
	RC rc;
	//创建系统表文件和系统列文件
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
建表操作：
1.系统表文件和系统列文件的初始化
2.创建数据表文件
*/
//done
RC CreateTable(char *relName, int attrCount, AttrInfo *attributes){
	RC rc;
	char  *pData;
	RM_FileHandle *rm_table, *rm_column;
	RID *rid;
	int recordsize;//数据表的每条记录的大小
	AttrInfo * attrtmp = attributes;

	//打开系统表文件和系统列文件
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
	//向系统表中填充信息
	pData = (char *)malloc(sizeof(SysTable));
	memcpy(pData, relName,21);//填充表名
	memcpy(pData + 21, &attrCount, sizeof(int));//填充属性列数
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
	free(rid);//释放申请的内存空间

	//向系统列中循环填充信息 一个表中包含多个属性列，就需要循环
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
	//创建数据表
	//计算recordsize的大小
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


//删表操作
//1.把表名为relName的表删除 
//2.将数据库中该表对应的信息置空
RC DropTable(char *relName){
	CFile tmp;
	RM_FileHandle *rm_table, *rm_column;
	RC rc;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	int attrcount;//临时 属性数量，属性长度，属性偏移

	//删除数据表文件
	tmp.Remove((LPCTSTR)relName);
	
	//将系统表和系统列中对应表的相关记录删除
	//打开系统表，系统列文件
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

	//通过getdata函数获取系统表信息,得到的信息保存在rectab中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			DeleteRec(rm_table, &(rectab.rid));
			break;
		}
	}
	//通过getdata函数获取系统列信息,得到的信息保存在reccol中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	//根据之前读取的系统表中信息，读取属性信息，结果保存在ctmp中
	for (int i = 0; i < attrcount && GetNextRec(&FileScan, &reccol) == SUCCESS;){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			DeleteRec(rm_column, &(reccol.rid));
			i++;
		}
	}

	//关闭文件句柄
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

//创建索引：
//1.创建索引文件
//2.对系统列中关于索引的两列信息的更新 
//3.读取索引对应的表中数据，并将这些数据按照b+树形式有序写入索引文件中 
//4.关闭相关文件
RC CreateIndex(char *indexName, char *relName, char *attrName){
	RM_FileHandle  *rm_table, *rm_column,*rm_data;
	RC rc;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	SysColumn *column, *cindex;
	IX_FileHandle *indexhandle;
	char *data;
	int attrcount;//临时 属性数量
	
	//读系统表，系统列文件
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

	//通过getdata函数获取系统表信息,得到的信息保存在rectab中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}

	//通过getdata函数获取系统列信息,得到的信息保存在reccol中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	column = (SysColumn *)malloc(attrcount*sizeof(SysColumn));
	cindex = (SysColumn *)malloc(sizeof(SysColumn));
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++, column++){
				memcpy(column->tabName, reccol.pData, 21);
				memcpy(column->attrName, reccol.pData + 21, 21);
				if ((strcmp(relName, column->tabName) == 0) && (strcmp(attrName, column->attrName) == 0)){
					cindex = column;
					//找到要建立索引对应的属性,，相关信息保存在cindex中
					break;
				}
				rc = GetNextRec(&FileScan, &reccol);
				if (rc != SUCCESS)
					break;
			}
			break;
		}
	}

	//第一步完成创建索引文件
	IX_CreateFile(indexName, cindex->attrType, cindex->attrLength);
	
	//第二步完成对系统列中关于索引的两列信息的更新
	memcpy(&(cindex->ix_flag), "1", sizeof(char));
	memcpy(cindex->ixName, indexName, 21);
	memcpy(&reccol, cindex, sizeof(SysColumn));
	UpdateRec(rm_column, &reccol);
	
	//第三步读取索引对应的表中数据，并将这些数据按照b+树形式有序写入索引文件中
	//读一个插一个，插一个，直到取不到数据完成插入
	indexhandle = (IX_FileHandle *)malloc(sizeof(IX_FileHandle));
	rc = IX_OpenFile(indexName, indexhandle);
	if(rc != SUCCESS){
		return rc;
	}//打开索引文件

	//通过getdata函数获取系统列信息,得到的信息保存在reccol中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_data, 0, NULL, NO_HINT);
	if (rc != SUCCESS){
		AfxMessageBox("初始化数据表文件扫描失败");
		return rc;
	}
	column = (SysColumn *)malloc(sizeof(SysColumn));
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		data = (char *)malloc(cindex->attrLength);
		memcpy(data, reccol.pData + cindex->attrOffset, cindex->attrLength);
		InsertEntry(indexhandle, data, &(reccol.rid));
	}
	
	//第四步，关闭文件，完成创建
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
	
	//第一步删除索引文件
	tmp.Remove((LPCTSTR)indexName);

	//第二步修改系统列中关于索引的信息
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS){
		AfxMessageBox("打开系统列文件失败");
		return rc;
	}
	//通过getdata函数获取系统列信息,得到的信息保存在reccol中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS){
		AfxMessageBox("初始化数据列文件扫描失败");
		return rc;
	}
	column = (SysColumn *)malloc(sizeof(SysColumn));
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		memcpy(index, reccol.pData + 43 + 2 * sizeof(int) + sizeof(AttrType), 21);//找到关于系统列中关于索引的那条记录
		if (strcmp(index, indexName) == 0){
			memcpy(reccol.pData + 42 + 2 * sizeof(int)+sizeof(AttrType), "0", 1);//将ix_flag置为0；
			rc = UpdateRec(rm_column, &reccol);
			if ( rc != SUCCESS){
				return rc;
			}
		}
	}

	// 第三步 关闭文件，完成删除
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS)
		return rc;
	return SUCCESS;
}

/*
函数实现思想：
通过读取系统表中的表信息及表的属性信息，
然后根据这些信息向数据表中插入数据，
插入操作类似createtable中向系统表和系统列的填充表及属性信息的操作
*/
//done
RC Insert(char *relName, int nValues, Value *values){
	RM_FileHandle *rm_data, *rm_table, *rm_column;
	char *value;//读取数据表信息
	RID *rid;
	RC rc;
	SysColumn *column, *ctmp;
	RM_FileScan FileScan;
	RM_Record rectab, reccol;
	int attrcount;//临时 属性数量，属性长度，属性偏移

	//打开数据表,系统表，系统列文件
	rm_data = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_data->bOpen = false;
	rc = RM_OpenFile(relName, rm_data);
	if (rc != SUCCESS){
		AfxMessageBox("打开数据表文件失败");
		return rc;
	}
	rm_table = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_table->bOpen = false;
	rc = RM_OpenFile("SYSTABLES", rm_table);
	if (rc != SUCCESS){
		AfxMessageBox("打开系统表文件失败");
		return rc;
	}
	rm_column = (RM_FileHandle *)malloc(sizeof(RM_FileHandle));
	rm_column->bOpen = false;
	rc = RM_OpenFile("SYSCOLUMNS", rm_column);
	if (rc != SUCCESS){
		AfxMessageBox("打开系统列文件失败");
		return rc;
	}
		//通过getdata函数获取系统表信息,得到的信息保存在rectab中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS){
		AfxMessageBox("初始化文件扫描失败");
		return rc;
	}
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}	
	}

	//判定是否为完全插入，如果不是，返回fail
	if (attrcount != nValues){
		AfxMessageBox("不是全纪录插入！");
		return FAIL;
	}
	//通过getdata函数获取系统列信息,得到的信息保存在reccol中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS){
		AfxMessageBox("初始化数据列文件扫描失败");
		return rc;
	}
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	//根据之前读取的系统表中信息，读取属性信息，结果保存在ctmp中
	column = (SysColumn *)malloc(attrcount*sizeof(SysColumn));
	ctmp = column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
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

	//向数据表中循环插入记录
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
	
	//关闭文件句柄
	rc = RM_CloseFile(rm_data);
	if (rc != SUCCESS){
		AfxMessageBox("关闭数据表失败");
		return rc;
	}
	free(rm_data);
	rc = RM_CloseFile(rm_table);
	if (rc != SUCCESS){
		AfxMessageBox("关闭系统表失败");
		return rc;
	}
	free(rm_table);
	rc = RM_CloseFile(rm_column);
	if (rc != SUCCESS){
		AfxMessageBox("关闭系统列失败");
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
	int i,torf;//是否符合删除条件
	int attrcount;//临时 属性数量
	int intleft,intright; 
	char *charleft,*charright;
	float floatleft,floatright;//临时 属性的值
	AttrType attrtype;

	//打开数据表,系统表，系统列文件
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

	//通过getdata函数获取系统表信息,得到的信息保存在rectab中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}

	//通过getdata函数获取系统列信息,得到的信息保存在reccol中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	//根据之前读取的系统表中信息，读取属性信息，结果保存在ctmp中
	column = (SysColumn *)malloc(attrcount*sizeof(SysColumn));
	ctmp = column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
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

	//通过getdata函数获取系统表信息,得到的信息保存在recdata中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_data, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的数据表中的记录,并将记录信息保存于recdata中
	while (GetNextRec(&FileScan, &recdata) == SUCCESS){	//取记录做判断
		for (i = 0, torf = 1,contmp = conditions;i < nConditions; i++, contmp++){//conditions条件逐一判断
			ctmpleft = ctmpright = column;//每次循环都要将遍历整个系统列文件，找到各个条件对应的属性
			//左属性右值
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->lhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpleft++;
				}
				//对conditions的某一个条件进行判断
				switch (ctmpleft->attrType){//判定属性的类型
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
			//右属性左值
			if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->rhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpright++;
				}
				//对conditions的某一个条件进行判断
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
			//左右均属性
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->lhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpleft++;
				}
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->rhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpright++;
				}
				//对conditions的某一个条件进行判断
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

	//关闭文件句柄
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
	//只能进行单值更新
	RM_FileHandle *rm_data, *rm_table, *rm_column;
	RC rc;
	RM_FileScan FileScan;
	RM_Record recdata, rectab, reccol;
	SysColumn *column, *ctmp,*cupdate,*ctmpleft,*ctmpright;
	Condition *contmp;
	int i, torf;//是否符合删除条件
	int attrcount;//临时 属性数量
	int intleft,intright;
	char *charleft,*charright;
	float floatleft,floatright;//临时 属性的值
	AttrType attrtype;

	//打开数据表,系统表，系统列文件
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

	//通过getdata函数获取系统表信息,得到的信息保存在rectab中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	while (GetNextRec(&FileScan, &rectab) == SUCCESS){
		if (strcmp(relName, rectab.pData) == 0){
			memcpy(&attrcount, rectab.pData + 21, sizeof(int));
			break;
		}
	}

	//通过getdata函数获取系统列信息,得到的信息保存在reccol中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的系统表中的记录,并将记录信息保存于rectab中
	//根据之前读取的系统表中信息，读取属性信息，结果保存在ctmp中
	column = (SysColumn *)malloc(attrcount*sizeof(SysColumn));
	cupdate = (SysColumn *)malloc(sizeof(SysColumn));
	ctmp = column;
	while (GetNextRec(&FileScan, &reccol) == SUCCESS){
		if (strcmp(relName, reccol.pData) == 0){//找到表名为relName的第一个记录，依次读取attrcount个记录
			for (int i = 0; i < attrcount; i++){
				memcpy(ctmp->tabName, reccol.pData, 21);
				memcpy(ctmp->attrName, reccol.pData + 21, 21);
				memcpy(&(ctmp->attrType), reccol.pData + 42, sizeof(AttrType));
				memcpy(&(ctmp->attrLength), reccol.pData + 42 + sizeof(AttrType), sizeof(int));
				memcpy(&(ctmp->attrOffset), reccol.pData + 42 + sizeof(int)+sizeof(AttrType), sizeof(int));
				if ((strcmp(relName,ctmp->tabName) == 0) && (strcmp(attrName,ctmp->attrName) == 0)){
					cupdate = ctmp;//找到要更新数据 对应的属性
				}
				rc = GetNextRec(&FileScan, &reccol);
				if (rc != SUCCESS)
					break;
				ctmp++;
			}
			break;
		}
	}

	//通过getdata函数获取系统表信息,得到的信息保存在recdata中
	FileScan.bOpen = false;
	rc = OpenScan(&FileScan, rm_data, 0, NULL, NO_HINT);
	if (rc != SUCCESS)
		return rc;
	//循环查找表名为relName对应的数据表中的记录,并将记录信息保存于recdata中
	while (GetNextRec(&FileScan, &recdata) == SUCCESS){
		for (i = 0, torf = 1, contmp = conditions; i < nConditions; i++, contmp++){//conditions条件逐一判断
			ctmpleft = ctmpright = column;//每次循环都要将遍历整个系统列文件，找到各个条件对应的属性
			//左属性右值
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->lhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpleft++;
				}
				//对conditions的某一个条件进行判断
				if (ctmpleft->attrType == ints){//判定属性的类型
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
			//右属性左值
			else  if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->rhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpright++;
				}
				//对conditions的某一个条件进行判断
				if (ctmpright->attrType == ints){//判定属性的类型
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
			//左右均属性
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->lhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->lhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->lhsAttr.relName, relName);
					}
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpleft++;
				}
				for (int j = 0; j < attrcount; j++){//attrcount个属性逐一判断
					if (contmp->rhsAttr.relName == NULL){//当条件中未指定表名时，默认为relName
						contmp->rhsAttr.relName = (char *)malloc(21);
						strcpy(contmp->rhsAttr.relName, relName);
					}
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					ctmpright++;
				}
				//对conditions的某一个条件进行判断
				if (ctmpright->attrType == ints && ctmpleft->attrType == ints){//判定属性的类型
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
	//关闭文件句柄
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

// 上边的索引没有完成，删除，更新操作中的条件只对左边属性右边值这种情况进行了判断，且只对类型为ints型的做了判断
//注意顺序！！！！！！！！！！！

RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char ** relations,
int nConditions, Condition *conditions, char **RES){
	RM_FileHandle *rm_data, *rm_table, *rm_column;
	RM_FileScan FileScan;
	RM_FileScan *relscan;
	RM_Record recdata, rectab, reccol;
	RM_Record *recdkr;//保存一条笛卡尔记录
	SysColumn *column, *ctmp,*ctmpleft,*ctmpright;
	Condition *contmp;
	int allattrcount = 0;//涉及的nRelations个数据表的属性数量之和
	int *attrcount;//各个数据表包含的属性数量
	int allcount = 1;//笛卡尔积的数量
	int torf;//是否符合删除条件
	int resultcount = 0;//结果数
	int intvalue,intleft,intright;
	char *charvalue,*charleft,*charright;
	float floatvalue,floatleft,floatright;
	RC rc;
	printrel *pr;
	char *res;
	AttrType attrtype;
	char *attention;

	pr = (printrel*)malloc(nSelAttrs * sizeof(printrel));
	//打开数据表,系统表，系统列文件
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
	//打开nRelations个数据表，并返回文件句柄到rm_data + i中
	rm_data = (RM_FileHandle *)malloc(nRelations*sizeof(RM_FileHandle));
	for (int i = 0; i <nRelations; i++){
		(rm_data + i)->bOpen = false;
		rc = RM_OpenFile(relations[i], rm_data + i);
		if (rc != SUCCESS)
			return rc;
	}//注意顺序： rm_data -> relations   : 0-(nRelations-1)  -> 0-(nRelations-1)

	//读取系统表，获取属性数量信息，
	//总数量保存于_allattrcount,各个关系的属性数量保存于*（_attrcount + i ）中
	attrcount = (int *)malloc(nRelations*sizeof(int));
	//找到各个表名对应的属性数量，并保存在_attrcount数组中
	for (int i = 0; i < nRelations; i++){
		FileScan.bOpen = false;
		rc = OpenScan(&FileScan, rm_table, 0, NULL, NO_HINT);//每次查找初始化！
		if (rc != SUCCESS)
			return rc;
		while (GetNextRec(&FileScan, &rectab) == SUCCESS){
			if (strcmp(relations[i], rectab.pData) == 0){
				memcpy(attrcount + i, rectab.pData + 21, sizeof(int));
				allattrcount += *(attrcount + i);//计算所涉及关系的属性的总和，保存在_allattrcount中
				break;
			}
		}
	}//注意顺序：_attrcount -> relations : 0-(nRelations-1) -> 0-(nRelations-1)
	//读取系统列，获取涉及数据表的全部属性信息
	//保存于以column为起始地址的空间中，顺序是按照relations表名的顺序
	column = (SysColumn *)malloc(allattrcount*sizeof(SysColumn));//申请_allattrcount个Syscolumn空间，用于保存nRealtions个关系的属性
	ctmp = column;
	for (int i = 0; i < nRelations; i++){
		FileScan.bOpen = false;
		rc = OpenScan(&FileScan, rm_column, 0, NULL, NO_HINT);
		if (rc != SUCCESS)
			return rc;//查找每个表的属性信息都要初始化扫描
		while (GetNextRec(&FileScan, &reccol) == SUCCESS){
			if (strcmp(relations[i], reccol.pData) == 0){//找到表名为*（relations + i）的第一个记录
				for (int j = 0; j < *(attrcount + i); j++){  //依次读取*（_attrcount + i）个属性
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
	//获取selAttrs对应的笛卡尔积的位置
	
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
	//读取数据表，获取记录数
	//保存在recordcount中，顺序是按照表名relations的顺序
	_recordcount = (int *)malloc(nRelations*sizeof(int));
	for (int i = 0; i < nRelations; i++){
		FileScan.bOpen = false;
		rc = OpenScan(&FileScan, rm_data + i, 0, NULL, NO_HINT);
		if (rc != SUCCESS)
			return rc;//查找每个表的属性信息都要初始化扫描
		*(_recordcount + i) = 0;
		while (GetNextRec(&FileScan, &recdata) == SUCCESS){
			*(_recordcount + i) += 1;
		}
	}
	//提取的信息包括各数据表属性数量，各数据表属性信息，各数据表的记录数
	//第一个字符串保存属性数量，第二个字符串保存记录数，接下来的nSelAttrs个字节保存属性名
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
	//通过笛卡尔算法来实现各个关系的连接
	counter = (int *)malloc(nRelations * sizeof(int));
	for (int i = 0; i < nRelations; i++){
		counter[i] = 0;
	}
	counterIndex = nRelations - 1;
	tablecount = nRelations;
	relscan = (RM_FileScan *)malloc(nRelations*sizeof(RM_FileScan));
	recdkr = (RM_Record *)malloc(nRelations*sizeof(RM_Record));
	//对nRelations个数据表分别初始化扫描，并计算笛卡尔积的条目数
	for (int i = 0; i < nRelations; i++){
		allcount *= _recordcount[i];
		(relscan + i)->bOpen = false;   
		rc = OpenScan(relscan + i, rm_data + i, 0, NULL, NO_HINT);
		if (rc != SUCCESS)
			return rc;
	}
	
	for (int i = 0; i < allcount; i++){
		//取出一条笛卡尔记录，一条笛卡尔记录对应nRelations个表中的一条记录
		//取出relations +j的数据表的的第counter[j]个记录，放在recdkr + j处
		//处理一条记录之后要将relscan 重新初始化扫描
		for (int j = 0; j < nRelations; j++){
			for (int k = 0; k != counter[j] + 1; k++){
				GetNextRec(relscan + j, recdkr + j);
			}
			(relscan + j)->bOpen = false;
			rc = OpenScan(relscan + j, rm_data + j, 0, NULL, NO_HINT);
			if (rc != SUCCESS)
				return rc;
		}
		//进行判断
		torf = 1;
		contmp = conditions;
		for (int x = 0; x < nConditions; x++, contmp++){//对nSelAttrs个条件逐一判断
			ctmpleft = ctmpright = column;//将ctmp重置到column处
			//左属性右值
			if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 0){
				for (int y = 0; y < allattrcount; y++, ctmpleft++){//从attrcount个属性找到对应于conditions表名和属性名对应的记录
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)//找到条件中的属性对应系统表中的属性相关信息
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){
						break;
					}
					if (y == allattrcount - 1){
						attention = strcat(contmp->lhsAttr.relName, ".");
						attention = strcat(attention, contmp->lhsAttr.attrName);
						attention = strcat(attention, "\r\n不存在，请检查sql语句");
						AfxMessageBox(attention);
						return FAIL;
					}
				}
				
				for (int z = 0; z < nRelations; z++){//找到这个条件的属性对应的表在relations数组中的下标位置    
					if (strcmp(relations[z], ctmpleft->tabName) == 0){
						//对conditions的某一个条件进行判断
						if (ctmpleft->attrType == ints){//判定属性的类型
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

			//右属性左值
			else  if (contmp->bLhsIsAttr == 0 && contmp->bRhsIsAttr == 1){
				for (int y = 0; y < allattrcount; y++, ctmpright++){//attrcount个属性逐一判断
					if ((strcmp(ctmpright->tabName, contmp->rhsAttr.relName) == 0)
						&& (strcmp(ctmpright->attrName, contmp->rhsAttr.attrName) == 0)){//根据表名属性名找到对应属性
						break;
					}
					if (y == allattrcount - 1){
						attention = strcat(contmp->rhsAttr.relName, ".");
						attention = strcat(attention, contmp->rhsAttr.attrName);
						attention = strcat(attention, "\r\n不存在，请检查sql语句");
						AfxMessageBox(attention);
						return FAIL;
					}
				}
				for (int z = 0; z < nRelations; z++){//找到这个条件的属性对应的表在relations数组中的下标位置    
					if (strcmp(relations[z], ctmpleft->tabName) == 0){
						//对conditions的某一个条件进行判断
						if (ctmpright->attrType == ints){//判定属性的类型
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
			//左右均属性
			else  if (contmp->bLhsIsAttr == 1 && contmp->bRhsIsAttr == 1){
				for (int y = 0; y < allattrcount; y++, ctmpleft++){
					if ((strcmp(ctmpleft->tabName, contmp->lhsAttr.relName) == 0)
						&& (strcmp(ctmpleft->attrName, contmp->lhsAttr.attrName) == 0)){
						break;
					}
					if (y == allattrcount - 1){
						attention = strcat(contmp->lhsAttr.relName, ".");
						attention = strcat(attention, contmp->lhsAttr.attrName);
						attention = strcat(attention, "\r\n不存在，请检查sql语句");
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
						attention = strcat(attention, "\r\n不存在，请检查sql语句");
						AfxMessageBox(attention);
						return FAIL;
					}
				}
				//对conditions的某一个条件进行判断
				for (int z = 0; z < nRelations; z++){//找到这个条件的属性对应的表在relations数组中的下标位置    
					if (strcmp(relations[z], ctmpleft->tabName) == 0){
						//对conditions的某一个条件进行判断
						if (ctmpleft->attrType == ints){//判定属性的类型
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
						//对conditions的某一个条件进行判断
						if (ctmpright->attrType == ints){//判定属性的类型
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
	resultcount++;//因为还要存储一行表头，所以此处要++
	charvalue = (char *)malloc(20);
	itoa(resultcount, charvalue, 10);
	memcpy(res + 20, charvalue, 20);
	free(charvalue);

	// 释放内存，关闭文件句柄
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
