#pragma once
#include "engine/string/engstring.h"
#include "util/unorgtypes.h"
#include "engine/mem.h"

class CDictionary;
class CArray;
class CTemplate;
union char16_s;
class CStrPool;
class CEmbeddedResFile;
class CPackedResFile;

union TDictDataValue
{
	int i;
	float f;
	const char* s;
	CDictionary* d;
	CArray* a;
	void* v;
};

class IContainer 
{
public:
	virtual ~IContainer();
	virtual int CountEntries() const;
	virtual bool GetNthEntry(int inIndex, const char** outName, unsigned char* outType, TDictDataValue* outValue, __int16* outFlags);
	virtual unsigned char GetContainerType() const;
	virtual int GetHandle();
};

class CDictionary : public IContainer 
{ 
public:
	/*
	virtual bool IsOfObjectType(unsigned int);
	virtual ~CDictionary();
	virtual void Clear();
	*/
	virtual void SetTemplate(CTemplate*);
	
	virtual CTemplate* GetTemplate();
	
	//CTemplate* GetBaseTemplate();
	//bool InheritsFromTemplate(CTemplate*);

	virtual int CountValues() const;
	virtual bool GetNthValue(int, const char**, unsigned char*, TDictDataValue*, __int16*);
	
	
	bool GetOptionalValue(const char* inKey, unsigned __int8* outType,
		TDictDataValue* outValue, unsigned __int16* outFlags);
	/*
	bool GetValue(const char*, unsigned char*, TDictDataValue*, __int16*);
	int CountDataTypesInDictRecursive(unsigned char);
	int CountValuesRecursive();
	bool GetOptionalBoolValue(const char*, bool);
	*/
	bool GetOptionalBoolValue(const char* inKeyName, bool* outValue, bool defaultValue);
	/*
	int GetOptionalIntValue(const char*);
	*/
	bool GetOptionalIntValue(const char* inKeyName, TDictDataValue* outValue, const int defaultValue);
	/*
	const char* GetOptionalStringValue(const char*);
	*/

	bool GetOptionalStringValue(const char*, const char**);
	

	bool GetStringValue(const char* inKeyName, const char** outValue);
	/*
	float GetOptionalFloatValue(const char*, float);
	bool GetOptionalFloatValue(const char*, float*, float);
	CDictionary* GetOptionalDictValue(const char*);
	bool GetOptionalDictValue(const char*, CDictionary**);
	CArray* GetOptionalArrayValue(const char*);
	bool GetOptionalArrayValue(const char*, CArray**);
	IContainer* GetOptionalIContainerValue(const char*);
	bool GetOptionalIContainerValue(const char*, IContainer**);
	bool GetOptionalFlagsValue(const char*, unsigned __int64*, const char**, int);
	bool GetOptionalFlagsValue(const char*, unsigned int*, const char**, int);
	bool GetOptionalChar16Value(const char*, char16_s*);
	bool GetOptionalTemplateValue(const char*, CTemplate**);
	bool GetBoolValue(const char*);
	bool GetBoolValue(const char*, bool*);
	int GetIntValue(const char*);
	bool GetIntValue(const char*, int*);

	


	bool GetStringValue(const char*, const char**);
	bool GetStringValueCopy(const char*, char*, int);
	float GetFloatValue(const char*);
	bool GetFloatValue(const char*, float*);
	*/
	//CDictionary* GetDictValue(const char*);

	bool GetDictValue(const char* inKeyName, CDictionary** outDict);

