//
//  validateEvidence.cpp
//      John Manferdelli
//
//  Description: evidence validation implementation
//
//
//  Copyright (c) 2011, Intel Corporation. Some contributions 
//    (c) John Manferdelli.  All rights reserved.
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
#include "jlmUtility.h"
#include "logging.h"
#include "jlmcrypto.h"
#include "tinyxml.h"
#include "validateEvidence.h"
#include "cryptoHelper.h"
#include "sha256.h"

#include <time.h>
#include <string.h>

// #include "policyglobals.h"


// ------------------------------------------------------------------------



bool  revoked(const char* szCert, const char* szPolicy)
{
    return false;
}


char* getChainElementSignedInfoPurpose(int evidenceType, void* pObject)
{
    TiXmlNode* nodeSignedInfo= NULL;
    TiXmlNode* nodePurpose=  NULL;
    TiXmlNode* pNode=  NULL;

    switch(evidenceType) {
      default:
      case NOEVIDENCE:
      case EMBEDDEDPOLICYPRINCIPAL:
      case KEYINFO:
        return NULL;
      case PRINCIPALCERT:
      case SIGNEDGRANT:
        nodeSignedInfo= Search((TiXmlNode*)pObject, "ds:SignedInfo");
        break;
      case QUOTECERTIFICATE:
        nodeSignedInfo= Search((TiXmlNode*)pObject, "QuotedInfo");
        break;
    }
    if(nodeSignedInfo==NULL) {
        fprintf(g_logFile, "getChainElementSignedInfoPurpose: Cant find it\n");
        return NULL;
    }

    nodePurpose= Search(nodeSignedInfo, "Purpose");
    if(nodePurpose==NULL) {
        fprintf(g_logFile, "getChainElementSignedInfoPurpose: Cant find it\n");
        return NULL;
    }

    pNode= ((TiXmlElement*)nodePurpose)->FirstChild();
    if(pNode==NULL) {
        fprintf(g_logFile, "getChainElementSignedInfoPurpose: Cant get purpose\n");
        return NULL;
    }
    if(pNode->Value()==NULL) {
        fprintf(g_logFile, "getChainElementSignedInfoPurpose: Cant get purpose value\n");
        return NULL;
    }
    return strdup(pNode->Value());
}


TiXmlNode* getChainElementSignedInfo(int evidenceType, void* pObject)
{
    TiXmlNode* nodeSignedInfo=  NULL;

    switch(evidenceType) {
      default:
      case NOEVIDENCE:
      case EMBEDDEDPOLICYPRINCIPAL:
      case KEYINFO:
        return NULL;
      case PRINCIPALCERT:
        nodeSignedInfo= Search((TiXmlNode*)pObject, "ds:SignedInfo");
        break;
      case SIGNEDGRANT:
      case QUOTECERTIFICATE:
        nodeSignedInfo= Search((TiXmlNode*)pObject, "QuotedInfo");
        break;
    }
    if(nodeSignedInfo==NULL) {
        fprintf(g_logFile, "getChainElementSignedInfo: Cant find Validity Period\n");
        return false;
    }
    return nodeSignedInfo;
}


char* getChainElementPrincipalName(TiXmlNode* node, int evidenceType)
{
    switch(evidenceType) {
      default:
      case NOEVIDENCE:
      case EMBEDDEDPOLICYPRINCIPAL:
      case KEYINFO:
        return NULL;
      case PRINCIPALCERT:
      case SIGNEDGRANT:
      case QUOTECERTIFICATE:
        break;
    }
    return NULL;
}


char* getChainElementPrincipalType(TiXmlNode* node, int evidenceType)
{
    switch(evidenceType) {
      default:
      case NOEVIDENCE:
      case EMBEDDEDPOLICYPRINCIPAL:
      case KEYINFO:
        return NULL;
      case PRINCIPALCERT:
      case SIGNEDGRANT:
      case QUOTECERTIFICATE:
        break;
    }
    return NULL;
}


