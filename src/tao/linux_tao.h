//  File: linux_tao.h
//  Author: Tom Roeder <tmroeder@google.com>
//
//  Description: LinuxTao implements the Tao for the Linux
//  operating system
//
//  Copyright (c) 2013, Google Inc.  All rights reserved.
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
// limitations under the License.
#ifndef TAO_LINUX_TAO_H_
#define TAO_LINUX_TAO_H_

#include <list>
#include <mutex>
#include <set>
#include <string>

#include <keyczar/base/basictypes.h>  // DISALLOW_COPY_AND_ASSIGN
#include <keyczar/base/scoped_ptr.h>

#include "tao/attestation.pb.h"
#include "tao/keys.h"
#include "tao/tao.h"
#include "tao/tao_domain.h"

using std::list;
using std::mutex;
using std::set;
using std::string;

namespace tao {

class HostedProgramFactory;
class TaoChannel;
class TaoChildChannel;
class TaoDomain;

/// LinuxTao implements a generic implementation
/// of the Tao that can be configured for a variety of environments.
///
/// Similarly, the a KVM Guests as hosted programs
/// (using the KvmVmFactory instead of the ProcessFactory). In this case, the
/// interaction would be: TPMTaoChildChannel <-> LinuxTao <-> KvmUnixTaoChannel.
///
///
/// For example, a Linux OS installed on hardware with a TPM might work as
/// follows. 
///
/// The Tao within the Linux OS might use a PipeTaoChannel to communicate with
/// hosted programs running as processes. 
///
///
/// TPMTaoChildChannel <-> Linux Tao <-> PipeTaoChannel. The
/// TPMTaoChildChannel implements a shim for the TPM hardware to convert Tao
/// operations into TPM commands. 
///
/// A hosted program called CloudServer
/// would then have the following interactions: PipeTaoChildChannel <->
/// CloudServer. The PipeTaoChildChannel and the PipeTaoChannel communicate over
/// Unix pipes to send Tao messages between Linux's Tao and CloudServer. See the
/// apps/ folder for applications that implement exactly this setup:
/// apps/linux_tao_service.cc implements the LinuxTao, and apps/server.cc
/// implements CloudServer.
///
/// A Tao that has KVM guests as hosted programs might instead use
/// KvmUnixTaoChannel 
/// (using the KvmVmFactory instead of the ProcessFactory). In this case, the
/// interaction would be: TPMTaoChildChannel <-> LinuxTao <-> KvmUnixTaoChannel.
///
/// And the guest OS would have another instance of the LinuxTao that would have
/// the following interactions:
/// KvmUnixTaoChildChannel <-> LinuxTao <-> PipeTaoChannel. This version of
/// the LinuxTao in the Guest OS would use the ProcessFactory to start hosted
/// programs as processes in the guest.
///
/// In summary: each level of the Tao can have a TaoChildChannel to communicate
/// with its host Tao and has a TaoChannel to communicated with hosted programs.
/// Hosts use implementations of HostedProgramFactory to instantiate hosted
/// programs.
///

/// An implementation of the Tao for Linux. This implementation can take
/// different HostedProgramFactory implementations, different TaoChannel
/// implementations for communicating with its hosted programs, and different
/// TaoChildChannel implementations for communicating with its parent Tao. The
/// only assumptions LinuxTao makes are basic: it has the normal filesystem API
/// to which it can write files, and it is an intermediate Tao rather than the
/// root Tao.
class LinuxTao : public Tao {
 public:
  /// The LinuxTao is constructed with paths to keys and implementations of
  /// factories and channels. Ownership is taken for all relevant parameters.
  /// @param keys_path The directory storing keys and secrets.
  /// @param host_channel A channel to communicate with the parent Tao.
  /// @param child_channel A channel to communicate with hosted programs it
  /// starts.
  /// @param program_factory A factory that creates hosted programs in the OS.
  /// @param admin The configuration for this administrative domain.
  LinuxTao(const string &keys_path, TaoChildChannel *host_channel,
           TaoChannel *child_channel, HostedProgramFactory *program_factory,
           TaoDomain *admin)
      : admin_(admin),
        keys_(new Keys(keys_path, "linux_tao", Keys::Signing | Keys::Crypting)),
        host_channel_(host_channel),
        child_channel_(child_channel),
        program_factory_(program_factory) {}
  virtual ~LinuxTao() {}

  /// Start listening for Tao messages on channels.
  bool Listen();

  /// LinuxTao follows the normal semantics of the Tao for these methods.
  /// @{
  virtual bool Init();
  virtual bool Destroy() { return true; }
  virtual bool StartHostedProgram(const string &program,
                                  const list<string> &args, string *child_name);
  virtual bool RemoveHostedProgram(const string &child_name);
  virtual bool GetTaoFullName(string *tao_name) const;
  virtual bool GetLocalName(string *local_name) const;
  virtual bool GetPolicyName(string *policy_name) const;
  virtual bool InstallPolicyAttestation(const string &attestation);

  virtual bool GetRandomBytes(const string &child_name, size_t size,
                              string *bytes) const;
  virtual bool Seal(const string &child_name, const string &data, int policy,
                    string *sealed) const;
  virtual bool Unseal(const string &child_name, const string &sealed,
                      string *data, int *policy) const;
  virtual bool Attest(const string &child_name, const string &key_prin,
                      string *attestation) const;
  virtual bool ExtendName(string *child_name, const string &subprin);
  /// @}

  /// For purposes of test/debug, disable policy checking on unseal operations.
  /// @disable Whether or not to ignore policy for unseal operations.
  virtual void SetIgnoreUnsealPolicyForTesting(bool disable) {
    ignore_unseal_policy_for_testing_ = disable;
  }

 private:
  /// Configuration for this administrative domain.
  scoped_ptr<TaoDomain> admin_;

  /// Keys and attestations for signing and sealing.
  scoped_ptr<Keys> keys_;

  /// The policy key's attestation for our signing key.
  string policy_attestation_;

  /// The parent Tao's attestation for our signing key.
  string parent_attestation_;

  /// The channel to use for host communication.
  scoped_ptr<TaoChildChannel> host_channel_;

  /// A channel that handles all child connections.
  scoped_ptr<TaoChannel> child_channel_;

  /// A factory that can be used to start hosted programs.
  scoped_ptr<HostedProgramFactory> program_factory_;

  /// The set of hosted programs that the LinuxTao has started.
  set<string> running_children_;

  /// A count of how many children have been started so far.
  int last_child_id_;

  /// Our own full name, as obtained from host channel.
  string full_name_;

  /// A mutex for accessing the auth manager.
  mutable mutex auth_m_;

  /// A mutex for accessing and modifying running_children_.
  mutable mutex data_m_;

  /// Whether or not to ignore policy failures on unseal operations.
  bool ignore_unseal_policy_for_testing_;

  /// Send existing attestation to the Tao CA specified in the TaoDomain
  /// supplied at initialization and use the returned attestation in its place.
  bool GetTaoCAAttestation();

  DISALLOW_COPY_AND_ASSIGN(LinuxTao);
};
}  // namespace tao

#endif  // TAO_LINUX_TAO_H_