	/*
	CArray* GetArrayValue(const char*);
	bool GetArrayValue(const char*, CArray**);
	IContainer* GetIContainerValue(const char*);
	bool GetIContainerValue(const char*, IContainer**);
	int GetFloatArrayValue(const char*, float*, int);
	int GetIntArrayValue(const char*, int*, int);
	bool GetFlagsValue(const char*, unsigned __int64*, const char**, int);
	bool GetFlagsValue(const char*, unsigned int*, const char**, int);
	void GetChar16Value(const char*, char16_s*);
	void GetTemplateValue(const char*, CTemplate**);
	const char* GetOptionalStringValueCopy(const char*, char*, int, const char*);
	int GetOptionalFloatArrayValue(const char*, float*, int);
	int GetOptionalIntArrayValue(const char*, int*, int);
	bool GetIntAsIndexedString(const char*, int*, const char**, const int);
	void GetFlagsAsIndexedString(const char*, unsigned int*, const char**, int, const char*, unsigned int);
	bool GetOptionalVariableSizeStringArrayValue(const char*, char*, int, int, int*);
	void EnterInt(const char*, int);
	void EnterFloat(const char*, float);
	void EnterFloatArray(const char*, float*, int);
	void EnterIntArray(const char*, int*, int);
	void EnterBool(const char*, bool);
	void EnterStringNoCopy(const char*, const char*);
	virtual void EnterStringCopy(const char*, const char*, CStrPool*);
	void EnterDictNoCopy(const char*, CDictionary*);
	void EnterIndexedDictNoCopy(CDictionary*);
	void EnterArrayNoCopy(const char*, CArray*);
	void EnterDictionaryContentsNoCopy(CDictionary*);
	void EnterDictionaryFlagsNoCopy(char*, unsigned int, const char**, int, char*, int);
	void EnterDictionaryFlagsCopy(char*, unsigned int, const char**, int, CStrPool*);
	void EnterTemplateValue(const char*, CTemplate**);
	bool KeyExistsSelf(const char*);
	virtual bool KeyExists(const char*);
	*/

	virtual bool GetOptionalValueSelf(const char* inKeyName, unsigned char* outType,
		TDictDataValue* outValue, unsigned short* outFlags);
	
	virtual void EnterValue(
		const char* inHashKey, unsigned char inType,
		const TDictDataValue* inValue, unsigned short inFlags);
	
	
	virtual void EnterValueWithSourceLocation(const char* inKey, unsigned char inFlags,
		const TDictDataValue* inValue, unsigned short inKeyLen,
		const char* inSourceFileName, int inSourceLineNo);
	
	//virtual bool GetEntrySourceLocation(const char*, const char**, int*);

	virtual bool GetDictSourceLocation(const char** outSourceFileName, int* outSourceLineNo);
	
	virtual void SetDictSourceLocation(const char* inSourceFileName, int inSourceLineNo);
	/*
	virtual bool RemoveValue(const char*);
	virtual bool RemoveNthValue(int);
	virtual void SetNoOwnership();
	virtual bool SetValueFlags(const char*, __int16);
	void RenameKey(const char*, const char*);
	void EnterAllValuesNoOwnership(CDictionary*);
	virtual int CountEntries()  const;
	virtual bool GetNthEntry(int, const char**, unsigned char*, TDictDataValue*, __int16*);

	virtual unsigned char GetContainerType() const;

	static bool AreIdentical(CDictionary*, CDictionary*);
	static void Transfer(CDictionary*, CDictionary*, bool);
	*/

	static int s_heapDictContents;
};

class CTemplateOnlyDictionary : public CDictionary 
{
public:
	/*
	virtual void SetTemplate(CTemplate*);
	virtual CTemplate* GetTemplate();
	virtual void Clear();
	virtual int CountValues() const;
	virtual bool GetNthValue(int, const char**, unsigned char*, TDictDataValue*, unsigned __int16*);
	virtual bool GetOptionalValueSelf(const char*, unsigned char*, TDictDataValue*, unsigned __int16*);
	virtual void EnterValue(const char*, unsigned char, const TDictDataValue*, unsigned __int16);
	virtual bool RemoveValue(const char*);
	virtual void SetNoOwnership();
	virtual bool SetValueFlags(const char*, unsigned __int16);
	virtual ~CTemplateOnlyDictionary();
	*/
	CTemplate* m_template;
};