char* getChainElementSignatureAlgorithm(TiXmlNode* node, int evidenceType)
{
    TiXmlNode* nodeSigAlg= Search((TiXmlNode*)node, "ds:SignatureMethod");

    if(nodeCanonical==NULL) {
        fprintf(g_logFile, "getChainElementSignatureAlgorithm: Cant find it\n");
        return NULL;
    }
    if(((TiXmlElement*)nodeSigAlg)->Attribute("Algorithm")==NULL) {
        fprintf(g_logFile, "getChainElementSignatureAlgorithm: Cant algorithm value\n");
        return NULL;
    }

    // <ds:SignatureMethod 
    //   Algorithm="http://www.manferdelli.com/2011/Xml/algorithms/rsa1024-sha256-pkcspad#" />
    // <ds:SignatureMethod 
    //   Algorithm="http://www.manferdelli.com/2011/Xml/algorithms/rsa2048-sha256-pkcspad#" />
    // <ds:QuoteMethod Algorithm="Quote-Sha256FileHash-RSA1024" />
    return strdup(((TiXmlElement*)nodeSigAlg)->Attribute("Algorithm"));
}


char* getChainElementCanonicalizationAlgorithm(TiXmlNode* node, int evidenceType)
{
    TiXmlNode* nodeCanonical= NULL;

    switch(evidenceType) {
      default:
      case NOEVIDENCE:
      case EMBEDDEDPOLICYPRINCIPAL:
      case KEYINFO:
        return NULL;
      case PRINCIPALCERT:
      case SIGNEDGRANT:
      case QUOTECERTIFICATE:
        break;
    }
    TiXmlNode* nodeCanonical= Search((TiXmlNode*)node, "ds:CanonicalizationMethod");
    if(nodeCanonical==NULL) {
        fprintf(g_logFile, "getChainElementCanonicalizationAlgorithm: Cant find it\n");
        return NULL;
    }
    if(((TiXmlElement*)nodeCanonical)->Attribute("Algorithm")==NULL) {
        fprintf(g_logFile, "getChainElementCanonicalizationAlgorithm: Cant algorithm value\n");
        return NULL;
    }
    // <ds:CanonicalizationMethod 
    //    Algorithm="http://www.manferdelli.com/2011/Xml/canonicalization/tinyxmlcanonical#" />
    return strdup(((TiXmlElement*)nodeCanonical)->Attribute("Algorithm"));
}


bool getChainElementValidityPeriod(TiXmlNode* node, int evidenceType, 
                                   tm* pnotBefore, tm* pNotAfter)
{
    TiXmlNode* nodeValidity= Search((TiXmlNode*)node, "ValidityPeriod");
    TiXmlNode* pNode1= NULL;
    TiXmlNode* pNode2= NULL;

    if(nodeValidity==NULL) {
        fprintf(g_logFile, "getChainElementValidityPeriod: Cant find Validity Period\n");
        return false;
    }
    pNode1= Search(nodeValidity, "NotBefore");
    if(pNode1==NULL) {
        fprintf(g_logFile, "getChainElementValidityPeriod: Cant find NotBefore\n");
        return false;
    }
    pNode2= ((TiXmlElement*)pNode1)->FirstChild();
    if(pNode2==NULL) {
        fprintf(g_logFile, "getChainElementValidityPeriod: Cant get NotBefore value\n");
        return false;
    }
    if(!timeInfofromstring(pNode2->Value(), *pnotBefore)) {
        fprintf(g_logFile, "getChainElementValidityPeriod: Cant get NotBefore\n");
        return false;
    }
    pNode1= Search(nodeValidity, "NotAfter");
    if(pNode1==NULL) {
        fprintf(g_logFile, "getChainElementValidityPeriod:Cant find NotAfter\n");
        return false;
    }
    pNode2= ((TiXmlElement*)pNode1)->FirstChild();
    if(pNode2==NULL) {
        fprintf(g_logFile, "getChainElementValidityPeriod:Cant get NotAfter value\n");
        return false;
    }
    if(!timeInfofromstring(pNode2->Value(), *pnotAfter)) {
        fprintf(g_logFile, "getChainElementValidityPeriod: Cant get NotAfter\n");
        return false;
    }
    return true;
}


char* getChainElementRevocationPolicy(TiXmlNode* node, int evidenceType)
{
    return NULL;
}


char* getChainElementSignatureValue(int evidenceType, void* pObject)
{
    TiXmlNode* nodeSignatureValue= NULL;
    TiXmlNode* pNode= NULL;

    switch(evidenceType) {
      default:
      case NOEVIDENCE:
      case EMBEDDEDPOLICYPRINCIPAL:
      case KEYINFO:
        return NULL;
      case PRINCIPALCERT:
      case SIGNEDGRANT:
        nodeSignatureValue= Search((TiXmlNode*)node, "ds:SignatureValue");
        break;
      case QUOTECERTIFICATE:
        nodeSignatureValue= Search((TiXmlNode*)node, "QuoteValue");
        break;
    }
    if(nodeSignatureValue==NULL) {
        fprintf(g_logFile, "Cant find SignatureValue\n");
        return NULL;
    }
    pNode= ((TiXmlElement*)nodeSignatureValue)->FirstChild();
    if(pNode==NULL) {
        fprintf(g_logFile, "Cant find SignatureValue\n");
        return NULL;
    }
    if(pNode->Value()==NULL) {
        fprintf(g_logFile, "Cant obtain SignatureValue\n");
        return NULL;
    }
    return strdup(((TiXmlElement*)pNode)->Value());

}


