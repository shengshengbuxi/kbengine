// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com

#ifndef KBE_MYSQL_DB_RW_CONTEXT_H
#define KBE_MYSQL_DB_RW_CONTEXT_H

#include "common/common.h"
#include "common/memorystream.h"
#include "helper/debug_helper.h"
#include "db_interface/entity_table.h"

namespace KBEngine { 
namespace mysql {

/**
	��дɾ����ʱ���õ�������ȡ�����д��ĸ�����Ϣ��

	dbid��������������ʵ���dbid���ӱ���ǵ�ǰ��ѯ��dbid

	dbids��������dbids��ֻ��һ��dbid������ʵ���id�����ʵ�����ݴ���������������ӱ���֣���������ݽṹ��������һ���ӱ��ʱ��
		dbids�����������ӱ�����, ÿ��dbid����ʾ����ӱ��϶�Ӧ��ֵ���Ұ�������˳��ͬʱҲ��ʾ�������ж�Ӧλ�õ�ֵ��
		dbids = {
			123 : [xxx, xxx, ...], // 123Ϊ�����ϵ�ĳ��dbid������Ϊ���ӱ����븸��������ĵ�dbids��
			...
		}

	items�������������ֶ���Ϣ�������д�����ֶ���Ҳ�ж�Ӧ��Ҫдֵ��

	optable���ӱ�ṹ

	results��������ʱ��ѯ��������, ���ݵ����ж�Ӧitems�е�strkey����������dbids��������
	readresultIdx����Ϊresults�е�������dbids * items�����Ե���ĳЩ�ݹ����ʱ��������ݻ�������readresultIdx��������λ�á�

	parentTableDBID�������dbid
	parentTableName�����������

	tableName����ǰ�������
 */
class DBContext
{
public:
	/**
		�洢����Ҫ�����ı�item�ṹ
	*/
	struct DB_ITEM_DATA
	{
		DB_ITEM_DATA()
		{
			sqlkey = NULL;
		}

		char sqlval[MAX_BUF];
		const char* sqlkey;
		std::string extraDatas;
	};

	typedef std::vector< std::pair< std::string/*tableName*/, KBEShared_ptr< DBContext > > > DB_RW_CONTEXTS;
	typedef std::vector< KBEShared_ptr<DB_ITEM_DATA>  > DB_ITEM_DATAS;

	DBContext()
	{
		pEntityDBIDVersionData = NULL;
	}

	~DBContext()
	{
	}
	
	DB_ITEM_DATAS items;
	
	std::string tableName;
	std::string parentTableName;
	
	DBID parentTableDBID;
	DBID dbid;
	
	

	DB_RW_CONTEXTS optable;
	
	//DB_RW_CONTEXTS deleteOptable;

	std::string version;
	//std::unordered_map<std::string, KBEShare > deleteOptable;

	
	
	bool isEmpty;
	
	KBEUnordered_map<DBID, std::vector<DBID> > dbids;
	KBEUnordered_map<DBID, std::pair< std::vector<std::string>::size_type, std::vector<std::string> > > results;
	//KBEUnordered_map<DBID, std::string> versions;

	ENTITY_DBID_VERSION_DATA* pEntityDBIDVersionData;
	//std::vector<std::pair<DBID, std::string> >versions;
	//uint32 versionIndex
	

	//uint32 digest[5];
	

	

private:

};

}
}
#endif // KBE_MYSQL_DB_RW_CONTEXT_H