class CTemplate 
{
public:
	/*
	virtual ~CTemplate();
	void ChangeDict(CDictionary*);
	void SetFieldMetaData();
	bool IsFieldMetaData();
	*/
	const char* GetMetaDataFieldName();
	
	bool GetDefaultValue(const char* inKey, unsigned __int8* outType,
		TDictDataValue* outValue, unsigned __int16* outFlags);
	/*
	const char* GetName();
	CDictionary* GetDict();
	CDictionary* GetSelfDict();
	bool IsSubTemplate();
	bool IsSubTemplateOf(CTemplate*);
	CTemplate* GetSubTemplate(const char*);
	CTemplate* GetSubTemplateContainer(const char**);
	bool GetArrayType(const char*, unsigned char*, CTemplate**);
	CTemplate* GetParentTemplate();
	CTemplate* GetFieldMetaDataTemplate(const char*);
	CDictionary* GetFieldMetaDataDict(const char*);
	*/

	CTemplate();

	void Init(CDictionary* inDict);

	CDictionary* m_dict;
	CTemplateOnlyDictionary m_self;
	bool m_isFieldMeta;
	stringTableElement ele;
};

union UTemplatePtrIdx
{
	CTemplate* ptr;
	int idx;
};

union UEmbeddedData
{
	TDictDataValue dv;
	int i;
	float f;
	unsigned int s;
	unsigned int d;
	unsigned int a;
	void* v;
};

struct SKeyValueData
{
	unsigned int keyAndType;
	UEmbeddedData data;
};

class CEmbeddedDictionary : public CDictionary
{
public:
	//void GetTypeValueFlags(const SKeyValueData*, unsigned char*, TDictDataValue*, uint16_t*);
	//SKeyValueData* FindValue(const char*);

	//virtual ~CEmbeddedDictionary();
	//virtual void Clear();
	//virtual void SetTemplate(CTemplate*);
	virtual CTemplate* GetTemplate() override;
	//int32_t GetTemplateIdx();
	//virtual int32_t CountValues() const;
	//virtual bool GetNthValue(int32_t, const char**, unsigned char*, TDictDataValue*, uint16_t*);
	//virtual bool GetOptionalValueSelf(const char*, unsigned char*, TDictDataValue*, uint16_t*);
	//virtual void EnterValue(const char*, unsigned char, const TDictDataValue*, uint16_t);
	//virtual bool RemoveValue(const char*);
	//virtual void SetNoOwnership();
	//virtual bool SetValueFlags(const char*, uint16_t);
	//virtual uint32_t GetHandle();
	//uint32_t GetOffsetInResFile();
	//const SKeyValueData* GetNthKeyValueData(int32_t);
	//int16_t GetResFileIndex();


	CEmbeddedDictionary(CEmbeddedResFile* inResFile);

	UTemplatePtrIdx m_template;
	unsigned __int16 m_numValues;
	__int16 m_resFile;
	SKeyValueData m_keyValueData[];
};

class CArray : public IContainer
{
public:
	/*
	virtual void SetTemplate(CTemplate*);
	void InitArrayType(unsigned char, CTemplate*);
	virtual ~CArray();
	virtual CTemplate* GetTemplate() const;
	*/
	virtual void GetArrayType(unsigned char*, CTemplate**) const;
	virtual int32_t CountItems() const;
	virtual bool GetNthItem(int32_t, TDictDataValue*, uint16_t*);
	/*
	virtual bool GetNthIntItem(int32_t, int32_t*);
	virtual bool GetNthFloatItem(int32_t, float*);
	virtual bool GetNthVoidItem(int32_t, void*);
	virtual bool GetNthFloatArray(int32_t, int32_t, float*);
	virtual bool GetNthStringItem(int32_t, const char**);
	virtual bool GetNthDictItem(int32_t, CDictionary**);
	void EnterInt(int32_t);
	void EnterFloat(float);
	void EnterStringNoCopy(const char*);
	void EnterDictNoCopy(CDictionary*);
	virtual void SetNewLineSpan(int32_t);
	virtual int32_t GetNewLineSpan();
	*/
	virtual bool AppendItem(TDictDataValue*, uint16_t);
	/*
	virtual bool AppendArrayItemsNoCopy(CArray*);
	virtual bool SetItem(int32_t, TDictDataValue*, uint16_t);
	void DisposeItem(TDictDataValue*, uint16_t);
	virtual int32_t CountEntries() const;
	virtual bool GetNthEntry(int32_t, const char**, unsigned char*, TDictDataValue*, uint16_t*);
	virtual unsigned char GetContainerType() const;
	*/