KeyInfo* getChainElementSubjectKey(int evidenceType, void* pObject)
{
    TiXmlNode* nodeSignedInfo= getChainElementSignedInfo(evidenceType, pObject);
    TiXmlNode* pNode= NULL;

    if(nodeSignedInfo==NULL) {
    }
    pNode= Search(nodeSignedInfo, "SubjectKey");
    if(pNode==NULL) {
        fprintf(g_logFile, "getChainElementSubjectKey: Cant SubjectKey\n");
        return NULL;
    }

    if(((TiXmlElement*)pNode)->FirstChild()==NULL) {
        fprintf(g_logFile, "getChainElementSubjectKey: Cant get keyInfo\n");
        return NULL;
    }
    return (KeyInfo*)RSAKeyfromKeyInfoNode(((TiXmlElement*)pNode)->FirstChild());
}


// -------------------------------------------------------------------------


/*
 *      VerifyChain is simplest evidence evaluation, validating a topmost
 *      (position 0) signed credential for a particular purpose.  The root
 *      key is at the highest numbered position and must be the same as the
 *      supplied root key.
 *      
 *      Except for the root key each piece of evidence, each object is 
 *      a parsed credential.  The SignedInfo in each piece of evidence 
 *      (except the root) must include must expose an EvidenceType, Purpose, 
 *      signature algorithm, canonicalization algorithm, a ValidityPeriod 
 *      and possibly RevocationPolicy.
 *
 *      Intermediate evidence must be PrincipalCert's and must expose a 
 *      PrincipalName, PrincipalType and Purpose, in addition to the other
 *      items.
 *
 *      The topmost certificate can be a PrincipalCert, Quote or
 *      SignedGrant.
 *
 *      VerifyChain confirms the key chain, verifies the ValidityPeriod,
 *      checks revocation (if there is a RevocationPolicy) and ensures
 *      that the Purpose of each statement is sufficient for the Purpose.
 */


int  VerifySignedEvidence(KeyInfo* pSignerKey, tm* pt, int evidenceType, void* pObject)
{
    tm          notBefore;
    tm          notAfter;
    char*       szCanonicalSignedBody= NULL;
    char*       szRevocationPolicy= NULL;
    char*       szSignatureValue= NULL;
    char*       szSignatureAlg= NULL;
    char*       szCanonicalAlg= NULL;
    int         iRet= VALID;

#ifdef TEST
    fprintf(g_logFile, "VerifySignedEvidence, type is %d\n", evidenceType);
    fflush(g_logFile);
#endif

    try {
        // SignedInfo
        TiXmlNode* nodeSignedInfo= getChainElementSignedInfo(evidenceType, pObject);
        if(nodeSignedInfo==NULL) {
            iRet= INVALIDSIG;
            throw("VerifySignedEvidence: can't get SignedInfo\n");
        }

        // revocation?
        szRevocationPolicy= getChainElementRevocationPolicy(nodeSignedInfo, evidenceType);
        if(szRevocationPolicy!=NULL) {
            iRet= INVALIDREVOKED;
            throw("VerifySignedEvidence does not support revocation yet\n");
        }

        // time valid?
        if(!getChainElementValidityPeriod(nodeSignedInfo, evidenceType, 
                                    &notBefore, &NotAfter)) {
            iRet= INVALIDPERIOD;
            throw("VerifySignedEvidence: can't get validity period\n");
        }
        if(!checktimeinInterval(*pt, notBefore, notAfter)) {
            iRet= INVALIDPERIOD;
            throw("VerifySignedEvidence: invalid validity period\n");
        }

        // signed properly?
        szSignatureAlg= getChainElementSignatureAlgorithm(nodeSignedInfo, evidenceType);
        if(szSignatureAlg==NULL) {
            iRet= INVALIDSIG;
            throw("VerifySignedEvidence: cant get signature algorithm \n");
        }
        szCanonicalAlg= getChainElementCanonicalizationAlgorithm(nodeSignedInfo, evidenceType);
        if(szCanonicalAlg==NULL) {
            iRet= INVALIDSIG;
            throw("VerifySignedEvidence: no canonicalization algorithm\n");
        }
        szCanonicalSignedBody= canonicalize(nodeSignedInfo);
        if(szCanonicalSignedBody==NULL) {
            iRet= INVALIDSIG;
            throw("VerifySignedEvidence: cant canonicalize signedInfo\n");
        }
        szSignatureValue= getChainElementsignatureValue(evidenceType, pObject);
        if(szSignatureValue==NULL) {
            iRet= INVALIDSIG;
            throw("VerifySignedEvidence: cant get signature value element\n");
        }

        // check hash
        if(!VerifyRSASha256SignaturefromSignedInfoandKey(*pSignerKey,
                                    szCanonicalSignedBody, szSignatureValue)) {
            iRet= INVALIDSIG;
            throw("VerifySignedEvidence: Verify Evidence failed\n");
        }
    }
    catch(const char* szError) {
        fprintf(g_logFile, "%s", szError);
    }

    if(szSignatureAlg!=NULL) {
        free(szSignatureAlg);
        szSignatureAlg= NULL;
    }
    if(szCanonicalAlg!=NULL) {
        free(szCanonicalAlg);
        szCanonicalAlg= NULL;
    }
    if(szCanonicalSignedBody!=NULL) {
        free(szCanonicalSignedBody);
        szCanonicalSignedBody= NULL;
    }
    if(szSignatureValue!=NULL) {
        free(szSignatureValue);
        szSignatureValue= NULL;
    }

   return iRet; 
}


