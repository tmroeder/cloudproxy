//  Copyright (c) 2014, Google Inc.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
#include <string>
#include <stdlib.h>
#include <taosupport.pb.h>

#ifndef __TAOSUPPORT_H__
#define __TAOSUPPORT_H__

#include "tao/fd_message_channel.h"
#include "tao/tao_rpc.h"
#include "tao/util.h"

#include "attestation.pb.h"

#include <taosupport.pb.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <string>
#include <list>

#ifndef byte
typedef unsigned char byte;
#endif

class TaoProgramData {
private:

  // Most of these should be private

  // Has InitTao initialized me successfully?
  bool  initialized_;
  // Channel to communicate with host.
  tao::FDMessageChannel* msg_;
  // Tao object interface
  tao::Tao* tao_;

  // Endorsement for AIK for TPM
  string endorsement_cert_;

  // Marshalled tao name (a Prin).
  string marshalled_tao_name_;
  // Printable tao name
  string tao_name_;

  string policy_cert_;
  X509* policyCertificate_;

  // Program key.
  string program_key_type_;
  EVP_PKEY* program_key_;

  // Der encoded and parsed X509 program certificate.
  string program_cert_;
  X509* programCertificate_;

  // Cert chain for Program Certificate.
  std::list<string> certs_in_chain_;

  int size_program_sym_key_;
  byte* program_sym_key_;
  string program_file_path_;

public:
  TaoProgramData();
  ~TaoProgramData();

  // Accessors
  bool GetTaoName(string* name);
  bool GetSymKeys(string* symkeys);

  bool GetPolicyCert(string* cert);
  X509* GetPolicyCertificate();
  void SetPolicyCertificate(X509* c);

  bool GetProgramKeyType(string* keyType);
  EVP_PKEY* GetProgramKey();

  X509* GetProgramCertificate();
  bool GetProgramCert(string* cert);
  void SetProgramCertificate(X509* c);

  std::list<string>* GetCertChain();

  void ClearProgramData();
  bool InitTao(tao::FDMessageChannel* msg, tao::Tao* tao, string&, string&,
               string& network, string& address, string& port);
  void Print();

  // Maybe the Initialize routines should be private.
  bool InitializeProgramKey(string& path, string& key_type, int key_size,
                            string& network, string& address, string& port,
                            string& hostType, string& hostCert);
  bool InitializeSymmetricKeys(string& path, int keysize);

  bool ExtendName(string& subprin);
  bool Seal(string& to_seal, string* sealed);
  bool Unseal(string& sealed, string* unsealed);
  bool Attest(string& to_attest, string* attested);

private:
  // This should be private.
  bool RequestDomainServiceCert(string& network, string& address, string& port,
          string& attestation_string, string& endorsement_cert,
          string* program_cert, std::list<string>* certsinChain);
};

class TaoChannel {
public:
  SslChannel peer_channel_;
  X509* peerCertificate_;
  string peer_name_;

  TaoChannel();
  ~TaoChannel();
  bool OpenTaoChannel(TaoProgramData& client_program_data,
                      string& serverAddress, string& port);
  void CloseTaoChannel();
  bool SendRequest(taosupport::SimpleMessage& out);
  bool GetRequest(taosupport::SimpleMessage* in);
  void Print();
};

bool GetKeyBytes(EVP_PKEY* pKey, string* bytes_out);
#endif