	unsigned __int8 m_primType;
	unsigned __int8 m_embeddedResFileIdx;
	unsigned __int16 m_embeddedNumValues;
};

class CEmbeddedArray : public CArray
{
public:
	CEmbeddedArray(CEmbeddedResFile* inResFile);
	UEmbeddedData m_data[];
};

template<typename T>
class IContainerT
{
public:
	virtual ~IContainerT() {}
	virtual bool  ICT_HasBeenInited() = 0;
	virtual void  ICT_InitFromHeap(int itemCount, unsigned int heap) = 0;
	virtual void  ICT_RemoveAll() = 0;
	virtual T* ICT_Append(const T& item) = 0;
	virtual int ICT_ItemCount() = 0;
	virtual T& ICT_Ref(int index) = 0;
};

template<typename T>
class CList : public IContainerT<T>
{
public:
	virtual bool  ICT_HasBeenInited() override { return false; }
	virtual void  ICT_InitFromHeap(int itemCount, unsigned int heap) override {}
	virtual void  ICT_RemoveAll() override {}
	virtual T* ICT_Append(const T& item) override { return nullptr; }
	virtual int ICT_ItemCount() override { return 0; }
	virtual T& ICT_Ref(int index) override 
	{
		static T dummy{};
		return dummy;
	}

	listState_s  m_list;   // +0x04
	DWORD        m_heap;   // +0x14
};

struct SHashEntry
{
	stringTableElement ele;
	unsigned __int8 type;
	unsigned __int16 flags;
	TDictDataValue v;
	SHashEntry* prev;
	SHashEntry* next;
};

struct SHashDictionaryBlock
{
	SHashDictionaryBlock* nextBlock;
	SHashEntry entries[8];
};

class CHashDictionary : public CDictionary
{
public:
	const char* m_thisDictsSourceFile;
	int m_thisDictsSourceLine;
	poolState m_freeHashEntries;
	SHashDictionaryBlock m_firstBlock;
	SHashDictionaryBlock* m_allocatedBlocksList;
	int m_contentsHeap;
	stringTable m_stringTable;
	stringTableElement* m_elementPtrs[64];
	stringTableElement* m_unnamedValues;
	int m_unnamedValuesCount;
	SHashEntry* m_contentsHead;
	SHashEntry* m_contentsTail;
	SHashEntry* m_cachedElement;
	int m_cachedElementIndex;
	CStrPool* m_keyPool;
	CTemplate* m_template;

	virtual CTemplate* GetTemplate() override;

	virtual void SetDictSourceLocation(const char* inSourceFileName, int inSourceLineNo) override;

	virtual void EnterValueWithSourceLocation(const char* inKey, unsigned char inFlags,
		const TDictDataValue* inValue, unsigned short inKeyLen,
		const char* inSourceFileName, int inSourceLineNo) override;

	virtual void EnterValue(
		const char* inHashKey, unsigned char inType,
		const TDictDataValue* inValue, unsigned short inFlags) override;

	virtual bool GetOptionalValueSelf(const char* inKeyName, unsigned char* outType,
		TDictDataValue* outValue, unsigned short* outFlags) override;

	virtual bool GetDictSourceLocation(const char** outSourceFileName, int* outSourceLineNo) override;

