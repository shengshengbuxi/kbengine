// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com

#ifndef KBE_WRITE_ENTITY_HELPER_H
#define KBE_WRITE_ENTITY_HELPER_H

// common include	
// #define NDEBUG
#include <sstream>
#include "common.h"
#include "sqlstatement.h"
#include "common/common.h"
#include "common/memorystream.h"
#include "helper/debug_helper.h"
#include "db_interface/db_interface.h"
#include "db_interface/entity_table.h"
#include "db_interface_mysql.h"

namespace KBEngine{ 

class WriteEntityHelper
{
public:
	WriteEntityHelper()
	{
	}

	virtual ~WriteEntityHelper()
	{
	}

	static SqlStatement* createSql(DBInterface* pdbi, DB_TABLE_OP opType, mysql::DBContext& context)
	{
		SqlStatement* pSqlcmd = NULL;


		switch(opType)
		{
		case TABLE_OP_UPDATE:
			if(context.dbid > 0)
				//pSqlcmd = new SqlStatementUpdate(pdbi, tableName, parentDBID, dbid, tableVal);
				pSqlcmd = new SqlStatementUpdate(pdbi, context.tableName, context.parentTableDBID, context.dbid, context.items, context.version);

			else
				pSqlcmd = new SqlStatementInsert(pdbi, context.tableName, context.parentTableDBID, context.dbid, context.items, context.version);
			break;
		case TABLE_OP_INSERT:
			pSqlcmd = new SqlStatementInsert(pdbi, context.tableName, context.parentTableDBID, context.dbid, context.items, context.version);
			break;
		case TABLE_OP_DELETE:
			break;
		case TABLE_OP_UPSERT:
			pSqlcmd = new SqlStatementUpsert(pdbi, context.tableName, context.parentTableDBID, context.dbid, context.items, context.version);
			break;
		default:
			KBE_ASSERT(false && "no support!\n");
		};

		return pSqlcmd;
	}

