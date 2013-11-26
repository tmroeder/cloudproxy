//
//  File: serviceChannel.h
//  Description: serviceChannel defines
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


//----------------------------------------------------------------------


#ifndef _SERVICECHANNEL__H
#define _SERVICECHANNEL__H

#include "tao.h"

#include "session.h"
#include "channel.h"
#include "safeChannel.h"
#include "resource.h"
#include "request.h"
#include "cert.h"
#include "accessControl.h"
#include "algs.h"
#include "timer.h"
#include <pthread.h>


#define  MAXNUMCLIENTS  50



//  thread for client channel
void* channelThread(void* ptr);


class serviceChannel {
public:
    bool                m_fThreadValid;
    int                 m_serverState;

    session             m_serverSession;
    bool                m_fChannelAuthenticated;
    int                 m_fdChannel;
    safeChannel         m_oSafeChannel;

    taoHostServices*    m_ptaoHost;
    taoEnvironment*     m_ptaoEnvironment;

    fileServices        m_fileServices;
    metaData*           m_pMetaData;

    serviceChannel();
    ~serviceChannel();

    bool                initServiceChannel(safeChannel* pSafeChannel, 
                                           taoHostServices* ptaoHost, 
                                           taoEnvironment * ptaoEnv);
    int                 processRequests();
    bool                serviceChannel();
};


#endif


//-------------------------------------------------------------------------