	void InitDictionary();
	void AddBlockToFreePool(SHashDictionaryBlock* inBlock);

	SHashEntry* AllocEntry(const char* inHashKey);
	void DiscardElement(SHashEntry* ele);
	static int s_objectHeap;
};

class CDictionaryAllocator 
{
public:
	virtual ~CDictionaryAllocator();
	virtual CDictionary* AllocDictionary(int32_t);
	virtual CArray* AllocArray(unsigned char, CTemplate*, int32_t);
	virtual CStrPool* GetKeyPool();
	virtual CStrPool* GetStrValuePool();
	CDictionaryAllocator(const CDictionaryAllocator&);
	CDictionaryAllocator();
};

class CDictionaryFileParser
{
public:
	struct __declspec(align(4)) SInputFile
	{
		char m_file[256];
		int m_lineNo;
		yy_buffer_state* m_lexBuffer;
		char* m_fileBuffer;
		int m_fileSize;
		int m_remaining;
		bool m_deleteFileBuffer;
	};

	CDictionaryFileParser(CStrPool* inKeyPool, CStrPool* inStrValuePool,
		unsigned int inFlags, CList<CTemplate*>* templateList);

	bool PushInputFile(const char* inFileName);
	char* ParseFileName(char* outBuffer, int inBufferSize, const char* inFileNameConst);
	void LoadFileIntoBuffer(const char* inFileName,
		char** outBuffer, int* outSize);

	bool PushInputFileMem(char* buffer, int bufferLen,
		const char* inBufferName, bool deleteFileBuffer);

	CDictionary* DoParse();

	CDictionary* AllocDictionary(bool inIsForTemplateUse);

	CDictionary* PushTemplate(char* inTemplateName);

	CArray* PushArray(int inPrimType, CTemplate* inTemplate, const char* inKey);

	CTemplate* PrevTemplate();

	CDictionary* PushDict(bool inIsTemplate);

	const char* CopyValueString(const char* inStr);

	int PopInputFile();

	void EnterValue(
		const char* inKey,
		unsigned __int8 inType,
		const TDictDataValue* inValue);

	unsigned int m_flags;
	CStrPool* m_KeyPool;
	CStrPool* m_StrValuePool;
	CDictionaryAllocator* m_allocator;
	bool m_rootDictSourceLineSet;
	CList<CTemplate*>* m_templateList;
	CList<char const*>* m_includeList;
	CStrPool* m_includeListStrs;
	BYTE m_Stack[256];
	int m_StackIndex;
	char m_origFile[256];
	CDictionaryFileParser::SInputFile m_fileStack[16];
	int m_fileStackIndex;
	char m_filePathPrefix[256];
};

struct SPackedStrPoolHeader
{
	int size;
	int hashTableOffset;
	int numBuckets;
};

class CPackedStrPoolReader
{
public:
	enum EPoolType : __int32
	{
		k_keyPool = 0x0,
		k_strValPool = 0x1,
		k_both = 0x2,
	};
	virtual ~CPackedStrPoolReader() {}
	CPackedStrPoolReader::EPoolType m_type;
	CPackedResFile* m_resFile;
	SPackedStrPoolHeader m_header;
};

class CPackedStrPoolBuilder : public CStrPool
{
	CSimpleHashStrPool m_pool;
	int m_closedHashSize;
};

class CPackedResFile
{
public:
	virtual void Free() {}
	char* m_bufferHeader;
	bool m_ownsBuffer;
	CDictionary* m_rootDict;
	CPackedStrPoolReader* m_keyReader;
	CPackedStrPoolReader* m_strValueReader;
	char* m_dictData;
	int m_dictDataSize;
	char* m_keyData;
	int m_keyDataSize;
	char* m_strData;
	int m_strDataSize;
	CPackedStrPoolBuilder* m_keyPoolBuilder;
};

