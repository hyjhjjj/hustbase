#ifndef SYS_MANAGER_H_H
#define SYS_MANAGER_H_H

#include "IX_Manager.h"
#include "PF_Manager.h"
#include "RM_Manager.h"
#include "str.h"

//ϵͳ����ģ��
RC CreateDb(char *dbPath);
RC DropDb(char *dbName);
RC OpenDb(char *dbName);
RC CloseDb();
RC CreateTable(char *relName, int attrCount, AttrInfo *attributes);
RC DropTable(char *relName);
RC CreateIndex(char *indexName, char *relName, char *attrName);
RC DropIndex(char *indexName);
RC Insert(char *relName, int nValues, Value *values);
RC Delete(char *relName, int nConditions, Condition *conditions);
RC Update(char *relName, char *attrName, Value *Value, int nConditions, Condition *conditions);

//��ѯ����ģ��
RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char ** relations,
	int nConditions, Condition *conditions, char **res);
void handle();

#endif