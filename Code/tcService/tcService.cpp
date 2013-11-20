//  File: tcService.cpp
//  Description: tcService implementation
//
//  Copyright (c) 2012, John Manferdelli.  All rights reserved.
//     Some contributions Copyright (c) 2012, Intel Corporation. 
//
// Use, duplication and disclosure of this file and derived works of
// this file are subject to and licensed under the Apache License dated
// January, 2004, (the "License").  This License is contained in the
// top level directory originally provided with the CloudProxy Project.
// Your right to use or distribute this file, or derived works thereof,
// is subject to your being bound by those terms and your use indicates
// consent to those terms.
//
// If you distribute this file (or portions derived therefrom), you must
// include License in or with the file and, in the event you do not include
// the entire License in the file, the file must contain a reference
// to the location of the License.


#include "jlmTypes.h"
#include "logging.h"
#include "tcIO.h"
#include "jlmcrypto.h"
#include "tcService.h"
#include "keys.h"
#include "sha256.h"
#include "buffercoding.h"
#include "fileHash.h"
#include "jlmUtility.h"

#include "policyCert.inc"
#include "tao.h"

#ifdef TPMSUPPORT
#include "TPMHostsupport.h"
#endif
#include "linuxHostsupport.h"

#include "vault.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef LINUX
#include <linux/un.h>
#else
#include <sys/un.h>
#endif
#include <errno.h>

#ifdef KVMTCSERVICE
#include "kvmHostsupport.h"
#include <libvirt/libvirt.h>
#endif


tcServiceInterface      g_myService;
int                     g_servicepid= 0;
extern bool             g_fterminateLoop;
u32                     g_fservicehashValid= false;
u32                     g_servicehashType= 0;
int                     g_servicehashSize= 0;
byte                    g_servicehash[32]= {
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                        };

#define NUMPROCENTS 200

// reset from environment variable CPProgramDirectory, if defined
const char*     g_progDirectory= "/home/jlm/jlmcrypt";
#include "taoSetupglobals.h"


// ---------------------------------------------------------------------------


bool uidfrompid(int pid, int* puid)
{
    char        szfileName[256];
    struct stat fattr;

    sprintf(szfileName, "/proc/%d/stat", pid);
    if((lstat(szfileName, &fattr))!=0) {
        printf("uidfrompid: stat failed\n");
        return false;
    }
    *puid= fattr.st_uid;
    return true;
}


// ------------------------------------------------------------------------------


void serviceprocEnt::print()
{
    fprintf(g_logFile, "procid: %ld, ", (long int)m_procid);
    if(m_szexeFile!=NULL)
        fprintf(g_logFile, "file: %s, ", m_szexeFile);
    fprintf(g_logFile, "hash size: %d, ", m_sizeHash);
    PrintBytes("", m_rgHash, m_sizeHash);
}


serviceprocTable::serviceprocTable()
{
    m_numFree= 0;
    m_numFilled= 0;
    m_pFree= NULL;
    m_pMap= NULL;
    m_rgProcMap= NULL;
    m_rgProcEnts= NULL;
#ifdef KVMTCSERVICE
    m_vmconnection= NULL;
    m_vmdomain= NULL;
#endif
}


serviceprocTable::~serviceprocTable()
{
    // delete m_rgProcEnts;
    // delete m_rgProcMap;
    // m_numFree= 0;
    // m_numFilled= 0;
    // m_pFree= NULL;
    // m_pMap= NULL;
}


bool serviceprocTable::initprocTable(int size)
{
    int             i;
    serviceprocMap* p;

    m_rgProcEnts= new serviceprocEnt[size];
    m_rgProcMap= new serviceprocMap[size];
    p= &m_rgProcMap[0];
    m_pMap= NULL;
    m_pFree= p;
    for(i=0; i<(size-1); i++) {
        p= &m_rgProcMap[i];
        p->pElement= &m_rgProcEnts[i];
        p->pNext= &m_rgProcMap[i+1];
    }
    m_rgProcMap[size-1].pElement= &m_rgProcEnts[size-1];
    m_rgProcMap[size-1].pNext= NULL;
    m_numFree= size;
    m_numFilled= 0;
    return true;
}


#ifdef KVMTCSERVICE
bool serviceprocTable::addprocEntry(int procid, const char* file, int an, char** av,
                                    int sizeHash, byte* hash, virConnectPtr* ppvmconnection,
                                    virDomainPtr*  ppvmdomain)
#else
bool serviceprocTable::addprocEntry(int procid, const char* file, int an, char** av,
                                    int sizeHash, byte* hash)
#endif
{
    if(m_pFree==NULL)
        return false;
    if(sizeHash>32)
        return false;

    serviceprocMap* pMap= m_pFree;
    m_pFree= pMap->pNext;
    serviceprocEnt* pEnt= pMap->pElement;
    m_numFilled++;
    m_numFree--;
    pEnt->m_procid= procid;
    pEnt->m_sizeHash= sizeHash;
    memcpy(pEnt->m_rgHash, hash, sizeHash);
    pEnt->m_szexeFile= strdup(file);
    pMap->pNext= m_pMap;
    m_pMap= pMap;
#ifdef KVMTCSERVICE
    m_vmconnection= *ppvmconnection;
    m_vmdomain= *ppvmdomain;
#endif
    return true;
}