class CPackedStructure
{
public:
	virtual bool ReadNext(const char**, unsigned char*, TDictDataValue*, uint16_t*) {}

	unsigned __int8 m_packedTypeCombined;
	CPackedResFile* m_resFile;
	int m_headerOffset;
	CTemplate* m_template;
	int m_eleCount;
	CPackedStructure* m_children;
	CPackedStructure* m_sibling;
	CPackedStructure* m_parent;
	CTemplate* m_templateDefinedByThisDict;
	int m_iterOffset;
	int m_iterElementIndex;
	int m_iterPackedChildrenEncountered;

	static int s_objectHeap;
};

template<typename T>
struct CStdGetHandle
{
	// Standard handle accessor -- no additional data
};

template<typename T, typename TGetHandle, int TBits, int TFlags = 0>
struct CHandleManager
{
	T* m_array;
	unsigned int m_numItems;
	unsigned int m_itemSize;
	unsigned int m_THandleCounter;
};

template<typename T, typename TGetHandle, int TBits>
struct CMgdList : IContainerT<T>
{
	char* m_array;
	unsigned int* m_hndlArray;
	poolState                          m_pool;
	CList<unsigned int>                m_hndlList;
	CHandleManager<T, TGetHandle, TBits, 0> m_hndlMgr;
	int                                m_maxItems;
	unsigned int                       m_itemSize;

	void Init(int memGrp, int maxItems)
	{
		const unsigned int itemSize = sizeof(T);
		const unsigned int arrayBytes = itemSize * maxItems;
		const unsigned int hndlBytes = sizeof(unsigned int) * maxItems;

		// Allocate item array
		unsigned __int8* itemMem = nullptr;
		if ((int)arrayBytes > 0)
			itemMem = (unsigned __int8*)memAllocAlignCore(
				arrayBytes, memGrp, 0,
				"engine/util/CMgdList.h", 57, nullptr, 1);

		m_array = (char*)itemMem;

		// Allocate handle array
		unsigned int* hndlMem = nullptr;
		if ((int)hndlBytes > 0)
			hndlMem = (unsigned int*)memAllocAlignCore(
				hndlBytes, memGrp, 0,
				"engine/util/CMgdList.h", 58, nullptr, 1);

		m_hndlArray = hndlMem;

		if (itemMem && hndlMem)
		{
			memset(itemMem, 0, arrayBytes);
			memset(hndlMem, 0, hndlBytes);

			// Init pool
			m_pool.objects = itemMem;
			m_pool.freeCount = 0;
			m_pool.totalCount = 0;
			m_pool.free = nullptr;
			((DWORD*)&m_pool)[4] = itemSize;
			poolAddObjectsArray(&m_pool, (char*)itemMem, maxItems);

			// Init handle list
			m_hndlList.m_list.maxSize = maxItems;
			m_hndlList.m_list.count = 0;
			m_hndlList.m_list.flags = 0;
			m_hndlList.m_list.items = (unsigned __int8*)hndlMem;

			// Init handle manager
			m_hndlMgr.m_array = (T*)itemMem;
			m_hndlMgr.m_numItems = maxItems;
			m_hndlMgr.m_itemSize = itemSize;
			m_hndlMgr.m_THandleCounter = 1;

			m_maxItems = maxItems;
			m_itemSize = itemSize;
		}
		else
		{
			memFreeFlags((char*)itemMem, 1);
			memFreeFlags((char*)hndlMem, 1);
			m_array = nullptr;
			m_hndlArray = nullptr;
		}
	}

	virtual bool  ICT_HasBeenInited() override { return false; }
	virtual void  ICT_InitFromHeap(int itemCount, unsigned int heap) override {}
	virtual void  ICT_RemoveAll() override {}
	virtual T* ICT_Append(const T& item) override { return nullptr; }
	virtual int ICT_ItemCount() override { return 0; }
	virtual T& ICT_Ref(int index) override
	{
		static T dummy{};
		return dummy;
	}
};