int  VerifyChain(RSAKey& rootKey, const char* szPurpose, tm* pt, 
                 int npieces, int* rgType, void** rgObject)

//  Only RSA is supported for now.
{
    int         i;
    int         iRet= VALID;
    time_t      timer;
    RSAKey*     pSignerKey= NULL;
    char*       szPurpose= NULL;

#ifdef TEST
    fprintf(g_logFile, "VerifyChain %d\n", npiecesEvidence);
    fflush(g_logFile);
#endif

    try {

        // now if not specified
        if(pt==NULL) {
            time(&timer);
            pt= gmtime((const time_t*)&timer);
        }

        // root at bottom?
        if(rgType[npieces-1]!= KEYINFO || !sameRSAKey((RSAKey*)rgObject, &rootKey)) {
            iRet= INVALIDROOT;
            throw("VerifyChain: Chain does not end with root\n");
        }

        for(i=0; i<(npieces-1); i++) {

            // check purpose
            szPurpose= getChainElementSignedInfoPurpose(rgType[i], rgObject[i]);
            if(purposeNode==NULL) {
                iRet= INVALIDPURPOSE;
                throw("VerifyChain: cant get signer key\n");
            }
            // Fix: check

            // get signer's key
            pSignerKey= (RSAKeyInfo*) getChainElementSubjectKey(rgType[i+1], rgObject[i+1]);
            if(pSignerKey==NULL) {
                iRet= INVALIDPARENT;
                throw("VerifyChain: cant get signer key\n");
            }

            // check evidence
            iRet= VerifySignedEvidence((KeyInfo*)pSignerKey, pt, rgType[i], rgObject[i]);
        }
    }
    catch(const char* szError) {
        fprintf(g_logFile, "%s");
        fflush(g_logFile);
    }
    
#ifdef TEST
    fprintf(g_logFile, "VerifyChain returns %d\n", iRet);
    fflush(g_logFile);
#endif
    return iRet;
}


// -----------------------------------------------------------------------------


evidenceCollection::~evidenceCollection()
{
}


evidenceCollection::evidenceCollection()
{
    m_fParsed= false;
    m_fValid= false;
    m_iNumEvidenceLists= 0;
    m_rgiCollectionTypes= m_rgistaticCollectionTypes;
    m_rgCollectionList= m_rgstaticCollectionList;
}