	/**
		将数据更新到表中
	*/
	static bool writeDB(DB_TABLE_OP optype, DBInterface* pdbi, mysql::DBContext& context)
	{
		bool ret = true;

		if(!context.isEmpty)
		{
			SqlStatement* pSqlcmd = createSql(pdbi, optype, context);
	

			ret = pSqlcmd->query();
			context.dbid = pSqlcmd->dbid();
			context.version = pSqlcmd->version();
			delete pSqlcmd;
		}

		ENTITY_DBID_VERSION_DATA* entityDBIDVersionChildTableDatas = NULL;
		KBEUnordered_map<std::string, ENTITY_DBID_VERSION_DATA*>::iterator entityDBIDVersionChildTableDatasIt;

		if (context.pEntityDBIDVersionData != NULL)
		{
			entityDBIDVersionChildTableDatasIt = context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas.end();
		}

		if(optype == TABLE_OP_INSERT)
		{

			// 开始更新所有的子表
			mysql::DBContext::DB_RW_CONTEXTS::iterator iter1 = context.optable.begin();
			for(; iter1 != context.optable.end(); ++iter1)
			{
				mysql::DBContext& wbox = *(iter1->second.get());

				if (context.pEntityDBIDVersionData != NULL) 
				{
					if (entityDBIDVersionChildTableDatasIt == context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas.end() || entityDBIDVersionChildTableDatasIt->first != wbox.tableName)
					{
						entityDBIDVersionChildTableDatas = NULL;
						entityDBIDVersionChildTableDatasIt = context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas.find(wbox.tableName);
						if (entityDBIDVersionChildTableDatasIt != context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas.end())
							entityDBIDVersionChildTableDatas = entityDBIDVersionChildTableDatasIt->second;

						if (entityDBIDVersionChildTableDatas == NULL)
						{
							entityDBIDVersionChildTableDatas = new ENTITY_DBID_VERSION_DATA();
							context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas[wbox.tableName] = entityDBIDVersionChildTableDatas;
						}
					}

				}
				
				// 绑定表关系
				wbox.parentTableDBID = context.dbid;
				wbox.pEntityDBIDVersionData = entityDBIDVersionChildTableDatas;

				// 更新子表
				writeDB(optype, pdbi, wbox);

				if (wbox.dbid != 0 && entityDBIDVersionChildTableDatas != NULL) 
				{
					entityDBIDVersionChildTableDatas->dbids[context.dbid].push_back(wbox.dbid);
					entityDBIDVersionChildTableDatas->versions[wbox.dbid] = wbox.version;
				}
			}
		}
		else
		{
			KBEUnordered_map< std::string,  std::pair<uint32, uint32> > childDBIDIndexes;
			
		
			
			KBEUnordered_map< std::string, std::vector<std::pair<DBID, std::string> > > childTableQueryResult;

			if (context.pEntityDBIDVersionData == NULL)
			{
				mysql::DBContext::DB_RW_CONTEXTS::iterator iter1 = context.optable.begin();


				for(; iter1 != context.optable.end(); ++iter1)
				{
					mysql::DBContext& wbox = *iter1->second.get();

					KBEUnordered_map<std::string, std::vector<std::pair<DBID, std::string> > >::iterator iter = 
						childTableQueryResult.find(wbox.tableName);

					if(iter == childTableQueryResult.end())
					{
						std::vector<std::pair<DBID, std::string> > v;
						childTableQueryResult.insert(std::pair< std::string, std::vector<std::pair<DBID, std::string> > >(wbox.tableName, v));
					}
				}
				
				if(childTableQueryResult.size() > 1)
				{
					std::string sqlstr_getids;
					KBEUnordered_map< std::string, std::vector<std::pair<DBID, std::string> > >::iterator tabiter = childTableQueryResult.begin();
					for(; tabiter != childTableQueryResult.end();)
					{
						char sqlstr[MAX_BUF * 10];
						kbe_snprintf(sqlstr, MAX_BUF * 10, "select count(id), NULL from " ENTITY_TABLE_PERFIX "_%s where " TABLE_PARENTID_CONST_STR "=%" PRDBID " union all ", 
							tabiter->first.c_str(),
							context.dbid);
						
						sqlstr_getids += sqlstr;

						kbe_snprintf(sqlstr, MAX_BUF * 10, "select id, " TABLE_VERSION_CONST_STR " from " ENTITY_TABLE_PERFIX "_%s where " TABLE_PARENTID_CONST_STR "=%" PRDBID, 
							tabiter->first.c_str(),
							context.dbid);

						sqlstr_getids += sqlstr;
						if(++tabiter != childTableQueryResult.end())
							sqlstr_getids += " union all ";
					}
					
					if(pdbi->query(sqlstr_getids.c_str(), sqlstr_getids.size(), false))
					{
						MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(pdbi)->mysql());
						if(pResult)
						{
							MYSQL_ROW arow;
							int32 count = 0;
							tabiter = childTableQueryResult.begin();
							bool first = true;

							while((arow = mysql_fetch_row(pResult)) != NULL)
							{
								if(count == 0)
								{
									StringConv::str2value(count, arow[0]);
									if(!first || count <= 0)
										tabiter++;
									continue;
								}

								DBID old_dbid;
								StringConv::str2value(old_dbid, arow[0]);

								std::pair<DBID, std::string> pair = std::make_pair(old_dbid, arow[1]);
					

								tabiter->second.push_back(pair);
								count--;
								first = false;
							}

							mysql_free_result(pResult);
						}
					}
				}
				else if(childTableQueryResult.size() == 1)
				{
					KBEUnordered_map< std::string, std::vector<std::pair<DBID, std::string> > >::iterator tabiter = childTableQueryResult.begin();
					char sqlstr[MAX_BUF * 10];
					kbe_snprintf(sqlstr, MAX_BUF * 10, "select id, " TABLE_VERSION_CONST_STR " from " ENTITY_TABLE_PERFIX "_%s where " TABLE_PARENTID_CONST_STR "=%" PRDBID, 
						tabiter->first.c_str(),
						context.dbid);

					if(pdbi->query(sqlstr, strlen(sqlstr), false))
					{
						MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(pdbi)->mysql());
						if(pResult)
						{
							MYSQL_ROW arow;
							while((arow = mysql_fetch_row(pResult)) != NULL)
							{
								DBID old_dbid;
								StringConv::str2value(old_dbid, arow[0]);


								std::pair<DBID, std::string> pair = std::make_pair(old_dbid, arow[1]);
					
								tabiter->second.push_back(pair);
							}

							mysql_free_result(pResult);
						}
					}
				}
			}


