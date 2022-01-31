#ifndef H_CFACTORYDIRECTORY
#define H_CFACTORYDIRECTORY

#include "idirectory.h"

namespace DIRECTORY
{
//	Get access to a directory of a file system.
//	The Factory can be used to create a directory object
//	for every file system accessable.
//	Example:
//	CStdString strShare="iso9660://";
//	IDirectory* pDir=CFactoryDirectory::Create(strShare);
//	pDir pointer can be used to access a directory and retrieve it's content

//	When different types of shares have to be accessed use CVirtualDirectory

class CFactoryDirectory
{
public:
	static IDirectory* Create(const CStdString& strPath);
};

}
#endif //H_CFACTORYDIRECTORY