// NamedPipeServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
// Server sample
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <accctrl.h>
#include <aclapi.h>

using namespace std;

BOOL GetImpersonationLevel() {
	DWORD dwLen = 0;
	BOOL bRes = FALSE;
	HANDLE hToken = NULL;
	LPVOID pBuffer = NULL;


	bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
	if (!bRes) return FALSE;

	bRes = GetTokenInformation(hToken, TokenImpersonationLevel, NULL, 0, &dwLen);

	pBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLen);
	if (NULL == pBuffer) return FALSE;

	if (!GetTokenInformation(hToken, TokenImpersonationLevel, pBuffer, dwLen, &dwLen))
	{
		CloseHandle(hToken);
		HeapFree(GetProcessHeap(), 0, pBuffer);
		return FALSE;
	}


	if (*((SECURITY_IMPERSONATION_LEVEL*)pBuffer) == SecurityImpersonation)
	{
		cout << "Impersonation Level is : SecurityImpersonation" << endl;
	}
	if (*((SECURITY_IMPERSONATION_LEVEL*)pBuffer) == SecurityAnonymous)
	{
		cout << "Impersonation Level is : SecurityAnonymous" << endl;
	}
	if (*((SECURITY_IMPERSONATION_LEVEL*)pBuffer) == SecurityIdentification)
	{
		cout << "Impersonation Level is : SecurityIdentification" << endl;
	}
	if (*((SECURITY_IMPERSONATION_LEVEL*)pBuffer) == SecurityDelegation)
	{
		cout << "Impersonation Level is : SecurityDelegation" << endl;
	}

	//cout << "Impersonation Level is : " << *(SECURITY_IMPERSONATION_LEVEL *)pBuffer << endl;
	CloseHandle(hToken);
	HeapFree(GetProcessHeap(), 0, pBuffer);
	return bRes;
}
void main(void)
{
	// ============================================================================
	// Create Security Descriptor
	DWORD dwRes, dwDisposition;
	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	PACL pACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea[2];
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
		SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	SECURITY_ATTRIBUTES sa;
	LONG lRes;
	HKEY hkSub = NULL;

	// Create a well-known SID for the Everyone group.
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&pEveryoneSID))
	{
		printf("AllocateAndInitializeSid Error %u\n", GetLastError());
		return;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow Everyone read access to the key. (changed to PROCESS_ALL_ACCESS)
	ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
	ea[0].grfAccessPermissions = FILE_ALL_ACCESS;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)pEveryoneSID;

	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pAdminSID))
	{
		printf("AllocateAndInitializeSid Error %u\n", GetLastError());
		return;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow the Administrators group full access to
	// the key.
	ea[1].grfAccessPermissions = PROCESS_ALL_ACCESS;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR)pAdminSID;

	// Create a new ACL that contains the new ACEs.
	dwRes = SetEntriesInAcl(2, ea, NULL, &pACL);
	if (ERROR_SUCCESS != dwRes)
	{
		printf("SetEntriesInAcl Error %u\n", GetLastError());
		return;
	}

	// Initialize a security descriptor.  
	pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
		SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (NULL == pSD)
	{
		printf("LocalAlloc Error %u\n", GetLastError());
		return;
	}

	if (!InitializeSecurityDescriptor(pSD,
		SECURITY_DESCRIPTOR_REVISION))
	{
		printf("InitializeSecurityDescriptor Error %u\n",
			GetLastError());
		return;
	}

	// Add the ACL to the security descriptor. 
	if (!SetSecurityDescriptorDacl(pSD,
		TRUE,     // bDaclPresent flag   
		pACL,
		FALSE))   // not a default DACL 
	{
		printf("SetSecurityDescriptorDacl Error %u\n",
			GetLastError());
		return;
	}

	// Initialize a security attributes structure.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	// =============================================================================


	HANDLE PipeHandle;
	DWORD BytesRead;
	CHAR buffer[256];
	if ((PipeHandle = CreateNamedPipeA("\\\\.\\pipe\\SinaNamedPipe", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE , 1, 0, 0, 1000, &sa)) == INVALID_HANDLE_VALUE)
	{
		printf("CreateNamedPipe failed with error %d\n", GetLastError());
		return;
	}

	printf("Server is now running\n");

	if (ConnectNamedPipe(PipeHandle, NULL) == 0)
	{
		printf("ConnectNamedPipe failed with error %d\n", GetLastError());
		CloseHandle(PipeHandle);
		return;
	}

	if (ReadFile(PipeHandle, buffer, sizeof(buffer), &BytesRead, NULL) <= 0)
	{
		printf("ReadFile failed with error %d\n", GetLastError());
		CloseHandle(PipeHandle);
		return;
	}

	if (!ImpersonateNamedPipeClient(PipeHandle))
	{
		printf("Impersonation was not successfull ! error %d \n", GetLastError());

	}
	else
	{
		printf("Impersonation was successfull !\n");

	}

	// ********************** Do something *************************
	GetImpersonationLevel();

	// *************************************************************

	// Revert to self 
	RevertToSelf();

	printf("%.*s\n", BytesRead, buffer);

	if (DisconnectNamedPipe(PipeHandle) == 0)
	{
		printf("DisconnectNamedPipe failed with error %d\n", GetLastError());
		return;
	}

	CloseHandle(PipeHandle);
}