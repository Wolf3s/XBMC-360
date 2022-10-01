#ifndef H_CFILE
#define H_CFILE

#include "FileBase.h"
#include "utils\StdString.h"
#include "utils\BitstreamStats.h"

class CURL;

// Indicate that caller can handle truncated reads, where function returns before entire buffer has been filled
#define READ_TRUNCATED 0x1

// Calcuate bitrate for file while reading
#define READ_BITRATE   0x10

namespace XFILE
{
enum DIR_CACHE_TYPE
{
  DIR_CACHE_NEVER = 0, ///< Never cache this directory to memory
  DIR_CACHE_ONCE,      ///< Cache this directory to memory for each fetch (so that FileExists() checks are fast)
  DIR_CACHE_ALWAYS     ///< Always cache this directory to memory, so that each additional fetch of this folder will utilize the cache (until it's cleared)
};
class CFile 
{
public:
	CFile();
	virtual ~CFile();

	bool Open(const CStdString& strURLFile, unsigned int iFlags = 0);
	bool OpenForWrite(const CStdString& strFileName, bool bOverWrite = false);
	unsigned int Read(void* lpBuf, unsigned int uiBufSize, unsigned int flags);
	int Write(const void* lpBuf, int64_t uiBufSize);
	__int64 GetPosition();
	__int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
	__int64 GetLength();
	static bool Rename(const CStdString& strFileName, const CStdString& strNewFileName);
	static bool Delete(const CStdString& strFileName);
	int GetChunkSize() { if (m_pFile) return m_pFile->GetChunkSize(); return 0; }
	BitstreamStats* GetBitstreamStats() { return m_bitStreamStats; }
	void Close();

	static bool Exists(const CStdString& strFileName);
	static int  Stat(const CStdString& strFileName, struct __stat64* buffer);

private:
	CFileBase* m_pFile;
	unsigned int m_iFlags;
	BitstreamStats* m_bitStreamStats;
	//IFile...
	CStdString m_strFileMask;  ///< Holds the file mask specified by SetMask()
	bool m_allowPrompting;    ///< If true, the directory handlers may prompt the user
	DIR_CACHE_TYPE m_cacheDirectory;    ///< If !DIR_CACHE_NONE the directory is cached by g_directoryCache (defaults to DIR_CACHE_NONE)
	bool m_useFileDirectories; ///< If true the directory may allow file directories (defaults to false)
	bool m_extFileInfo;       ///< If true the GetDirectory call can retrieve extra file information (defaults to true)
};

};
#endif //H_CFILE