bool evidenceCollection::parseEvidenceCollection(const char* szEvidenceCollection)
{
    TiXmlDocument   doc;
    TiXmlElement*   pRootElement= NULL;
    TiXmlNode*      pNode= NULL;
    const char*           szElt= NULL;
    evidenceList*   pEvidenceList= NULL;
    int             n= 0;

    if(!doc.Parse(szEvidenceCollection)) {
        fprintf(g_logFile, "Can't parse Evidence Collection\n");
        return false;
    }

    pRootElement= doc.RootElement();
    pRootElement->QueryIntAttribute ("count", &m_iNumEvidenceLists);
    if(m_iNumEvidenceLists>STATICNUMCOLLECTIONELTS) {
        fprintf(g_logFile, "Too many collection elements\n");
        return false;
    }

    pNode= pRootElement->FirstChild();
    while(pNode!=NULL) {
        szElt= ((TiXmlElement*)pNode)->Value();
        if(strcmp(szElt,"EvidenceList")==0) {
            pEvidenceList= new evidenceList();
            if(!pEvidenceList->parseEvidenceList((TiXmlElement*)pNode)) {
                return false;
            }
            m_rgiCollectionTypes[n]= pEvidenceList->m_rgiEvidenceTypes[0];
            m_rgCollectionList[n]= pEvidenceList;
            if(n>=STATICNUMCOLLECTIONELTS) {
                fprintf(g_logFile, "Too many collection elements\n");
                return false;
            }
            n++;
        }
        pNode= pNode->NextSibling();
    }

    if(n!=m_iNumEvidenceLists) {
        fprintf(g_logFile, "Evidence collection mismatch %d %d\n", n, m_iNumEvidenceLists);
        return false;
    }

    m_fParsed= true;
    return m_fParsed;
}


bool evidenceCollection::validateEvidenceCollection(RSAKey* pRootKey)
{
    int     i;

    for(i=0;i<m_iNumEvidenceLists; i++) {
        if(!m_rgCollectionList[i]->validateEvidenceList(pRootKey, NULL)) {
            fprintf(g_logFile, "Failing on %d\n",i);
            return false;
        }
    }
    m_fValid= true;
    return m_fValid;
}


evidenceList::evidenceList()
{
    m_fParsed= false;
    m_fValid= false;
    m_iNumPiecesofEvidence= 0;
    m_rgiEvidenceTypes= m_rgistaticEvidenceTypes;
    m_rgEvidence= m_rgstaticEvidence;
}


evidenceList::~evidenceList()
{
}


bool    evidenceList::parseEvidenceList(TiXmlElement* pRootElement)
{
    TiXmlNode*          pNode= NULL;
    TiXmlNode*          pNode1= NULL;
    int                 n= 0;
    char*               szEvidence= NULL;
    PrincipalCert*      pCert= NULL;
    SignedAssertion*    pAssert= NULL;

    if(strcmp(pRootElement->Value(),"EvidenceList")!=0) {
        fprintf(g_logFile, "Should be EvidenceList %s\n", pRootElement->Value());
        return false;
    }
    pRootElement->QueryIntAttribute ("count", &m_iNumPiecesofEvidence);
    if(m_iNumPiecesofEvidence>STATICNUMLISTELTS) {
        fprintf(g_logFile, "Too many list elements\n");
        return false;
    }

    pNode= pRootElement->FirstChild();
    while(pNode!=NULL) {
        // find EvidenceType
        if((pNode1=Search(pNode, "SignedGrant"))!=NULL) {
            m_rgiEvidenceTypes[n]= SIGNEDGRANT;
        }
        else if((pNode1=Search(pNode, "Certificate"))!=NULL) {
            m_rgiEvidenceTypes[n]= PRINCIPALCERT;
        }
        else {
            fprintf(g_logFile, "Unknown evidence type\n");
            return false;
        }
        if(n>=STATICNUMLISTELTS) {
            fprintf(g_logFile, "Too many collection elements\n");
            return false;
        }

        // parse Evidence
        szEvidence= canonicalize(pNode);
        switch(m_rgiEvidenceTypes[n]) {
          case SIGNEDGRANT:
            pAssert= new SignedAssertion();
            if(!pAssert->init(szEvidence)) {
                fprintf(g_logFile, "Can't init SignedAssertion\n");
                return false;
            }
            if(!pAssert->parseSignedAssertionElements()) {
                fprintf(g_logFile, "Can't parse SignedAssertion\n");
                return false;
            }
            m_rgEvidence[n]= (void*) pAssert;
            break;
          case PRINCIPALCERT:
            pCert= new PrincipalCert();
            if(!pCert->init(szEvidence)) {
                fprintf(g_logFile, "Can't init PrincipalCertElements\n");
                return false;
            }
            if(!pCert->parsePrincipalCertElements()) {
                fprintf(g_logFile, "Can't parse PrincipalCertElements\n");
                return false;
            }
            m_rgEvidence[n]= (void*) pCert;
            break;
          default:
           fprintf(g_logFile, "Unknown Evidence type 1\n");
            return false;
        }
        free(szEvidence);
        szEvidence= NULL;
        n++;
        pNode= pNode->NextSibling();
    }

    if(n!=m_iNumPiecesofEvidence) {
        fprintf(g_logFile, "Evidence list mismatch %d %d\n", n, m_iNumPiecesofEvidence);
        return false;
    }

    m_fParsed= true;
    return m_fParsed;
}