			KBEUnordered_map<std::string,  std::pair<uint32, uint32> >::iterator childDBIDIndexesIt = childDBIDIndexes.end();

			mysql::DBContext::DB_RW_CONTEXTS::iterator iter1 = context.optable.begin();
			for (; iter1 != context.optable.end(); ++iter1)
			{
				mysql::DBContext& wbox = *iter1->second.get();
				

				if (context.pEntityDBIDVersionData != NULL)
				{
					if (entityDBIDVersionChildTableDatasIt == context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas.end() || entityDBIDVersionChildTableDatasIt->first != wbox.tableName)
					{
						entityDBIDVersionChildTableDatas = NULL;
						entityDBIDVersionChildTableDatasIt = context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas.find(wbox.tableName);
						if (entityDBIDVersionChildTableDatasIt != context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas.end())
							entityDBIDVersionChildTableDatas = entityDBIDVersionChildTableDatasIt->second;

						if (entityDBIDVersionChildTableDatas == NULL)
						{
							entityDBIDVersionChildTableDatas = new ENTITY_DBID_VERSION_DATA();
							context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas[wbox.tableName] = entityDBIDVersionChildTableDatas;
						}
					}
				}
				
			
				wbox.parentTableDBID = context.dbid;
				wbox.pEntityDBIDVersionData = entityDBIDVersionChildTableDatas;

				if (childDBIDIndexesIt == childDBIDIndexes.end() || childDBIDIndexesIt->first != wbox.tableName)
				{
					childDBIDIndexesIt = childDBIDIndexes.find(wbox.tableName);
					
					if (childDBIDIndexesIt == childDBIDIndexes.end()) 
					{
						if (context.pEntityDBIDVersionData != NULL)
						{
							childDBIDIndexes[wbox.tableName] = std::pair<uint32, uint32>(0, entityDBIDVersionChildTableDatas->dbids[context.dbid].size());
						}
						else
						{
							childDBIDIndexes[wbox.tableName] = std::pair<uint32, uint32>(0, childTableQueryResult[wbox.tableName].size());
						}
					}

				}
		
								
				if (wbox.isEmpty)
					continue;
			

				if (childDBIDIndexes[wbox.tableName].second > childDBIDIndexes[wbox.tableName].first) 
				{
					if (context.pEntityDBIDVersionData != NULL)
					{
						wbox.dbid = entityDBIDVersionChildTableDatas->dbids[context.dbid][childDBIDIndexes[wbox.tableName].first];
						wbox.version = entityDBIDVersionChildTableDatas->versions[wbox.dbid];
					}
					else
					{
						wbox.dbid = childTableQueryResult[wbox.tableName][childDBIDIndexes[wbox.tableName].first].first;
						wbox.version = childTableQueryResult[wbox.tableName][childDBIDIndexes[wbox.tableName].first].second;
					}

				}

				
				writeDB(optype, pdbi, wbox);

				if (childDBIDIndexes[wbox.tableName].second > childDBIDIndexes[wbox.tableName].first) 
					childDBIDIndexes[wbox.tableName].first++;
				else 
				{
					if (entityDBIDVersionChildTableDatas != NULL)
						entityDBIDVersionChildTableDatas->dbids[context.dbid].push_back(wbox.dbid);
				}
					
				if (entityDBIDVersionChildTableDatas != NULL)
					entityDBIDVersionChildTableDatas->versions[wbox.dbid] = wbox.version;
			}

			childDBIDIndexesIt = childDBIDIndexes.begin();