serviceprocEnt*  serviceprocTable::getEntfromprocId(int procid)
{
    serviceprocMap* pMap= m_pMap;
    serviceprocEnt* pEnt;

    while(pMap!=NULL) {
        pEnt= pMap->pElement;
        if(pEnt->m_procid==procid) {
            return pEnt;
        }
        pMap= pMap->pNext;
    }
    return NULL;
}


void   serviceprocTable::removeprocEntry(int procid)
{
    serviceprocMap* pMap;
    serviceprocMap* pDelete;
    serviceprocEnt* pEnt;

    if(m_pMap==NULL)
        return;

    pEnt= m_pMap->pElement;
    if(pEnt->m_procid==procid) {
        pMap= m_pMap;
        m_pMap= pMap->pNext;
        pMap->pNext= m_pFree;
        m_pFree= pMap;
        m_numFree++;
        m_numFilled--;
        return;
    }
     
    pMap= m_pMap;   
    while(pMap->pNext!=NULL) {
        pDelete= pMap->pNext;
        pEnt= pDelete->pElement;
        if(pEnt->m_procid==procid) {
            pMap->pNext= pDelete->pNext;
            pDelete->pNext= m_pFree;
            m_pFree= pDelete;
            pEnt->m_procid= -1;
            m_numFree++;
            m_numFilled--;
            return;
        }
        pMap= pDelete;
    }
    return;
}


bool serviceprocTable::gethashfromprocId(int procid, int* psize, byte* hash)
{
    serviceprocEnt* pEnt= getEntfromprocId(procid);

    if(pEnt==NULL)
        return false;

    *psize= pEnt->m_sizeHash;
    memcpy(hash, pEnt->m_rgHash, *psize);
    return true;
}


void serviceprocTable::print()
{
    serviceprocMap* pMap= m_pMap;
    serviceprocEnt* pEnt;

    while(pMap!=NULL) {
        pEnt= pMap->pElement;
        pEnt->print();
        pMap= pMap->pNext;
    }

    fprintf(g_logFile, "proc table %d entries, %d free\n\n", 
                m_numFilled, m_numFree);
    return;
}


// -------------------------------------------------------------------


tcServiceInterface::tcServiceInterface()
{
}


tcServiceInterface::~tcServiceInterface()
{
}