bool    evidenceList::validateEvidenceList(RSAKey* pRootKey, RSAKey* pTopKey)
{
    int             iVerify;

#ifdef TEST
    fprintf(g_logFile, "evidenceList::validateEvidenceList()\n");
#endif

    if(!m_fParsed) {
        fprintf(g_logFile, "Evidence List not parsed\n");
        return false;
    }

    if(pRootKey==NULL) {
        fprintf(g_logFile, "Policy principal key is NULL\n");
        return false;
    }

    if(m_iNumPiecesofEvidence<STATICNUMLISTELTS) {
        m_rgiEvidenceTypes[m_iNumPiecesofEvidence]= EMBEDDEDPOLICYPRINCIPAL;
        m_rgEvidence[m_iNumPiecesofEvidence]= pRootKey;
        m_iNumPiecesofEvidence++;
    }
    else {
        fprintf(g_logFile, "Too many list elements\n");
        return false;
    }

    iVerify= VerifyEvidenceList(NULL, m_iNumPiecesofEvidence, m_rgiEvidenceTypes,
                                (void**) m_rgEvidence, pRootKey, pTopKey);
    if(iVerify>0)
        m_fValid= true;
    else
        m_fValid= false;

#ifdef TEST
    if(m_fValid)
        fprintf(g_logFile, "evidenceList::validateEvidenceList() returns true\n");
    else
        fprintf(g_logFile, "evidenceList::validateEvidenceList() returns false\n");
    fflush(g_logFile);
#endif
    return m_fValid;
}


// -------------------------------------------------------------------------------


const char* s_EvidenceListStart= "<EvidenceList count='%d'>\n";
const char* s_EvidenceListStop= "</EvidenceList>\n";


char* consttoEvidenceList(const char* szEvidence, const char* szEvidenceSupport)
{

    if(szEvidence==NULL) {
        return NULL;
    }

    TiXmlDocument   listDoc;
    TiXmlNode*      pNode= NULL;
    int             numpiecesofEvidence= 0;
    int             len= strlen(szEvidenceSupport)+strlen(szEvidence)+128;
    char*           buf= (char*) malloc(len);
    char*           p= buf;
    int             left= len;
    int             n;
    char*           szNode= NULL;

    if(buf==NULL) {
        return NULL;
    }

    if(szEvidenceSupport==NULL) {
        // append szEvidence
        sprintf(buf, s_EvidenceListStart, 1);
        n= strlen(p);
        p+= n;
        left-= n;
        if(!safeTransfer(&p, &left, szEvidence)) {
            return NULL;
        }
        if(!safeTransfer(&p, &left, s_EvidenceListStop)) {
            return NULL;
        }
        return strdup(buf);
    }


    if(listDoc.Parse(szEvidenceSupport)) {
        return NULL;
    }

    TiXmlElement* pRootElement= listDoc.RootElement();
    pRootElement->QueryIntAttribute ("count", &numpiecesofEvidence);
    numpiecesofEvidence++;

    // append szEvidence
    sprintf(buf, s_EvidenceListStart, numpiecesofEvidence);
    n= strlen(p);
    p+= n;
    left-= n;
    if(!safeTransfer(&p, &left, szEvidence)) {
        return NULL;
    }

    // add evidence support
    pNode= pRootElement->FirstChild();
    while(pNode!=NULL) {
        if(pNode->Type()==TiXmlNode::TINYXML_ELEMENT) {
            szNode= canonicalize(pNode);
            if(szNode==NULL) {
                return NULL;
            }
            if(!safeTransfer(&p, &left, szEvidence)) {
                free(szNode);
                return NULL;
            }
            free(szNode);
            szNode= NULL;
        }
        pNode= pNode->NextSibling();
    }
    if(!safeTransfer(&p, &left, s_EvidenceListStop)) {
        return NULL;
    }
    return strdup(buf);
}


// ----------------------------------------------------------------------------


