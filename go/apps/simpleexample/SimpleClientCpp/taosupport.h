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
#include <taosupport.pb.h>

#ifndef __TAOSUPPORT_H__
#define __TAOSUPPORT_H__

#include "tao/fd_message_channel.h"
#include "tao/tao_rpc.h"
#include "tao/util.h"
#include <taosupport.pb.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

typedef unsigned char byte;

void PrintBytes(int n, byte* in);

class TaoProgramData {
public:
  bool  initialized_;
  FDMessageChannel* msg_;
  Tao* tao_;
  string tao_name_;
  int size_policy_cert_;
  byte* policy_cert_;
  // need a key representation
  int size_program_sym_key_;
  byte* program_sym_key_;
  int size_program_cert_;
  byte* program_cert_;
  string program_file_path_;

  TaoProgramData();
  ~TaoProgramData();
  void ClearProgramData();
  bool InitTao(FDMessageChannel* msg, Tao* tao, string&, string&);
  void Print();

  bool InitializeProgramKey(string& path, int keysize,
          byte* keys, RSA** myKey);
  bool InitializeSymmetricKeys(string& path, int keysize, int* key_size_out,
          byte* keys);
  bool Seal(string& to_seal, string* sealed);
  bool Unseal(string& sealed, string* unsealed);
  bool Attest(string& to_attest, string* attested);
  bool RequestDomainServiceCert(string& network, string& address,
          RSA* myKey, RSA* verifyKey,
          int* size_cert, byte* cert);
};

class TaoChannel {
public:
  string server_name_;
  int fd_;

  TaoChannel();
  ~TaoChannel();
  bool OpenTaoChannel(TaoProgramData& client_program_data,
                      string& serverAddress);
  void CloseTaoChannel();
  bool SendRequest(taosupport::SimpleMessage& out);
  bool GetRequest(taosupport::SimpleMessage* in);
  void Print();
};
#endif