TCSERVICE_RESULT tcServiceInterface::initService(const char* execfile, int an, char** av)
{
    u32     hashType= 0;
    int     sizehash= SHA256DIGESTBYTESIZE;
    byte    rgHash[SHA256DIGESTBYTESIZE];

    if(!getfileHash(execfile, &hashType, &sizehash, rgHash)) {
        fprintf(g_logFile, "initService: getfileHash failed %s\n", execfile);
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TEST
    fprintf(g_logFile, "initService size hash %d\n", sizehash);
    PrintBytes("getfile hash: ", rgHash, sizehash);
#endif

    if(sizehash>SHA256DIGESTBYTESIZE)
        return TCSERVICE_RESULT_FAILED;
    g_servicehashType= hashType;
    g_servicehashSize= sizehash;
    memcpy(g_servicehash, rgHash, sizehash);
    g_fservicehashValid= true;

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::GetOsPolicyCert(u32* pType, int* psize, 
                                                    byte* rgBuf)
{
    if(!m_trustedHome.policyCertValid())
        return TCSERVICE_RESULT_DATANOTVALID ;
    if(*psize<m_trustedHome.policyCertSize())
        return TCSERVICE_RESULT_BUFFERTOOSMALL;
    if(!m_trustedHome.copyPolicyCert(rgBuf)) {
        return TCSERVICE_RESULT_FAILED;
    }
    *pType= m_trustedHome.policyCertType();
    *psize= m_trustedHome.policyCertSize();

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::tcServiceInterface::GetOsCert(u32* pType,
                        int* psizeOut, byte* rgOut)
{
    if(!m_trustedHome.myCertValid())
        return TCSERVICE_RESULT_DATANOTVALID ;
    if(*psizeOut<m_trustedHome.myCertSize())
        return TCSERVICE_RESULT_BUFFERTOOSMALL;
    memcpy(rgOut, m_trustedHome.myCertPtr(), 
           m_trustedHome.myCertSize());
    *psizeOut= m_trustedHome.myCertSize();
    if(m_trustedHome.myCertType()==KEYTYPERSA1024INTERNALSTRUCT)
        *pType= KEYTYPERSA1024SERIALIZED;
    else if(m_trustedHome.myCertType()==KEYTYPERSA2048INTERNALSTRUCT)
        *pType= KEYTYPERSA2048SERIALIZED;
    else
        *pType= m_trustedHome.myCertType();

    return TCSERVICE_RESULT_SUCCESS;
}



TCSERVICE_RESULT tcServiceInterface::GetOsEvidence(int* psizeOut, byte* rgOut)
{
    if(!m_trustedHome.myEvidenceValid())
        return TCSERVICE_RESULT_DATANOTVALID ;
    if(*psizeOut<m_trustedHome.myEvidenceSize())
        return TCSERVICE_RESULT_BUFFERTOOSMALL;
    memcpy(rgOut, m_trustedHome.myEvidencePtr(), *psizeOut);
    *psizeOut= m_trustedHome.myEvidenceSize();

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::GetHostedMeasurement(int pid, u32* phashType, 
                                                        int* psize, byte* rgBuf)
{
#ifdef KVMTCSERVICE
    // map procid to KVM main pid
    int     newpid= m_pidMapper.getbasepid(pid);
    if(newpid>0) {
        pid= newpid;
    }
#endif
    if(!m_procTable.gethashfromprocId(pid, psize, rgBuf)) {
        return TCSERVICE_RESULT_FAILED;
    }
    *phashType= HASHTYPEJLMPROGRAM;
    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::GetOsHash(u32* phashType, int* psize, 
                                               byte* rgOut)
{
    if(!m_trustedHome.measurementValid())
        return TCSERVICE_RESULT_DATANOTVALID ;
    if(*psize<m_trustedHome.measurementSize())
        return TCSERVICE_RESULT_BUFFERTOOSMALL;
    if(!m_trustedHome.copyMeasurement(rgOut)) {
        return TCSERVICE_RESULT_FAILED;
    }
    *psize= m_trustedHome.measurementSize();
    *phashType= m_trustedHome.measurementType();

    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::GetServiceHash(u32* phashType, int* psize, 
                                                    byte* rgOut)
{
    if(!g_fservicehashValid)
        return TCSERVICE_RESULT_FAILED;
    *phashType= g_servicehashType;
    if(*psize<g_servicehashSize)
        return TCSERVICE_RESULT_FAILED;
    *psize= g_servicehashSize;
    memcpy(rgOut, g_servicehash, *psize);

    return TCSERVICE_RESULT_SUCCESS;
}


#ifdef KVMTCSERVICE


#define MAXPROGNAME 512


char* programNamefromFileName(const char* fileName)
{
    char*   p= (char*) fileName;
    char*   q;
    char*   r;
    char    progNameBuf[MAXPROGNAME];

    if(fileName==NULL)
        return NULL;
#ifdef TEST
    fprintf(g_logFile, "fileName: %s\n", fileName);
#endif
    while(*p!='\0')
        p++;
    q= p-1;
    while((--p)!=fileName) {
        if(*p=='/') {
            break;
        }
        if(*p=='.') {
            break;
        }
    }

    if(*p=='/') {
        r= p+1;
    }
    else if(*p=='.') {
        q= p-1;
        r= q;
        while(r>=fileName) {
            if(*r=='/')
                break;
            r--;
        }
    }
    else {
        r= (char*)fileName-1;
    }
    if((q-r)>=(MAXPROGNAME-1))
        return NULL;
    r++;
    p= progNameBuf;
    while(r<=q)
        *(p++)= *(r++);
    *p= '\0';
    q= strdup(progNameBuf);
    return q;
}


#define MAXMLBUF 8192


TCSERVICE_RESULT tcServiceInterface::StartApp(int procid, int an, const char** av, 
                                              int* poutsize, byte* out)
{
    u32             uType= 0;
    int             size= SHA256DIGESTBYTESIZE;
    byte            rgHash[SHA256DIGESTBYTESIZE];
    int             pid= 0;
    int             uid= -1;
    const char*     szsys= "qemu:///system";
    const char*     vmName= NULL;
    const char*     xmlTemplate= NULL;
    const char*     kernelFile= NULL;
    const char*     initramFile= NULL;
    const char*     imageFile= NULL;
    char            buf[MAXMLBUF];

    // if an= 3
    //      av[0] is name of VM
    //      av[1] is xml template
    //      av[2] is image file
    // if an=5
    //      av[0] is name of VM
    //      av[1] is xml (template first)
    //      av[2] is kernel file
    //      av[3] is initram file
    //      av[4] is image file
#ifdef TEST
    fprintf(g_logFile, "tcServiceInterface::StartApp(VM), %d args\n", an);
#endif

    // lock file

    if(an==3) {

        vmName= av[0];
        xmlTemplate= av[1];
        imageFile= av[2];

        if((strlen(xmlTemplate)+strlen(imageFile)+strlen(vmName)+4)>MAXMLBUF) {
            fprintf(g_logFile, "tcServiceInterface::StartApp: resulting XML too large\n");
            return false;
        }

#ifdef LOCKFILE
        struct flock    lock;
        int             ret;
        int             fd= open(file, O_RDONLY);

        // F_UNLCK
        lock.l_type= F_WRLCK;
        lock.l_start= 0;
        lock.l_len= SEEK_SET;
        lock.l_pid= getpid();
        ret= fcntl(fd, F_SETLK, &lock);
#endif

        sprintf(buf, xmlTemplate, vmName, imageFile);
        if(!getfileHash(imageFile, &uType, &size, rgHash)) {
            fprintf(g_logFile, "StartApp : getfilehash failed %s\n", imageFile);
            return TCSERVICE_RESULT_FAILED;
        }
    }
    else if(an==5) {

        vmName= av[0];
        xmlTemplate= av[1];
        kernelFile= av[2];
        initramFile= av[3];
        imageFile= av[4];

        const char* nav[2];

        if((strlen(xmlTemplate)+strlen(imageFile)+strlen(vmName)+
                                strlen(kernelFile)+strlen(initramFile)+4)>MAXMLBUF) {
            fprintf(g_logFile, "tcServiceInterface::StartApp: resulting XML too large\n");
            return false;
        }

        nav[0]= kernelFile;
        nav[1]= initramFile;
        if(!getcombinedfileHash(2, nav, &uType, &size, rgHash)) {
            fprintf(g_logFile, "startLinuxvm error: getcombinedfilehash failed\n");
            return false;
        }
        // programname xml kernel-file initram-file-name virtual-disk-image
        sprintf(buf, xmlTemplate, vmName, kernelFile, initramFile, imageFile);
    }
    else {
        fprintf(g_logFile, "StartApp : wrong arguments\n");
        return TCSERVICE_RESULT_FAILED;
    }

    // look up uid for procid
    if(!uidfrompid(procid, &uid)) {
        fprintf(g_logFile, "StartApp: cant get uid from procid\n");
        return TCSERVICE_RESULT_FAILED;
    }

    {

       virConnectPtr    vmconnection= NULL;
       virDomainPtr     vmdomain= NULL;

       if(!startKvmVM(vmName, szsys,  buf, &vmconnection, &vmdomain)) {
           fprintf(g_logFile, "StartApp : cant start VM\n");
           return TCSERVICE_RESULT_FAILED;
       }

       if((pid=m_pidMapper.initKvm(vmName))<0) {
           fprintf(g_logFile, "StartApp : cant map pids\n");
           return TCSERVICE_RESULT_FAILED;
       }

       // record procid and hash
      if(!g_myService.m_procTable.addprocEntry(pid, vmName, 0, (char**) NULL, 
                                   size, rgHash, &vmconnection, &vmdomain)) {
           fprintf(g_logFile, "StartApp: cant add to proc table\n");
           return TCSERVICE_RESULT_FAILED;
       }
    }
#ifdef TEST
    fprintf(g_logFile, "\nProc table after create. pid: %d, serviceid: %d\n", 
            pid, g_servicepid);
    g_myService.m_procTable.print();
    fflush(g_logFile);
#endif
#ifdef LOCKFILE
        close(fd);
#endif

    *poutsize= sizeof(int);
    *((int*)out)= pid;
    return TCSERVICE_RESULT_SUCCESS;
}
#endif


#ifndef KVMTCSERVICE
TCSERVICE_RESULT tcServiceInterface::StartApp(tcChannel& chan,
                                int procid, int an, const char** av, 
                                int* poutsize, byte* out)
{
    u32     uType= 0;
    int     size= SHA256DIGESTBYTESIZE;
    byte    rgHash[SHA256DIGESTBYTESIZE];
    int     child= 0;
    int     uid= -1;
    const char* execName= NULL;

#ifdef TEST
    fprintf(g_logFile, "tcServiceInterface::StartApp, %d args\n", an);
    fflush(g_logFile);
#endif

    // av[0] is file to execute
    if(an>30 || an<1) {
        return TCSERVICE_RESULT_FAILED;
    }
    execName= av[0];
    
    // lock file
#ifdef LOCKFILE
    struct flock lock;
    int     ret;
    int     fd= open(execName, O_RDONLY);

    // F_UNLCK
    lock.l_type= F_WRLCK;
    lock.l_start= 0;
    lock.l_len= SEEK_SET;
    lock.l_pid= getpid();
    ret= fcntl(fd, F_SETLK, &lock);
#endif

    if(!getfileHash(execName, &uType, &size, rgHash)) {
        fprintf(g_logFile, "StartApp : getfilehash failed %s\n", execName);
        return TCSERVICE_RESULT_FAILED;
    }

    child= fork();
    if(child<0) {
        fprintf(g_logFile, "StartApp: fork failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
    if(child==0) {
        chan.CloseBuf();
    }

    if(child>0) {
        // look up uid for procid
        if(!uidfrompid(procid, &uid)) {
            fprintf(g_logFile, "StartApp: cant get uid from procid\n");
            return TCSERVICE_RESULT_FAILED;
        }

        // setuid to correct user (uid, eid and saved id)
        setresuid(uid, uid, uid);

        // record procid and hash
        if(!g_myService.m_procTable.addprocEntry(child, execName, 0, (char**) NULL, 
                                                 size, rgHash)) {
            fprintf(g_logFile, "StartApp: cant add to proc table\n");
            return TCSERVICE_RESULT_FAILED;
        }
#ifdef TCTEST
        fprintf(g_logFile, "\nProc table after create. child: %d, serviceid: %d\n", 
                child, g_servicepid);
        g_myService.m_procTable.print();
#endif
#ifdef LOCKFILE
        close(fd);
#endif
    }

    // child
    if(child==0) {
#ifdef LOCKFILE
        // drop lock
        // this actually drops it a bit too soon
        // we can leave it locked or change owner or copy it somewhere
        lock.l_type= F_UNLCK;
        lock.l_start= 0;
        lock.l_len= SEEK_SET;
        lock.l_pid= getpid();   // is this right?
        ret= fcntl(fd, F_SETLK, &lock);
        close(fd);
#endif

        // start Linux guest application
        if(execve((char*)execName, (char**)av, NULL)<0) {
            fprintf(g_logFile, "StartApp: execvp %s failed\n", av[0]);
            exit(1);
        }
    }

    *poutsize= sizeof(int);
    *((int*)out)= child;
    return TCSERVICE_RESULT_SUCCESS;
}
#endif


TCSERVICE_RESULT tcServiceInterface::SealFor(int procid, int sizeIn, byte* rgIn, 
                                             int* psizeOut, byte* rgOut)
//  Sealed value is hash-size hash size-in rgIn
{
    byte    rgHash[32];
    int     hashSize= 0;

#ifdef KVMTCSERVICE
    // map procid to KVM main pid
    int     newprocid= m_pidMapper.getbasepid(procid);
    if(newprocid>0) {
        procid= newprocid;
    }
#endif
    if(!m_procTable.gethashfromprocId(procid, &hashSize, rgHash)) {
        fprintf(g_logFile, "SealFor can't find hash in procTable %ld\n", (long int)procid);
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TCTEST
    fprintf(g_logFile, "SealFor: %ld(proc), %d(hashsize), %d (size seal)\n",
           (long int)procid, hashSize, sizeIn);
#endif
    if(!m_trustedHome.Seal(hashSize, rgHash, sizeIn, rgIn,
                       psizeOut, rgOut)) {
        fprintf(g_logFile, "SealFor: seal failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TCTEST
    fprintf(g_logFile, "tcServiceInterface::SealFor\n");
#endif
    
    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::UnsealFor(int procid, int sizeIn, byte* rgIn, 
                                               int* psizeOut, byte* rgOut)
{
    byte    rgHash[32];
    int     hashSize= 0;

#ifdef KVMTCSERVICE
    // map procid to KVM main pid
    int     newprocid= m_pidMapper.getbasepid(procid);
    if(newprocid>0) {
        procid= newprocid;
    }
#endif
    if(!m_procTable.gethashfromprocId(procid, &hashSize, rgHash)) {
        fprintf(g_logFile, "UnsealFor can't find hash in procTable %ld\n", (long int)procid);
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TCTEST
    fprintf(g_logFile, "UnsealFor: %ld(proc), %d(hashsize), %d (size seal)\n",
           (long int)procid, hashSize, sizeIn);
#endif
    if(!m_trustedHome.Unseal(hashSize, rgHash, sizeIn, rgIn,
                       psizeOut, rgOut)) {
        fprintf(g_logFile, "UnsealFor: unseal failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
    return TCSERVICE_RESULT_SUCCESS;
}


TCSERVICE_RESULT tcServiceInterface::AttestFor(int procid, int sizeIn, byte* rgIn, 
                                               int* psizeOut, byte* rgOut)
{
    byte    rgHash[32];
    int     hashSize= 32;

#ifdef KVMTCSERVICE
    // map procid to KVM main pid
    int     newprocid= m_pidMapper.getbasepid(procid);
    if(newprocid>0) {
        procid= newprocid;
    }
#endif
    if(!m_procTable.gethashfromprocId(procid, &hashSize, rgHash)) {
        fprintf(g_logFile, "tcServiceInterface::AttestFor lookup failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TEST
    fprintf(g_logFile, "tcServiceInterface::AttestFor procid: %d\n", 
            procid);
#endif
    if(!m_trustedHome.Attest(hashSize, rgHash, sizeIn, rgIn,
                       psizeOut, rgOut)) {
        fprintf(g_logFile, "tcServiceInterface::AttestFor trustedHome AtitestFor failed\n");
        return TCSERVICE_RESULT_FAILED;
    }
#ifdef TEST1
        fprintf(g_logFile, 
                "tcServiceInterface::AttestFor succeeded new output buf size %d\n",
               *psizeOut);
        PrintBytes((char*)"Attest value: ", rgOut, *psizeOut);
#endif
    return TCSERVICE_RESULT_SUCCESS;
}


// ------------------------------------------------------------------------------


bool  serviceRequest(tcChannel& chan, bool* pfTerminate)
{
    int                 procid;
    int                 origprocid;
    u32                 uReq;
    u32                 uStatus;

    char*               szappexecfile= NULL;

    int                 sizehash= SHA256DIGESTBYTESIZE;
    byte                hash[SHA256DIGESTBYTESIZE];

    int                 inparamsize;
    byte                inparams[PARAMSIZE];

    int                 outparamsize;
    byte                outparams[PARAMSIZE];

    int                 size;
    byte                rgBuf[PARAMSIZE];

    int                 pid;
#ifdef KVMTCSERVICE
    int                 newpid;
#endif
    u32                 uType= 0;
    int                 an;
    char*               av[10];

#ifdef TEST
    fprintf(g_logFile, "Entering serviceRequest\n");
    fflush(g_logFile);
#endif

    // get request
    inparamsize= PARAMSIZE;
    if(!chan.gettcBuf(&procid, &uReq, &uStatus, &origprocid, &inparamsize, inparams)) {
        fprintf(g_logFile, "serviceRequest: gettcBuf failed\n");
        return false;
    }
    if(uStatus==TCIOFAILED) {
        chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        return false;
    }

#ifdef TEST
    fprintf(g_logFile, "serviceRequest after get procid: %d, req, %d, origprocid %d\n", 
           procid, uReq, origprocid); 
    fflush(g_logFile);
#endif

#ifdef KVMGUESTOSTCSERVICE 
    // DEBUG
    fprintf(g_logFile, "special serviceRequest\n");
    fflush(g_logFile);
    if(uReq!=TCSERVICESTARTAPPFROMTCSERVICE && uReq!=TCSERVICESTARTAPPFROMAPP) {
        if(uStatus==TCIOFAILED) {
                chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
                return false;
        }
    }
#endif

    switch(uReq) {

      case TCSERVICEGETPOLICYKEYFROMTCSERVICE:
        size= PARAMSIZE;
        if(g_myService.GetOsPolicyCert(&uType, &size, rgBuf)!=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: getpolicyKey failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }

        outparamsize= encodeTCSERVICEGETPOLICYKEYFROMOS(uType, size, rgBuf, 
                                      PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: TCSERVICEGETPOLICYKEYFROMTCSERVICE buffer too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (policyKey) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEGETOSHASHFROMTCSERVICE:
        size= PARAMSIZE;
        if(g_myService.GetOsHash(&uType, &size, rgBuf)!=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: getosHash failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= encodeTCSERVICEGETOSHASHFROMTCSERVICE(uType, size, rgBuf, 
                                      PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICEGETOSHASHFROMTCSERVICE buffer too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (getosHash) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEGETOSCREDSFROMTCSERVICE:
        size= PARAMSIZE;
        if(g_myService.GetOsEvidence(&size, rgBuf)!=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: getosCredsfailed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= encodeTCSERVICEGETOSCREDSFROMTCSERVICE(uType, size, rgBuf, 
                                      PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICEGETOSCREDSFROMTCSERVICE buffer too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)){
            fprintf(g_logFile, "serviceRequest: sendtcBuf (getosCreds) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICESEALFORFROMTCSERVICE:
        // Input buffer to decode:
        //  size of sealdata || sealedata
        //  decodeTCSERVICESEALFORFROMAPP outputs
        //  size of sealdata, sealedata
        //  sealfor returns m= ENC(hashsize||hash||sealsize||sealdata)
        //  returned buffer is sizeof(m) || m
        outparamsize= PARAMSIZE;
        if(!decodeTCSERVICESEALFORFROMAPP(&outparamsize, outparams, inparams)) {
            fprintf(g_logFile, "serviceRequest: TCSERVICESEALFORFROMTCSERVICE buffer too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        size= PARAMSIZE;
#ifdef TCTEST1
        fprintf(g_logFile, "about to sealFor %d\n", outparamsize);
        PrintBytes("bytes to seal: ", outparams, outparamsize);
#endif
        if(g_myService.SealFor(origprocid, outparamsize, outparams, &size, rgBuf)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: sealFor failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= encodeTCSERVICESEALFORFROMTCSERVICE(size, rgBuf, PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICESEALFORFROMTCSERVICE buf too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (sealFor) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEUNSEALFORFROMTCSERVICE:
        // outparamsize is sizeof(m)m outparams is m from above
        // unsealfor returns sizof unsealed data || unsealeddata
        outparamsize= PARAMSIZE;
        if(!decodeTCSERVICEUNSEALFORFROMAPP(&outparamsize, outparams, inparams)) {
            fprintf(g_logFile, "serviceRequest: service loop TCSERVICEUNSEALFORFROMTCSERVICE failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        size= PARAMSIZE;
#ifdef TCTEST
        fprintf(g_logFile, "about to UnsealFor %d\n", outparamsize);
#endif
        if(g_myService.UnsealFor(origprocid, outparamsize, outparams, &size, rgBuf)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: UnsealFor failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
#ifdef TCTEST1
        PrintBytes("return from UnsealFor:\n", rgBuf, size);
#endif
        outparamsize= encodeTCSERVICEUNSEALFORFROMAPP(size, rgBuf, PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICESEALFORFROMTCSERVICE buf too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (unsealFor) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEGETPROGHASHFROMTCSERVICE:
        if(!decodeTCSERVICEGETPROGHASHFROMAPP(&pid, inparams)) {
            fprintf(g_logFile, "serviceRequest: TCSERVICEGETPROGHASHFROMTCSERVICE failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }

        // Process
#ifdef TEST
        fprintf(g_logFile, "looking up hash for pid %d\n", pid);
        g_myService.m_procTable.print();
        fflush(g_logFile);
#endif
        // When the tcService for a guest calls, it doesn't know its
        // id (the pid of the KVM/QEMU process).  It is known to the
        // driver that forwards the message so we replace it here
        if(g_myService.m_trustedHome.envType()==PLATFORMTYPEKVMHYPERVISOR) {
            pid= origprocid;
        }

#ifdef KVMTCSERVICE
         // map procid to KVM main pid
        newpid= g_myService.m_pidMapper.getbasepid(pid);
        if(newpid>0) {
            pid= newpid;
        }
#endif
        // get hash
        sizehash= SHA256DIGESTBYTESIZE;
        uType= SHA256HASH;
        if(!g_myService.m_procTable.gethashfromprocId(pid, &sizehash, hash)) {
            fprintf(g_logFile, 
                    "hash not found setting to 0; pid: %d, procid: %d, origpid: %d\n",
                    pid, procid, origprocid);
            memset(hash, 0, sizehash);
        }
#ifdef TEST1
        fprintf(g_logFile, "program hash for pid found\n");
        PrintBytes("Hash: ", hash, sizehash);
        fflush(g_logFile);
#endif
        outparamsize= encodeTCSERVICEGETPROGHASHFROMSERVICE(uType, sizehash, 
                                hash, PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICEGETPROGHASHFROMSERVICE buf too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (getproghash) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICEATTESTFORFROMTCSERVICE:
        outparamsize= PARAMSIZE;
        if(!decodeTCSERVICEATTESTFORFROMAPP(&outparamsize, outparams, inparams)) {
            fprintf(g_logFile, "serviceRequest: TCSERVICEATTESTFORFROMTCSERVICE failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        size= PARAMSIZE;
        if(g_myService.AttestFor(origprocid, outparamsize, outparams, &size, rgBuf)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: AttestFor failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= encodeTCSERVICEATTESTFORFROMAPP(size, rgBuf, PARAMSIZE, outparams);
        if(outparamsize<0) {
            fprintf(g_logFile, "serviceRequest: encodeTCSERVICEATTESTFORFROMAPP buf too small\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (AttestFor) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICESTARTAPPFROMTCSERVICE:
#ifdef TCTEST1
        fprintf(g_logFile, "serviceRequest, TCSERVICESTARTAPPFROMTCSERVICE, decoding\n");
#endif
        an= 10;
        if(!decodeTCSERVICESTARTAPPFROMAPP(&an, (char**) av, inparams)) {
            fprintf(g_logFile, "serviceRequest: decodeTCSERVICESTARTAPPFROMTCSERVICE failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        outparamsize= PARAMSIZE;
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, about to StartHostedProgram %s, for %d\n",
                av[0], origprocid);
        fflush(g_logFile);
#endif
#ifdef KVMTCSERVICE
        if(g_myService.StartApp(origprocid, an, (const char**) av,
                                    &outparamsize, outparams)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: StartHostedProgram failed %s\n", szappexecfile);
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
#else
        if(g_myService.StartApp(chan, origprocid, an, (const char**) av,
                                    &outparamsize, outparams)
                !=TCSERVICE_RESULT_SUCCESS) {
            fprintf(g_logFile, "serviceRequest: StartHostedProgram failed %s\n", szappexecfile);
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
#endif
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, StartHostedProgram succeeded, about to send\n");
#endif
        if(!chan.sendtcBuf(procid, uReq, TCIOSUCCESS, origprocid, outparamsize, outparams)) {
            fprintf(g_logFile, "serviceRequest: sendtcBuf (startapp) failed\n");
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
            return false;
        }
        return true;

      case TCSERVICETERMINATE:  // no reply required
#ifdef TEST
        fprintf(g_logFile, "serviceRequest, TCSERVICETERMINATE\n");
        fprintf(g_logFile, "serviceRequest, removeprocEntry %d\n", origprocid);
        g_myService.m_procTable.print();
#endif
        return true;

      default:
            chan.sendtcBuf(procid, uReq, TCIOFAILED, origprocid, 0, NULL);
        return false;
    }
}


// ------------------------------------------------------------------------------


int main(int an, char** av)
{
    int                 iRet= 0;
    TCSERVICE_RESULT    ret;
    bool                fInitKeys= false;
    int                 i;
    bool                fTerminate= false;
    bool                fServiceStart;

    initLog(g_logName);

#ifdef TEST
    fprintf(g_logFile, "%s started\n\n", g_myServiceName);
    fflush(g_logFile);
#endif

#ifdef KVMTCSERVICE
    virConnectPtr       vmconnection= NULL;
    virDomainPtr        vmdomain= NULL;
#endif

    // set executable path
    const char*     definedprogDirectory= getenv("CPProgramDirectory");
    if(definedprogDirectory!=NULL) {
        g_progDirectory= definedprogDirectory;
    }

    for(i=0; i<an; i++) {
        if(strcmp(av[i], "-help")==0) {
            fprintf(g_logFile, "\nUsage: tcService.exe [-initKeys]\n");
            return 0;
        }
        if(strcmp(av[i], "-initKeys")==0) {
            fInitKeys= true;
        }
    }

    // set the signal disposition of SIGCHLD to not create zombies
    struct sigaction sigAct;
    memset(&sigAct, 0, sizeof(sigAct));
    sigAct.sa_handler = SIG_DFL;
    sigAct.sa_flags = SA_NOCLDWAIT; // don't zombify child processes
    int sigRv = sigaction(SIGCHLD, &sigAct, NULL);
    if (sigRv < 0) {
        fprintf(g_logFile, "Failed to set signal disposition for SIGCHLD\n");
    } else {
        fprintf(g_logFile, "Set SIGCHLD to avoid zombies\n");
    }

    g_servicepid= getpid();
    const char** parameters = NULL;
    int parameterCount = 0;

    if(!initAllCrypto()) {
        fprintf(g_logFile, "tcService main: can't initcrypto\n");
        iRet= 1;
        goto cleanup;
    }

    if(!g_myService.m_procTable.initprocTable(NUMPROCENTS)) {
        fprintf(g_logFile, "tcService main: Cant init proctable\n");
        iRet= 1;
        goto cleanup;
    }
#ifdef TEST
    fprintf(g_logFile, "tcService main: proctable init complete\n\n");
#endif

    ret= g_myService.initService(g_serviceexecFile, 0, NULL);
    if(ret!=TCSERVICE_RESULT_SUCCESS) {
        fprintf(g_logFile, "tcService main: initService failed %s\n", g_serviceexecFile);
        iRet= 1;
        goto cleanup;
    }
#ifdef TEST
    fprintf(g_logFile, "tcService main: initService succeeds\n\n");
#endif

    // init Host and Environment
    g_myService.m_taoHostInitializationTimer.Start();
    if(!g_myService.m_host.HostInit(g_hostplatform, g_hostProvider,
                                    g_hostDirectory, g_hostsubDirectory,
                                    parameterCount, parameters)) {
        fprintf(g_logFile, "tcService main: can't init host\n");
        iRet= 1;
        goto cleanup;
    }
    g_myService.m_taoHostInitializationTimer.Stop();

#ifdef TEST
    fprintf(g_logFile, "tcService main: after HostInit, pid: %d\n",
            g_servicepid);
    fflush(g_logFile);
#endif

    if(fInitKeys) {
        taoFiles  fileNames;

        if(!fileNames.initNames(g_hostDirectory, g_clientsubDirectory)) {
            fprintf(g_logFile, "tcService::main: cant init names\n");
            iRet= 1;
            goto cleanup;
        }
        unlink(fileNames.m_szsymFile);
        unlink(fileNames.m_szprivateFile);
        unlink(fileNames.m_szcertFile);
        unlink(fileNames.m_szAncestorEvidence);
    }

    g_myService.m_taoEnvInitializationTimer.Start();
    if(!g_myService.m_trustedHome.EnvInit(g_envplatform, g_progName, DOMAIN, 
                                          g_hostDirectory, g_clientsubDirectory,
                                          &g_myService.m_host, g_serviceProvider,
                                          0, NULL)) {
        fprintf(g_logFile, "tcService main: can't init environment\n");
        iRet= 1;
        goto cleanup;
    }
    g_myService.m_taoEnvInitializationTimer.Stop();

    // add self proctable entry
#ifdef KVMTCSERVICE
    g_myService.m_procTable.addprocEntry(g_servicepid, strdup(g_serviceexecFile), 0, NULL,
                                      g_myService.m_trustedHome.measurementSize(),
                                      g_myService.m_trustedHome.measurementPtr(),
                                      &vmconnection, &vmdomain);
#else
    g_myService.m_procTable.addprocEntry(g_servicepid, strdup(g_serviceexecFile), 0, NULL,
                                      g_myService.m_trustedHome.measurementSize(),
                                      g_myService.m_trustedHome.measurementPtr());
#endif
   
#ifdef TEST
    fprintf(g_logFile, "tcService main: after EnvInit\n");
    fprintf(g_logFile, "\ntcService main: initprocEntry succeeds\n");
    g_myService.m_procTable.print();
    fflush(g_logFile);
#endif

    while(!g_fterminateLoop) {
        fServiceStart= serviceRequest(
                        g_myService.m_trustedHome.m_linuxEnvChannel.m_reqChannel, 
                        &fTerminate);
#ifdef TEST
        if(fServiceStart)
            fprintf(g_logFile, "tcService main: successful service\n\n");
        else
            fprintf(g_logFile, "tcService main: unsuccessful service\n\n");
#else 
        UNUSEDVAR(fServiceStart);
#endif
    }

#ifdef TEST
    fprintf(g_logFile, "tcService main: tcService ending\n");
    g_myService.m_procTable.print();
#endif

cleanup:
#ifdef TEST
    if(iRet!=0)
        fprintf(g_logFile, "tcService returns with error\n");
    else
        fprintf(g_logFile, "tcService returns successful\n");
#endif
    g_myService.m_trustedHome.EnvClose();
    g_myService.m_host.HostClose();
    closeLog();
    return iRet;
}


// ------------------------------------------------------------------------------