			for (; childDBIDIndexesIt != childDBIDIndexes.end(); ++childDBIDIndexesIt)
			{
				

				std::string sqlstr = "delete from " ENTITY_TABLE_PERFIX "_";
				sqlstr += childDBIDIndexesIt->first;
				sqlstr += " where " TABLE_ID_CONST_STR " = ";

				std::string sqlstr1;

				uint32 delIndex = childDBIDIndexesIt->second.first;
				uint32 delCount = childDBIDIndexesIt->second.second - delIndex;

;
		
			
				while (delCount > 0)
				{
					sqlstr1 = sqlstr;

					DBID dbid = 0;

					if (context.pEntityDBIDVersionData != NULL)
					{
						ENTITY_DBID_VERSION_DATA* childDBIDsVersionsData = context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas[childDBIDIndexesIt->first];

						std::vector<DBID>& dbids = childDBIDsVersionsData->dbids[context.dbid];

						dbid = dbids[delIndex];
					}
					else
					{
						dbid = childTableQueryResult[childDBIDIndexesIt->first][delIndex].first;
					}

					char valuestr[MAX_BUF];
					kbe_snprintf(valuestr, MAX_BUF, "%" PRDBID, dbid);

					sqlstr1 += valuestr;
					bool ret = pdbi->query(sqlstr1.c_str(), sqlstr1.size(), false);
					KBE_ASSERT(ret);


					if (context.pEntityDBIDVersionData != NULL)
					{
						ENTITY_DBID_VERSION_DATA* childDBIDsVersionsData = context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas[childDBIDIndexesIt->first];

						std::vector<DBID>& dbids = childDBIDsVersionsData->dbids[context.dbid];
					
						mysql::DBContext wbox;

						wbox.parentTableName = context.tableName;
						wbox.tableName = childDBIDIndexesIt->first;
						wbox.parentTableDBID = context.dbid;
						wbox.dbid = dbid;
						wbox.isEmpty = true;
						wbox.pEntityDBIDVersionData = childDBIDsVersionsData;
						wbox.version = wbox.pEntityDBIDVersionData->versions[wbox.dbid];
					
						KBEUnordered_map<std::string, ENTITY_DBID_VERSION_DATA*>& delGrandsonDBIDsVersionsTable =  context.pEntityDBIDVersionData->entityDBIDVersionChildTableDatas[wbox.tableName]->entityDBIDVersionChildTableDatas;

						for (KBEUnordered_map<std::string, ENTITY_DBID_VERSION_DATA*>::iterator delGrandsonDBIDsVersionsTableIt = delGrandsonDBIDsVersionsTable.begin();
							delGrandsonDBIDsVersionsTableIt != delGrandsonDBIDsVersionsTable.end(); ++delGrandsonDBIDsVersionsTableIt) 
						{
							std::vector<DBID>& delGrandsonDBIDs = delGrandsonDBIDsVersionsTableIt->second->dbids[wbox.dbid];

							for (std::vector<DBID>::iterator delGrandsonDBIDsIt = delGrandsonDBIDs.begin(); delGrandsonDBIDsIt != delGrandsonDBIDs.end(); ++delGrandsonDBIDsIt)
							{
								mysql::DBContext* grandsonContext = new mysql::DBContext();
								grandsonContext ->parentTableName = wbox.tableName;
								grandsonContext ->tableName = delGrandsonDBIDsVersionsTableIt->first;
								grandsonContext->parentTableDBID = wbox.dbid;
								grandsonContext->dbid = *delGrandsonDBIDsIt;
								grandsonContext->isEmpty = true;
								grandsonContext->version = delGrandsonDBIDsVersionsTableIt->second->versions[grandsonContext->dbid];

								wbox.optable.push_back(std::pair<std::string, KBEShared_ptr< mysql::DBContext > >(grandsonContext ->tableName, KBEShared_ptr< mysql::DBContext >(grandsonContext)));
							}
						
						}
						writeDB(optype, pdbi, wbox);

						wbox.pEntityDBIDVersionData->versions.erase(wbox.dbid);
						dbids.erase(childDBIDsVersionsData->dbids[context.dbid].begin()+delIndex);
					} 
					else
					{
						

						mysql::DBContext::DB_RW_CONTEXTS::iterator iter1 = context.optable.begin();
						for(; iter1 != context.optable.end(); ++iter1)
						{
							mysql::DBContext& wbox = *iter1->second.get();
							if(wbox.tableName == childDBIDIndexesIt->first)
							{
								
								DBID dbid = childTableQueryResult[wbox.tableName][delIndex].first;
							
								wbox.parentTableDBID = context.dbid;
								wbox.dbid = dbid;
								wbox.isEmpty = true;

								// 更新子表
								writeDB(optype, pdbi, wbox);

								childTableQueryResult[wbox.tableName].erase(childTableQueryResult[wbox.tableName].begin()+delIndex);
								break;
							}
						}
					}
					delCount--;
				}
						
			}
		}
		return ret;
	}

protected:

};

}
#endif // KBE_WRITE_ENTITY_HELPER_H

